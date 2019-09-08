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
#ifndef UDP_HPP
#define UDP_HPP

#include "../axi_utils.hpp"
#include "../packet.hpp"
#include "../ipv4/ipv4.hpp"
//#include "../ipv6/ipv6.hpp"

#define IP_VERSION 4

#if IP_VERSION == 6
typedef ipv6Meta ipMeta;
#else
typedef ipv4Meta ipMeta;
#endif

const uint32_t UDP_HEADER_SIZE = 64;
const uint16_t UDP_PROTOCOL = 0x11;

struct ipUdpMeta
{
	ap_uint<128> their_address;
	ap_uint<16> their_port;
	ap_uint<16> my_port;
	ap_uint<16>	length;
	ipUdpMeta() {}
	ipUdpMeta(ap_uint<128> addr, ap_uint<16> tport, ap_uint<16> mport, ap_uint<16> len)
		:their_address(addr), their_port(tport), my_port(mport), length(len) {}
};

struct udpMeta
{
	ap_uint<16> their_port;
	ap_uint<16> my_port;
	ap_uint<16> length;
	bool 		valid;
	udpMeta()
		:valid(true) {}
	udpMeta(ap_uint<16> tport, ap_uint<16> mport, ap_uint<16> len)
		:their_port(tport), my_port(mport), length(len), valid(true) {}
	udpMeta(ap_uint<16> tport, ap_uint<16> mport, ap_uint<16> len, bool valid)
		:their_port(tport), my_port(mport), length(len), valid(valid) {}
};


/**
 * [15, 0] = srcPort;
 * [31, 16] = dstPort;
 * [47, 32] = length;
 * [63, 48] = checksum;
 */
template <int W>
class udpHeader : public packetHeader<W, UDP_HEADER_SIZE> {
	using packetHeader<W, UDP_HEADER_SIZE>::header;

public:
	udpHeader() {}

	void setSrcPort(const ap_uint<16> port)
	{
		header(15,0) = reverse(port);
	}
	ap_uint<16> getSrcPort()
	{
		return reverse((ap_uint<16>)header(15,0));
	}
	void setDstPort(const ap_uint<16> port)
	{
		header(31,16) = reverse(port);
	}
	ap_uint<16> getDstPort()
	{
		return reverse((ap_uint<16>)header(31,16));
	}
	void setLength(ap_uint<16> len)
	{
		header(47,32) = reverse(len);
	}
	ap_uint<16> getLength()
	{
		return reverse((ap_uint<16>)header(47,32));
	}
};

template <int WIDTH>
void udp(		hls::stream<ipMeta>&		s_axis_rx_meta,
				hls::stream<net_axis<WIDTH> >&	s_axis_rx_data,
				hls::stream<ipUdpMeta>&	m_axis_rx_meta,
				hls::stream<net_axis<WIDTH> >&	m_axis_rx_data,
				hls::stream<ipUdpMeta>&	s_axis_tx_meta,
				hls::stream<net_axis<WIDTH> >&	s_axis_tx_data,
				hls::stream<ipMeta>&		m_axis_tx_meta,
				hls::stream<net_axis<WIDTH> >&	m_axis_tx_data,
				ap_uint<128>		reg_ip_address,
				ap_uint<16>			reg_listen_port);

#endif
