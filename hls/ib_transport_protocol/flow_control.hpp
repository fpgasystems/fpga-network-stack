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

#include "../../axi_utils.hpp"
#include "../ib_transport_protocol.hpp"
#include "hls_math.h"
using namespace hls;

const uint32_t FLOW_ORDER = 5;
const uint32_t FLOW_OUTSTANDING = 32;//exp2(FLOW_OUTSTANDING_ORDER);

struct flowTableEntry
{
	ap_uint<FLOW_ORDER> head;
    ap_uint<FLOW_ORDER> tail; 
    ap_uint<1> issued;
    ap_uint<FLOW_OUTSTANDING> rd_active;
    ap_uint<24> curr_ssn;
    ap_uint<24> curr_msn;
};

template <int INSTID = 0>
void flow_control(	
    stream<flowUpdReq>&	req2flow_upd,
    stream<flowUpdAck>&	ack2flow_upd,
    stream<flowTableEntry>& flow2req_rsp,
    stream<flowTableEntry>&	flow2ack_rsp
) {
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	static flowTableEntry flow_table[MAX_QPS];
#if defined( __VITIS_HLS__)
	#pragma HLS bind_storage variable=flow_table type=RAM_2P impl=BRAM
#else
	#pragma HLS RESOURCE variable=flow_table core=RAM_2P_BRAM
#endif

    flowUpdAck ackUpd;

    if (!ack2flow_upd.empty()) {
        ack2flow_upd.read(ackUpd);
        if(ackUpd.write) {
            flow_table[ackUpd.qpn].tail = ackUpd.tail;
            flow_table[ackUpd.qpn].curr_msn = ackUpd.curr_msn;
            if(ackUpd.tail == flow_table[ackUpd.qpn].head)
                flow_table[ackUpd.qpn].issued = 0;
        } else {
            flow2ack_rsp.write(flow_table[ackUpd.qpn]);
        }
    } else if (!req2flow_upd.empty()) {
        req2flow_upd.read(reqUpd);
        if(reqUpd.write) {

        } else {

        }
    }
}