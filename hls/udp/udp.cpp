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
#include "udp.hpp"

void process_udp(	stream<axiWord>& input,
					stream<udpMeta>& metaOut,
					stream<axiWord>& output,
               ap_uint<16>       regListenPort)
{
#pragma HLS inline off
#pragma HLS pipeline II=1

	static udpHeader<AXI_WIDTH> pu_header;
	static bool metaWritten = false;
	axiWord currWord;

	if (!input.empty())
	{
		input.read(currWord);

		/*std::cout << "UDP: ";
		print(std::cout, currWord);
		std::cout << std::endl;*/

		pu_header.parseWord( currWord.data);
		if (pu_header.isReady())
		{
			//Check Dst Port
			ap_uint<16> dstPort = pu_header.getDstPort();
			//std::cout << "UDP dst Port: " << (uint16_t)dstPort << std::endl;
			if (dstPort == regListenPort)
			{
				output.write(currWord);
			}
			if (!metaWritten)
			{
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

void generate_udp(	stream<udpMeta>& metaIn,
					stream<axiWord>& input,
					stream<axiWord>& output)
{
#pragma HLS inline off
#pragma HLS pipeline II=1

	enum fsmState {META, HEADER, PARTIAL_HEADER, BODY};
	static fsmState state = META;
	static udpHeader<AXI_WIDTH> header; //TODO meta or header??

	udpMeta meta;
	axiWord currWord;

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
			state = HEADER;
		}
		break;
	case HEADER:
		if (header.consumeWord(currWord.data)) //TODO this gives a timing of 5.6ns
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
			header.consumePartialWord(currWord.data);
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

/*void check_udp_checksum(	stream<axiWord>& input,
						stream<axiWord>& output)
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

void udp(		hls::stream<ipMeta>&		s_axis_rx_meta,
				hls::stream<axiWord>&	s_axis_rx_data,
				hls::stream<ipUdpMeta>&	m_axis_rx_meta,
				hls::stream<axiWord>&	m_axis_rx_data,
				hls::stream<ipUdpMeta>&	s_axis_tx_meta,
				hls::stream<axiWord>&	s_axis_tx_data,
				hls::stream<ipMeta>&		m_axis_tx_meta,
				hls::stream<axiWord>&	m_axis_tx_data,
				ap_uint<128>		reg_ip_address,
				ap_uint<16>			reg_listen_port)
{
#pragma HLS DATAFLOW
#pragma HLS INTERFACE ap_ctrl_none register port=return
//#pragma HLS INLINE

#pragma HLS resource core=AXI4Stream variable=s_axis_rx_meta metadata="-bus_bundle s_axis_rx_meta"
#pragma HLS resource core=AXI4Stream variable=s_axis_rx_data metadata="-bus_bundle s_axis_rx_data"
#pragma HLS resource core=AXI4Stream variable=m_axis_rx_meta metadata="-bus_bundle m_axis_rx_meta"
#pragma HLS resource core=AXI4Stream variable=m_axis_rx_data metadata="-bus_bundle m_axis_rx_data"
#pragma HLS resource core=AXI4Stream variable=s_axis_tx_meta metadata="-bus_bundle s_axis_tx_meta"
#pragma HLS resource core=AXI4Stream variable=s_axis_tx_data metadata="-bus_bundle s_axis_tx_data"
#pragma HLS resource core=AXI4Stream variable=m_axis_tx_meta metadata="-bus_bundle m_axis_tx_meta"
#pragma HLS resource core=AXI4Stream variable=m_axis_tx_data metadata="-bus_bundle m_axis_tx_data"
#pragma HLS DATA_PACK variable=s_axis_rx_meta
#pragma HLS DATA_PACK variable=m_axis_rx_meta
#pragma HLS DATA_PACK variable=s_axis_tx_meta
#pragma HLS DATA_PACK variable=m_axis_tx_meta
#pragma HLS INTERFACE ap_stable register port=reg_ip_address
#pragma HLS INTERFACE ap_stable register port=reg_listen_port


	/*
	 * RX PATH
	 */
	static hls::stream<axiWord>	rx_udp2shiftFifo("rx_udp2shiftFifo");
	static hls::stream<udpMeta>	rx_udpMetaFifo("rx_udpMetaFifo");
	#pragma HLS STREAM depth=2 variable=rx_udp2shiftFifo
	#pragma HLS STREAM depth=2 variable=rx_udpMetaFifo

	process_udp(s_axis_rx_data, rx_udpMetaFifo, rx_udp2shiftFifo, reg_listen_port);
	rshiftWordByOctet<axiWord, AXI_WIDTH, 2>(((UDP_HEADER_SIZE%AXI_WIDTH)/8), rx_udp2shiftFifo, m_axis_rx_data);

	merge_rx_meta(s_axis_rx_meta, rx_udpMetaFifo, m_axis_rx_meta);

	/*
	 * TX PATH
	 */
	static hls::stream<axiWord>	tx_shift2udpFifo("tx_shift2udpFifo");
	static hls::stream<axiWord>	tx_udp2shiftFifo("tx_udp2shiftFifo");
	static hls::stream<udpMeta>	tx_udpMetaFifo("tx_udpMetaFifo");
	#pragma HLS STREAM depth=2 variable=tx_shift2udpFifo
	#pragma HLS STREAM depth=2 variable=tx_udp2shiftFifo
	#pragma HLS STREAM depth=2 variable=tx_udpMetaFifo

	split_tx_meta(s_axis_tx_meta, m_axis_tx_meta, tx_udpMetaFifo);

	lshiftWordByOctet<AXI_WIDTH, 1>(((UDP_HEADER_SIZE%AXI_WIDTH)/8), s_axis_tx_data, tx_shift2udpFifo);

	generate_udp(tx_udpMetaFifo, tx_shift2udpFifo, m_axis_tx_data);
}
