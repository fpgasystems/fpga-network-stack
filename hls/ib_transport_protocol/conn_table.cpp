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
#include "conn_table.hpp"
#include <rocev2_config.hpp> //defines MAX_QPS

void conn_table(	hls::stream<ap_uint<16> >&	tx_ibhconnTable_req,
						hls::stream<ifConnReq>&		qpi2connTable_req,
						hls::stream<connTableEntry>&	tx_connTable2ibh_rsp)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	static connTableEntry conn_table[MAX_QPS];
	//#pragma HLS bind_storage variable=conn_table type=RAM_2P impl=BRAM

	ap_uint<16> txRequest;
	ifConnReq ifRequest;

	if (!tx_ibhconnTable_req.empty())
	{
		tx_ibhconnTable_req.read(txRequest);
		//std::cout << "Requested conn data for: " << txRequest << std::endl;
		tx_connTable2ibh_rsp.write(conn_table[txRequest]);
	}
	else if (!qpi2connTable_req.empty())
	{
		qpi2connTable_req.read(ifRequest);
		//std::cout << "Storing conn data for: " << ifRequest.qpn << std::endl;
		conn_table[ifRequest.qpn].remote_qpn = ifRequest.remote_qpn;
		conn_table[ifRequest.qpn].remote_ip_address = ifRequest.remote_ip_address;
		conn_table[ifRequest.qpn].remote_udp_port = ifRequest.remote_udp_port;
	}
}
