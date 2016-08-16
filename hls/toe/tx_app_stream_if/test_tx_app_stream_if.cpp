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
#include "tx_app_stream_if.hpp"
#include <iostream>


using namespace hls;

void simStateTable(stream<stateQuery>& req, stream<sessionState>& rsp)
{
	if (!req.empty())
	{
		req.read();
		rsp.write(ESTABLISHED);
	}
}

void simTxSar(stream<txAppTxSarQuery>& req, stream<txAppTxSarPush>& push, stream<txAppTxSarReply>& rsp)
{
	txAppTxSarQuery query;
	if (!req.empty())
	{
		req.read(query);
		if (!query.write)
		{
			rsp.write(txAppTxSarReply(query.sessionID, 0xad01, 0x2911));
		}
	}

	if (!push.empty())
	{
		push.read();
	}
}


int main(int argc, char* argv[])
{
	stream<ap_uint<16> >			appTxDataReqMetaData;
	stream<axiWord>					appTxDataReq("appTxDataReq");
	stream<sessionState>			stateTable2txApp_rsp;
	stream<txAppTxSarReply>			txSar2txApp_upd_rsp; //TODO rename
	stream<mmStatus>				txBufferWriteStatus;
	stream<ap_int<17> >				appTxDataRsp;
	stream<stateQuery>				txApp2stateTable_req; //make ap_uint<16>
	stream<txAppTxSarQuery>			txApp2txSar_upd_req; //TODO rename
	stream<mmCmd>					txBufferWriteCmd;
	stream<axiWord>					txBufferWriteData;
	stream<txAppTxSarPush>			txApp2txSar_app_push;
	stream<event>					txAppStream2eventEng_setEvent;


	axiWord input;
	axiWord output;

	//initial
	ap_uint<16> sessionID = 3;
	appTxDataReqMetaData.write(sessionID);
	input.data = 2353456;
	input.keep = 0xff;
	input.last = 0;
	appTxDataReq.write(input);
	input.data = 908348249;
	input.keep = 0x3f;
	input.last = 1;
	appTxDataReq.write(input);

	event ev;
	ap_int<17>  returnCode;
	int count = 0;
	int replyCount;
	while (count < 500)
	{
		tx_app_stream_if(	appTxDataReqMetaData,
							appTxDataReq,
							stateTable2txApp_rsp,
							txSar2txApp_upd_rsp,
							txBufferWriteStatus,
							appTxDataRsp,
							txApp2stateTable_req,
							txApp2txSar_upd_req,
							txBufferWriteCmd,
							txBufferWriteData,
							txApp2txSar_app_push,
							txAppStream2eventEng_setEvent);
		simStateTable(txApp2stateTable_req, stateTable2txApp_rsp);
		simTxSar(txApp2txSar_upd_req, txApp2txSar_app_push, txSar2txApp_upd_rsp);

		if (!txBufferWriteCmd.empty())
		{
			txBufferWriteCmd.read();
			replyCount = count + 200;
		}
		if (!txBufferWriteData.empty())
		{
			txBufferWriteData.read(output);
			std::cout << output.data << "\t" << output.keep << "\t" << output.last << std::endl;
		}
		if (!txAppStream2eventEng_setEvent.empty())
		{
			txAppStream2eventEng_setEvent.read(ev);
			std::cout << "Event: " << ev.sessionID << " " <<  ev.type <<  std::endl;
		}
		if (!appTxDataRsp.empty())
		{
			appTxDataRsp.read(returnCode);
			std::cout << "Return: " << returnCode <<  std::endl;
		}
		if (count == replyCount)
		{
			txBufferWriteStatus.write(mmStatus({0, 0, 0, 0, 1}));
		}
		count++;
	}


	return 0;
}
