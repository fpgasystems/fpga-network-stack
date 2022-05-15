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
#include "ipv4.hpp"

void compute_ipv4_checksum(	hls::stream<net_axis<64> >&	dataIn,
							hls::stream<net_axis<64> >&	dataOut,
							hls::stream<subSums<4> >&	subSumFiFoOut,
							const bool					skipChecksum)
{
	#pragma HLS PIPELINE II=1
	#pragma HLS INLINE off

	static subSums<4>	cics_ip_sums;
	static ap_uint<4> cics_ipHeaderLen;
	static ap_uint<3> cics_wordCount = 0;

	ap_uint<16> temp;

	if (!dataIn.empty())
	{
		net_axis<64> currWord = dataIn.read();
		dataOut.write(currWord);

		switch (cics_wordCount)
		{
		case 0:
			cics_ipHeaderLen = currWord.data.range(3, 0);

			for (int i = 0; i < 4; i++)
			{
				#pragma HLS unroll
				temp(7, 0) = currWord.data.range(i*16+15, i*16+8);
				temp(15, 8) = currWord.data.range(i*16+7, i*16);
				cics_ip_sums.sum[i] += temp;
				cics_ip_sums.sum[i] = (cics_ip_sums.sum[i] + (cics_ip_sums.sum[i] >> 16)) & 0xFFFF;
			}
			cics_ipHeaderLen -= 2; 
			cics_wordCount++;
			break;
		case 1:
			for (int i = 0; i < 4; i++)
			{
				#pragma HLS unroll
				temp(7, 0) = currWord.data.range(i*16+15, i*16+8);
				temp(15, 8) = currWord.data.range(i*16+7, i*16);
				if (!skipChecksum || i != 1)
				{
					cics_ip_sums.sum[i] += temp;
					cics_ip_sums.sum[i] = (cics_ip_sums.sum[i] + (cics_ip_sums.sum[i] >> 16)) & 0xFFFF;
				}
			}
			cics_ipHeaderLen -= 2; 
			cics_wordCount++;
			break;
		default:
			switch (cics_ipHeaderLen)
			{
			case 0:
				//length 0 means we are just handling payload
				break;
			case 1:
				// Sum up part 0-1
				for (int i = 0; i < 2; i++)
				{
					#pragma HLS unroll
					temp(7, 0) = currWord.data.range(i*16+15, i*16+8);
					temp(15, 8) = currWord.data.range(i*16+7, i*16);
					cics_ip_sums.sum[i] += temp;
					cics_ip_sums.sum[i] = (cics_ip_sums.sum[i] + (cics_ip_sums.sum[i] >> 16)) & 0xFFFF;
				}
				cics_ipHeaderLen = 0;
				subSumFiFoOut.write(subSums<4>(cics_ip_sums));
				break;
			default:
				// Sum up everything
				for (int i = 0; i < 4; i++)
				{
				#pragma HLS unroll
					temp(7, 0) = currWord.data.range(i*16+15, i*16+8);
					temp(15, 8) = currWord.data.range(i*16+7, i*16);
					cics_ip_sums.sum[i] += temp;
					cics_ip_sums.sum[i] = (cics_ip_sums.sum[i] + (cics_ip_sums.sum[i] >> 16)) & 0xFFFF;
				}
				if (cics_ipHeaderLen == 2)
				{
					subSumFiFoOut.write(subSums<4>(cics_ip_sums));
				}
				cics_ipHeaderLen -= 2;
				break;
			} // switch ipHeaderLen
			break;
		} // switch WORD_N
		if (currWord.last)
		{
			for (int i = 0; i < 4; i++)
			{
				#pragma HLS unroll
				cics_ip_sums.sum[i] = 0;
			}
			cics_wordCount = 0;
		}
	}
}

