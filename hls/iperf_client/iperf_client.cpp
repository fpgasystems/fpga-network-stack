/************************************************
Copyright (c) 2019, Systems Group, ETH Zurich.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
************************************************/
#include "iperf_client_config.hpp"
#include "iperf_client.hpp"
#include <iostream>

//Buffers responses coming from the TCP stack
void status_handler(hls::stream<appTxRsp>&				txStatus,
							hls::stream<internalAppTxRsp>&	txStatusBuffer)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	if (!txStatus.empty())
	{
		appTxRsp resp = txStatus.read();
		txStatusBuffer.write(internalAppTxRsp(resp.sessionID, resp.error));
	}
}

template <int WIDTH>
void client(hls::stream<ipTuple>&				openConnection,
            hls::stream<openStatus>& 			openConStatus,
				hls::stream<ap_uint<16> >&			closeConnection,
				hls::stream<appTxMeta>&				txMetaData,
				hls::stream<net_axis<WIDTH> >& 	txData,
				hls::stream<internalAppTxRsp>&	txStatus,
				hls::stream<bool>&					startSignal,
				hls::stream<bool>&					stopSignal,
				ap_uint<1>		runExperiment,
				ap_uint<1>		dualModeEn,
				ap_uint<14>		useConn,
				ap_uint<8> 		pkgWordCount,
				ap_uint<8>		packetGap,
            ap_uint<32>    timeInSeconds,
				ap_uint<32>		regIpAddress0,
				ap_uint<32>		regIpAddress1,
				ap_uint<32>		regIpAddress2,
				ap_uint<32>		regIpAddress3,
				ap_uint<32>		regIpAddress4,
				ap_uint<32>		regIpAddress5,
				ap_uint<32>		regIpAddress6,
				ap_uint<32>		regIpAddress7,
				ap_uint<32>		regIpAddress8,
				ap_uint<32>		regIpAddress9)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	enum iperfFsmStateType {IDLE, INIT_CON, WAIT_CON, CONSTRUCT_HEADER, INIT_RUN, START_PKG, CHECK_REQ, WRITE_PKG, CHECK_TIME, PKG_GAP};
	static iperfFsmStateType iperfFsmState = IDLE;

	static ap_uint<14> numConnections = 0;
	static ap_uint<16> currentSessionID;
	static ap_uint<14> sessionIt = 0;
	static ap_uint<14> closeIt = 0;
	static bool timeOver = false;
	static ap_uint<8> wordCount = 0;
	static ap_uint<4> ipAddressIdx = 0;
	static iperfTcpHeader<WIDTH> header;
	static ap_uint<8> packetGapCounter = 0;


	/*
	 * CLIENT FSM
	 */
	switch (iperfFsmState)
	{
	case IDLE:
		//done do nothing
		sessionIt = 0;
		closeIt = 0;
		numConnections = 0;
		timeOver = false;
		ipAddressIdx = 0;
		if (runExperiment)
		{
			iperfFsmState = INIT_CON;
		}
		break;
	case INIT_CON:
		if (sessionIt < useConn)
		{
			ipTuple openTuple;
			switch (ipAddressIdx)
			{
			case 0:
				openTuple.ip_address = regIpAddress0;
				break;
			case 1:
				openTuple.ip_address = regIpAddress1;
				break;
			case 2:
				openTuple.ip_address = regIpAddress2;
				break;
			case 3:
				openTuple.ip_address = regIpAddress3;
				break;
			case 4:
				openTuple.ip_address = regIpAddress4;
				break;
			case 5:
				openTuple.ip_address = regIpAddress5;
				break;
			case 6:
				openTuple.ip_address = regIpAddress6;
				break;
			case 7:
				openTuple.ip_address = regIpAddress7;
				break;
			case 8:
				openTuple.ip_address = regIpAddress8;
				break;
			case 9:
				openTuple.ip_address = regIpAddress9;
				break;
			}
			openTuple.ip_port = 5001;
			openConnection.write(openTuple);
			ipAddressIdx++;
			if (ipAddressIdx == 10)
			{
 				ipAddressIdx = 0;
			}
		}
		sessionIt++;
		if (sessionIt == useConn)
		{
			sessionIt = 0;
			iperfFsmState = WAIT_CON;
		}
		break;
	case WAIT_CON:
		if (!openConStatus.empty())
		{
			openStatus status = openConStatus.read();
			if (status.success)
			{
				//experimentID[sessionIt] = status.sessionID;
				std::cout << "Connection successfully opened." << std::endl;
				txMetaData.write(appTxMeta(status.sessionID, IPERF_TCP_HEADER_SIZE/8));
				numConnections++;
			}
			else
			{
				std::cout << "Connection could not be opened." << std::endl;
			}
			sessionIt++;
			if (sessionIt == useConn) //maybe move outside
			{
				startSignal.write(true);
				sessionIt = 0;
				iperfFsmState = CONSTRUCT_HEADER;
			}
		}
		break;
	case CONSTRUCT_HEADER:
		header.clear();
		header.setDualMode(dualModeEn);
		header.setListenPort(5001);
		header.setSeconds(timeInSeconds);
		if (sessionIt == numConnections)
		{
			sessionIt = 0;
			iperfFsmState = CHECK_REQ;
		}
		else if (!txStatus.empty())
		{
			internalAppTxRsp resp = txStatus.read();
			if (resp.error == 0)
			{
				currentSessionID = resp.sessionID;
				iperfFsmState = INIT_RUN;
			}
			else
			{
				//Check if connection was torn down
				if (resp.error == 1)
				{
					std::cout << "Connection was torn down. " << resp.sessionID << std::endl;
					numConnections--;
				}
			}
		}
		break;
	case INIT_RUN:
		{
			net_axis<WIDTH> headerWord;
			headerWord.last = 0;

			if (header.consumeWord(headerWord.data) < (WIDTH/8))
			{
				headerWord.last = 1;
				txMetaData.write(appTxMeta(currentSessionID, pkgWordCount*(WIDTH/8)));
				sessionIt++;
				iperfFsmState = CONSTRUCT_HEADER;
			}
			headerWord.keep = ~0;
			if (headerWord.last)
			{
				if (WIDTH == 128)
				{
					headerWord.keep(15, 8) = 0;
				}
				if (WIDTH > 128)
				{
					headerWord.keep((WIDTH/8)-1, 24) = 0;
				}
			}
			txData.write(headerWord);

		}
		break;
	case CHECK_REQ:
		if (!txStatus.empty())
		{
			internalAppTxRsp resp = txStatus.read();
			if (resp.error == 0)
			{
				currentSessionID = resp.sessionID;
				iperfFsmState = START_PKG;
			}
			else
			{
				//Check if connection  was torn down
				if (resp.error == 1)
				{
					std::cout << "Connection was torn down. " << resp.sessionID << std::endl;
					numConnections--;
				}
				else
				{
					txMetaData.write(appTxMeta(resp.sessionID, pkgWordCount*(WIDTH/8)));
					//sessionIt++;
				}
			}
		}
		break;
	case START_PKG:
		{
			net_axis<WIDTH> currWord;
			for (int i = 0; i < (WIDTH/64); i++)
			{
				#pragma HLS UNROLL
				currWord.data(i*64+63, i*64) = 0x3736353433323130;
				currWord.keep(i*8+7, i*8) = 0xff;
			}
			currWord.last = 0;
			txData.write(currWord);
			wordCount = 1;
			iperfFsmState = WRITE_PKG;
		}
		break;
	case WRITE_PKG:
	{
		wordCount++;
		net_axis<WIDTH> currWord;
		for (int i = 0; i < (WIDTH/64); i++) 
		{
			#pragma HLS UNROLL
			currWord.data(i*64+63, i*64) = 0x3736353433323130;
			currWord.keep(i*8+7, i*8) = 0xff;
		}
		currWord.last = (wordCount == pkgWordCount);
		txData.write(currWord);
		if (currWord.last)
		{
			wordCount = 0;
			iperfFsmState = (packetGap != 0) ? PKG_GAP : CHECK_TIME;
		}
	}
		break;
	case CHECK_TIME:
		if (timeOver && closeIt == numConnections)
		{
			iperfFsmState = IDLE;
		}
		else
		{
			if (!timeOver)
			{
				txMetaData.write(appTxMeta(currentSessionID, pkgWordCount*(WIDTH/8)));
			}
			else
			{
				closeConnection.write(currentSessionID);
				closeIt++;
			}
			
			if (closeIt != numConnections)
			{
				iperfFsmState = CHECK_REQ;
			}
		}
		break;
	case PKG_GAP:
		packetGapCounter++;
		if (packetGapCounter == packetGap)
		{
			packetGapCounter = 0;
			iperfFsmState = CHECK_TIME;
		}
		break;
	} //switch

	//clock
	if (!stopSignal.empty())
	{
		stopSignal.read(timeOver);
	}
	
}

