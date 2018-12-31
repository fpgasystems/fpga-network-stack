/************************************************
Copyright (c) 2016, Xilinx, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.// Copyright (c) 2015 Xilinx, Inc.
************************************************/

#include "ip_handler.hpp"

/** @ingroup ip_handler
 *  Detects the MAC protocol in the header of the packet, ARP and IP packets are forwared accordingly,
 *  packets of other protocols are discarded
 *  @param[in]		dataIn
 *  @param[out]		ARPdataOut
 *  @param[out]		IPdataOut
 */
void detect_eth_protocol(	hls::stream<axiWord>&	dataIn,
							hls::stream<ap_uint<16> >&	etherTypeFifo,
							hls::stream<axiWord> &dataOut)
{
	#pragma HLS PIPELINE II=1
	#pragma HLS INLINE off

	enum stateType {FIRST, MIDDLE, LAST};
	static stateType state = FIRST;
	static ethHeader<AXI_WIDTH> header;
	static axiWord prevWord;
	static bool metaWritten = false;

	switch (state)
	{
	case FIRST:
		if (!dataIn.empty())
		{
			axiWord word = dataIn.read();
			header.parseWord(word.data);
			prevWord = word;
			state = MIDDLE;
			if (word.last)
			{
				state = LAST;
			}
		}
		break;
	case MIDDLE:
		if (!dataIn.empty())
		{
			axiWord word = dataIn.read();
			header.parseWord(word.data);

			if (!metaWritten)
			{
				etherTypeFifo.write(header.getEthertype());
				std::cout << "ether type: " << std::hex << header.getEthertype() << std::endl;
				metaWritten = true;
#if AXI_WIDTH < 128
				if (header.getEthertype() == ARP)
#endif
				{
					dataOut.write(prevWord);
				}
			}
			else
			{
				dataOut.write(prevWord);
			}
			prevWord = word;
			if (word.last)
			{
				state = LAST;
			}
		}
		break;
	case LAST:
		if (!metaWritten)
		{
			etherTypeFifo.write(header.getEthertype());
			std::cout << "ether type: " << header.getEthertype() << std::endl;
		}
		dataOut.write(prevWord);
		header.clear();
		metaWritten = false;
		state = FIRST;
		break;
	}//switch
}

void route_by_eth_protocol(	hls::stream<ap_uint<16> >&	etherTypeFifoIn,
							hls::stream<axiWord> &dataIn,
							hls::stream<axiWord> &ARPdataOut,
							hls::stream<axiWord> &IPdataOut,
							hls::stream<axiWord>& IPv6dataOut)
{
	#pragma HLS PIPELINE II=1
	#pragma HLS INLINE off

	static ap_uint<1> rep_fsmState = 0;
	static ap_uint<16> rep_etherType;

	switch (rep_fsmState)
	{
	case 0:
		if (!etherTypeFifoIn.empty() && !dataIn.empty())
		{
			rep_etherType = etherTypeFifoIn.read();
			axiWord word = dataIn.read();
			if (rep_etherType == ARP)
			{
				ARPdataOut.write(word);
			}
			else if (rep_etherType == IPv4)
			{
				IPdataOut.write(word);
			}
			else if (rep_etherType == IPv6)
			{
				IPv6dataOut.write(word);
			}
			if (!word.last)
			{
				rep_fsmState = 1;
			}
		}
		break;
	case 1:
		if (!dataIn.empty())
		{
			axiWord word = dataIn.read();
			if (rep_etherType == ARP)
			{
				ARPdataOut.write(word);
			}
			else if (rep_etherType == IPv4)
			{
				IPdataOut.write(word);
			}
			else if (rep_etherType == IPv6)
			{
				IPv6dataOut.write(word);
			}

			if (word.last)
			{
				rep_fsmState = 0;
			}
		}
		break;
	} //switch
}

/** @ingroup ip_handler
 *  Checks IP checksum and removes MAC wrapper, writes valid into @param ipValidBuffer
 *  @param[in]		dataIn, incoming data stream
 *  @param[in]		myIpAddress, our IP address which is set externally
 *  @param[out]		dataOut, outgoing data stream
 *  @param[out]		ipValidFifoOut
 */