void compute_ipv4_checksum(	hls::stream<net_axis<128> >&	dataIn,
							hls::stream<net_axis<128> >&	dataOut,
							hls::stream<subSums<8> >&	subSumFiFoOut,
							const bool					skipChecksum)
{
	#pragma HLS PIPELINE II=1
	#pragma HLS INLINE off

	static subSums<8>	cics_ip_sums;
	static ap_uint<4> cics_ipHeaderLen;
	static ap_uint<3> cics_wordCount = 0;

	ap_uint<16> temp;

	if (!dataIn.empty())
	{
		net_axis<128> currWord = dataIn.read();
		dataOut.write(currWord);

		switch (cics_wordCount)
		{
		case 0:
			cics_ipHeaderLen = currWord.data(3, 0);

			for (int i = 0; i < 8; i++)
			{
				#pragma HLS unroll
				temp(7, 0) = currWord.data.range(i*16+15, i*16+8);
				temp(15, 8) = currWord.data.range(i*16+7, i*16);
				if (!skipChecksum || i != 5)
				{
					cics_ip_sums.sum[i] += temp;
					cics_ip_sums.sum[i] = (cics_ip_sums.sum[i] + (cics_ip_sums.sum[i] >> 16)) & 0xFFFF;
				}
			}
			cics_ipHeaderLen -= 4; 
			cics_wordCount++;
			break;
		default:
			switch (cics_ipHeaderLen)
			{
			case 0:
				//length 0 means we are just handling payload
				break;
			case 1:
				// Sum up part 0-1
				for (int i = 0; i < 2; i++)
				{
					#pragma HLS unroll
					temp(7, 0) = currWord.data.range(i*16+15, i*16+8);
					temp(15, 8) = currWord.data.range(i*16+7, i*16);
					cics_ip_sums.sum[i] += temp;
					cics_ip_sums.sum[i] = (cics_ip_sums.sum[i] + (cics_ip_sums.sum[i] >> 16)) & 0xFFFF;
				}
				cics_ipHeaderLen = 0;
				subSumFiFoOut.write(subSums<8>(cics_ip_sums));
				break;
			case 2:
				// Sum up part 0-3
				for (int i = 0; i < 4; i++)
				{
					#pragma HLS unroll
					temp(7, 0) = currWord.data.range(i*16+15, i*16+8);
					temp(15, 8) = currWord.data.range(i*16+7, i*16);
					cics_ip_sums.sum[i] += temp;
					cics_ip_sums.sum[i] = (cics_ip_sums.sum[i] + (cics_ip_sums.sum[i] >> 16)) & 0xFFFF;
				}
				cics_ipHeaderLen = 0;
				subSumFiFoOut.write(subSums<8>(cics_ip_sums));
				break;
			case 3:
				// Sum up part 0-5
				for (int i = 0; i < 6; i++)
				{
					#pragma HLS unroll
					temp(7, 0) = currWord.data.range(i*16+15, i*16+8);
					temp(15, 8) = currWord.data.range(i*16+7, i*16);
					cics_ip_sums.sum[i] += temp;
					cics_ip_sums.sum[i] = (cics_ip_sums.sum[i] + (cics_ip_sums.sum[i] >> 16)) & 0xFFFF;
				}
				cics_ipHeaderLen = 0;
				subSumFiFoOut.write(subSums<8>(cics_ip_sums));
				break;
			default:
				// Sum up everything
				for (int i = 0; i < 8; i++)
				{
				#pragma HLS unroll
					temp(7, 0) = currWord.data.range(i*16+15, i*16+8);
					temp(15, 8) = currWord.data.range(i*16+7, i*16);
					cics_ip_sums.sum[i] += temp;
					cics_ip_sums.sum[i] = (cics_ip_sums.sum[i] + (cics_ip_sums.sum[i] >> 16)) & 0xFFFF;
				}
				if (cics_ipHeaderLen == 4)
				{
					subSumFiFoOut.write(subSums<8>(cics_ip_sums));
				}
				cics_ipHeaderLen -= 4;
				break;
			} // switch ipHeaderLen
			break;
		} // switch WORD_N
		if (currWord.last)
		{
			for (int i = 0; i < 8; i++)
			{
				#pragma HLS unroll
				cics_ip_sums.sum[i] = 0;
			}
			cics_wordCount = 0;
		}
	}
}

