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
#include "ipv4_config.hpp"
#include "ipv4.hpp"

template <int WIDTH>
void process_ipv4(	stream<net_axis<WIDTH> >&		dataIn,
					stream<ap_uint<4> >&	process2dropLengthFifo,
					stream<ipv4Meta>&			MetaOut,
					stream<net_axis<WIDTH> >&		dataOut)
{
#pragma HLS INLINE off
#pragma HLS pipeline II=1

	static ipv4Header<WIDTH> header;
	static ap_uint<4> headerWordsDropped = 0;
	static bool metaWritten = false;

	net_axis<WIDTH> currWord;

	if (!dataIn.empty())
	{
		dataIn.read(currWord);
		header.parseWord(currWord.data);

		if (header.isReady())
		{
			dataOut.write(currWord);
			if (!metaWritten)
			{
				std::cout << "IP HEADER: src address: " << header.getSrcAddr() << ", length: " << header.getLength() << std::endl;
				process2dropLengthFifo.write(header.getHeaderLength() - headerWordsDropped);
				MetaOut.write(ipv4Meta(header.getSrcAddr(), header.getLength()));
				metaWritten = true;
			}
		}

		headerWordsDropped += (WIDTH/32);
		if (currWord.last)
		{
			metaWritten = false;
			header.clear();
			headerWordsDropped = 0;
		}
	}
}

template <int WIDTH>
void generate_ipv4( stream<ipv4Meta>&		txEng_ipMetaDataFifoIn,
					stream<net_axis<WIDTH> >&	tx_shift2ipv4Fifo,
					stream<net_axis<WIDTH> >&	m_axis_tx_data,
					ap_uint<32>			local_ipv4_address,
					ap_uint<8>			protocol)
{
#pragma HLS INLINE off
#pragma HLS pipeline II=1

	enum fsmStateType {META, HEADER, PARTIAL_HEADER, BODY};
	static fsmStateType gi_state=META;
	static ipv4Header<WIDTH> header;

	ipv4Meta meta;
	net_axis<WIDTH> currWord;
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
			header.setProtocol(protocol);
			if (IPV4_HEADER_SIZE >= WIDTH)
			{
				gi_state = HEADER;
			}
			else
			{
				gi_state = PARTIAL_HEADER;
			}
		}
		break;
	case HEADER:
		if (header.consumeWord(currWord.data) < (WIDTH/8))
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
			header.consumeWord(currWord.data);
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

template <int WIDTH>
void ipv4_generate_ipv4( stream<ipv4Meta>&		txEng_ipMetaDataFifoIn,
					stream<net_axis<WIDTH> >&	tx_shift2ipv4Fifo,
					stream<net_axis<WIDTH> >&	m_axis_tx_data,
					ap_uint<32>			local_ipv4_address,
					ap_uint<8>			protocol)
{
#pragma HLS INLINE off
#pragma HLS pipeline II=1

	enum fsmStateType {META, HEADER, PARTIAL_HEADER, BODY};
	static fsmStateType gi_state=META;
	static ipv4Header<WIDTH> header;

	ipv4Meta meta;
	net_axis<WIDTH> currWord;
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
			header.setProtocol(protocol);
			if (IPV4_HEADER_SIZE >= WIDTH)
			{
				gi_state = HEADER;
			}
			else
			{
				gi_state = PARTIAL_HEADER;
			}
		}
		break;
	case HEADER:
		if (header.consumeWord(currWord.data) < (WIDTH/8))
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
			header.consumeWord(currWord.data);
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


