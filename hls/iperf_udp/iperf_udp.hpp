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
#ifndef IPERF_UDP_HPP
#define IPERF_UDP_HPP

#include "iperf_udp_config.hpp"
#include "../axi_utils.hpp"
#include "../packet.hpp"
#include "../udp/udp.hpp"

#ifndef __SYNTHESIS__
static const ap_uint<40> END_TIME_100ms	= 5;
static const ap_uint<40> END_TIME_1s	= 10;
static const ap_uint<40> END_TIME_5s	= 1000;
static const ap_uint<40> END_TIME_30s	= 1000;
static const ap_uint<40> END_TIME_120s	= 1000;

#else
static const ap_uint<40> END_TIME_100ms	= 15625000;
static const ap_uint<40> END_TIME_1s	= 156250000;
static const ap_uint<40> END_TIME_5s	= 781250000;
static const ap_uint<40> END_TIME_30s	= 4687500000;
static const ap_uint<40> END_TIME_120s	= 18750000000;
#endif


static const ap_uint<16> MY_PORT = 32768;

const uint32_t IPERF_HEADER_SIZE = 192; 

/**
 * Iperf Header
 */
template <int W>
class iperfHeader : public packetHeader<W, IPERF_HEADER_SIZE> {
	using packetHeader<W, IPERF_HEADER_SIZE>::header;

public:
	iperfHeader()
	{
		//this is just for simplicity, to make the header multiple of 64
		header(127, 96) = 0x35343332;
		header(159,128) = 0;
		header(191,160) = 0x33323130;
	}

	void setSeqNumber(ap_uint<32> seqNumb, ap_uint<1> isLast)
	{
		header(31,0) = reverse(seqNumb);
		header[7] = isLast;
	}
	void setSeconds(ap_uint<32> seconds)
	{
		header(63, 32) = reverse(seconds);
	}
	void setMicroSeconds(ap_uint<32> microseconds)
	{
		header(95, 64) = reverse(microseconds);
	}
};


void iperf_udp(	hls::stream<ipUdpMeta>&	rxMetaData,
				hls::stream<net_axis<DATA_WIDTH> >&	rxData,
				hls::stream<ipUdpMeta>&	txMetaData,
				hls::stream<net_axis<DATA_WIDTH> >&	txData,
				ap_uint<1>		runExperiment,
				ap_uint<8>		pkgWordCount,
				ap_uint<8>		packetGap,
				ap_uint<32>		targetIpAddress);
#endif
