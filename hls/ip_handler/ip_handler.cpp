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
#include "ip_handler_config.hpp"
#include "ip_handler.hpp"
#include "../ethernet/ethernet.hpp"
#include "../ipv4/ipv4.hpp"

/**
 *  Detects the MAC protocol in the header of the packet, the Ethertype is written etherTypeFifo
 */
template <int WIDTH>
void detect_eth_protocol(	hls::stream<net_axis<WIDTH> >&	dataIn,
							hls::stream<ap_uint<16> >&	etherTypeFifo,
							hls::stream<net_axis<WIDTH> > &dataOut)
{
	#pragma HLS PIPELINE II=1
	#pragma HLS INLINE off

	static ethHeader<WIDTH> header;
	static bool metaWritten = false;

	if (!dataIn.empty())
	{
		net_axis<WIDTH> word = dataIn.read();
		header.parseWord(word.data);
		std::cout << "DETECT ETH: ";
		printLE(std::cout, word);
		std::cout << std::endl;
		if (header.isReady() && !metaWritten)
		{
			etherTypeFifo.write(header.getEthertype());
			std::cout << "ether type: " << std::hex << header.getEthertype() << std::endl;
			metaWritten = true;
		}

		dataOut.write(word);
		if (word.last)
		{
			header.clear();
			metaWritten = false;
		}
	}
}

template <int WIDTH>
void route_by_eth_protocol(	hls::stream<ap_uint<16> >&	etherTypeFifoIn,
							hls::stream<net_axis<WIDTH> > &dataIn,
							hls::stream<net_axis<WIDTH> > &ARPdataOut,
							hls::stream<net_axis<WIDTH> > &IPdataOut,
							hls::stream<net_axis<WIDTH> >& IPv6dataOut)
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
			net_axis<WIDTH> word = dataIn.read();
			if (rep_etherType == ARP)
			{
				ARPdataOut.write(word);
			}
			else if (rep_etherType == IPv4 && WIDTH > 64)
			{
				IPdataOut.write(word);
			}
			else if (rep_etherType == IPv6 && WIDTH > 64)
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
			net_axis<WIDTH> word = dataIn.read();
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

template <int WIDTH>
void extract_ip_meta(hls::stream<net_axis<WIDTH> >&		dataIn,
						hls::stream<net_axis<WIDTH> >&		dataOut,
						hls::stream<ap_uint<8> >&	ipv4Protocol,
						hls::stream<bool>&			validIpAddressFifo,
						ap_uint<32>			myIpAddress)
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
			validIpAddressFifo.write((dstIpAddress == myIpAddress) || (dstIpAddress == 0xFFFFFFFF));
			ipv4Protocol.write(header.getProtocol());

			metaWritten = true;
		}

		if (currWord.last)
		{
			metaWritten = false;
			header.clear();
		}
	}
}


/** 
 *  Reads a packed and its valid flag in, if the packet is valid it is forwarded,
 *  otherwise it is dropped
 */
template <int WIDTH>
void ip_invalid_dropper(hls::stream<net_axis<WIDTH> >&		dataIn,
						hls::stream<bool>&			validChecksumFifoIn,
						hls::stream<bool>&			validIpAddressFifoIn,
						hls::stream<net_axis<WIDTH> >&		dataOut,
						hls::stream<bool>&			ipv4ValidFifoOut)
{
	#pragma HLS PIPELINE II=1
	#pragma HLS INLINE off

	enum iid_StateType {GET_VALID, FWD, DROP};
	static iid_StateType iid_state = GET_VALID;

	switch (iid_state)
	{
	case GET_VALID: //Drop1
		if (!validChecksumFifoIn.empty() && !validIpAddressFifoIn.empty())
		{
			bool validChecksum = validChecksumFifoIn.read();
			bool validIpAddress = validIpAddressFifoIn.read();
			if (validChecksum && validIpAddress)
			{
				std::cout << "forwarding packet" << std::endl;
				ipv4ValidFifoOut.write(true);
				iid_state = FWD;
			}
			else
			{
				std::cout << "dropping packet, checksum valid: " << validChecksum << ", ip valid: " << validIpAddress << std::endl;
				ipv4ValidFifoOut.write(false);
				iid_state = DROP;
			}
		}
		break;
	case FWD:
		if(!dataIn.empty())
		{
			net_axis<WIDTH> currWord = dataIn.read();
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
			net_axis<WIDTH> currWord = dataIn.read();
			if (currWord.last)
			{
				iid_state = GET_VALID;
			}
		}
		break;
	} // switch
}

