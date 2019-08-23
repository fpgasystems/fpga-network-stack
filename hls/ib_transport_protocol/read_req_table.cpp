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
#include "read_req_table.hpp"
#include <rocev2_config.hpp> //defines MAX_QPS

void read_req_table(stream<txReadReqUpdate>&	tx_readReqTable_upd,
#if !RETRANS_EN
					stream<rxReadReqUpdate>&	rx_readReqTable_upd_req)
#else
					stream<rxReadReqUpdate>&	rx_readReqTable_upd_req,
					stream<rxReadReqRsp>&		rx_readReqTable_upd_rsp)
#endif
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	static readReqTableEntry  req_table[MAX_QPS];
	#pragma HLS RESOURCE variable=req_table core=RAM_2P_BRAM

	txReadReqUpdate update;
	rxReadReqUpdate request;

	if (!tx_readReqTable_upd.empty())
	{
		tx_readReqTable_upd.read(update);
		req_table[update.qpn].max_fwd_readreq = update.max_fwd_readreq;
	}
	else if (!rx_readReqTable_upd_req.empty())
	{
		rx_readReqTable_upd_req.read(request);
		if (request.write)
		{
			req_table[request.qpn].oldest_outstanding_readreq = request.oldest_outstanding_readreq;
		}
#if RETRANS_EN
		else
		{
			bool valid = (req_table[request.qpn].oldest_outstanding_readreq < req_table[request.qpn].max_fwd_readreq);
			rx_readReqTable_upd_rsp.write(rxReadReqRsp(req_table[request.qpn].oldest_outstanding_readreq, valid));
		}
#endif
	}
}
