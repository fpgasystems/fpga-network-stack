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
				stream<ap_uint<16> >&	txMetaData, stream<axiWord>& txData,
				stream<ap_int<17> >&	txStatus,
				stream<bool>&			startSignal,
				stream<bool>&			stopSignal,
				ap_uint<1>		runExperiment,
				ap_uint<1>		dualModeEn,
				ap_uint<14>		useConn,
				ap_uint<8> 		pkgWordCount,
				ap_uint<32>		regIpAddress0,
				ap_uint<32>		regIpAddress1,
				ap_uint<32>		regIpAddress2,
				ap_uint<32>		regIpAddress3)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	enum iperfFsmStateType {IDLE, INIT_CON, WAIT_CON, INIT_RUN, START_PKG, WRITE_PKG, CHECK_TIME};
	static iperfFsmStateType iperfFsmState = IDLE;
	static ap_uint<16> experimentID[10000];
	#pragma HLS RESOURCE variable=experimentID core=RAM_2P_LUTRAM
	#pragma HLS DEPENDENCE variable=experimentID inter false

	static ap_uint<14> sessionIt = 0;
	static ap_uint<14> closeIt = 0;
	static bool timeOver = false;
	static ap_uint<8> wordCount = 0;
	static ap_uint<2> initWordCount = 0;
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
			switch (sessionIt(1,0))
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
			}
			openTuple.ip_port = 5001;
			openConnection.write(openTuple);
		}
		sessionIt++;
		if (sessionIt == useConn)
		{
			sessionIt = 0;
			iperfFsmState = WAIT_CON;
		}
		break;
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
				txMetaData.write(experimentID[sessionIt]);
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
				currWord.data = 0x0000000089130000;
				currWord.keep = 0xff;
				currWord.last = 0;
				txData.write(currWord);
				initWordCount++;
				break;
			case 2:
				currWord.data = 0x18fcffff00000000;
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
			txMetaData.write(experimentID[sessionIt]);
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
		if (wordCount < pkgWordCount)
		{
			currWord.data = 0x3736353433323130;
			currWord.keep = 0xff;
			currWord.last = (wordCount == (pkgWordCount-1));
			txData.write(currWord);
			wordCount++;
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

	enum consumeFsmStateType {WAIT_PKG, CONSUME};
	static consumeFsmStateType  serverFsmState = WAIT_PKG;
	static ap_uint<1> listenFsm = 0;
	static bool listenDone = false;

	appNotification notification;
	axiWord receiveWord;

	// Open Port 5001
	if (!listenDone)
	{
		switch (listenFsm)
		{
		case 0:
			listenPort.write(5001);
			listenFsm++;
			break;
		case 1:
			if (!listenPortStatus.empty())
			{
				listenPortStatus.read(listenDone);
				listenFsm++;
			}
			break;
		}
	}

	if (!notifications.empty() && !readRequest.full())
	{
		notifications.read(notification);

		if (notification.length != 0)
		{
			readRequest.write(appReadRequest(notification.sessionID, notification.length));
		}
	}
	receiveWord.last = 0;
	switch (serverFsmState)
	{
	case WAIT_PKG:
		if (!rxMetaData.empty() && !rxData.empty())
		{
			rxMetaData.read();
			rxData.read(receiveWord);
			serverFsmState = CONSUME;
		}
		break;
	case CONSUME:
		if (!rxData.empty())
		{
			rxData.read(receiveWord);
		}
		break;
	}
	if (receiveWord.last == 1)
	{
		serverFsmState = WAIT_PKG;
	}
}

void clock (stream<bool>&	startSignal,
			stream<bool>&	stopSignal)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	static bool startClock = false;
	static ap_uint<40> time;

	if (startClock)
	{
		if (time < END_TIME_120)
		{
			time++;
		}
		else
		{
			startClock = false;
			stopSignal.write(true);
		}
	}
	else
	{
		time = 0;
	}

	if (!startSignal.empty())
	{
		startSignal.read(startClock);
	}
}
void iperf_client(	stream<ap_uint<16> >& listenPort, stream<bool>& listenPortStatus,
					stream<appNotification>& notifications, stream<appReadRequest>& readRequest,
					stream<ap_uint<16> >& rxMetaData, stream<axiWord>& rxData,
					stream<ipTuple>& openConnection, stream<openStatus>& openConStatus,
					stream<ap_uint<16> >& closeConnection,
					stream<ap_uint<16> >& txMetaData, stream<axiWord>& txData,
					stream<ap_int<17> >& txStatus,
					ap_uint<1>		runExperiment,
					ap_uint<1>		dualModeEn,
					ap_uint<14>		useConn,
					ap_uint<8>		pkgWordCount,
					ap_uint<32>		regIpAddress0,
					ap_uint<32>		regIpAddress1,
					ap_uint<32>		regIpAddress2,
					ap_uint<32>		regIpAddress3)
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

	#pragma HLS INTERFACE ap_stable register port=runExperiment
	#pragma HLS INTERFACE ap_stable register port=dualModeEn
	#pragma HLS INTERFACE ap_stable register port=regIpAddress0
	#pragma HLS INTERFACE ap_stable register port=regIpAddress1
	#pragma HLS INTERFACE ap_stable register port=regIpAddress2
	#pragma HLS INTERFACE ap_stable register port=regIpAddress3

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
			regIpAddress0,
			regIpAddress1,
			regIpAddress2,
			regIpAddress3);

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
			stopSignalFifo);
}