void check_ip_checksum(	hls::stream<axiWord>&		dataIn,
						ap_uint<32>			myIpAddress,
						hls::stream<axiWord>&	dataOut,
						hls::stream<subSums>&	iph_subSumsFifoIn)
{
	#pragma HLS PIPELINE II=1
	#pragma HLS INLINE off

	static ap_uint<17> cics_ip_sums[4] = {0, 0, 0, 0};
	static ap_uint<8> cics_ipHeaderLen = 0;
	static ap_uint<3> cics_wordCount = 0;
	static ap_uint<32> cics_dstIpAddress = 0;

	ap_uint<16> temp;

	if (!dataIn.empty())
	{
		axiWord currWord = dataIn.read();
		dataOut.write(currWord);
		//printLE(std::cout, currWord);
		//std::cout << std::endl;
		switch (cics_wordCount)
		{
		case 0:
		case 1:
			if (cics_wordCount == 0)
			{
				cics_ipHeaderLen = currWord.data(3, 0);
				std::cout << "header len" << cics_ipHeaderLen << std::endl;
			}
			for (int i = 0; i < 4; i++)
			{
			#pragma HLS unroll
				//ap_uint<16> temp;
				temp(7, 0) = currWord.data.range(i*16+15, i*16+8);
				temp(15, 8) = currWord.data.range(i*16+7, i*16);
				cics_ip_sums[i] += temp;
				cics_ip_sums[i] = (cics_ip_sums[i] + (cics_ip_sums[i] >> 16)) & 0xFFFF;
			}
			cics_ipHeaderLen -= 2;
			cics_wordCount++;
			break;
		default:
			if (cics_wordCount == 2)
			{
				cics_wordCount++;
				cics_dstIpAddress = currWord.data(31, 0);
				std::cout << "dest ip address" << std::hex << cics_dstIpAddress << std::endl;
			}
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
					cics_ip_sums[i] += temp;
					cics_ip_sums[i] = (cics_ip_sums[i] + (cics_ip_sums[i] >> 16)) & 0xFFFF;
				}
				cics_ipHeaderLen = 0;
				iph_subSumsFifoIn.write(subSums(cics_ip_sums, ((cics_dstIpAddress == myIpAddress) || (cics_dstIpAddress == 0xFFFFFFFF))));
				break;
			default:
				// Sum up everything
				for (int i = 0; i < 4; i++)
				{
				#pragma HLS unroll
					//ap_uint<16> temp;
					temp(7, 0) = currWord.data.range(i*16+15, i*16+8);
					temp(15, 8) = currWord.data.range(i*16+7, i*16);
					cics_ip_sums[i] += temp;
					cics_ip_sums[i] = (cics_ip_sums[i] + (cics_ip_sums[i] >> 16)) & 0xFFFF;
				}
				if (cics_ipHeaderLen == 2)
				{
					iph_subSumsFifoIn.write(subSums(cics_ip_sums, ((cics_dstIpAddress == myIpAddress) || (cics_dstIpAddress == 0xFFFFFFFF))));
				}
				cics_ipHeaderLen -= 2;
				break;
			}
			break;
		} // switch WORD_N
		if (currWord.last)
		{
			cics_wordCount = 0;
			if (currWord.last)
			{
				for (int i = 0; i < 4; i++)
				{
					#pragma HLS unroll
					cics_ip_sums[i] = 0;
				}
			}
		}
	}
}

void iph_check_ip_checksum(	hls::stream<subSums>&		iph_subSumsFifoOut,
							hls::stream<bool>&			iph_validFifoOut)
{
	#pragma HLS PIPELINE II=1
	#pragma HLS INLINE off

	if (!iph_subSumsFifoOut.empty()) {
		subSums icic_ip_sums = iph_subSumsFifoOut.read();
		icic_ip_sums.sum0 += icic_ip_sums.sum2;
		icic_ip_sums.sum1 += icic_ip_sums.sum3;
		icic_ip_sums.sum0 = (icic_ip_sums.sum0 + (icic_ip_sums.sum0 >> 16)) & 0xFFFF;
		icic_ip_sums.sum1 = (icic_ip_sums.sum1 + (icic_ip_sums.sum1 >> 16)) & 0xFFFF;
		icic_ip_sums.sum0 += icic_ip_sums.sum1;
		icic_ip_sums.sum0 = (icic_ip_sums.sum0 + (icic_ip_sums.sum0 >> 16)) & 0xFFFF;
		icic_ip_sums.sum0 = ~icic_ip_sums.sum0;
		std::cout << "checksum" << icic_ip_sums.sum0(15,0) << std::endl;
		std::cout << "ipMatch" << icic_ip_sums.ipMatch << std::endl;
		iph_validFifoOut.write((icic_ip_sums.sum0(15, 0) == 0x0000) && icic_ip_sums.ipMatch);
	}
}

