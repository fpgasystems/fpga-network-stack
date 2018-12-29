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
#include "ipv4.hpp"

void process_ipv4(	stream<axiWord>&		dataIn,
					stream<ap_uint<4> >&	process2dropLengthFifo,
					stream<ipv4Meta>&			MetaOut,
					stream<axiWord>&		dataOut)
{
#pragma HLS INLINE off
#pragma HLS pipeline II=1

	static ipv4Header<AXI_WIDTH> header;
	static bool metaWritten = false;

	axiWord currWord;

	if (!dataIn.empty())
	{
		dataIn.read(currWord);
		header.parseWord(currWord.data);

		if (header.isReady())
		{
			dataOut.write(currWord);
			if (!metaWritten)
			{
				process2dropLengthFifo.write(header.getHeaderLength()-4);
				MetaOut.write(ipv4Meta(header.getSrcAddr(), header.getLength()));
				metaWritten = true;
			}
		}
		if (currWord.last)
		{
			metaWritten = false;
			header.clear();
		}
	}
}

void drop_optional_header(	stream<ap_uint<4> >&	process2dropLengthFifo,
							stream<axiWord>&	process2dropFifo,
							stream<axiWord>&	dataOut)
{
#pragma HLS INLINE off
#pragma HLS pipeline II=1

	enum fsmStateType {META, DROP, BODY, SHIFT, LAST};
	static fsmStateType doh_state = META;
	static ap_uint<4> length;

	static axiWord prevWord;
	axiWord currWord;
	axiWord sendWord;

	//TODO length deduction depends on AXI_WIDTH
	switch (doh_state)
	{
	case META:
		if (!process2dropLengthFifo.empty() && !process2dropFifo.empty())
		{
			process2dropLengthFifo.read(length);
			if (length > 1)
			{
				doh_state = DROP;
			}
			else
			{
				process2dropFifo.read(prevWord);
				doh_state = SHIFT;
			}
		}
		break;
	case DROP:
		if (!process2dropFifo.empty())
		{
			process2dropFifo.read(prevWord);
			length -= 2;
			if (length == 1)
			{
				doh_state = SHIFT;
			} else if (length == 0)
			{
				doh_state = BODY;
			}
		}
		break;
	case BODY:
		if (!process2dropFifo.empty())
		{
			process2dropFifo.read(currWord);
			dataOut.write(currWord);
			if (currWord.last)
			{
				doh_state = META;
			}
		}
		break;
	case SHIFT:
		if (!process2dropFifo.empty())
		{
			process2dropFifo.read(currWord);
			sendWord.data(31, 0) = prevWord.data(63, 32);
			sendWord.keep(3, 0) = prevWord.keep(7, 4);
			sendWord.data(63, 32) = currWord.data(31, 0);
			sendWord.keep(7, 4) = currWord.keep(3, 0);
			sendWord.last = (currWord.keep[4] == 0);
			dataOut.write(sendWord);
			prevWord = currWord;
			if (sendWord.last)
			{
				doh_state = META;
			}
			else if (currWord.last)
			{
				doh_state = LAST;
			}
		}
		break;
	case LAST:
		sendWord.data(31, 0) = prevWord.data(63, 32);
		sendWord.keep(3, 0) = prevWord.keep(7, 4);
		sendWord.data(63, 32) = 0;
		sendWord.keep(7, 4) = 0x0;
		sendWord.last = 0x1;
		dataOut.write(sendWord);
		doh_state = META;
		break;
	} //switch
}

