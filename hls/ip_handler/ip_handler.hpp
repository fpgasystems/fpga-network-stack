/*
 * Copyright (c) 2019, Systems Group, ETH Zurich
 * Copyright (c) 2016, Xilinx, Inc.
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
#include "../axi_utils.hpp"

const uint16_t ARP = 0x0806;
const uint16_t IPv4 = 0x0800;
const uint16_t IPv6 = 0x86DD;

const uint8_t ICMP = 0x01;
const uint8_t ICMPv6 = 0x3a;
const uint8_t UDP = 0x11;
const uint8_t TCP = 0x06;

template <int WIDTH>
void ip_handler(hls::stream<net_axis<WIDTH> >&		s_axis_raw,
				hls::stream<net_axis<WIDTH> >&		m_axis_ARP,
				hls::stream<net_axis<WIDTH> >&		m_axis_ICMPv6,
				hls::stream<net_axis<WIDTH> >&		m_axis_IPv6UDP,
				hls::stream<net_axis<WIDTH> >&		m_axis_ICMP,
				hls::stream<net_axis<WIDTH> >&		m_axis_UDP,
				hls::stream<net_axis<WIDTH> >&		m_axis_TCP,
				hls::stream<net_axis<WIDTH> >&		m_axis_ROCE,
				ap_uint<32>				myIpAddress);