/** @ingroup ip_handler
 *  Reads a packed and its valid flag in, if the packet is valid it is forwarded,
 *  otherwise it is dropped
 *  @param[in]		inData, incoming data stream
 *  @param[in]		ipValidFifoIn, FIFO containing valid flag to indicate if packet is valid,
 *  @param[out]		outData, outgoing data stream
 */
void ip_invalid_dropper(hls::stream<axiWord>&		dataIn,
						hls::stream<bool>&			ipValidFifoIn,
						hls::stream<axiWord>&		dataOut)
{
	#pragma HLS PIPELINE II=1
	#pragma HLS INLINE off

	enum iid_StateType {GET_VALID, FWD, DROP};
	static iid_StateType iid_state = GET_VALID;

	axiWord currWord;
	bool valid;

	switch (iid_state)
	{
	case GET_VALID: //Drop1
		if (!ipValidFifoIn.empty())
		{
			ipValidFifoIn.read(valid);
			if (valid)
			{
				std::cout << "forwarding packet" << std::endl;
				iid_state = FWD;
			}
			else
			{
				std::cout << "dropping packet" << std::endl;
				iid_state = DROP;
			}
		}
		break;
	case FWD:
		if(!dataIn.empty())
		{
			dataIn.read(currWord);
			dataOut.write(currWord);
			if (currWord.last)
			{
				iid_state = GET_VALID;
			}
		}
		break;
	case DROP:
		if(!dataIn.empty())
		{
			dataIn.read(currWord);
			if (currWord.last)
			{
				iid_state = GET_VALID;
			}
		}
		break;
	} // switch
}


/** @ingroup ip_hanlder
 *
 */
void cut_length(hls::stream<axiWord> &dataIn, hls::stream<axiWord> &dataOut)
{
	#pragma HLS PIPELINE II=1
	#pragma HLS INLINE off

	enum cl_stateType {PKG, DROP};
	static cl_stateType cl_state = PKG;
	static ap_uint<16> cl_wordCount = 0;
	static ap_uint<16> cl_totalLength = 0;
	//static bool cl_drop = false;

	axiWord currWord;
	ap_uint<4> leftLength = 0;

	switch (cl_state)
	{
	case PKG:
		if (!dataIn.empty() && !dataOut.full())
		{
			dataIn.read(currWord);
			switch (cl_wordCount)
			{
			case 0:
				cl_totalLength(7, 0) = currWord.data(31, 24);
				cl_totalLength(15, 8) = currWord.data(23, 16);
				break;
			default:
				if (((cl_wordCount+1)*8) >= cl_totalLength) //last real world
				{
					if (currWord.last == 0)
					{
						cl_state = DROP;
					}
					currWord.last = 1;
					leftLength = cl_totalLength - (cl_wordCount*8);
					currWord.keep = lenToKeep(leftLength);
				}
				break;
			}
			dataOut.write(currWord);
			cl_wordCount++;
			if (currWord.last)
			{
				cl_wordCount = 0;
			}
		} //emtpy
		break;
	case DROP:
		if (!dataIn.empty())
		{
			dataIn.read(currWord);
			if (currWord.last)
			{
				cl_state = PKG;
			}
		}
		break;
	} //switch
}

/** @ingroup ip_handler
 *  Detects IP protocol in the packet. ICMP, UDP and TCP packets are forwarded, packets of other IP protocols are discarded.
 *  @param[in]		dataIn, incoming data stream
 *  @param[out]		ICMPdataOut, outgoing ICMP (Ping) data stream
 *  @param[out]		UDPdataOut, outgoing UDP data stream
 *  @param[out]		TCPdataOut, outgoing TCP data stream
 */
