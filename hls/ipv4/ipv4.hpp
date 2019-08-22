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
#pragma once

#include "../axi_utils.hpp"
#include "../packet.hpp"

const uint32_t IPV4_HEADER_SIZE = 160;

struct ipv4Meta
{
	ap_uint<32> their_address;
	ap_uint<16> length;
	//TODO what aobut my address??
	ipv4Meta() {}
	ipv4Meta(ap_uint<32> addr, ap_uint<16> len)
		:their_address(addr), length(len) {}
	//for IPv6 TODO fix this in the future
	ipv4Meta(ap_uint<128> addr, ap_uint<16> len)
			:their_address(addr(127,96)), length(len) {}
};

template <int N>
struct subSums
{
	ap_uint<17>		sum[N];
};


void compute_ipv4_checksum(	hls::stream<net_axis<64> >&	dataIn,
							hls::stream<net_axis<64> >&	dataOut,
							hls::stream<subSums<4> >&	subSumFiFoOut,
							const bool					skipChecksum=false);

void compute_ipv4_checksum(	hls::stream<net_axis<128> >&	dataIn,
							hls::stream<net_axis<128> >&	dataOut,
							hls::stream<subSums<8> >&	subSumFiFoOut,
							const bool					skipChecksum=false);

void compute_ipv4_checksum(	hls::stream<net_axis<256> >&	dataIn,
							hls::stream<net_axis<256> >&	dataOut,
							hls::stream<subSums<16> >&	subSumFiFoOut,
							const bool					skipChecksum=false);

void compute_ipv4_checksum(	hls::stream<net_axis<512> >&	dataIn,
							hls::stream<net_axis<512> >&	dataOut,
							hls::stream<subSums<32> >&		subSumFiFoOut,
							const bool						skipChecksum=false);

template <int N>
void check_ipv4_checksum(	hls::stream<subSums<N> >&	subSumFiFoIn,
							hls::stream<bool>&			checkSumFifoOut)
{
	#pragma HLS PIPELINE II=1
	#pragma HLS INLINE off

	if (!subSumFiFoIn.empty())
	{
		subSums<N> icic_ip_sums = subSumFiFoIn.read();

		if (N >= 32)
		{
			for (int i = 0; i < 16; i++)
			{
				#pragma HLS unroll
				icic_ip_sums.sum[i] += icic_ip_sums.sum[i+16];
				icic_ip_sums.sum[i] = (icic_ip_sums.sum[i] + (icic_ip_sums.sum[i] >> 16)) & 0xFFFF;
			}
		}

		if (N >= 16)
		{
			for (int i = 0; i < 8; i++)
			{
				#pragma HLS unroll
				icic_ip_sums.sum[i] += icic_ip_sums.sum[i+8];
				icic_ip_sums.sum[i] = (icic_ip_sums.sum[i] + (icic_ip_sums.sum[i] >> 16)) & 0xFFFF;
			}
		}

		if (N >= 8)
		{
			for (int i = 0; i < 4; i++)
			{
				#pragma HLS unroll
				icic_ip_sums.sum[i] += icic_ip_sums.sum[i+4];
				icic_ip_sums.sum[i] = (icic_ip_sums.sum[i] + (icic_ip_sums.sum[i] >> 16)) & 0xFFFF;
			}
		}

		//N >= 4 -> 64bit data
		icic_ip_sums.sum[0] += icic_ip_sums.sum[2];
		icic_ip_sums.sum[1] += icic_ip_sums.sum[3];
		icic_ip_sums.sum[0] = (icic_ip_sums.sum[0] + (icic_ip_sums.sum[0] >> 16)) & 0xFFFF;
		icic_ip_sums.sum[1] = (icic_ip_sums.sum[1] + (icic_ip_sums.sum[1] >> 16)) & 0xFFFF;
		icic_ip_sums.sum[0] += icic_ip_sums.sum[1];
		icic_ip_sums.sum[0] = (icic_ip_sums.sum[0] + (icic_ip_sums.sum[0] >> 16)) & 0xFFFF;
		icic_ip_sums.sum[0] = ~icic_ip_sums.sum[0];
		std::cout << "checked checksum: " << std::hex << (uint16_t) icic_ip_sums.sum[0](15, 0) << std::endl;
		checkSumFifoOut.write(icic_ip_sums.sum[0](15, 0) == 0);
	}

}

template <int N>
void finalize_ipv4_checksum(hls::stream<subSums<N> >&	subSumFiFoIn,
							hls::stream<ap_uint<16> >&	checkSumFifoOut)
{
	#pragma HLS PIPELINE II=1
	#pragma HLS INLINE off

	if (!subSumFiFoIn.empty())
	{
		subSums<N> icic_ip_sums = subSumFiFoIn.read();

		if (N >= 32)
		{
			for (int i = 0; i < 16; i++)
			{
				#pragma HLS unroll
				icic_ip_sums.sum[i] += icic_ip_sums.sum[i+16];
				icic_ip_sums.sum[i] = (icic_ip_sums.sum[i] + (icic_ip_sums.sum[i] >> 16)) & 0xFFFF;
			}
		}

		if (N >= 16)
		{
			for (int i = 0; i < 8; i++)
			{
				#pragma HLS unroll
				icic_ip_sums.sum[i] += icic_ip_sums.sum[i+8];
				icic_ip_sums.sum[i] = (icic_ip_sums.sum[i] + (icic_ip_sums.sum[i] >> 16)) & 0xFFFF;
			}
		}

		if (N >= 8)
		{
			for (int i = 0; i < 4; i++)
			{
				#pragma HLS unroll
				icic_ip_sums.sum[i] += icic_ip_sums.sum[i+4];
				icic_ip_sums.sum[i] = (icic_ip_sums.sum[i] + (icic_ip_sums.sum[i] >> 16)) & 0xFFFF;
			}
		}

		//N >= 4 -> 64bit data
		icic_ip_sums.sum[0] += icic_ip_sums.sum[2];
		icic_ip_sums.sum[1] += icic_ip_sums.sum[3];
		icic_ip_sums.sum[0] = (icic_ip_sums.sum[0] + (icic_ip_sums.sum[0] >> 16)) & 0xFFFF;
		icic_ip_sums.sum[1] = (icic_ip_sums.sum[1] + (icic_ip_sums.sum[1] >> 16)) & 0xFFFF;
		icic_ip_sums.sum[0] += icic_ip_sums.sum[1];
		icic_ip_sums.sum[0] = (icic_ip_sums.sum[0] + (icic_ip_sums.sum[0] >> 16)) & 0xFFFF;
		icic_ip_sums.sum[0] = ~icic_ip_sums.sum[0];
		std::cout << "finalized checksum: " << std::hex << (uint16_t) icic_ip_sums.sum[0](15, 0) << std::endl;
		checkSumFifoOut.write(icic_ip_sums.sum[0](15, 0));
	}
}

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

template <int WIDTH>
void ipv4(	hls::stream<net_axis<WIDTH> >&	s_axis_rx_data,
			hls::stream<ipv4Meta>&	m_axis_rx_meta,
			hls::stream<net_axis<WIDTH> >&	m_axis_rx_data,
			hls::stream<ipv4Meta>&	s_axis_tx_meta,
			hls::stream<net_axis<WIDTH> >&	s_axis_tx_data,
			hls::stream<net_axis<WIDTH> >&	m_axis_tx_data,
			ap_uint<32>			local_ipv4_address,
			ap_uint<8>			protocol);
