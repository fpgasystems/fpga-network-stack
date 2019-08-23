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

void read_req_table(stream<txReadReqUpdate>&	tx_readReqTable_upd,
#if !RETRANS_EN
					stream<rxReadReqUpdate>&	rx_readReqTable_upd_req);
#else
					stream<rxReadReqUpdate>&	rx_readReqTable_upd_req,
					stream<rxReadReqRsp>&		rx_readReqTable_upd_rsp);
#endif