void generate_ipv4( stream<ipv4Meta>&		txEng_ipMetaDataFifoIn,
					stream<axiWord>&	tx_shift2ipv4Fifo,
					stream<axiWord>&	m_axis_tx_data,
					ap_uint<32>			local_ipv4_address)
{
#pragma HLS INLINE off
#pragma HLS pipeline II=1

	enum fsmStateType {META, HEADER, PARTIAL_HEADER, BODY};
	static fsmStateType gi_state=META;
	static ipv4Header<AXI_WIDTH> header;

	ipv4Meta meta;
	axiWord currWord;
	ap_uint<16>  length;

	switch (gi_state)
	{
	case META:
		if (!txEng_ipMetaDataFifoIn.empty())
		{
			txEng_ipMetaDataFifoIn.read(meta);
			header.clear();

			length = meta.length + 20;
			header.setLength(length);
			header.setDstAddr(meta.their_address);
			header.setSrcAddr(local_ipv4_address);
			header.setProtocol(UDP_PROTOCOL);

			gi_state = HEADER;
		}
		break;
	case HEADER:
		if (header.consumeWord(currWord.data))
		{
			/*currWord.keep = 0xFFFFFFFF; //TODO, set as much as required
			currWord.last = 0;
			m_axis_tx_data.write(currWord);*/
			gi_state = PARTIAL_HEADER;
		}
		//else
		{
			currWord.keep = 0xFFFFFFFF; //TODO, set as much as required
			currWord.last = 0;
			m_axis_tx_data.write(currWord);
			//gi_state = PARTIAL_HEADER;
		}
		break;
	case PARTIAL_HEADER:
		if (!tx_shift2ipv4Fifo.empty())
		{
			tx_shift2ipv4Fifo.read(currWord);
			header.consumePartialWord(currWord.data);
			m_axis_tx_data.write(currWord);
			gi_state = BODY;

			if (currWord.last)
			{
				gi_state = META;
			}
		}
		break;
	case BODY:
		if (!tx_shift2ipv4Fifo.empty())
		{
			tx_shift2ipv4Fifo.read(currWord);
			m_axis_tx_data.write(currWord);
			if (currWord.last)
			{
				gi_state = META;
			}
		}
		break;
	}
}

void ipv4(		hls::stream<axiWord>&	s_axis_rx_data,
				hls::stream<ipv4Meta>&		m_axis_rx_meta,
				hls::stream<axiWord>&	m_axis_rx_data,
				hls::stream<ipv4Meta>&		s_axis_tx_meta,
				hls::stream<axiWord>&	s_axis_tx_data,
				hls::stream<axiWord>&	m_axis_tx_data,
				ap_uint<32>			local_ipv4_address)
{
#pragma HLS DATAFLOW
#pragma HLS INTERFACE ap_ctrl_none register port=return
//#pragma HLS INLINE

#pragma HLS resource core=AXI4Stream variable=s_axis_rx_data metadata="-bus_bundle s_axis_rx_data"
#pragma HLS resource core=AXI4Stream variable=m_axis_rx_meta metadata="-bus_bundle m_axis_rx_meta"
#pragma HLS resource core=AXI4Stream variable=m_axis_rx_data metadata="-bus_bundle m_axis_rx_data"
#pragma HLS resource core=AXI4Stream variable=s_axis_tx_meta metadata="-bus_bundle s_axis_tx_meta"
#pragma HLS resource core=AXI4Stream variable=s_axis_tx_data metadata="-bus_bundle s_axis_tx_data"
#pragma HLS resource core=AXI4Stream variable=m_axis_tx_data metadata="-bus_bundle m_axis_tx_data"
#pragma HLS DATA_PACK variable=m_axis_rx_meta
#pragma HLS DATA_PACK variable=s_axis_tx_meta
#pragma HLS INTERFACE ap_stable register port=local_ipv4_address

	/*
	 * FIFOs
	 */
	static hls::stream<ap_uint<4> > rx_process2dropLengthFifo("rx_process2dropLengthFifo");
	static hls::stream<axiWord> rx_process2dropFifo("rx_process2dropFifo");
	static hls::stream<axiWord> tx_shift2ipv4Fifo("tx_shift2ipv4Fifo");
	#pragma HLS STREAM depth=2 variable=rx_process2dropLengthFifo
	#pragma HLS STREAM depth=8 variable=rx_process2dropFifo
	#pragma HLS STREAM depth=8 variable=tx_shift2ipv4Fifo

	/*
	 * RX PATH
	 */
	process_ipv4(s_axis_rx_data, rx_process2dropLengthFifo, m_axis_rx_meta, rx_process2dropFifo);
	//TODO maybe assume no optional header fields!
	drop_optional_header(rx_process2dropLengthFifo, rx_process2dropFifo, m_axis_rx_data);

	/*
	 * TX PATH
	 */
	lshiftWordByOctet<AXI_WIDTH, 2>(((IPV4_HEADER_SIZE%AXI_WIDTH)/8), s_axis_tx_data, tx_shift2ipv4Fifo);
	generate_ipv4(s_axis_tx_meta, tx_shift2ipv4Fifo, m_axis_tx_data, local_ipv4_address);}
