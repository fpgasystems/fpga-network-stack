/************************************************
Copyright (c) 2018, Systems Group, ETH Zurich.
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
#include "iperf_client.hpp"
#include <iostream>

using namespace hls;

void client(	stream<ipTuple>&		openConnection, stream<openStatus>& openConStatus,
				stream<ap_uint<16> >&	closeConnection,
				stream<appTxMeta>&	txMetaData, stream<axiWord>& txData,
				stream<ap_int<17> >&	txStatus,
				stream<bool>&			startSignal,
				stream<bool>&			stopSignal,
				ap_uint<1>		runExperiment,
				ap_uint<1>		dualModeEn,
				ap_uint<14>		useConn,
				ap_uint<8> 		pkgWordCount,
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

	enum iperfFsmStateType {IDLE, INIT_CON, WAIT_INIT, WAIT_CON, INIT_RUN, START_PKG, WRITE_PKG, CHECK_TIME};
	static iperfFsmStateType iperfFsmState = IDLE;
	static ap_uint<16> experimentID[10000];
	#pragma HLS RESOURCE variable=experimentID core=RAM_2P_LUTRAM
	#pragma HLS DEPENDENCE variable=experimentID inter false

	static ap_uint<14> sessionIt = 0;
	static ap_uint<14> closeIt = 0;
	static bool timeOver = false;
	static ap_uint<8> wordCount = 0;
	static ap_uint<2> initWordCount = 0;
	static ap_uint<8> initTimer = 0;
	static ap_uint<4> ipAddressIdx = 0;
	ipTuple openTuple;
	openStatus status;


	axiWord currWord;

	/*
	 * CLIENT FSM
	 */
	switch (iperfFsmState)
	{
	case INIT_CON:
		//if (useConn[sessionIt])
		if (sessionIt < useConn)
		{
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
      //iperfFsmState = WAIT_INIT;
		sessionIt++;
		if (sessionIt == useConn)
		{
			sessionIt = 0;
			iperfFsmState = WAIT_CON;
		}
		break;
   /*case WAIT_INIT:
      initTimer++;
      if (initTimer == 100) {
         initTimer = 0;
         iperfFsmState = INIT_CON;
      }
      break;*/
	case WAIT_CON:
		//if (useConn[sessionIt])
		if (sessionIt < useConn)
		{
			if (!openConStatus.empty())
			{
				openConStatus.read(status);
				if (status.success)
				{
					experimentID[sessionIt] = status.sessionID;
					//validID[sessionIt] = true;
					//sessionIt++;
					/*if (sessionIt == 0)
					{
						startClock = true;
						iperfFsmState = START_PKG;
					}*/
					std::cout << "Connection successfully opened." << std::endl;
				}
				else
				{
					//iperfFsmState = IDLE;
					std::cout << "Connection could not be opened." << std::endl;
				}
				sessionIt++;
				if (sessionIt == useConn) //maybe move outside
				{
					startSignal.write(true);
					sessionIt = 0;
					iperfFsmState = INIT_RUN;
				}
			}
		}
		else
		{
			startSignal.write(true);
			sessionIt = 0;
			iperfFsmState = INIT_RUN;
		}
		break;
	case INIT_RUN:
		//if (useConn[sessionIt])
		if (sessionIt < useConn)
		{
			switch (initWordCount)
			{
			case 0:
				txMetaData.write(appTxMeta(experimentID[sessionIt], 3*8));
				if (dualModeEn == 0)
				{
					currWord.data = 0x0100000000000000;
				}
				else
				{
					currWord.data = 0x0100000001000080; //run now
					//currWord.data = 0x0100000000000080; //run after
				}
				currWord.keep = 0xff;
				currWord.last = 0;
				txData.write(currWord);
				initWordCount++;
				break;
			case 1:
				currWord.data = 0x0000000089130000; //0x1389 -> 5001 listen port for dual test -L
				currWord.keep = 0xff;
				currWord.last = 0;
				txData.write(currWord);
				initWordCount++;
				break;
			case 2:
            //time is specified as -1 * (seconds * 100)
            currWord.data(63,32) = reverse(timeInSeconds);
            currWord.data(31,0) = 0;
				//currWord.data = 0x18fcffff00000000;
				currWord.keep = 0xff;
				currWord.last = 1;
				txData.write(currWord);
				initWordCount = 0;
				sessionIt++;
				if (sessionIt == useConn)
				{
					sessionIt = 0;
					iperfFsmState = START_PKG;
				}
				break;
			}
		}
		else //switch
		{
			sessionIt = 0;
			iperfFsmState = START_PKG;
		}
		break;
	case START_PKG:
		//if (useConn[sessionIt])
		if (sessionIt < useConn)
		{
			txMetaData.write(appTxMeta(experimentID[sessionIt], pkgWordCount*8));
			currWord.data = 0x3736353433323130;
			currWord.keep = 0xff;
			currWord.last = 0;
			txData.write(currWord); //FIXME first ever word needs to be different.
			wordCount = 0;
			sessionIt++;
			iperfFsmState = WRITE_PKG;
		}
		else //should never be here
		{
			sessionIt = 0;
		}
		break;
	case WRITE_PKG:
		wordCount++;
		if (wordCount < pkgWordCount)
		{
			currWord.data = 0x3736353433323130;
			currWord.keep = 0xff;
			currWord.last = (wordCount == (pkgWordCount-1));
			txData.write(currWord);
		}
		else
		{
			iperfFsmState = CHECK_TIME; // Go to wait state instead
		}
		break;
	case CHECK_TIME:
		//if (time == END_TIME)
		if (timeOver)
		{
			if (closeIt < useConn)
			{
				closeConnection.write(experimentID[closeIt]);
				closeIt++;
				if (closeIt == useConn)
				{
					iperfFsmState = IDLE;
				}
			}
			else
			{
				closeIt = 0;
			}
		}
		else
		{
			iperfFsmState = START_PKG;
		}
		break;
	case IDLE:
		//done do nothing
		sessionIt = 0;
		closeIt = 0;
		timeOver = false;
		if (runExperiment)
		{
			iperfFsmState = INIT_CON;
		}
		break;
	}

	//clock
	if (!stopSignal.empty())
	{
		stopSignal.read(timeOver);
	}
	/*if (startClock)
	{
		if (time < END_TIME)
		{
			time++;
		}
	}
	else
	{
		time = 0;
	}*/

	//dummy code
	if (!txStatus.empty())
	{
		txStatus.read();
	}
}