void compute_ipv4_checksum(	hls::stream<net_axis<256> >&	dataIn,
							hls::stream<net_axis<256> >&	dataOut,
							hls::stream<subSums<16> >&	subSumFiFoOut,
							const bool					skipChecksum)
{
	#pragma HLS PIPELINE II=1
	#pragma HLS INLINE off

	static subSums<16>	cics_ip_sums;
	static ap_uint<4> cics_ipHeaderLen;
	static ap_uint<3> cics_wordCount = 0;

	ap_uint<16> temp;

	if (!dataIn.empty())
	{
		net_axis<256> currWord = dataIn.read();
		dataOut.write(currWord);

		switch (cics_wordCount)
		{
		case 0:
			cics_ipHeaderLen = currWord.data(3, 0);

			for (int i = 0; i < 16; i++)
			{
				#pragma HLS unroll
				temp(7, 0) = currWord.data.range(i*16+15, i*16+8);
				temp(15, 8) = currWord.data.range(i*16+7, i*16);
				if (!skipChecksum || i != 5)
				{
					if ((i/2) < cics_ipHeaderLen)
					{
						cics_ip_sums.sum[i] += temp;
						cics_ip_sums.sum[i] = (cics_ip_sums.sum[i] + (cics_ip_sums.sum[i] >> 16)) & 0xFFFF;
					}
				}
			}
			if (cics_ipHeaderLen > 8)
			{
				cics_ipHeaderLen -= 8;
			}
			else
			{
				subSumFiFoOut.write(subSums<16>(cics_ip_sums));
				cics_ipHeaderLen = 0;
			}
			cics_wordCount++;
			break;
		default:
			switch (cics_ipHeaderLen)
			{
			case 0:
				//length 0 means we are just handling payload
				break;
			case 1:
				// Sum up part 0-1
				for (int i = 0; i < 2; i++)
				{
					#pragma HLS unroll
					temp(7, 0) = currWord.data.range(i*16+15, i*16+8);
					temp(15, 8) = currWord.data.range(i*16+7, i*16);
					cics_ip_sums.sum[i] += temp;
					cics_ip_sums.sum[i] = (cics_ip_sums.sum[i] + (cics_ip_sums.sum[i] >> 16)) & 0xFFFF;
				}
				cics_ipHeaderLen = 0;
				subSumFiFoOut.write(subSums<16>(cics_ip_sums));
				break;
			case 2:
				// Sum up part 0-3
				for (int i = 0; i < 4; i++)
				{
					#pragma HLS unroll
					temp(7, 0) = currWord.data.range(i*16+15, i*16+8);
					temp(15, 8) = currWord.data.range(i*16+7, i*16);
					cics_ip_sums.sum[i] += temp;
					cics_ip_sums.sum[i] = (cics_ip_sums.sum[i] + (cics_ip_sums.sum[i] >> 16)) & 0xFFFF;
				}
				cics_ipHeaderLen = 0;
				subSumFiFoOut.write(subSums<16>(cics_ip_sums));
				break;
			case 3:
				// Sum up part 0-5
				for (int i = 0; i < 6; i++)
				{
					#pragma HLS unroll
					temp(7, 0) = currWord.data.range(i*16+15, i*16+8);
					temp(15, 8) = currWord.data.range(i*16+7, i*16);
					cics_ip_sums.sum[i] += temp;
					cics_ip_sums.sum[i] = (cics_ip_sums.sum[i] + (cics_ip_sums.sum[i] >> 16)) & 0xFFFF;
				}
				cics_ipHeaderLen = 0;
				subSumFiFoOut.write(subSums<16>(cics_ip_sums));
				break;
			case 4:
				// Sum up part 0-7
				for (int i = 0; i < 8; i++)
				{
					#pragma HLS unroll
					temp(7, 0) = currWord.data.range(i*16+15, i*16+8);
					temp(15, 8) = currWord.data.range(i*16+7, i*16);
					cics_ip_sums.sum[i] += temp;
					cics_ip_sums.sum[i] = (cics_ip_sums.sum[i] + (cics_ip_sums.sum[i] >> 16)) & 0xFFFF;
				}
				cics_ipHeaderLen = 0;
				subSumFiFoOut.write(subSums<16>(cics_ip_sums));
				break;
			case 5:
				// Sum up part 0-9
				for (int i = 0; i < 10; i++)
				{
					#pragma HLS unroll
					temp(7, 0) = currWord.data.range(i*16+15, i*16+8);
					temp(15, 8) = currWord.data.range(i*16+7, i*16);
					cics_ip_sums.sum[i] += temp;
					cics_ip_sums.sum[i] = (cics_ip_sums.sum[i] + (cics_ip_sums.sum[i] >> 16)) & 0xFFFF;
				}
				cics_ipHeaderLen = 0;
				subSumFiFoOut.write(subSums<16>(cics_ip_sums));
				break;
			case 6:
				// Sum up part 0-11
				for (int i = 0; i < 12; i++)
				{
					#pragma HLS unroll
					temp(7, 0) = currWord.data.range(i*16+15, i*16+8);
					temp(15, 8) = currWord.data.range(i*16+7, i*16);
					cics_ip_sums.sum[i] += temp;
					cics_ip_sums.sum[i] = (cics_ip_sums.sum[i] + (cics_ip_sums.sum[i] >> 16)) & 0xFFFF;
				}
				cics_ipHeaderLen = 0;
				subSumFiFoOut.write(subSums<16>(cics_ip_sums));
				break;
			case 7:
				// Sum up part 0-13
				for (int i = 0; i < 14; i++)
				{
					#pragma HLS unroll
					temp(7, 0) = currWord.data.range(i*16+15, i*16+8);
					temp(15, 8) = currWord.data.range(i*16+7, i*16);
					cics_ip_sums.sum[i] += temp;
					cics_ip_sums.sum[i] = (cics_ip_sums.sum[i] + (cics_ip_sums.sum[i] >> 16)) & 0xFFFF;
				}
				cics_ipHeaderLen = 0;
				subSumFiFoOut.write(subSums<16>(cics_ip_sums));
				break;
			default:
				// Sum up everything
				for (int i = 0; i < 16; i++)
				{
				#pragma HLS unroll
					temp(7, 0) = currWord.data.range(i*16+15, i*16+8);
					temp(15, 8) = currWord.data.range(i*16+7, i*16);
					cics_ip_sums.sum[i] += temp;
					cics_ip_sums.sum[i] = (cics_ip_sums.sum[i] + (cics_ip_sums.sum[i] >> 16)) & 0xFFFF;
				}
				if (cics_ipHeaderLen == 8)
				{
					subSumFiFoOut.write(subSums<16>(cics_ip_sums));
				}
				cics_ipHeaderLen -= 8;
				break;
			} // switch ipHeaderLen
			break;
		} // switch WORD_N
		if (currWord.last)
		{
			for (int i = 0; i < 16; i++)
			{
				#pragma HLS unroll
				cics_ip_sums.sum[i] = 0;
			}
			cics_wordCount = 0;
		}
	}
}