template <int WIDTH>
void server(	hls::stream<ap_uint<16> >&		listenPort,
				hls::stream<bool>&				listenPortStatus,
				hls::stream<appNotification>&	notifications,
				hls::stream<appReadRequest>&	readRequest,
				hls::stream<ap_uint<16> >&		rxMetaData,
				hls::stream<net_axis<WIDTH> >&	rxData)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

   enum listenFsmStateType {OPEN_PORT, WAIT_PORT_STATUS};
   static listenFsmStateType listenState = OPEN_PORT;
	enum consumeFsmStateType {WAIT_PKG, CONSUME};
	static consumeFsmStateType  serverFsmState = WAIT_PKG;
	#pragma HLS RESET variable=listenState

	switch (listenState)
	{
	case OPEN_PORT:
		// Open Port 5001
		listenPort.write(5001);
		listenState = WAIT_PORT_STATUS;
		break;
	case WAIT_PORT_STATUS:
		if (!listenPortStatus.empty())
		{
			bool open = listenPortStatus.read();
			if (!open)
			{
				listenState = OPEN_PORT;
			}
		}
		break;
	}
	
	if (!notifications.empty())
	{
		appNotification notification = notifications.read();

		if (notification.length != 0)
		{
			readRequest.write(appReadRequest(notification.sessionID, notification.length));
		}
	}

	switch (serverFsmState)
	{
	case WAIT_PKG:
		if (!rxMetaData.empty() && !rxData.empty())
		{
			rxMetaData.read();
			net_axis<WIDTH> receiveWord = rxData.read();
			if (!receiveWord.last)
			{
				serverFsmState = CONSUME;
			}
		}
		break;
	case CONSUME:
		if (!rxData.empty())
		{
			net_axis<WIDTH> receiveWord = rxData.read();
			if (receiveWord.last)
			{
				serverFsmState = WAIT_PKG;
			}
		}
		break;
	}
}

