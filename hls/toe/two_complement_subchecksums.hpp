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
#include "../ipv4/ipv4.hpp"

template <int WIDTH, int DUMMY>
void two_complement_subchecksums(hls::stream<net_axis<WIDTH> >&		dataIn,
                                 hls::stream<net_axis<WIDTH> >&		dataOut,
                                 hls::stream<subSums<WIDTH/16> >&		txEng_subChecksumsFifoOut)
{
   #pragma HLS PIPELINE II=1
   #pragma HLS INLINE off

	static subSums<WIDTH/16> tcts_tcp_sums;

	if (!dataIn.empty())
	{
		net_axis<WIDTH> currWord = dataIn.read();
		dataOut.write(currWord);

		for (int i = 0; i < WIDTH/16; i++)
		{
         #pragma HLS UNROLL

			ap_uint<16> temp;
			if (currWord.keep(i*2+1, i*2) == 0x3)
			{
				temp(7, 0) = currWord.data(i*16+15, i*16+8);
				temp(15, 8) = currWord.data(i*16+7, i*16);
				tcts_tcp_sums.sum[i] += temp;
				tcts_tcp_sums.sum[i] = (tcts_tcp_sums.sum[i] + (tcts_tcp_sums.sum[i] >> 16)) & 0xFFFF;
			}
			else if (currWord.keep[i*2] == 0x1)
			{
				temp(7, 0) = 0;
				temp(15, 8) = currWord.data(i*16+7, i*16);
				tcts_tcp_sums.sum[i] += temp;
				tcts_tcp_sums.sum[i] = (tcts_tcp_sums.sum[i] + (tcts_tcp_sums.sum[i] >> 16)) & 0xFFFF;
			}
		}
		if(currWord.last == 1)
		{
			txEng_subChecksumsFifoOut.write(tcts_tcp_sums);
			for (int i = 0; i < WIDTH/16; i++)
			{
				#pragma HLS UNROLL
				tcts_tcp_sums.sum[i] = 0;
			}
		}
	}
}
