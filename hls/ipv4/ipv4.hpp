/*
 * Copyright (c) 2018, Systems Group, ETH Zurich
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
#ifndef IP4_HPP
#define IP4_HPP

#include "../axi_utils.hpp"
#include "../packet.hpp"

const uint32_t IPV4_HEADER_SIZE = 160;

struct ipv4Meta
{
	ap_uint<32> their_address;
	ap_uint<16> length;
	ipv4Meta() {}
	ipv4Meta(ap_uint<32> addr, ap_uint<16> len)
		:their_address(addr), length(len) {}
	//for IPv6 TODO fix this in the future
	ipv4Meta(ap_uint<128> addr, ap_uint<16> len)
			:their_address(addr(127,96)), length(len) {}
};

/**
 * [7:4] = version;
 * [3:0] = IHL;
 * [13:8] = DSCP;
 * [15:14] = ECN;
 * [31:16] = length;
 * [47:32] = Idendification;
 * [50:48] = Flags;
 * [63:51] = fragment offset;
 * [71:64] = TTL;
 * [79:72] = Protocol;
 * [95:80] = HeaderChecksum;
 * [127:96] = SrcAddr;
 * [159:128] = DstAddr;
 * [...] = IHL;
 */
template <int W>
class ipv4Header : public packetHeader<W, IPV4_HEADER_SIZE> {
	using packetHeader<W, IPV4_HEADER_SIZE>::header;

public:
	ipv4Header()
	{
		header(7, 0) = 0x45; // version & IHL
		header(71, 64) = 0x40; // TTL
	}

	void setSrcAddr(const ap_uint<32>& addr)
	{
		header(127,96) = addr;
	}
	ap_uint<32> getSrcAddr()
	{
		return header(127,96);
	}
	void setDstAddr(const ap_uint<32>& addr)
	{
		header(159,128) = addr;
	}
	ap_uint<32> getDstAddr()
	{
		return header(159,128);
	}
	void setLength(const ap_uint<16> len)
	{
		header(31,16) = reverse(len);
	}
	ap_uint<16> getLength()
	{
		return reverse((ap_uint<16>)header(31,16));
	}
	void setProtocol(const ap_uint<8>& protocol)
	{
		header(79, 72) = protocol;
	}
	ap_uint<8> getProtocol()
	{
		return header(79, 72);
	}
	ap_uint<4> getHeaderLength()
	{
		return header(3, 0);
	}
};

void ipv4(		hls::stream<axiWord>&	s_axis_rx_data,
				hls::stream<ipv4Meta>&		m_axis_rx_meta,
				hls::stream<axiWord>&	m_axis_rx_data,
				hls::stream<ipv4Meta>&		s_axis_tx_meta,
				hls::stream<axiWord>&	s_axis_tx_data,
				hls::stream<axiWord>&	m_axis_tx_data,
				ap_uint<32>			local_ipv4_address);

#endif
