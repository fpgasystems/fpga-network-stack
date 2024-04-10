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
#include "../ib_transport_protocol/ib_transport_protocol.hpp"
#include <rocev2_config.hpp> //defines MAX_QPS

//PSN, page 293, 307, 345
struct stateTableEntry
{
	//window
	ap_uint<24> resp_epsn;
	ap_uint<24> resp_old_outstanding;

	ap_uint<24> req_next_psn;
	ap_uint<24> req_old_unack;
	ap_uint<24> req_old_valid; //required? can be computed?
	ap_uint<3>	retryCounter;
};

struct ifStateReq
{
	ap_uint<16> qpn;
	qpState		newState;
	ap_uint<24> remote_psn;
	ap_uint<24> local_psn;
	bool		write;
	ifStateReq() {}
	ifStateReq(ap_uint<24> qpn)
		:qpn(qpn), write(false) {}
	ifStateReq(ap_uint<16> qpn, qpState s, ap_uint<24> rpsn, ap_uint<24> lpsn)
		:qpn(qpn), newState(s), remote_psn(rpsn), local_psn(lpsn), write(true) {}
};

struct rxStateReq
{
	ap_uint<16> qpn;
	ap_uint<24> epsn;
	ap_uint<3>	retryCounter;
	bool		isResponse;
	bool		write;
	rxStateReq() :isResponse(false), write(false) {}
	rxStateReq(ap_uint<24> qpn, bool isRsp)
		:qpn(qpn), isResponse(isRsp), write(false) {}
	rxStateReq(ap_uint<16> qpn, ap_uint<24> psn, bool isRsp)
		:qpn(qpn), epsn(psn), isResponse(isRsp), retryCounter(0x7), write(true) {}
	rxStateReq(ap_uint<16> qpn, ap_uint<24> psn, ap_uint<3> rc, bool isRsp)
		:qpn(qpn), epsn(psn), isResponse(isRsp), retryCounter(rc), write(true) {}
};


struct rxStateRsp
{
	//window
	ap_uint<24> epsn;
	ap_uint<24> oldest_outstanding_psn;
	ap_uint<24> max_forward; //used for reponses, page 346

	ap_uint<3>	retryCounter;

	rxStateRsp() {}
	rxStateRsp(ap_uint<24> epsn, ap_uint<24> old)
		:epsn(epsn), oldest_outstanding_psn(old), max_forward(0), retryCounter(0) {}
	rxStateRsp(ap_uint<24> epsn, ap_uint<24> old, ap_uint<24> maxf)
		:epsn(epsn), oldest_outstanding_psn(old), max_forward(maxf), retryCounter(0) {}
	rxStateRsp(ap_uint<24> epsn, ap_uint<24> old, ap_uint<24> maxf, ap_uint<3> rc)
		:epsn(epsn), oldest_outstanding_psn(old), max_forward(maxf), retryCounter(rc) {}
};

struct txStateReq
{
	ap_uint<16> qpn;
	ap_uint<24> psn;
	bool		write;
	txStateReq() :write(false) {}
	txStateReq(ap_uint<24> qpn)
		:qpn(qpn), write(false) {}
	txStateReq(ap_uint<16> qpn, ap_uint<24> psn)
		:qpn(qpn), psn(psn), write(true) {}
};

template <int INSTID>
void state_table(	hls::stream<rxStateReq>& rxIbh2stateTable_upd_req,
						hls::stream<txStateReq>& txIbh2stateTable_upd_req,
						hls::stream<ifStateReq>& qpi2stateTable_upd_req,
						hls::stream<rxStateRsp>& stateTable2rxIbh_rsp,
						hls::stream<stateTableEntry>& stateTable2txIbh_rsp,
						hls::stream<stateTableEntry>& stateTable2qpi_rsp);


template <int INSTID = 0>
void state_table(		
	hls::stream<rxStateReq>& rxIbh2stateTable_upd_req,
	hls::stream<txStateReq>& txIbh2stateTable_upd_req,
	hls::stream<ifStateReq>& qpi2stateTable_upd_req,
	hls::stream<rxStateRsp>& stateTable2rxIbh_rsp,
	hls::stream<stateTableEntry>& stateTable2txIbh_rsp,
	hls::stream<stateTableEntry>& stateTable2qpi_rsp
) {
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	static stateTableEntry state_table[MAX_QPS];
#if defined( __VITIS_HLS__)
	#pragma HLS bind_storage variable=state_table type=RAM_2P impl=BRAM
#else
	#pragma HLS RESOURCE variable=state_table core=RAM_2P_BRAM
#endif

	rxStateReq rxRequest;
	txStateReq txRequest;
	ifStateReq ifRequest;

	if (!rxIbh2stateTable_upd_req.empty())
	{
		rxIbh2stateTable_upd_req.read(rxRequest);
		if (rxRequest.write)
		{
			if (rxRequest.isResponse)
			{
				state_table[rxRequest.qpn].req_old_unack = rxRequest.epsn;
			}
			else
			{
				state_table[rxRequest.qpn].resp_epsn = rxRequest.epsn;
				state_table[rxRequest.qpn].retryCounter = rxRequest.retryCounter;
				//state_table[rxRequest.qpn].sendNAK = rxRequest.epsn;
			}
		}
		else
		{
			stateTableEntry entry = state_table[rxRequest.qpn(15,0)];
			if (rxRequest.isResponse)
			{
				stateTable2rxIbh_rsp.write(rxStateRsp(entry.req_old_unack, entry.req_old_valid, entry.req_next_psn-1));
			}
			else
			{
				stateTable2rxIbh_rsp.write(rxStateRsp(entry.resp_epsn, entry.resp_old_outstanding, entry.resp_epsn, entry.retryCounter));
			}
		}
	}
	else if (!txIbh2stateTable_upd_req.empty())
	{
		txIbh2stateTable_upd_req.read(txRequest);
		if (txRequest.write)
		{
			state_table[txRequest.qpn].req_next_psn = txRequest.psn;
		}
		else
		{
			stateTable2txIbh_rsp.write(state_table[txRequest.qpn]);
		}
	}
	else if (!qpi2stateTable_upd_req.empty())
	{
		qpi2stateTable_upd_req.read(ifRequest);
		if (ifRequest.write)
		{
			std::cout << std::hex << "[STATE TABLE " << INSTID << "]: setup new connection, psn " << ifRequest.remote_psn << std::endl;
			//state_table[ifRequest.qpn].state = ifRequest.newState;
			//state_table[ifRequest.qpn].prevOpCode = RC_RDMA_WRITE_LAST;
			state_table[ifRequest.qpn].resp_epsn = ifRequest.local_psn;
			state_table[ifRequest.qpn].resp_old_outstanding = ifRequest.local_psn;
			state_table[ifRequest.qpn].req_next_psn = ifRequest.remote_psn;
			state_table[ifRequest.qpn].req_old_unack = ifRequest.remote_psn;
			state_table[ifRequest.qpn].req_old_valid = ifRequest.remote_psn;
			state_table[ifRequest.qpn].retryCounter = 0xF;
			//state_table[ifRequest.qpn].sendNAK = false;

			//state_table[ifRequest.qpn].r_key = ifRequest.r_key;
			//state_table[ifRequest.qpn].virtual_address = ifRequest.virtual_address;
		}
		else
		{
			stateTable2qpi_rsp.write(state_table[ifRequest.qpn(15,0)]);
		}
	}

}
