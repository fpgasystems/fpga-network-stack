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

int main()
{

	hls::stream<ap_uint<16> > listenPort("listenPort");
	hls::stream<bool> listenPortStatus("listenPortStatus");
	hls::stream<appNotification> notifications;
	hls::stream<appReadRequest> readRequest;
	hls::stream<ap_uint<16> > rxMetaData;
	hls::stream<net_axis<64> > rxData;
	hls::stream<ipTuple> openConnection;
	hls::stream<openStatus> openConStatus;
	hls::stream<ap_uint<16> > closeConnection;
	hls::stream<appTxMeta> txMetaData;
	hls::stream<net_axis<64> > txData;
	hls::stream<appTxRsp>	txStatus;

	int count = 0;
	int portOpened = -1;
	while (count < 50)
	{
		echo_server_application(	listenPort, listenPortStatus,
									notifications, readRequest,
									rxMetaData, rxData,
									openConnection, openConStatus,
									closeConnection,
									txMetaData, txData,
									txStatus);
		if (!listenPort.empty())
		{
			ap_uint<16> port = listenPort.read();
			std::cout << std::dec << "opening port: " << port << std::endl;
			listenPortStatus.write(true);
			portOpened = 0;
		}
		count++;
	}
	return portOpened;
}
