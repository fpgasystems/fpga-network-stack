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
#include "mac_ip_encode_config.hpp"
#include "mac_ip_encode.hpp"
#include "../ethernet/ethernet.hpp"
#include "../ipv4/ipv4.hpp"

template <int WIDTH>
void extract_ip_address(hls::stream<net_axis<WIDTH> >&		dataIn,
						hls::stream<net_axis<WIDTH> >&		dataOut,
						hls::stream<ap_uint<32> >&	arpTableOut,
						ap_uint<32>					regSubNetMask,
						ap_uint<32>					regDefaultGateway)

{
	#pragma HLS PIPELINE II=1
	#pragma HLS INLINE off

	static ipv4Header<WIDTH> header;
	static bool metaWritten = false;

	if (!dataIn.empty())
	{
		net_axis<WIDTH> currWord = dataIn.read();
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

template <int WIDTH>
void insert_ip_checksum(hls::stream<net_axis<WIDTH> >&		dataIn,
						hls::stream<ap_uint<16> >&	checksumFifoIn,
						hls::stream<net_axis<WIDTH> >&		dataOut)
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
			net_axis<WIDTH> currWord = dataIn.read();
			checksumFifoIn.read(checksum);
         if (WIDTH > 64)
         {
			   currWord.data(95, 80) = reverse(checksum);
         }

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
			net_axis<WIDTH> currWord = dataIn.read();
         if (WIDTH == 64)
         {
			   currWord.data(31, 16) = reverse(checksum);
         }

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
			net_axis<WIDTH> currWord = dataIn.read();
			dataOut.write(currWord);
			if (currWord.last)
			{
				wordCount = 0;
			}
		}
		break;
	}
}

template <int WIDTH>
void handle_arp_reply (	hls::stream<arpTableReply>&		arpTableIn,
						hls::stream<net_axis<WIDTH> >&	dataIn,
						hls::stream<ethHeader<WIDTH> >&	headerOut,
						hls::stream<net_axis<WIDTH> >&	dataOut,
						ap_uint<48>					myMacAddress)

{
	#pragma HLS PIPELINE II=1
	#pragma HLS INLINE off

	enum fsmStateType {ARP, FWD, DROP};
	static fsmStateType har_state = ARP;

	switch (har_state)
	{
	case ARP:
		if (!arpTableIn.empty() && !dataIn.empty())
		{
			arpTableReply reply = arpTableIn.read();
			net_axis<WIDTH> word = dataIn.read();

			if (reply.hit)
			{
				//Construct Header
				ethHeader<WIDTH> header;
				header.clear();
				header.setDstAddress(reply.macAddress);
				header.setSrcAddress(myMacAddress);
				header.setEthertype(0x0800);
				headerOut.write(header);

				dataOut.write(word);

				if (!word.last)
				{
					har_state = FWD;
				}
			}
			else
			{
				if (!word.last)
				{
					har_state = DROP;
				}
			}
			
		}
		break;
	case FWD:
		if (!dataIn.empty())
		{
			net_axis<WIDTH> word = dataIn.read();
			dataOut.write(word);
			if (word.last)
			{
				har_state = ARP;
			}
		}
		break;
	case DROP:
		if (!dataIn.empty())
		{
			net_axis<WIDTH> word = dataIn.read();
			if (word.last)
			{
				har_state = ARP;
			}
		}
		break;
	}
}