void compute_ipv4_checksum(	hls::stream<net_axis<512> >&	dataIn,
							hls::stream<net_axis<512> >&	dataOut,
							hls::stream<subSums<32> >&		subSumFiFoOut,
							const bool						skipChecksum)
{
	#pragma HLS PIPELINE II=1
	#pragma HLS INLINE off

	static subSums<32>	cics_ip_sums;
	static ap_uint<4> cics_ipHeaderLen;
	static bool cics_firstWord = true;

	ap_uint<16> temp;

	if (!dataIn.empty())
	{
		net_axis<512> currWord = dataIn.read();
		dataOut.write(currWord);

		if (cics_firstWord)
		{
			cics_ipHeaderLen = currWord.data(3, 0);

			for (int i = 0; i < 32; i++)
			{
				#pragma HLS unroll
				temp = reverse((ap_uint<16>) currWord.data(i*16+15, i*16));
				if (!skipChecksum || i != 5)
				{
					if ((i/2) < cics_ipHeaderLen)
					{
						cics_ip_sums.sum[i] += temp;
						cics_ip_sums.sum[i] = (cics_ip_sums.sum[i] + (cics_ip_sums.sum[i] >> 16)) & 0xFFFF;
					}
				}
			}
			subSumFiFoOut.write(subSums<32>(cics_ip_sums));
			cics_firstWord = false;
		}

		if (currWord.last)
		{
			for (int i = 0; i < 32; i++)
			{
				#pragma HLS unroll
				cics_ip_sums.sum[i] = 0;
			}
			cics_firstWord = true;
		}
	}
}


