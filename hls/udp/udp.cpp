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
#include "udp_config.hpp"
#include "udp.hpp"

template <int WIDTH>
void process_udp(	stream<net_axis<WIDTH> >& input,
					stream<udpMeta>& metaOut,
					stream<net_axis<WIDTH> >& output,
               ap_uint<16>       regListenPort)
{
#pragma HLS inline off
#pragma HLS pipeline II=1

	static udpHeader<WIDTH> pu_header;
	static bool metaWritten = false;
	net_axis<WIDTH> currWord;

	if (!input.empty())
	{
		input.read(currWord);

		/*std::cout << "UDP: ";
		print(std::cout, currWord);
		std::cout << std::endl;*/

		pu_header.parseWord( currWord.data);

		if (metaWritten && pu_header.getDstPort() == regListenPort && WIDTH <= UDP_HEADER_SIZE)
		{
			output.write(currWord);
		}

		if (pu_header.isReady())
		{
			//Check Dst Port
			ap_uint<16> dstPort = pu_header.getDstPort();
			if (dstPort == regListenPort && WIDTH > UDP_HEADER_SIZE)
			{
				output.write(currWord);
			}
			if (!metaWritten)
			{
				std::cout << "UDP dst Port: " << (uint16_t)dstPort << std::endl;
				metaOut.write(udpMeta(pu_header.getSrcPort(), dstPort, pu_header.getLength(), dstPort == regListenPort));
				metaWritten = true;
			}
		}
		if (currWord.last)
		{
			metaWritten = false;
			pu_header.clear();
		}
	}
}

template <int WIDTH>
void generate_udp(	stream<udpMeta>& metaIn,
					stream<net_axis<WIDTH> >& input,
					stream<net_axis<WIDTH> >& output)
{
#pragma HLS inline off
#pragma HLS pipeline II=1

	enum fsmState {META, HEADER, PARTIAL_HEADER, BODY};
	static fsmState state = META;
	static udpHeader<WIDTH> header; //TODO meta or header??

	udpMeta meta;
	net_axis<WIDTH> currWord;

	switch (state)
	{
	case META:
		if (!metaIn.empty())
		{
			metaIn.read(meta);
			header.clear();
			header.setDstPort(meta.their_port);
			header.setSrcPort(meta.my_port);
			header.setLength(meta.length);
			if (UDP_HEADER_SIZE >= WIDTH)
			{
				state = HEADER;
			}
			else
			{
				state = PARTIAL_HEADER;
			}
		}
		break;
	case HEADER:
		if (header.consumeWord(currWord.data) < (WIDTH/8))
		{
			state = PARTIAL_HEADER;
		}
		currWord.keep = 0xFFFFFFFF; //TODO, set as much as required
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

/*void check_udp_checksum(	stream<net_axis<WIDTH> >& input,
						stream<net_axis<WIDTH> >& output)
{
}*/


//TODO compact ipMeta
void split_tx_meta(	stream<ipUdpMeta>&	metaIn,
					stream<ipMeta>&		metaOut0,
					stream<udpMeta>&	metaOut1)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	ipUdpMeta meta;
	if (!metaIn.empty())
	{
		metaIn.read(meta);
		//Add 8 bytes for UDP header
		ap_uint<16> tempLen = meta.length+8;
		metaOut0.write(ipMeta(meta.their_address, tempLen));
		metaOut1.write(udpMeta(meta.their_port, meta.my_port, tempLen));
	}
}

void merge_rx_meta(	stream<ipMeta>&		ipMetaIn,
					stream<udpMeta>&	udpMetaIn,
					stream<ipUdpMeta>&	metaOut)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	ipMeta meta0;
	udpMeta meta1;

	if (!ipMetaIn.empty() && !udpMetaIn.empty())
	{
		ipMetaIn.read(meta0);
		udpMetaIn.read(meta1);
		if (meta1.valid)
		{
			metaOut.write(ipUdpMeta(meta0.their_address, meta1.their_port, meta1.my_port, meta1.length));
		}
	}
}


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
				ap_uint<16>			reg_listen_port)
{
#pragma HLS INLINE



	/*
	 * RX PATH
	 */
	static hls::stream<net_axis<WIDTH> >	rx_udp2shiftFifo("rx_udp2shiftFifo");
	static hls::stream<udpMeta>	rx_udpMetaFifo("rx_udpMetaFifo");
	#pragma HLS STREAM depth=2 variable=rx_udp2shiftFifo
	#pragma HLS STREAM depth=2 variable=rx_udpMetaFifo
	#pragma HLS aggregate  variable=rx_udpMetaFifo compact=bit

	process_udp<WIDTH>(s_axis_rx_data, rx_udpMetaFifo, rx_udp2shiftFifo, reg_listen_port);
	udp_rshiftWordByOctet<net_axis<WIDTH>, WIDTH, 2>(((UDP_HEADER_SIZE%WIDTH)/8), rx_udp2shiftFifo, m_axis_rx_data);

	merge_rx_meta(s_axis_rx_meta, rx_udpMetaFifo, m_axis_rx_meta);

	/*
	 * TX PATH
	 */
	static hls::stream<net_axis<WIDTH> >	tx_shift2udpFifo("tx_shift2udpFifo");
	static hls::stream<net_axis<WIDTH> >	tx_udp2shiftFifo("tx_udp2shiftFifo");
	static hls::stream<udpMeta>	tx_udpMetaFifo("tx_udpMetaFifo");
	#pragma HLS STREAM depth=2 variable=tx_shift2udpFifo
	#pragma HLS STREAM depth=2 variable=tx_udp2shiftFifo
	#pragma HLS STREAM depth=2 variable=tx_udpMetaFifo

	split_tx_meta(s_axis_tx_meta, m_axis_tx_meta, tx_udpMetaFifo);

	udp_lshiftWordByOctet<WIDTH, 1>(((UDP_HEADER_SIZE%WIDTH)/8), s_axis_tx_data, tx_shift2udpFifo);

	generate_udp<WIDTH>(tx_udpMetaFifo, tx_shift2udpFifo, m_axis_tx_data);
}