template <int WIDTH>
void insert_ethernet_header(hls::stream<ethHeader<WIDTH> >&		headerIn,
							hls::stream<net_axis<WIDTH> >&		dataIn,
							hls::stream<net_axis<WIDTH> >&		dataOut)
{
	#pragma HLS PIPELINE II=1
	#pragma HLS INLINE off

	enum fsmStateType {HEADER, PARTIAL_HEADER, BODY};
	static fsmStateType ge_state = (ETH_HEADER_SIZE >= WIDTH) ? HEADER : PARTIAL_HEADER;
	static ethHeader<WIDTH> header;

	switch (ge_state)
	{
	case HEADER:
	{
		if (!headerIn.empty()) // This works because for 64bit there is only one full header word 
		{
			headerIn.read(header);
			net_axis<WIDTH> currWord;
			//Always holds, no check required
			header.consumeWord(currWord.data);
			ge_state = PARTIAL_HEADER;
			currWord.keep = ~0;
			currWord.last = 0;
			dataOut.write(currWord);
		}
		break;
	}
	case PARTIAL_HEADER:
		if ((!headerIn.empty() || (ETH_HEADER_SIZE >= WIDTH)) && !dataIn.empty())
		{
			if (ETH_HEADER_SIZE < WIDTH)
			{
				headerIn.read(header);
			}
			net_axis<WIDTH> currWord = dataIn.read();
			header.consumeWord(currWord.data);
			dataOut.write(currWord);

			if (!currWord.last)
			{
				ge_state = BODY;
			}
			else
			{
				ge_state = (ETH_HEADER_SIZE >= WIDTH) ? HEADER : PARTIAL_HEADER;
			}
		}
		break;
	case BODY:
		if (!dataIn.empty())
		{
			net_axis<WIDTH> currWord = dataIn.read();
			dataOut.write(currWord);
			if (currWord.last)
			{
				ge_state = (ETH_HEADER_SIZE >= WIDTH) ? HEADER : PARTIAL_HEADER;
			}
		}
		break;
	} //switch

}

template <int WIDTH>
void generate_ethernet(	hls::stream<net_axis<WIDTH> >&		dataIn,
						hls::stream<arpTableReply>&	arpTableIn,
						hls::stream<net_axis<WIDTH> >&		dataOut,
						ap_uint<48>					myMacAddress)
{
	#pragma HLS PIPELINE II=1
	#pragma HLS INLINE off

	enum fsmStateType {META, HEADER, PARTIAL_HEADER, BODY, DROP};
	static fsmStateType ge_state=META;
	static ethHeader<WIDTH> header;

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
				if (ETH_HEADER_SIZE >= WIDTH)
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
		net_axis<WIDTH> currWord;
		if (header.consumeWord(currWord.data) < (WIDTH/8))
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
			net_axis<WIDTH> currWord = dataIn.read();
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
			net_axis<WIDTH> currWord = dataIn.read();
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
			net_axis<WIDTH> currWord = dataIn.read();
			if (currWord.last)
			{
				ge_state = META;
			}
		}
		break;
	}//switch
}