void clock( hls::stream<bool>&	startSignal,
            hls::stream<bool>&	stopSignal,
            ap_uint<64>          timeInCycles)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

   enum swStateType {WAIT_START, RUN};
   static swStateType sw_state = WAIT_START;
   static ap_uint<48> time = 0;

   switch (sw_state)
   {
   case WAIT_START:
      if (!startSignal.empty())
      {
         startSignal.read();
         time = 0;
         sw_state = RUN;
      }
      break;
   case RUN:
      time++;
      if (time == timeInCycles)
      {
         stopSignal.write(true);
         sw_state = WAIT_START;
      }
      break;
   }
}


void iperf_client(	hls::stream<ap_uint<16> >& listenPort,
					hls::stream<bool>& listenPortStatus,
					hls::stream<appNotification>& notifications,
					hls::stream<appReadRequest>& readRequest,
					hls::stream<ap_uint<16> >& rxMetaData,
					hls::stream<net_axis<DATA_WIDTH> >& rxData,
					hls::stream<ipTuple>& openConnection,
					hls::stream<openStatus>& openConStatus,
					hls::stream<ap_uint<16> >& closeConnection,
					hls::stream<appTxMeta>& txMetaData,
					hls::stream<net_axis<DATA_WIDTH> >& txData,
					hls::stream<appTxRsp>& txStatus,
					ap_uint<1>		runExperiment,
					ap_uint<1>		dualModeEn,
					ap_uint<14>		useConn,
					ap_uint<8>		pkgWordCount,
					ap_uint<8>		packetGap,
               ap_uint<32>    timeInSeconds,
               ap_uint<64>    timeInCycles,
					ap_uint<32>		regIpAddress0,
					ap_uint<32>		regIpAddress1,
					ap_uint<32>		regIpAddress2,
					ap_uint<32>		regIpAddress3,
					ap_uint<32>		regIpAddress4,
					ap_uint<32>		regIpAddress5,
					ap_uint<32>		regIpAddress6,
					ap_uint<32>		regIpAddress7,
					ap_uint<32>		regIpAddress8,
					ap_uint<32>		regIpAddress9)

