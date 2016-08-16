/************************************************
Copyright (c) 2016, Xilinx, Inc.
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
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.// Copyright (c) 2015 Xilinx, Inc.
************************************************/
#include "echo_server_application.hpp"

using namespace hls;

/** @ingroup echo_server_application
 *
 */
void echo_server_application(stream<ap_uint<16> >& listenPort, stream<bool>& listenPortStatus,
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

	//static stream<ap_uint<16> > ea_lengths("ea_lengths");
	//static bool ea_writePkgOut = false;
	static bool listenDone = false;
	static bool waitPortStatus = false;

	appNotification notification;
	ap_uint<16> sessionID;
	ap_uint<16> length;
	//txMetaData txMeta;

	axiWord currWord;

	openStatus newConn;
	ipTuple tuple;
	// Dummy code should never be executed, this is necessary because every streams has to be written/read
	if (!openConStatus.empty())
	{
		openConStatus.read(newConn);
		tuple.ip_address = 0x0a010101;
		tuple.ip_port = 0x3412;
		openConnection.write(tuple);
		if (newConn.success)
		{
			closeConnection.write(newConn.sessionID);
			//closePort.write(tuple.ip_port);
		}
	}

	// Open/Listen on Port at startup
	if (!listenDone && !waitPortStatus)
	{
#ifndef __SYNTHESIS__
		listenPort.write(5001);
		//listenPort.write(7);
#else
		//listenPort.write(11213);
		listenPort.write(5001);
#endif
		//listenPort.write(80);
		waitPortStatus = true;
	}
	// Check if listening on Port was successful, otherwise try again
	else if (waitPortStatus && !listenPortStatus.empty())
	{
		listenPortStatus.read(listenDone);
		waitPortStatus = false;
	}

	// Receive notifications, about new data which is available
	if (!notifications.empty())
	{
		notifications.read(notification);
		std::cout << notification.ipAddress << "\t" << notification.dstPort << std::endl;
		if (notification.length != 0)
		{
			readRequest.write(appReadRequest(notification.sessionID, notification.length));
			//ea_lengths.write(notification.length);
		}
	}

	//currWord.last = 0;
	// Reads new data from memory and writes it back into memory
	// Read & write metadata only once per package
	static ap_uint<1> ea_fsmState = 0;
	//static ap_uint<16> ea_dataSessionID;

	switch (ea_fsmState)
	{
	case 0:
		if (!rxMetaData.empty())
		{
			rxMetaData.read(sessionID);
			txMetaData.write(sessionID);
			ea_fsmState++;
		}
		break;
	case 1:
		if (!rxData.empty())
		{
			rxData.read(currWord);
			txData.write(currWord);
			if (currWord.last)
			{
				ea_fsmState++;
			}
		}
		break;
	}
	/*if (!rxData.empty()){
		if (!rxMetaData.empty() && !ea_writePkgOut)
		{
			rxMetaData.read(sessionID);
			rxData.read(currWord);
			//ea_lengths.read(length);
			txMetaData.write(sessionID);
			txData.write(currWord);
			ea_writePkgOut = true;
		}
		// Write the rest of the package
		else if(ea_writePkgOut)
		{
			rxData.read(currWord);
			txData.write(currWord);
		}
	}
	// Check for end of incoming package
	if (ea_writePkgOut && currWord.last)
	{
		ea_writePkgOut = false;
	}*/

	if (!txStatus.empty()) //Make Checks
	{
		txStatus.read();
	}
}
