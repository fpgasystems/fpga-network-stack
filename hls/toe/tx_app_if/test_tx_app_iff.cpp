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
#include "tx_app_if.hpp"
#include <iostream>

using namespace hls;

int main()
{
	stream<ipTuple>					appOpenConnReq;
	stream<ap_uint<16> >			closeConnReq;
	stream<sessionLookupReply>		sLookup2txApp_rsp;
	stream<ap_uint<16> >			portTable2txApp_port_rsp;
	stream<sessionState>			stateTable2txApp_upd_rsp;
	stream<openStatus>				conEstablishedIn; //alter
	stream<openStatus>				appOpenConnRsp;
	stream<fourTuple>				txApp2sLookup_req;
	//stream<ap_uint<1> >&			txApp2portTable_port_req;
	stream<stateQuery>				txApp2stateTable_upd_req;
	stream<event>					txApp2eventEng_setEvent;

	portTable2txApp_port_rsp.write(32768);
	stateQuery query;
	extendedEvent ev;
	openStatus status;
	int count = 0;
	while (count < 500)
	{
		if (count == 10)
		{
			appOpenConnReq.write(ipTuple({0x0a010101, 5001}));
		}
		if (count == 400)
		{
			closeConnReq.write(0);
		}
		tx_app_if(	appOpenConnReq,
					closeConnReq,
					sLookup2txApp_rsp,
					portTable2txApp_port_rsp,
					stateTable2txApp_upd_rsp,
					conEstablishedIn, //alter
					appOpenConnRsp,
					txApp2sLookup_req,
					//stream<ap_uint<1> >&			txApp2portTable_port_req,
					txApp2stateTable_upd_req,
					txApp2eventEng_setEvent);
		if (!txApp2sLookup_req.empty())
		{
			txApp2sLookup_req.read();
			sLookup2txApp_rsp.write(sessionLookupReply(0, 1));
		}
		if (!txApp2stateTable_upd_req.empty())
		{
			txApp2stateTable_upd_req.read(query);
			if (query.write)
			{
				std::cout << "state update: " << query.state << std::endl;
			}
			else
			{
				std::cout << "state request" << std::endl;
				//TODO check others
				stateTable2txApp_upd_rsp.write(ESTABLISHED);
			}

		}
		if (!txApp2eventEng_setEvent.empty())
		{
			txApp2eventEng_setEvent.read(ev);
			std::cout << "ev type: "<< ev.type << std::endl;
		}
		if (!appOpenConnRsp.empty())
		{
			appOpenConnRsp.read(status);
			std::cout << "open con response: " << status.success <<  std::endl;
		}
		count++;
	}
	return 0;
}