void detect_ipv4_protocol(stream<axiWord> &dataIn,
						stream<axiWord> &ICMPdataOut,
						stream<axiWord> &UDPdataOut,
						stream<axiWord> &TCPdataOut)
{
	#pragma HLS PIPELINE II=1
	#pragma HLS INLINE off

	enum dip_stateType {PKG, LEFTOVER};
	static dip_stateType dip_state = PKG;
	static ap_uint<8> dip_ipProtocol;
	static ap_uint<2> dip_wordCount = 0;
	static axiWord dip_prevWord;
	//static bool dip_leftToWrite = false;

	axiWord currWord;

	switch (dip_state)
	{
	case PKG:
		if (!dataIn.empty())
		{
			dataIn.read(currWord);
			switch (dip_wordCount)
			{
			case 0:
				dip_wordCount++;
				break;
			default:
				if (dip_wordCount == 1)
				{
					dip_ipProtocol = currWord.data(15, 8);
					dip_wordCount++;
				}
				// There is not default, if package does not match any case it is automatically dropped
				switch (dip_ipProtocol)
				{
				case ICMP:
					ICMPdataOut.write(dip_prevWord);
					break;
				case UDP:
					UDPdataOut.write(dip_prevWord);
					break;
				case TCP:
					TCPdataOut.write(dip_prevWord);
					break;
				}
				break;
			}
			dip_prevWord = currWord;
			if (currWord.last)
			{
				dip_wordCount = 0;
				dip_state = LEFTOVER;
			}
		}
		break;
	case LEFTOVER:
		if (!ICMPdataOut.full() && !UDPdataOut.full() && !TCPdataOut.full())
		{
			switch (dip_ipProtocol)
			{
			case ICMP:
				ICMPdataOut.write(dip_prevWord);
				break;
			case UDP:
				UDPdataOut.write(dip_prevWord);
				break;
			case TCP:
				TCPdataOut.write(dip_prevWord);
				break;
			}
			dip_state = PKG;
		}
		break;
	} //switch

}

void detect_ipv6_protocol(	hls::stream<axiWord>& dataIn,
							hls::stream<axiWord>& icmpv6DataOut,
							hls::stream<axiWord>& ipv6UdpDataOut)
{
	#pragma HLS PIPELINE II=1
	#pragma HLS INLINE off

	static ap_uint<1> state = 0;
	static ap_uint<8> nextHeader;

	switch (state)
	{
	case 0:
		if (!dataIn.empty())
		{
			// [11:0] Version, TrafficClass, [31:12] FlowLabel, [47:31] PayLoadLen, [55:48] nextHeader, [63:56] hop limit
			axiWord currWord = dataIn.read();
			printLE(std::cout, currWord);
			std::cout << std::endl;

			nextHeader = currWord.data(55, 48);
			std::cout << "IPv6 next header: " << std::hex << nextHeader << std::endl;

			if (nextHeader == ICMPv6)
			{
				icmpv6DataOut.write(currWord);
			}
			else if (nextHeader == UDP)
			{
				ipv6UdpDataOut.write(currWord);
			}

			if (!currWord.last)
			{
				state = 1;
			}
		}
		break;
	case 1:
		if (!dataIn.empty())
		{
			axiWord currWord = dataIn.read();
			if (nextHeader == ICMPv6)
			{
				icmpv6DataOut.write(currWord);
			}
			else if (nextHeader == UDP)
			{
				ipv6UdpDataOut.write(currWord);
			}

			if (currWord.last)
			{
				state = 0;
			}
		}
		break;
	}
}

/** @ingroup ip_handler
 *  @param[in]		s_axis_raw, incoming data stream
 *  @param[in]		myIpAddress, our IP address
 *  @param[out]		m_axis_ARP, outgoing ARP data stream
 *  @param[out]		m_axis_ICMP, outgoing ICMP (Ping) data stream
 *  @param[out]		m_axis_UDP, outgoing UDP data stream
 *  @param[out]		m_axis_TCP, outgoing TCP data stream
 */
