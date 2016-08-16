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
#include "rx_app_stream_if.hpp"
#include <iostream>

using namespace hls;


int main()
{
	stream<appReadRequest>		appRxDataReq;
	stream<rxSarAppd>			rxSar2rxApp_upd_rsp;
	stream<ap_uint<16> >		appRxDataRspMetadata;
	stream<rxSarAppd>			rxApp2rxSar_upd_req;
	stream<mmCmd>				rxBufferReadCmd;

	rxSarAppd req;
	mmCmd cmd;
	ap_uint<16> meta;

	int count = 0;
	while (count < 50)
	{
		rx_app_stream_if(	appRxDataReq,
							rxSar2rxApp_upd_rsp,
							appRxDataRspMetadata,
							rxApp2rxSar_upd_req,
							rxBufferReadCmd);
		if (!rxApp2rxSar_upd_req.empty())
		{
			rxApp2rxSar_upd_req.read(req);
			if (!req.write)
			{
				req.appd = 2435;
				rxSar2rxApp_upd_rsp.write(req);
			}
		}

		if (!rxBufferReadCmd.empty())
		{
			rxBufferReadCmd.read(cmd);
			std::cout << "Cmd: " << cmd.saddr << std::endl;
		}
		if (!appRxDataRspMetadata.empty())
		{
			appRxDataRspMetadata.read(meta);
			std::cout << "Meta: " << meta << std::endl;
		}

		if (count == 20)
		{
			appRxDataReq.write(appReadRequest(25, 89));

		}
		count++;
	}
	return 0;
}
