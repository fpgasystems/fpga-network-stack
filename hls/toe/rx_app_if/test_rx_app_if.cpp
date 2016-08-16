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
#include "rx_app_if.hpp"
#include <iostream>

using namespace hls;

int main()
{
	stream<ap_uint<16> >				appListenPortReq;
	stream<bool>						portTable2rxApp_listen_rsp;
	stream<bool>						appListenPortRsp;
	stream<ap_uint<16> >				rxApp2porTable_listen_req;

	bool response;
	int count = 0;
	while (count < 50)
	{
		rx_app_if(	appListenPortReq,
					portTable2rxApp_listen_rsp,
					appListenPortRsp,
					rxApp2porTable_listen_req);
		if (!rxApp2porTable_listen_req.empty())
		{
			rxApp2porTable_listen_req.read();
			portTable2rxApp_listen_rsp.write(true);
		}

		if (count == 20)
		{
			appListenPortReq.write(80);
		}
		if (!appListenPortRsp.empty())
		{
			appListenPortRsp.read(response);
			std::cout << "Response: " << ((response) ? "true" : "false") << std::endl;
		}


		count++;
	}
	return 0;
}