template <int WIDTH>
void ipv4(		hls::stream<net_axis<WIDTH> >&	s_axis_rx_data,
				hls::stream<ipv4Meta>&		m_axis_rx_meta,
				hls::stream<net_axis<WIDTH> >&	m_axis_rx_data,
				hls::stream<ipv4Meta>&		s_axis_tx_meta,
				hls::stream<net_axis<WIDTH> >&	s_axis_tx_data,
				hls::stream<net_axis<WIDTH> >&	m_axis_tx_data,
				ap_uint<32>			local_ipv4_address,
				ap_uint<8>			protocol)
{
#pragma HLS INLINE


	/*
	 * FIFOs
	 */
	static hls::stream<ap_uint<4> > rx_process2dropLengthFifo("rx_process2dropLengthFifo");
	static hls::stream<net_axis<WIDTH> > rx_process2dropFifo("rx_process2dropFifo");
	static hls::stream<net_axis<WIDTH> > tx_shift2ipv4Fifo("tx_shift2ipv4Fifo");
	#pragma HLS STREAM depth=2 variable=rx_process2dropLengthFifo
	#pragma HLS STREAM depth=8 variable=rx_process2dropFifo
	#pragma HLS STREAM depth=8 variable=tx_shift2ipv4Fifo

	/*
	 * RX PATH
	 */
	process_ipv4(s_axis_rx_data, rx_process2dropLengthFifo, m_axis_rx_meta, rx_process2dropFifo);
	//Assumes for WIDTH > 64 no optional fields
	ipv4_drop_optional_ip_header(rx_process2dropLengthFifo, rx_process2dropFifo, m_axis_rx_data);

	/*
	 * TX PATH
	 */
	ipv4_lshiftWordByOctet<WIDTH, 2>(((IPV4_HEADER_SIZE%WIDTH)/8), s_axis_tx_data, tx_shift2ipv4Fifo);
	ipv4_generate_ipv4(s_axis_tx_meta, tx_shift2ipv4Fifo, m_axis_tx_data, local_ipv4_address, protocol);
}

void ipv4_top(		hls::stream<ap_axiu<DATA_WIDTH, 0, 0, 0> >&	s_axis_rx_data,
				hls::stream<ipv4Meta>&		m_axis_rx_meta,
				hls::stream<ap_axiu<DATA_WIDTH, 0, 0, 0> >&	m_axis_rx_data,
				hls::stream<ipv4Meta>&		s_axis_tx_meta,
				hls::stream<ap_axiu<DATA_WIDTH, 0, 0, 0> >&	s_axis_tx_data,
				hls::stream<ap_axiu<DATA_WIDTH, 0, 0, 0> >&	m_axis_tx_data,
				ap_uint<32>			local_ipv4_address,
				ap_uint<8>			protocol)
{
#pragma HLS DATAFLOW disable_start_propagation
#pragma HLS INTERFACE ap_ctrl_none port=return

#pragma HLS INTERFACE axis register port=s_axis_rx_data
#pragma HLS INTERFACE axis register port=m_axis_rx_meta
#pragma HLS INTERFACE axis register port=m_axis_rx_data
#pragma HLS INTERFACE axis register port=s_axis_tx_meta
#pragma HLS INTERFACE axis register port=s_axis_tx_data
#pragma HLS INTERFACE axis register port=m_axis_tx_data
#pragma HLS aggregate  variable=m_axis_rx_meta compact=bit
#pragma HLS aggregate  variable=s_axis_tx_meta compact=bit
#pragma HLS INTERFACE ap_none register port=local_ipv4_address
#pragma HLS INTERFACE ap_none register port=protocol

	static hls::stream<net_axis<DATA_WIDTH> > s_axis_rx_data_internal;
	#pragma HLS STREAM depth=2 variable=s_axis_rx_data_internal
	static hls::stream<net_axis<DATA_WIDTH> > s_axis_tx_data_internal;
	#pragma HLS STREAM depth=2 variable=s_axis_tx_data_internal
	static hls::stream<net_axis<DATA_WIDTH> > m_axis_rx_data_internal;
	#pragma HLS STREAM depth=2 variable=m_axis_rx_data_internal
	static hls::stream<net_axis<DATA_WIDTH> > m_axis_tx_data_internal;
	#pragma HLS STREAM depth=2 variable=m_axis_tx_data_internal


	convert_axis_to_net_axis<DATA_WIDTH>(s_axis_rx_data, 
							s_axis_rx_data_internal);

	convert_axis_to_net_axis<DATA_WIDTH>(s_axis_tx_data, 
							s_axis_tx_data_internal);

	convert_net_axis_to_axis<DATA_WIDTH>(m_axis_rx_data_internal, 
							m_axis_rx_data);

	convert_net_axis_to_axis<DATA_WIDTH>(m_axis_tx_data_internal, 
							m_axis_tx_data);

   	ipv4<DATA_WIDTH>(s_axis_rx_data_internal,
        m_axis_rx_meta,
        m_axis_rx_data_internal,
        s_axis_tx_meta,
        s_axis_tx_data_internal,
        m_axis_tx_data_internal,
        local_ipv4_address,
		protocol);

};


