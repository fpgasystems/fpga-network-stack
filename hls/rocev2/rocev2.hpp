/*
 * Copyright (c) 2019, Systems Group, ETH Zurich
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#pragma once

#define IP_VERSION 4

#include "../axi_utils.hpp"
#include "../ipv4/ipv4.hpp"
#include "../ipv6/ipv6.hpp"
#include "../udp/udp.hpp"
#include "../ib_transport_protocol/ib_transport_protocol.hpp"
//#include "../pointer_chasing/pointer_chasing.hpp"

#define DISABLE_CRC_CHECK

#if IP_VERSION == 6
typedef ipv6Meta ipMeta;
#else
typedef ipv4Meta ipMeta;
#endif

template <int WIDTH>
void rocev2(hls::stream<net_axis<WIDTH> >&	s_axis_rx_data,
				hls::stream<txMeta>&	s_axis_tx_meta,
				hls::stream<net_axis<WIDTH> >&	s_axis_tx_data,
				hls::stream<net_axis<WIDTH> >&	m_axis_tx_data,
				//Memory
				hls::stream<routedMemCmd>&		m_axis_mem_write_cmd,
				hls::stream<routedMemCmd>&		m_axis_mem_read_cmd,
				hls::stream<routed_net_axis<WIDTH> >&	m_axis_mem_write_data,
				hls::stream<net_axis<WIDTH> >&	s_axis_mem_read_data,
				//Interface
				hls::stream<qpContext>&	s_axis_qp_interface,
				hls::stream<ifConnReq>&	s_axis_qp_conn_interface,
				//Pointer chasing
#if POINTER_CHASING_EN
				hls::stream<ptrChaseMeta>&	m_axis_rx_pcmeta,
				hls::stream<ptrChaseMeta>&	s_axis_tx_pcmeta,
#endif

				ap_uint<128>		reg_ip_address,

				//Debug output
				ap_uint<32>& regCrcDropPkgCount,
				ap_uint<32>& 	 regInvalidPsnDropCount);

