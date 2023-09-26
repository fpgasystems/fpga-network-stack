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

#include "iperf.hpp"


void iperf(	stream<ap_uint<16> >& listenPort, stream<bool>& listenPortStatus,
			// This is disabled for the time being, because it adds complexity/potential issues
			//stream<ap_uint<16> >& closePort,
			stream<appNotification>& notifications, stream<appReadRequest>& readRequest,
			stream<ap_uint<16> >& rxMetaData, stream<axiWord>& rxData,
			stream<ipTuple>& openConnection, stream<openStatus>& openConStatus,
			stream<ap_uint<16> >& closeConnection,
			stream<ap_uint<16> >& txMetaData, stream<axiWord>& txData,
			stream<ap_int<17> >& txStatus)
{
	#pragma HLS PIPELINE II=1

	#pragma HLS resource core=AXI4Stream variable=listenPort metadata="-bus_bundle m_axis_listen_port"
	#pragma HLS resource core=AXI4Stream variable=listenPortStatus metadata="-bus_bundle s_axis_listen_port_status"
	//#pragma HLS resource core=AXI4Stream variable=closePort metadata="-bus_bundle m_axis_close_port"

	#pragma HLS resource core=AXI4Stream variable=notifications metadata="-bus_bundle s_axis_notifications"
	#pragma HLS resource core=AXI4Stream variable=readRequest metadata="-bus_bundle m_axis_read_package"
	#pragma HLS aggregate  variable=notifications compact=bit
	#pragma HLS aggregate  variable=readRequest compact=bit

	#pragma HLS resource core=AXI4Stream variable=rxMetaData metadata="-bus_bundle s_axis_rx_metadata"
	#pragma HLS resource core=AXI4Stream variable=rxData metadata="-bus_bundle s_axis_rx_data"
	#pragma HLS aggregate  variable=rxData compact=bit

	#pragma HLS resource core=AXI4Stream variable=openConnection metadata="-bus_bundle m_axis_open_connection"
	#pragma HLS resource core=AXI4Stream variable=openConStatus metadata="-bus_bundle s_axis_open_status"
	#pragma HLS aggregate  variable=openConnection compact=bit
	#pragma HLS aggregate  variable=openConStatus compact=bit

	#pragma HLS resource core=AXI4Stream variable=closeConnection metadata="-bus_bundle m_axis_close_connection"

	#pragma HLS resource core=AXI4Stream variable=txMetaData metadata="-bus_bundle m_axis_tx_metadata"
	#pragma HLS resource core=AXI4Stream variable=txData metadata="-bus_bundle m_axis_tx_data"
	#pragma HLS resource core=AXI4Stream variable=txStatus metadata="-bus_bundle s_axis_tx_status"
	#pragma HLS aggregate  variable=txMetaData compact=bit
	#pragma HLS aggregate  variable=txData compact=bit

	static bool listenDone = false;
	static bool runningExperiment = false;
	static ap_uint<1> listenFsm = 0;

	openStatus newConStatus;
	appNotification notification;
	ipTuple tuple;

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

	axiWord transmitWord;

	// In case we are connecting back
	if (!openConStatus.empty())
	{
		openConStatus.read(newConStatus);
		txMetaData.write(0);
		transmitWord.data = 0x3736353433323130;
		transmitWord.keep = 0xff;
		transmitWord.last = 1;
		txData.write(transmitWord);
		if (newConStatus.success)
		{
			closeConnection.write(newConStatus.sessionID);
		}
	}

	if (!notifications.empty())
	{
		notifications.read(notification);

		if (notification.length != 0)
		{
			readRequest.write(appReadRequest(notification.sessionID, notification.length));
		}
		else // closed
		{
			runningExperiment = false;
		}
	}

	enum consumeFsmStateType {WAIT_PKG, CONSUME, HEADER_2, HEADER_3};
	static consumeFsmStateType  serverFsmState = WAIT_PKG;
	ap_uint<16> sessionID;
	axiWord currWord;
	currWord.last = 0;
	static bool dualTest = false;
	static ap_uint<32> mAmount = 0;

	switch (serverFsmState)
	{
	case WAIT_PKG:
		if (!rxMetaData.empty() && !rxData.empty())
		{
			rxMetaData.read(sessionID);
			rxData.read(currWord);
			if (!runningExperiment)
			{
				if (currWord.data(31, 0) == 0x00000080) // Dual test
				{
					dualTest = true;
				}
				else
				{
					dualTest = false;
				}

				runningExperiment = true;
				serverFsmState = HEADER_2;
			}
			else
			{
				serverFsmState = CONSUME;
			}
		}
		break;
	case HEADER_2:
		if (!rxData.empty())
		{
			rxData.read(currWord);
			if (dualTest)
			{
				tuple.ip_address = 0x0a010101;
				tuple.ip_port = currWord.data(31, 16);
				openConnection.write(tuple);
			}
			serverFsmState = HEADER_3;
		}
		break;
	case HEADER_3:
		if (!rxData.empty())
		{
			rxData.read(currWord);
			mAmount = currWord.data(63, 32);
			serverFsmState = CONSUME;
		}
		break;
	case CONSUME:
		if (!rxData.empty())
		{
			rxData.read(currWord);
		}
		break;
	}
	if (currWord.last == 1)
	{
		serverFsmState = WAIT_PKG;
	}

	if (!txStatus.empty())
	{
		txStatus.read();
	}
}
