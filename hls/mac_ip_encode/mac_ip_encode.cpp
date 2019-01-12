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
#include "mac_ip_encode.hpp"
#include "../ethernet/ethernet.hpp"
#include "../ipv4/ipv4.hpp"

void extract_ip_address(hls::stream<axiWord>&		dataIn,
						hls::stream<axiWord>&		dataOut,
						hls::stream<ap_uint<32> >&	arpTableOut,
						ap_uint<32>					regSubNetMask,
						ap_uint<32>					regDefaultGateway)

{
	#pragma HLS PIPELINE II=1
	#pragma HLS INLINE off

	static ipv4Header<AXI_WIDTH> header;
	static bool metaWritten = false;

	if (!dataIn.empty())
	{
		axiWord currWord = dataIn.read();
		header.parseWord(currWord.data);
		dataOut.write(currWord);

		if (header.isReady() && !metaWritten)
		{
			ap_uint<32> dstIpAddress = header.getDstAddr();
			if ((dstIpAddress & regSubNetMask) == (regDefaultGateway & regSubNetMask))
			{
				arpTableOut.write(dstIpAddress);
			}
			else
			{
				arpTableOut.write(regDefaultGateway);
			}
			metaWritten = true;
		}

		if (currWord.last)
		{
			metaWritten = false;
			header.clear();
		}
	}
}

void insert_ip_checksum(hls::stream<axiWord>&		dataIn,
						hls::stream<ap_uint<16> >&	checksumFifoIn,
						hls::stream<axiWord>&		dataOut)
{
	#pragma HLS PIPELINE II=1
	#pragma HLS INLINE off

	static ap_uint<4> wordCount = 0;
	static ap_uint<16> checksum;

	switch (wordCount)
	{
	case 0:
		if (!dataIn.empty() && !checksumFifoIn.empty())
		{
			axiWord currWord = dataIn.read();
			checksumFifoIn.read(checksum);
#if AXI_WIDTH > 64
			currWord.data(95, 80) = reverse(checksum);
#endif
			dataOut.write(currWord);
			wordCount++;
			if (currWord.last)
			{
				wordCount = 0;
			}
		}
		break;
	case 1:
		if (!dataIn.empty())
		{
			axiWord currWord = dataIn.read();

#if AXI_WIDTH == 64
			currWord.data(31, 16) = reverse(checksum);
#endif
			dataOut.write(currWord);
			wordCount++;
			if (currWord.last)
			{
				wordCount = 0;
			}
		}
		break;
	default:
		if (!dataIn.empty())
		{
			axiWord currWord = dataIn.read();
			dataOut.write(currWord);
			if (currWord.last)
			{
				wordCount = 0;
			}
		}
		break;
	}
}


void generate_ethernet(	hls::stream<axiWord>&		dataIn,
						hls::stream<arpTableReply>&	arpTableIn,
						hls::stream<axiWord>&		dataOut,
						ap_uint<48>					myMacAddress)
{
	#pragma HLS PIPELINE II=1
	#pragma HLS INLINE off

	enum fsmStateType {META, HEADER, PARTIAL_HEADER, BODY, DROP};
	static fsmStateType ge_state=META;
	static ethHeader<AXI_WIDTH> header;

	arpTableReply reply;

	switch (ge_state)
	{
	case META:
		if (!arpTableIn.empty())
		{
			arpTableIn.read(reply);
			header.clear();

			if (reply.hit)
			{
				header.setDstAddress(reply.macAddress);
				header.setSrcAddress(myMacAddress);
				header.setEthertype(0x0800);
				if (ETH_HEADER_SIZE >= AXI_WIDTH)
				{
					ge_state = HEADER;
				}
				else
				{
					ge_state = PARTIAL_HEADER;
				}
			}
			else
			{
				ge_state = DROP;
			}
		}
		break;
	case HEADER:
	{
		axiWord currWord;
		if (header.consumeWord(currWord.data) < (AXI_WIDTH/8))
		{
			ge_state = PARTIAL_HEADER;
		}
		currWord.keep = ~0;
		currWord.last = 0;
		dataOut.write(currWord);
		break;
	}
	case PARTIAL_HEADER:
		if (!dataIn.empty())
		{
			axiWord currWord = dataIn.read();
			header.consumeWord(currWord.data);
			dataOut.write(currWord);
			ge_state = BODY;
			if (currWord.last)
			{
				ge_state = META;
			}
		}
		break;
	case BODY:
		if (!dataIn.empty())
		{
			axiWord currWord = dataIn.read();
			dataOut.write(currWord);
			if (currWord.last)
			{
				ge_state = META;
			}
		}
		break;
	case DROP:
		if (!dataIn.empty())
		{
			axiWord currWord = dataIn.read();
			if (currWord.last)
			{
				ge_state = META;
			}
		}
		break;
	}//switch
}

