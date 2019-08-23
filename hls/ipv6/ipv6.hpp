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

#include "../axi_utils.hpp"
#include "../packet.hpp"

const uint32_t IPV6_HEADER_SIZE = 320;


struct ipv6Meta
{
	ap_uint<128> their_address;
	ap_uint<16> length;
	ap_uint<8>  next_header;
	ipv6Meta() {}
	ipv6Meta(ap_uint<128> addr, ap_uint<16> len, ap_uint<8> next)
		:their_address(addr), length(len), next_header(next) {}
};


/**
 * version = w(3,0);
 * traffic_class = w(11,4);
 * flow_label = w(31,12);
 * payload_length = w(47,32);
 * next_header = w(55,48);
 * hop_limits = w(63,56);
 * src_address = w(191,64);
 * dst_address(63, 0) = w(255,192);
 */
template <int W>
class ipv6Header : public packetHeader<W, IPV6_HEADER_SIZE> {
	using packetHeader<W, IPV6_HEADER_SIZE>::header;

public:
	ipv6Header()
	{
		header(7, 4)  = 6;
		header(55,48) = 0x11;
		header(63,56) = 0xFF; //hop limit
	}
	void setPayloadLen(ap_uint<16> len)
	{
		header(47,32) = reverse(len);
	}
	ap_uint<16> getPayloadLen()
	{
		return reverse((ap_uint<16>)header(47,32));
	}
	void setSrcAddress(const ap_uint<128> address)
	{
		header(191,64)= address;
	}
	ap_uint<128> getSrcAddress()
	{
		return header(191,64);
	}
	void setDstAddress(ap_uint<128> address)
	{
		header(319,192) = address;
	}
	ap_uint<128> getDstAddress()
	{
		return header(319,192);
	}
	void setNextHeader(ap_uint<8> next)
	{
		header(55,48) = next;
	}
	ap_uint<8> getNextHeader()
	{
		return header(55,48);
	}
};


template <int WIDTH>
void ipv6(	stream<net_axis<WIDTH> >&	s_axis_rx_data,
			stream<ipv6Meta>&	m_axis_rx_meta,
			stream<net_axis<WIDTH> >&	m_axis_rx_data,
			stream<ipv6Meta>&	s_axis_tx_meta,
			stream<net_axis<WIDTH> >&	s_axis_tx_data,
			stream<net_axis<WIDTH> >&	m_axis_tx_data,
			ap_uint<128>		reg_ip_address);