void server(	stream<ap_uint<16> >& listenPort, stream<bool>& listenPortStatus,
				stream<appNotification>& notifications, stream<appReadRequest>& readRequest,
				stream<ap_uint<16> >& rxMetaData, stream<axiWord>& rxData)
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
			axiWord receiveWord = rxData.read();
         if (!receiveWord.last)
         {
			   serverFsmState = CONSUME;
         }
		}
		break;
	case CONSUME:
		if (!rxData.empty())
		{
			axiWord receiveWord = rxData.read();
         if (receiveWord.last)
         {
            serverFsmState = WAIT_PKG;
         }
		}
		break;
	}
}

void clock (stream<bool>&	startSignal,
			stream<bool>&	stopSignal,
         ap_uint<64> timeInCycles)
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


void iperf_client(	stream<ap_uint<16> >& listenPort, stream<bool>& listenPortStatus,
					stream<appNotification>& notifications, stream<appReadRequest>& readRequest,
					stream<ap_uint<16> >& rxMetaData, stream<axiWord>& rxData,
					stream<ipTuple>& openConnection, stream<openStatus>& openConStatus,
					stream<ap_uint<16> >& closeConnection,
					stream<appTxMeta>& txMetaData, stream<axiWord>& txData,
					stream<ap_int<17> >& txStatus,
					ap_uint<1>		runExperiment,
					ap_uint<1>		dualModeEn,
					ap_uint<14>		useConn,
					ap_uint<8>		pkgWordCount,
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
	#pragma HLS DATAFLOW
	#pragma HLS INTERFACE ap_ctrl_none port=return

	#pragma HLS resource core=AXI4Stream variable=listenPort metadata="-bus_bundle m_axis_listen_port"
	#pragma HLS resource core=AXI4Stream variable=listenPortStatus metadata="-bus_bundle s_axis_listen_port_status"
	//#pragma HLS resource core=AXI4Stream variable=closePort metadata="-bus_bundle m_axis_close_port"

	#pragma HLS resource core=AXI4Stream variable=notifications metadata="-bus_bundle s_axis_notifications"
	#pragma HLS resource core=AXI4Stream variable=readRequest metadata="-bus_bundle m_axis_read_package"
	#pragma HLS DATA_PACK variable=notifications
	#pragma HLS DATA_PACK variable=readRequest

	#pragma HLS resource core=AXI4Stream variable=rxMetaData metadata="-bus_bundle s_axis_rx_metadata"
	#pragma HLS resource core=AXI4Stream variable=rxData metadata="-bus_bundle s_axis_rx_data"
	#pragma HLS DATA_PACK variable=rxData

	#pragma HLS resource core=AXI4Stream variable=openConnection metadata="-bus_bundle m_axis_open_connection"
	#pragma HLS resource core=AXI4Stream variable=openConStatus metadata="-bus_bundle s_axis_open_status"
	#pragma HLS DATA_PACK variable=openConnection
	#pragma HLS DATA_PACK variable=openConStatus

	#pragma HLS resource core=AXI4Stream variable=closeConnection metadata="-bus_bundle m_axis_close_connection"

	#pragma HLS resource core=AXI4Stream variable=txMetaData metadata="-bus_bundle m_axis_tx_metadata"
	#pragma HLS resource core=AXI4Stream variable=txData metadata="-bus_bundle m_axis_tx_data"
	#pragma HLS resource core=AXI4Stream variable=txStatus metadata="-bus_bundle s_axis_tx_status"
	#pragma HLS DATA_PACK variable=txMetaData
	#pragma HLS DATA_PACK variable=txData

	#pragma HLS INTERFACE ap_none register port=runExperiment
	#pragma HLS INTERFACE ap_none register port=dualModeEn
   #pragma HLS INTERFACE ap_none register port=useConn
   #pragma HLS INTERFACE ap_none register port=pkgWordCount
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

	static stream<bool>		startSignalFifo("startSignalFifo");
	static stream<bool>		stopSignalFifo("stopSignalFifo");
	#pragma HLS STREAM variable=startSignalFifo depth=2
	#pragma HLS STREAM variable=stopSignalFifo depth=2

	/*
	 * Client
	 */
	client(	openConnection,
			openConStatus,
			closeConnection,
			txMetaData,
			txData,
			txStatus,
			startSignalFifo,
			stopSignalFifo,
			runExperiment,
			dualModeEn,
			useConn,
			pkgWordCount,
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
	server(	listenPort,
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