{
	#pragma HLS DATAFLOW disable_start_propagation
	#pragma HLS INTERFACE ap_ctrl_none port=return

	#pragma HLS INTERFACE axis register port=listenPort name=m_axis_listen_port
	#pragma HLS INTERFACE axis register port=listenPortStatus name=s_axis_listen_port_status

	#pragma HLS INTERFACE axis register port=notifications name=s_axis_notifications
	#pragma HLS INTERFACE axis register port=readRequest name=m_axis_read_package
	#pragma HLS aggregate  variable=notifications compact=bit
	#pragma HLS aggregate  variable=readRequest compact=bit

	#pragma HLS INTERFACE axis register port=rxMetaData name=s_axis_rx_metadata
	#pragma HLS INTERFACE axis register port=rxData name=s_axis_rx_data

	#pragma HLS INTERFACE axis register port=openConnection name=m_axis_open_connection
	#pragma HLS INTERFACE axis register port=openConStatus name=s_axis_open_status
	#pragma HLS aggregate  variable=openConnection compact=bit
	#pragma HLS aggregate  variable=openConStatus compact=bit

	#pragma HLS INTERFACE axis register port=closeConnection name=m_axis_close_connection

	#pragma HLS INTERFACE axis register port=txMetaData name=m_axis_tx_metadata
	#pragma HLS INTERFACE axis register port=txData name=m_axis_tx_data
	#pragma HLS INTERFACE axis register port=txStatus name=s_axis_tx_status
	#pragma HLS aggregate  variable=txMetaData compact=bit
	#pragma HLS aggregate  variable=txStatus compact=bit

	#pragma HLS INTERFACE ap_none register port=runExperiment
	#pragma HLS INTERFACE ap_none register port=dualModeEn
	#pragma HLS INTERFACE ap_none register port=useConn
	#pragma HLS INTERFACE ap_none register port=pkgWordCount
	#pragma HLS INTERFACE ap_none register port=packetGap
	#pragma HLS INTERFACE ap_none register port=timeInSeconds
	#pragma HLS INTERFACE ap_none register port=timeInCycles
	#pragma HLS INTERFACE ap_none register port=regIpAddress0
	#pragma HLS INTERFACE ap_none register port=regIpAddress1
	#pragma HLS INTERFACE ap_none register port=regIpAddress2
	#pragma HLS INTERFACE ap_none register port=regIpAddress3
	#pragma HLS INTERFACE ap_none register port=regIpAddress4
	#pragma HLS INTERFACE ap_none register port=regIpAddress5
	#pragma HLS INTERFACE ap_none register port=regIpAddress6
	#pragma HLS INTERFACE ap_none register port=regIpAddress7
	#pragma HLS INTERFACE ap_none register port=regIpAddress8
	#pragma HLS INTERFACE ap_none register port=regIpAddress9

	static hls::stream<bool>		startSignalFifo("startSignalFifo");
	static hls::stream<bool>		stopSignalFifo("stopSignalFifo");
	#pragma HLS STREAM variable=startSignalFifo depth=2
	#pragma HLS STREAM variable=stopSignalFifo depth=2

	//This is required to buffer up to 1024 reponses => supporting up to 1024 connections
	static hls::stream<internalAppTxRsp>	txStatusBuffer("txStatusBuffer");
	#pragma HLS STREAM variable=txStatusBuffer depth=1024

	/*
	 * Client
	 */
	status_handler(txStatus, txStatusBuffer);

	client<DATA_WIDTH>(	openConnection,
			openConStatus,
			closeConnection,
			txMetaData,
			txData,
			txStatusBuffer,
			startSignalFifo,
			stopSignalFifo,
			runExperiment,
			dualModeEn,
			useConn,
			pkgWordCount,
			packetGap,
			timeInSeconds,
			regIpAddress0,
			regIpAddress1,
			regIpAddress2,
			regIpAddress3,
			regIpAddress4,
			regIpAddress5,
			regIpAddress6,
			regIpAddress7,
			regIpAddress8,
			regIpAddress9);

	/*
	 * Server
	 */
	server<DATA_WIDTH>(	listenPort,
			listenPortStatus,
			notifications,
			readRequest,
			rxMetaData,
			rxData);

	/*
	 * Clock
	 */
	clock( startSignalFifo,
			stopSignalFifo,
			timeInCycles);
}