template <int WIDTH>
void mac_ip_encode( hls::stream<net_axis<WIDTH> >&			dataIn,
					hls::stream<arpTableReply>&		arpTableIn,
					hls::stream<net_axis<WIDTH> >&			dataOut,
					hls::stream<ap_uint<32> >&		arpTableOut,
					ap_uint<48>					myMacAddress,
					ap_uint<32>					regSubNetMask,
					ap_uint<32>					regDefaultGateway)
{
	#pragma HLS INLINE

	// FIFOs
	static hls::stream<net_axis<WIDTH> > dataStreamBuffer0("dataStreamBuffer0");
	static hls::stream<net_axis<WIDTH> > dataStreamBuffer1("dataStreamBuffer1");
	static hls::stream<net_axis<WIDTH> > dataStreamBuffer2("dataStreamBuffer2");
	static hls::stream<net_axis<WIDTH> > dataStreamBuffer3("dataStreamBuffer3");
	static hls::stream<net_axis<WIDTH> > dataStreamBuffer4("dataStreamBuffer4");

	#pragma HLS stream variable=dataStreamBuffer0 depth=2
	#pragma HLS stream variable=dataStreamBuffer1 depth=32
	#pragma HLS stream variable=dataStreamBuffer2 depth=2
	#pragma HLS stream variable=dataStreamBuffer3 depth=2
	#pragma HLS stream variable=dataStreamBuffer4 depth=2
	#pragma HLS aggregate  variable=dataStreamBuffer0 compact=bit
	#pragma HLS aggregate  variable=dataStreamBuffer1 compact=bit
	#pragma HLS aggregate  variable=dataStreamBuffer2 compact=bit
	#pragma HLS aggregate  variable=dataStreamBuffer3 compact=bit
	#pragma HLS aggregate  variable=dataStreamBuffer4 compact=bit

	static hls::stream<subSums<WIDTH/16> >	subSumFifo("subSumFifo");
	static hls::stream<ap_uint<16> >		checksumFifo("checksumFifo");
	static hls::stream<ethHeader<WIDTH> >	headerFifo("headerFifo");
	#pragma HLS stream variable=subSumFifo depth=2
	#pragma HLS stream variable=checksumFifo depth=16
	#pragma HLS stream variable=headerFifo depth=2
	#pragma HLS aggregate  variable=headerFifo compact=bit

	extract_ip_address(dataIn, dataStreamBuffer0, arpTableOut, regSubNetMask, regDefaultGateway);

	mac_compute_ipv4_checksum(dataStreamBuffer0, dataStreamBuffer1, subSumFifo, true);
	mac_finalize_ipv4_checksum<WIDTH/16>(subSumFifo, checksumFifo);

	insert_ip_checksum(dataStreamBuffer1, checksumFifo, dataStreamBuffer2);


	handle_arp_reply(arpTableIn, dataStreamBuffer2, headerFifo, dataStreamBuffer3, myMacAddress);
	mac_lshiftWordByOctet<WIDTH, 1>(((ETH_HEADER_SIZE%WIDTH)/8), dataStreamBuffer3, dataStreamBuffer4);
	insert_ethernet_header(headerFifo, dataStreamBuffer4, dataOut);

	//generate_ethernet(dataStreamBuffer3, arpTableIn, dataOut, myMacAddress);
}

void mac_ip_encode_top( hls::stream<ap_axiu<DATA_WIDTH, 0, 0, 0> >&			dataIn,
					hls::stream<arpTableReply>&		arpTableIn,
					hls::stream<ap_axiu<DATA_WIDTH, 0, 0, 0> >&			dataOut,
					hls::stream<ap_uint<32> >&		arpTableOut,
					ap_uint<48>					myMacAddress,
					ap_uint<32>					regSubNetMask,
					ap_uint<32>					regDefaultGateway)
{
	#pragma HLS DATAFLOW disable_start_propagation
	#pragma HLS INTERFACE ap_ctrl_none port=return

	#pragma HLS INTERFACE axis register port=dataIn name=s_axis_ip
	#pragma HLS INTERFACE axis register port=dataOut name=m_axis_ip
	#pragma HLS INTERFACE axis register port=arpTableIn name=s_axis_arp_lookup_reply
	#pragma HLS INTERFACE axis register port=arpTableOut name=m_axis_arp_lookup_request

	#pragma HLS aggregate  variable=arpTableIn compact=bit

	#pragma HLS INTERFACE ap_none register port=myMacAddress
	#pragma HLS INTERFACE ap_none register port=regSubNetMask
	#pragma HLS INTERFACE ap_none register port=regDefaultGateway

	static hls::stream<net_axis<DATA_WIDTH> > dataIn_internal;
	#pragma HLS STREAM depth=2 variable=dataIn_internal
	static hls::stream<net_axis<DATA_WIDTH> > dataOut_internal;
	#pragma HLS STREAM depth=2 variable=dataOut_internal

	convert_axis_to_net_axis<DATA_WIDTH>(dataIn, 
							dataIn_internal);

	convert_net_axis_to_axis<DATA_WIDTH>(dataOut_internal, 
							dataOut);

   	mac_ip_encode<DATA_WIDTH>( dataIn_internal,
                              arpTableIn,
                              dataOut_internal,
                              arpTableOut,
                              myMacAddress,
                              regSubNetMask,
                              regDefaultGateway);

}