void ip_handler(hls::stream<axiWord>&		s_axis_raw,
				hls::stream<axiWord>&		m_axis_ARP,
				hls::stream<axiWord>&		m_axis_ICMPv6,
				hls::stream<axiWord>&		m_axis_IPv6UDP,
				hls::stream<axiWord>&		m_axis_ICMP,
				hls::stream<axiWord>&		m_axis_UDP,
				hls::stream<axiWord>&		m_axis_TCP,
				ap_uint<32>				myIpAddress)
{
#pragma HLS DATAFLOW
#pragma HLS INTERFACE ap_ctrl_none register port=return
#pragma HLS INLINE off

	/*#pragma HLS INTERFACE axis port=s_axis_raw
	#pragma HLS INTERFACE axis port=m_axis_ARP
	#pragma HLS INTERFACE axis port=m_axis_ICMP
	#pragma HLS INTERFACE axis port=m_axis_UDP
	#pragma HLS INTERFACE axis port=m_axis_TCP*/ // This shit leads to Combinatorial Loops
	#pragma  HLS resource core=AXI4Stream variable=s_axis_raw metadata="-bus_bundle s_axis_raw"
	#pragma  HLS resource core=AXI4Stream variable=m_axis_ARP metadata="-bus_bundle m_axis_ARP"
	#pragma  HLS resource core=AXI4Stream variable=m_axis_ICMPv6 metadata="-bus_bundle m_axis_ICMPv6"
	#pragma  HLS resource core=AXI4Stream variable=m_axis_IPv6UDP metadata="-bus_bundle m_axis_IPv6UDP"
	#pragma  HLS resource core=AXI4Stream variable=m_axis_ICMP metadata="-bus_bundle m_axis_ICMP"
	#pragma  HLS resource core=AXI4Stream variable=m_axis_UDP metadata="-bus_bundle m_axis_UDP"
	#pragma HLS resource core=AXI4Stream variable=m_axis_TCP metadata="-bus_bundle m_axis_TCP"

	#pragma HLS INTERFACE ap_stable register port=myIpAddress

	static hls::stream<ap_uint<16> > etherTypeFifo("etherTypeFifo");
	static hls::stream<axiWord>		ethDataFifo("ethDataFifo");
	static hls::stream<axiWord>		ipv4ShiftFifo("ipv4ShiftFifo");
	static hls::stream<axiWord>		ipv6ShiftFifo("ipv6ShiftFifo");

	static hls::stream<axiWord>		ipDataFifo("ipDataFifo");
	static hls::stream<axiWord>		ipDataCheckFifo("ipDataCheckFifo");
	static hls::stream<axiWord>		ipDataDropFifo("ipDataDropFifo");
	static hls::stream<axiWord>		ipDataCutFifo("ipDataCutFifo");
	static hls::stream<subSums>		iph_subSumsFifoOut("iph_subSumsFifoOut");
	static hls::stream<bool>			ipValidFifo("ipValidFifo");
	#pragma HLS STREAM variable=etherTypeFifo		depth=2
	#pragma HLS STREAM variable=ethDataFifo		depth=4
	#pragma HLS STREAM variable=ipv4ShiftFifo depth=2
	#pragma HLS STREAM variable=ipv6ShiftFifo depth=2
	#pragma HLS STREAM variable=ipDataFifo depth=2
	#pragma HLS STREAM variable=ipDataCheckFifo depth=64 //8, must hold IP header for checksum checking, max. 15 x 32bit
	#pragma HLS STREAM variable=ipDataDropFifo depth=2
	#pragma HLS STREAM variable=ipDataCutFifo depth=2
	#pragma HLS STREAM variable=iph_subSumsFifoOut depth=2
	#pragma HLS STREAM variable=ipValidFifo depth=2
	#pragma HLS DATA_PACK variable=ipDataFifo
	#pragma HLS DATA_PACK variable=ipDataCheckFifo
	#pragma HLS DATA_PACK variable=ipDataDropFifo
	#pragma HLS DATA_PACK variable=iph_subSumsFifoOut
	#pragma HLS DATA_PACK variable=ipDataCutFifo

	static stream<axiWord> ipv6DataFifo("ipv6DataFifo");
	#pragma HLS STREAM variable=ipv6DataFifo depth=2


	detect_eth_protocol(s_axis_raw, etherTypeFifo, ethDataFifo);

	route_by_eth_protocol(etherTypeFifo, ethDataFifo, m_axis_ARP, ipv4ShiftFifo, ipv6ShiftFifo);

	rshiftWordByOctet<axiWord, AXI_WIDTH, 1>(((ETH_HEADER_SIZE%AXI_WIDTH)/8), ipv4ShiftFifo, ipDataFifo);	
	rshiftWordByOctet<axiWord, AXI_WIDTH, 2>(((ETH_HEADER_SIZE%AXI_WIDTH)/8), ipv6ShiftFifo, ipv6DataFifo);	

	check_ip_checksum(ipDataFifo, myIpAddress, ipDataCheckFifo, iph_subSumsFifoOut);

	iph_check_ip_checksum(iph_subSumsFifoOut, ipValidFifo);

	ip_invalid_dropper(ipDataCheckFifo, ipValidFifo, ipDataDropFifo);

	cut_length(ipDataDropFifo, ipDataCutFifo);

	detect_ipv4_protocol(ipDataCutFifo, m_axis_ICMP, m_axis_UDP, m_axis_TCP);

	detect_ipv6_protocol(ipv6DataFifo, m_axis_ICMPv6, m_axis_IPv6UDP);
}
