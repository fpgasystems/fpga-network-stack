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
#pragma once

#include "../axi_utils.hpp"
#include <rocev2_config.hpp> //defines MAX_QPS
using namespace hls;

struct txReadReqUpdate
{
	ap_uint<16> qpn;
	ap_uint<24> max_fwd_readreq;
	txReadReqUpdate() {}
	txReadReqUpdate(ap_uint<16> qpn, ap_uint<24> maxf)
		:qpn(qpn), max_fwd_readreq(maxf) {}
};

struct rxReadReqUpdate
{
	ap_uint<16> qpn;
	ap_uint<24> oldest_outstanding_readreq;
	bool write;
	rxReadReqUpdate() : write(false) {}
	rxReadReqUpdate(ap_uint<16> qpn)
		:qpn(qpn), oldest_outstanding_readreq(0), write(false) {}
	rxReadReqUpdate(ap_uint<16> qpn, ap_uint<24> old)
		:qpn(qpn), oldest_outstanding_readreq(old), write(true) {}
};

struct rxReadReqRsp
{
	ap_uint<24> oldest_outstanding_readreq;
	bool valid;
	rxReadReqRsp() {}
	rxReadReqRsp(ap_uint<24> old, bool valid)
		:oldest_outstanding_readreq(old), valid(valid) {}

};

struct readReqTableEntry
{
	ap_uint<24> oldest_outstanding_readreq;
	ap_uint<24> max_fwd_readreq;
};

template <int INSTID>
void read_req_table(stream<txReadReqUpdate>&	tx_readReqTable_upd,
#ifdef RETRANS_EN
					stream<rxReadReqUpdate>&	rx_readReqTable_upd_req,
					stream<rxReadReqRsp>&		rx_readReqTable_upd_rsp);
#else
					stream<rxReadReqUpdate>&	rx_readReqTable_upd_req);
#endif

template <int INSTID = 0>
void read_req_table(stream<txReadReqUpdate>&	tx_readReqTable_upd,
#ifdef RETRANS_EN
					stream<rxReadReqUpdate>&	rx_readReqTable_upd_req,
					stream<rxReadReqRsp>&		rx_readReqTable_upd_rsp)
#else
					stream<rxReadReqUpdate>&	rx_readReqTable_upd_req)
#endif
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	static readReqTableEntry  req_table[MAX_QPS];
#if defined( __VITIS_HLS__)
	#pragma HLS bind_storage variable=req_table type=RAM_2P impl=BRAM
#else
	#pragma HLS RESOURCE variable=req_table core=RAM_2P_BRAM
#endif

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
#ifdef RETRANS_EN
		else
		{
			bool valid = (req_table[request.qpn].oldest_outstanding_readreq < req_table[request.qpn].max_fwd_readreq);
			rx_readReqTable_upd_rsp.write(rxReadReqRsp(req_table[request.qpn].oldest_outstanding_readreq, valid));
		}
#endif
	}
}
