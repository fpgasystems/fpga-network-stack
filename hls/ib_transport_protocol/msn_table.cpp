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
#include "msn_table.hpp"
#include <rocev2_config.hpp> //defines MAX_QPS

void msn_table(hls::stream<rxMsnReq>&		rxExh2msnTable_upd_req,
					hls::stream<ap_uint<16> >&	txExh2msnTable_req,
					hls::stream<ifMsnReq>&	if2msnTable_init,
					hls::stream<dmaState>&		msnTable2rxExh_rsp,
					hls::stream<txMsnRsp>&	msnTable2txExh_rsp)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	static dmaState  msn_table[MAX_QPS];
	#pragma HLS bind_storage variable=msn_table type=RAM_2P impl=BRAM

	rxMsnReq rxRequest;
	ifMsnReq ifRequest;
	ap_uint<16> qpn;

	//TODO init channel

	if (!rxExh2msnTable_upd_req.empty())
	{
		rxExh2msnTable_upd_req.read(rxRequest);
		if (rxRequest.write)
		{
			msn_table[rxRequest.qpn].msn = rxRequest.msn;
			msn_table[rxRequest.qpn].vaddr = rxRequest.vaddr;
			msn_table[rxRequest.qpn].dma_length= rxRequest.dma_length;
		}
		else
		{
			msnTable2rxExh_rsp.write(dmaState(msn_table[rxRequest.qpn]));
		}
	}
	else if (!txExh2msnTable_req.empty())
	{
		txExh2msnTable_req.read(qpn);
		msnTable2txExh_rsp.write(txMsnRsp(msn_table[qpn].msn, msn_table[qpn].r_key));
	}
	else if (!if2msnTable_init.empty()) //move up??
	{
		std::cout << "MSN init for QPN: " << qpn << std::endl;
		if2msnTable_init.read(ifRequest);
		msn_table[ifRequest.qpn].msn = 0;
		msn_table[ifRequest.qpn].vaddr = 0; //TODO requried?
		msn_table[ifRequest.qpn].dma_length = 0;  //TODO requried?
		msn_table[ifRequest.qpn].r_key = ifRequest.r_key;
	}
}