void mac_compute_ipv4_checksum(	hls::stream<net_axis<512> >&	dataIn,
							hls::stream<net_axis<512> >&	dataOut,
							hls::stream<subSums<32> >&		subSumFiFoOut,
							const bool						skipChecksum)
{
	#pragma HLS PIPELINE II=1
	#pragma HLS INLINE off

	static subSums<32>	cics_ip_sums;
	static ap_uint<4> cics_ipHeaderLen;
	static bool cics_firstWord = true;

	ap_uint<16> temp;

	if (!dataIn.empty())
	{
		net_axis<512> currWord = dataIn.read();
		dataOut.write(currWord);

		if (cics_firstWord)
		{
			cics_ipHeaderLen = currWord.data(3, 0);

			for (int i = 0; i < 32; i++)
			{
				#pragma HLS unroll
				temp = reverse((ap_uint<16>) currWord.data(i*16+15, i*16));
				if (!skipChecksum || i != 5)
				{
					if ((i/2) < cics_ipHeaderLen)
					{
						cics_ip_sums.sum[i] += temp;
						cics_ip_sums.sum[i] = (cics_ip_sums.sum[i] + (cics_ip_sums.sum[i] >> 16)) & 0xFFFF;
					}
				}
			}
			subSumFiFoOut.write(subSums<32>(cics_ip_sums));
			cics_firstWord = false;
		}

		if (currWord.last)
		{
			for (int i = 0; i < 32; i++)
			{
				#pragma HLS unroll
				cics_ip_sums.sum[i] = 0;
			}
			cics_firstWord = true;
		}
	}
}

void ip_handler_compute_ipv4_checksum(	hls::stream<net_axis<512> >&	dataIn,
							hls::stream<net_axis<512> >&	dataOut,
							hls::stream<subSums<32> >&		subSumFiFoOut,
							const bool						skipChecksum)
{
	#pragma HLS PIPELINE II=1
	#pragma HLS INLINE off

	static subSums<32>	cics_ip_sums;
	static ap_uint<4> cics_ipHeaderLen;
	static bool cics_firstWord = true;

	ap_uint<16> temp;

	if (!dataIn.empty())
	{
		net_axis<512> currWord = dataIn.read();
		dataOut.write(currWord);

		if (cics_firstWord)
		{
			cics_ipHeaderLen = currWord.data(3, 0);

			for (int i = 0; i < 32; i++)
			{
				#pragma HLS unroll
				temp = reverse((ap_uint<16>) currWord.data(i*16+15, i*16));
				if (!skipChecksum || i != 5)
				{
					if ((i/2) < cics_ipHeaderLen)
					{
						cics_ip_sums.sum[i] += temp;
						cics_ip_sums.sum[i] = (cics_ip_sums.sum[i] + (cics_ip_sums.sum[i] >> 16)) & 0xFFFF;
					}
				}
			}
			subSumFiFoOut.write(subSums<32>(cics_ip_sums));
			cics_firstWord = false;
		}

		if (currWord.last)
		{
			for (int i = 0; i < 32; i++)
			{
				#pragma HLS unroll
				cics_ip_sums.sum[i] = 0;
			}
			cics_firstWord = true;
		}
	}
}