void mac_ip_encode( hls::stream<axiWord>&			dataIn,
					hls::stream<arpTableReply>&		arpTableIn,
					hls::stream<axiWord>&			dataOut,
					hls::stream<ap_uint<32> >&		arpTableOut,
					ap_uint<48>					myMacAddress,
					ap_uint<32>					regSubNetMask,
					ap_uint<32>					regDefaultGateway)
{
	#pragma HLS DATAFLOW
	#pragma HLS INTERFACE ap_ctrl_none port=return

	#pragma HLS resource core=AXI4Stream variable=dataIn metadata="-bus_bundle s_axis_ip"
	#pragma HLS resource core=AXI4Stream variable=dataOut metadata="-bus_bundle m_axis_ip"

	#pragma HLS resource core=AXI4Stream variable=arpTableIn metadata="-bus_bundle s_axis_arp_lookup_reply"
	#pragma HLS resource core=AXI4Stream variable=arpTableOut metadata="-bus_bundle m_axis_arp_lookup_request"
	#pragma HLS DATA_PACK variable=arpTableIn

	#pragma HLS INTERFACE ap_stable register port=myMacAddress
	#pragma HLS INTERFACE ap_stable register port=regSubNetMask
	#pragma HLS INTERFACE ap_stable register port=regDefaultGateway

	// FIFOs
	static hls::stream<axiWord> dataStreamBuffer0("dataStreamBuffer0");
	static hls::stream<axiWord> dataStreamBuffer1("dataStreamBuffer1");
	static hls::stream<axiWord> dataStreamBuffer2("dataStreamBuffer2");
	static hls::stream<axiWord> dataStreamBuffer3("dataStreamBuffer3");
	#pragma HLS stream variable=dataStreamBuffer0 depth=2
	#pragma HLS stream variable=dataStreamBuffer1 depth=32
	#pragma HLS stream variable=dataStreamBuffer2 depth=2
	#pragma HLS stream variable=dataStreamBuffer3 depth=2
	#pragma HLS DATA_PACK variable=dataStreamBuffer0
	#pragma HLS DATA_PACK variable=dataStreamBuffer1
	#pragma HLS DATA_PACK variable=dataStreamBuffer2
	#pragma HLS DATA_PACK variable=dataStreamBuffer3

	static hls::stream<subSums<AXI_WIDTH/16> >		subSumFifo("subSumFifo");
	static hls::stream<ap_uint<16> >	checksumFifo("checksumFifo");
	#pragma HLS stream variable=subSumFifo depth=2
	#pragma HLS stream variable=checksumFifo depth=16


	extract_ip_address(dataIn, dataStreamBuffer0, arpTableOut, regSubNetMask, regDefaultGateway);

	compute_ipv4_checksum(dataStreamBuffer0, dataStreamBuffer1, subSumFifo, true);
	finalize_ipv4_checksum<AXI_WIDTH/16>(subSumFifo, checksumFifo);

	insert_ip_checksum(dataStreamBuffer1, checksumFifo, dataStreamBuffer2);

	lshiftWordByOctet<AXI_WIDTH, 1>(((ETH_HEADER_SIZE%AXI_WIDTH)/8), dataStreamBuffer2, dataStreamBuffer3);
	generate_ethernet(dataStreamBuffer3, arpTableIn, dataOut, myMacAddress);
}
