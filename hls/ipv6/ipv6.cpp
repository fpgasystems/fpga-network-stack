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
#include "ipv6_config.hpp"
#include "ipv6.hpp"


template <int WIDTH>
void process_ipv6(	stream<net_axis<WIDTH> >&	input,
					stream<ipv6Meta>&		metaOut,
					stream<net_axis<WIDTH> >&	output)
{
#pragma HLS inline off
#pragma HLS pipeline II=1

	static ipv6Header<WIDTH> pi_header;
	static bool metaWritten = false;

	net_axis<WIDTH> currWord;

	if (!input.empty())
	{
		input.read(currWord);
		pi_header.parseWord(currWord.data);
		if (pi_header.isReady())
		{
			output.write(currWord);
			if (!metaWritten)
			{
				metaOut.write(ipv6Meta(pi_header.getSrcAddress(), pi_header.getPayloadLen(), pi_header.getNextHeader()));
				metaWritten = true;
			}
		}
		if (currWord.last)
		{
			metaWritten = false;
			pi_header.clear();
		}
	}
}

//TODO get value form GRH
template <int WIDTH>
void generate_ipv6(	stream<ipv6Meta>&		metaIn,
					stream<net_axis<WIDTH> >&	input,
					stream<net_axis<WIDTH> >&	output,
					ap_uint<128>		reg_ip_address)
{
#pragma HLS inline off
#pragma HLS pipeline II=1

	enum fsmState {META, HEADER, PARTIAL_HEADER, BODY};
	static fsmState state;
	static ipv6Header<WIDTH> header;

	ipv6Meta meta;
	net_axis<WIDTH> currWord;

	switch (state)
	{
	case META:
		if (!metaIn.empty())
		{
			metaIn.read(meta);
			//TODO fixed
			//std::cout << "my address: ";
			//print(std::cout, reg_ip_address);
			//std::cout << std::endl;
			header.clear(); //TODO this kilss the II, removed header clear
			header.setDstAddress(meta.their_address); //TODO this is weird
			header.setSrcAddress(reg_ip_address);
			header.setNextHeader(meta.next_header);
			header.setPayloadLen(meta.length);
			state = HEADER;
		}
		break;
	case HEADER:
		if (header.consumeWord(currWord.data) < (WIDTH/8))
		{
			state = PARTIAL_HEADER;
		}
		currWord.keep = ~0; //0xFFFFFFFF; //TODO, set as much as required
		currWord.last = 0;
		output.write(currWord);
		break;
	case PARTIAL_HEADER:
		if (!input.empty())
		{
			input.read(currWord);
			header.consumeWord(currWord.data);
			output.write(currWord);
			state = BODY;
			if (currWord.last)
			{
				state = META;
			}
		}
		break;
	case BODY:
		if (!input.empty())
		{
			input.read(currWord);
			output.write(currWord);
			if (currWord.last)
			{
				state = META;
			}
		}
		break;
	}
}

template <int WIDTH>
void ipv6(	hls::stream<net_axis<WIDTH> >&	s_axis_rx_data,
				hls::stream<ipv6Meta>&	m_axis_rx_meta,
				hls::stream<net_axis<WIDTH> >&	m_axis_rx_data,
				hls::stream<ipv6Meta>&	s_axis_tx_meta,
				hls::stream<net_axis<WIDTH> >&	s_axis_tx_data,
				hls::stream<net_axis<WIDTH> >&	m_axis_tx_data,
				ap_uint<128>		reg_ip_address)
{
#pragma HLS INLINE

	/*
	 * RX PATH
	 */
	static stream<net_axis<WIDTH> >	rx_ipv62shiftFifo("rx_ipv62shiftFifo");
	//TODO DATAPACKs
	#pragma HLS STREAM depth=8 variable=rx_ipv62shiftFifo

	process_ipv6(s_axis_rx_data, m_axis_rx_meta, rx_ipv62shiftFifo);
	rshiftWordByOctet<net_axis<WIDTH>, WIDTH, 1>(((IPV6_HEADER_SIZE%WIDTH)/8), rx_ipv62shiftFifo, m_axis_rx_data);

	/*
	 * TX PATH
	 */
	static stream<net_axis<WIDTH> >	tx_shift2ipv6Fifo("tx_shift2ipv6Fifo");
	#pragma HLS STREAM depth=8 variable=tx_shift2ipv6Fifo

	lshiftWordByOctet<WIDTH, 2>(((IPV6_HEADER_SIZE%WIDTH)/8), s_axis_tx_data, tx_shift2ipv6Fifo);
	generate_ipv6(s_axis_tx_meta, tx_shift2ipv6Fifo, m_axis_tx_data, reg_ip_address);
}


void ipv6_top(	hls::stream<net_axis<DATA_WIDTH> >&	s_axis_rx_data,
					hls::stream<ipv6Meta>&	m_axis_rx_meta,
					hls::stream<net_axis<DATA_WIDTH> >&	m_axis_rx_data,
					hls::stream<ipv6Meta>&	s_axis_tx_meta,
					hls::stream<net_axis<DATA_WIDTH> >&	s_axis_tx_data,
					hls::stream<net_axis<DATA_WIDTH> >&	m_axis_tx_data,
					ap_uint<128>		reg_ip_address)
{
#pragma HLS DATAFLOW //disable_start_propagation
#pragma HLS INTERFACE ap_ctrl_none  port=return

#pragma HLS resource core=AXI4Stream variable=s_axis_rx_data metadata="-bus_bundle s_axis_rx_data"
#pragma HLS resource core=AXI4Stream variable=m_axis_rx_meta metadata="-bus_bundle m_axis_rx_meta"
#pragma HLS resource core=AXI4Stream variable=m_axis_rx_data metadata="-bus_bundle m_axis_rx_data"
#pragma HLS resource core=AXI4Stream variable=s_axis_tx_meta metadata="-bus_bundle s_axis_tx_meta"
#pragma HLS resource core=AXI4Stream variable=s_axis_tx_data metadata="-bus_bundle s_axis_tx_data"
#pragma HLS resource core=AXI4Stream variable=m_axis_tx_data metadata="-bus_bundle m_axis_tx_data"
#pragma HLS DATA_PACK variable=m_axis_rx_meta
#pragma HLS DATA_PACK variable=s_axis_tx_meta
#pragma HLS INTERFACE ap_none register port=reg_ip_address


	ipv6<DATA_WIDTH>(	s_axis_rx_data,
							m_axis_rx_meta,
							m_axis_rx_data,
							s_axis_tx_meta,
							s_axis_tx_data,
							m_axis_tx_data,
							reg_ip_address);

}