void udp_top(	hls::stream<ipMeta>&		s_axis_rx_meta,
				hls::stream<ap_axiu<DATA_WIDTH, 0, 0, 0> >&	s_axis_rx_data,
				hls::stream<ipUdpMeta>&	m_axis_rx_meta,
				hls::stream<ap_axiu<DATA_WIDTH, 0, 0, 0> >&	m_axis_rx_data,
				hls::stream<ipUdpMeta>&	s_axis_tx_meta,
				hls::stream<ap_axiu<DATA_WIDTH, 0, 0, 0> >&	s_axis_tx_data,
				hls::stream<ipMeta>&		m_axis_tx_meta,
				hls::stream<ap_axiu<DATA_WIDTH, 0, 0, 0> >&	m_axis_tx_data,
				ap_uint<128>		reg_ip_address,
				ap_uint<16>			reg_listen_port)

{
#pragma HLS DATAFLOW disable_start_propagation
#pragma HLS INTERFACE ap_ctrl_none port=return

#pragma HLS INTERFACE axis register port=s_axis_rx_meta
#pragma HLS INTERFACE axis register port=s_axis_rx_data
#pragma HLS INTERFACE axis register port=m_axis_rx_meta
#pragma HLS INTERFACE axis register port=m_axis_rx_data
#pragma HLS INTERFACE axis register port=s_axis_tx_meta
#pragma HLS INTERFACE axis register port=s_axis_tx_data
#pragma HLS INTERFACE axis register port=m_axis_tx_meta
#pragma HLS INTERFACE axis register port=m_axis_tx_data
#pragma HLS aggregate  variable=s_axis_rx_meta compact=bit
#pragma HLS aggregate  variable=m_axis_rx_meta compact=bit
#pragma HLS aggregate  variable=s_axis_tx_meta compact=bit
#pragma HLS aggregate  variable=m_axis_tx_meta compact=bit
#pragma HLS INTERFACE ap_none register port=reg_ip_address
#pragma HLS INTERFACE ap_none register port=reg_listen_port

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

   	udp<DATA_WIDTH>(s_axis_rx_meta,
                   s_axis_rx_data_internal,
                   m_axis_rx_meta,
                   m_axis_rx_data_internal,
                   s_axis_tx_meta,
                   s_axis_tx_data_internal,
                   m_axis_tx_meta,
                   m_axis_tx_data_internal,
                   reg_ip_address,
                   reg_listen_port);
}