template <int WIDTH>
void cut_length(hls::stream<net_axis<WIDTH> > &dataIn, hls::stream<net_axis<WIDTH> > &dataOut)
{
	#pragma HLS PIPELINE II=1
	#pragma HLS INLINE off

	enum cl_stateType {PKG, DROP};
	static cl_stateType cl_state = PKG;
	static ap_uint<16> cl_wordCount = 0;
	static ap_uint<16> cl_totalLength = 0;

	net_axis<WIDTH> currWord;
	ap_uint<4> leftLength = 0;

	switch (cl_state)
	{
	case PKG:
		if (!dataIn.empty())
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

void cut_length(hls::stream<net_axis<512> > &dataIn, hls::stream<net_axis<512> > &dataOut)
{
	#pragma HLS PIPELINE II=1
	#pragma HLS INLINE off

	enum cl_stateType {FIRST, REST};
	static cl_stateType cl_state = FIRST;

	switch (cl_state)
	{
	case FIRST:
		if (!dataIn.empty())
		{
			net_axis<512> currWord = dataIn.read();


			ap_uint<16> totalLength;
			totalLength(7, 0) = currWord.data(31, 24);
			totalLength(15, 8) = currWord.data(23, 16);

			if (currWord.last)
			{
				currWord.keep = lenToKeep(totalLength);
			}
			else
			{
				cl_state = REST;
			}
			dataOut.write(currWord);
		} //emtpy
		break;
	case REST:
		if (!dataIn.empty())
		{
			net_axis<512> currWord = dataIn.read();
			dataOut.write(currWord);
			if (currWord.last)
			{
				cl_state = FIRST;
			}
		}
		break;
	} //switch
}


/*
 *  Detects IP protocol in the packet. ICMP, UDP and TCP packets are forwarded, packets of other IP protocols are discarded.
 */
template <int WIDTH>
void detect_ipv4_protocol(	hls::stream<ap_uint<8> >&	ipv4ProtocolIn,
							hls::stream<bool>&			ipv4ValidIn,
							hls::stream<net_axis<WIDTH> >&		dataIn,
							hls::stream<net_axis<WIDTH> >&		ICMPdataOut,
							hls::stream<net_axis<WIDTH> >&		UDPdataOut,
							hls::stream<net_axis<WIDTH> >&		TCPdataOut)
{
	#pragma HLS PIPELINE II=1
	#pragma HLS INLINE off

	enum dip_stateType {META, PKG};
	static dip_stateType dip_state = META;
	static ap_uint<8> dip_ipProtocol;

	switch (dip_state)
	{
	case META:
		if (!ipv4ProtocolIn.empty() && !ipv4ValidIn.empty())
		{
			dip_ipProtocol = ipv4ProtocolIn.read();
			bool valid = ipv4ValidIn.read();
			if (valid)
			{
				dip_state = PKG;
			}
		}
		break;
	case PKG:
		if (!dataIn.empty())
		{
			net_axis<WIDTH> currWord = dataIn.read();
			// There is not default, if package does not match any case it is automatically dropped
			switch (dip_ipProtocol)
			{
			case ICMP:
				ICMPdataOut.write(currWord);
				break;
			case UDP:
				UDPdataOut.write(currWord);
				break;
			case TCP:
				TCPdataOut.write(currWord);
				break;
			}
			if (currWord.last)
			{
				dip_state = META;
			}
		}
		break;
	} //switch
}

template <int WIDTH>
void detect_ipv6_protocol(	hls::stream<net_axis<WIDTH> >& dataIn,
							hls::stream<net_axis<WIDTH> >& icmpv6DataOut,
							hls::stream<net_axis<WIDTH> >& ipv6UdpDataOut)
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
			net_axis<WIDTH> currWord = dataIn.read();
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
			net_axis<WIDTH> currWord = dataIn.read();
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

/**
 *  @param[in]		s_axis_raw, incoming data stream
 *  @param[in]		myIpAddress, our IP address
 *  @param[out]		m_axis_ARP, outgoing ARP data stream
 *  @param[out]		m_axis_ICMP, outgoing ICMP (Ping) data stream
 *  @param[out]		m_axis_UDP, outgoing UDP data stream
 *  @param[out]		m_axis_TCP, outgoing TCP data stream
 */
template <int WIDTH>
void ip_handler(hls::stream<net_axis<WIDTH> >&		s_axis_raw,
				hls::stream<net_axis<WIDTH> >&		m_axis_ARP,
				hls::stream<net_axis<WIDTH> >&		m_axis_ICMPv6,
				hls::stream<net_axis<WIDTH> >&		m_axis_IPv6UDP,
				hls::stream<net_axis<WIDTH> >&		m_axis_ICMP,
				hls::stream<net_axis<WIDTH> >&		m_axis_UDP,
				hls::stream<net_axis<WIDTH> >&		m_axis_TCP,
				hls::stream<net_axis<WIDTH> >&		m_axis_ROCE,
				ap_uint<32>				myIpAddress)
{
	#pragma HLS INLINE

	static hls::stream<ap_uint<16> > etherTypeFifo("etherTypeFifo");
	static hls::stream<net_axis<WIDTH> >		ethDataFifo("ethDataFifo");
	static hls::stream<net_axis<WIDTH> >		ipv4ShiftFifo("ipv4ShiftFifo");
	static hls::stream<net_axis<WIDTH> >		ipv6ShiftFifo("ipv6ShiftFifo");

	static hls::stream<net_axis<WIDTH> >		ipDataFifo("ipDataFifo");
	static hls::stream<net_axis<WIDTH> >		ipDataMetaFifo("ipDataMetaFifo");
	static hls::stream<net_axis<WIDTH> >		ipDataCheckFifo("ipDataCheckFifo");
	static hls::stream<net_axis<WIDTH> >		ipDataDropFifo("ipDataDropFifo");
	static hls::stream<net_axis<WIDTH> >		ipDataCutFifo("ipDataCutFifo");
	static hls::stream<net_axis<WIDTH> >		udpDataFifo("udpDataFifo");
	static hls::stream<subSums<WIDTH/16> >		iph_subSumsFifoOut("iph_subSumsFifoOut");
	static hls::stream<bool>			validChecksumFifo("validChecksumFifo");
	static hls::stream<bool>			validIpAddressFifo("validIpAddressFifo");
	static hls::stream<bool>			ipv4ValidFifo("ipv4ValidFifo");

	#pragma HLS STREAM variable=etherTypeFifo		depth=2
	#pragma HLS STREAM variable=ethDataFifo		depth=4
	#pragma HLS STREAM variable=ipv4ShiftFifo depth=2
	#pragma HLS STREAM variable=ipv6ShiftFifo depth=2
	#pragma HLS STREAM variable=ipDataFifo depth=2
	#pragma HLS STREAM variable=ipDataMetaFifo depth=2
	#pragma HLS STREAM variable=ipDataCheckFifo depth=64 //8, must hold IP header for checksum checking, max. 15 x 32bit
	#pragma HLS STREAM variable=ipDataDropFifo depth=2
	#pragma HLS STREAM variable=ipDataCutFifo depth=2
	#pragma HLS STREAM variable=udpDataFifo depth=2
	#pragma HLS STREAM variable=iph_subSumsFifoOut depth=2
	#pragma HLS STREAM variable=validChecksumFifo depth=4
	#pragma HLS STREAM variable=validIpAddressFifo depth=32
	#pragma HLS STREAM variable=ipv4ValidFifo depth=8

	#pragma HLS aggregate  variable=ipDataFifo compact=bit
	#pragma HLS aggregate  variable=ipDataCheckFifo compact=bit
	#pragma HLS aggregate  variable=ipDataDropFifo compact=bit
	#pragma HLS aggregate  variable=iph_subSumsFifoOut compact=bit
	#pragma HLS aggregate  variable=ipDataCutFifo compact=bit
	#pragma HLS aggregate  variable=udpDataFifo compact=bit

	static hls::stream<net_axis<WIDTH> > ipv6DataFifo("ipv6DataFifo");
	#pragma HLS STREAM variable=ipv6DataFifo depth=2
	static hls::stream<ap_uint<8> > ipv4ProtocolFifo("ipv4ProtocolFifo");
	#pragma HLS STREAM variable=ipv4ProtocolFifo depth=32


	detect_eth_protocol(s_axis_raw, etherTypeFifo, ethDataFifo);

	route_by_eth_protocol(etherTypeFifo, ethDataFifo, m_axis_ARP, ipv4ShiftFifo, ipv6ShiftFifo);

	ip_handler_rshiftWordByOctet<net_axis<WIDTH>, WIDTH, 1>(((ETH_HEADER_SIZE%WIDTH)/8), ipv4ShiftFifo, ipDataFifo);	
	ip_handler_rshiftWordByOctet<net_axis<WIDTH>, WIDTH, 3>(((ETH_HEADER_SIZE%WIDTH)/8), ipv6ShiftFifo, ipv6DataFifo);	

	extract_ip_meta(ipDataFifo, ipDataMetaFifo, ipv4ProtocolFifo, validIpAddressFifo, myIpAddress);

	ip_handler_compute_ipv4_checksum(ipDataMetaFifo, ipDataCheckFifo, iph_subSumsFifoOut);

	ip_handler_check_ipv4_checksum<WIDTH/16>(iph_subSumsFifoOut, validChecksumFifo);

	ip_invalid_dropper(ipDataCheckFifo, validChecksumFifo, validIpAddressFifo, ipDataDropFifo, ipv4ValidFifo);

	cut_length(ipDataDropFifo, ipDataCutFifo);

	detect_ipv4_protocol(ipv4ProtocolFifo, ipv4ValidFifo, ipDataCutFifo, m_axis_ICMP, udpDataFifo, m_axis_TCP);

	detect_ipv6_protocol(ipv6DataFifo, m_axis_ICMPv6, m_axis_IPv6UDP);

	// Altough RoCEv2 is using IPv4 & UDP, we cannot parse the IPv4 header here and remove it, due ot the ICRC of RoCE.
	// The best solution is to duplicate the packet and let the UDP/IP and RoCE stack do filtering according to the port number
	ip_handler_duplicate_stream(udpDataFifo, m_axis_UDP, m_axis_ROCE);

}

void ip_handler_top(hls::stream<ap_axiu<DATA_WIDTH, 0, 0, 0> >&		s_axis_raw,
					hls::stream<ap_axiu<DATA_WIDTH, 0, 0, 0> >&		m_axis_arp,
					hls::stream<ap_axiu<DATA_WIDTH, 0, 0, 0> >&		m_axis_icmpv6,
					hls::stream<ap_axiu<DATA_WIDTH, 0, 0, 0> >&		m_axis_ipv6udp,
					hls::stream<ap_axiu<DATA_WIDTH, 0, 0, 0> >&		m_axis_icmp,
					hls::stream<ap_axiu<DATA_WIDTH, 0, 0, 0> >&		m_axis_udp,
					hls::stream<ap_axiu<DATA_WIDTH, 0, 0, 0> >&		m_axis_tcp,
					hls::stream<ap_axiu<DATA_WIDTH, 0, 0, 0> >&		m_axis_roce,
					ap_uint<32>								myIpAddress)
{
	#pragma HLS DATAFLOW disable_start_propagation
	#pragma HLS INTERFACE ap_ctrl_none port=return

	#pragma HLS INTERFACE axis register port=s_axis_raw
	#pragma HLS INTERFACE axis register port=m_axis_arp
	#pragma HLS INTERFACE axis register port=m_axis_icmpv6
	#pragma HLS INTERFACE axis register port=m_axis_ipv6udp
	#pragma HLS INTERFACE axis register port=m_axis_icmp
	#pragma HLS INTERFACE axis register port=m_axis_udp
	#pragma HLS INTERFACE axis register port=m_axis_tcp // leads to Combinatorial Loops
	#pragma HLS INTERFACE axis register port=m_axis_roce
	
	#pragma HLS INTERFACE ap_none register port=myIpAddress


	static hls::stream<net_axis<DATA_WIDTH> > s_axis_raw_internal;
	#pragma HLS STREAM depth=2 variable=s_axis_raw_internal
	static hls::stream<net_axis<DATA_WIDTH> > m_axis_arp_internal;
	#pragma HLS STREAM depth=2 variable=m_axis_arp_internal
	static hls::stream<net_axis<DATA_WIDTH> > m_axis_icmpv6_internal;
	#pragma HLS STREAM depth=2 variable=m_axis_icmpv6_internal
	static hls::stream<net_axis<DATA_WIDTH> > m_axis_ipv6udp_internal;
	#pragma HLS STREAM depth=2 variable=m_axis_ipv6udp_internal
	static hls::stream<net_axis<DATA_WIDTH> > m_axis_icmp_internal;
	#pragma HLS STREAM depth=2 variable=m_axis_icmp_internal
	static hls::stream<net_axis<DATA_WIDTH> > m_axis_udp_internal;
	#pragma HLS STREAM depth=2 variable=m_axis_udp_internal
	static hls::stream<net_axis<DATA_WIDTH> > m_axis_tcp_internal;
	#pragma HLS STREAM depth=2 variable=m_axis_tcp_internal
	static hls::stream<net_axis<DATA_WIDTH> > m_axis_roce_internal;
	#pragma HLS STREAM depth=2 variable=m_axis_roce_internal

	convert_axis_to_net_axis<DATA_WIDTH>(s_axis_raw, 
							s_axis_raw_internal);

	convert_net_axis_to_axis<DATA_WIDTH>(m_axis_arp_internal, 
							m_axis_arp);

	convert_net_axis_to_axis<DATA_WIDTH>(m_axis_icmpv6_internal, 
							m_axis_icmpv6);

	convert_net_axis_to_axis<DATA_WIDTH>(m_axis_ipv6udp_internal, 
							m_axis_ipv6udp);

	convert_net_axis_to_axis<DATA_WIDTH>(m_axis_icmp_internal, 
							m_axis_icmp);

	convert_net_axis_to_axis<DATA_WIDTH>(m_axis_udp_internal, 
							m_axis_udp);

	convert_net_axis_to_axis<DATA_WIDTH>(m_axis_tcp_internal, 
							m_axis_tcp);

	convert_net_axis_to_axis<DATA_WIDTH>(m_axis_roce_internal, 
							m_axis_roce);

   	ip_handler<DATA_WIDTH>(s_axis_raw_internal,
                           m_axis_arp_internal,
                           m_axis_icmpv6_internal,
                           m_axis_ipv6udp_internal,
                           m_axis_icmp_internal,
                           m_axis_udp_internal,
                           m_axis_tcp_internal,
                           m_axis_roce_internal,
                           myIpAddress);
}
