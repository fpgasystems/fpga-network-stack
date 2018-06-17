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

using namespace hls;
//TODO move this to some utility file
ap_uint<8> lenToKeep(ap_uint<4> length)
{
  switch(length)
  {
  case 1:
    return 0x01;
    break;
  case 2:
    return 0x03;
    break;
  case 3:
    return 0x07;
    break;
  case 4:
    return 0x0F;
    break;
  case 5:
    return 0x1F;
    break;
  case 6:
    return 0x3F;
    break;
  case 7:
    return 0x7F;
    break;
  case 8:
    return 0xFF;
    break;
  }
}

/** @ingroup ip_handler
 *  Detects the MAC protocol in the header of the packet, ARP and IP packets are forwared accordingly,
 *  packets of other protocols are discarded
 *  @param[in]		dataIn
 *  @param[out]		ARPdataOut
 *  @param[out]		IPdataOut
 */
void detect_mac_protocol(stream<axiWord> &dataIn, stream<axiWord> &ARPdataOut, stream<axiWord> &IPdataOut)
{
#pragma HLS inline off
#pragma HLS pipeline II=1

	static ap_uint<1> dmp_fsmState = 0;
	static ap_uint<2> dmp_wordCount = 0;
	static ap_uint<16> dmp_macType;
	static axiWord dmp_prevWord;
	axiWord currWord;

	switch (dmp_fsmState)
	{
	case 0:
		if (!dataIn.empty() && !ARPdataOut.full() && !IPdataOut.full())
		{
			dataIn.read(currWord);
			switch (dmp_wordCount)
			{
			case 0:
				dmp_wordCount++;
				break;
			default:
				if (dmp_wordCount == 1)
				{
					dmp_macType(7, 0) = currWord.data(47, 40);
					dmp_macType(15,8) = currWord.data(39, 32);
					dmp_wordCount++;
				}
				if (dmp_macType == ARP)
				{
					ARPdataOut.write(dmp_prevWord);
				}
				else if (dmp_macType == IPv4)
				{
					IPdataOut.write(dmp_prevWord);
				}
				break;
			}
			dmp_prevWord = currWord;
			if (currWord.last)
			{
				dmp_wordCount = 0;
				dmp_fsmState = 1;
			}
		}
		break;
	case 1:
		if (!ARPdataOut.full() && !IPdataOut.full())
		{
			if (dmp_macType == ARP)
			{
				ARPdataOut.write(dmp_prevWord);
			}
			else if (dmp_macType == IPv4)
			{
				IPdataOut.write(dmp_prevWord);
			}
			dmp_fsmState = 0;
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
void check_ip_checksum(stream<axiWord>&		dataIn,
						ap_uint<32>			myIpAddress,
						stream<axiWord>&	dataOut,
						stream<subSums>&	iph_subSumsFifoIn)
						//stream<bool>&		ipValidFifoOut)
{
#pragma HLS INLINE off
#pragma HLS pipeline II=1
	enum cic_StateType {PACKET, LAST};
	static cic_StateType cics_state = PACKET;

	static ap_uint<17> cics_ip_sums[4] = {0, 0, 0, 0};
	static ap_uint<8> cics_ipHeaderLen = 0;
	static axiWord cics_prevWord;
	//static bool cics_wasLast = false;
	static ap_uint<3> cics_wordCount = 0;
	static ap_uint<32> cics_dstIpAddress = 0;

	axiWord currWord;
	axiWord sendWord;
	ap_uint<16> temp;

	currWord.last = 0;
	switch (cics_state)
	{
	case PACKET:
		if (!dataIn.empty() && !dataOut.full() && !iph_subSumsFifoIn.full())
		{
			dataIn.read(currWord);
			switch (cics_wordCount)
			{
			case 0:
				// This is MAC only we throw it out
				cics_ip_sums[0] = 0;
				cics_ip_sums[1] = 0;
				cics_ip_sums[2] = 0;
				cics_ip_sums[3] = 0;
				//cics_srcMacIpTuple.macAddress(15, 0) = currWord.data(63, 48);
				cics_wordCount++;
				break;
			case 1:
				//cics_srcMacIpTuple.macAddress(47, 16) = currWord.data(31, 0);
				//ap_uint<16> temp;
				temp(15, 8) = currWord.data(55, 48);
				temp(7, 0) = currWord.data(63, 56);
				cics_ip_sums[3] += temp;
				cics_ip_sums[3] = (cics_ip_sums[3] + (cics_ip_sums[3] >> 16)) & 0xFFFF;
				cics_ipHeaderLen = currWord.data.range(51, 48);
				cics_wordCount++;
				break;
			case 2:
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
			case 3: //maybe merge with WORD_2
				//cics_srcMacIpTuple.ipAddress = currWord.data(47, 16);
				cics_dstIpAddress(15, 0) = currWord.data(63, 48);
				for (int i = 0; i < 4; i++)
				{
				#pragma HLS unroll
					temp(7, 0) = currWord.data.range(i*16+15, i*16+8);
					temp(15, 8) = currWord.data.range(i*16+7, i*16);
					cics_ip_sums[i] += temp;
					cics_ip_sums[i] = (cics_ip_sums[i] + (cics_ip_sums[i] >> 16)) & 0xFFFF;
				}
				// write tcp len out
				cics_ipHeaderLen -= 2;
				//tcpLenBuffer.write(cics_ipTotalLen);
				cics_wordCount++;
				break;
			default:
				if (cics_wordCount == 4)
				{
					cics_dstIpAddress(31, 16) = currWord.data(15, 0);
				}
				//outData.write(sendWord);
				switch (cics_ipHeaderLen)
				{
				case 0:
					break;
				case 1:
					// Sum up part0
					temp(15, 8) = currWord.data(7, 0);
					temp(7, 0) = currWord.data(15, 8);
					cics_ip_sums[0] += temp;
					cics_ip_sums[0] = (cics_ip_sums[0] + (cics_ip_sums[0] >> 16)) & 0xFFFF;
					cics_ipHeaderLen = 0;
					//cpLen = 6;
					iph_subSumsFifoIn.write(subSums(cics_ip_sums, ((cics_dstIpAddress == myIpAddress) || (cics_dstIpAddress == 0xFFFFFFFF))));
					break;
				case 2:
					// Sum up part 0-2
					for (int i = 0; i < 3; i++)
					{
					#pragma HLS unroll
						temp(7, 0) = currWord.data.range(i*16+15, i*16+8);
						temp(15, 8) = currWord.data.range(i*16+7, i*16);
						cics_ip_sums[i] += temp;
						cics_ip_sums[i] = (cics_ip_sums[i] + (cics_ip_sums[i] >> 16)) & 0xFFFF;
					}
					cics_ipHeaderLen = 0;
					//tcpLen = 2;
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
					cics_ipHeaderLen -= 2;
					break;
				}
				break; //FIXED by code duplication TODO
			} // switch WORD_N
			if (cics_wordCount > 2)
			{
				// Send Word
				sendWord.data(15, 0) = cics_prevWord.data(63, 48);
				sendWord.data(63, 16) = currWord.data(47, 0);
				sendWord.keep(1, 0) = cics_prevWord.keep(7,6);
				sendWord.keep(7, 2) = currWord.keep(5, 0);
				sendWord.last = (currWord.keep[6] == 0);
				dataOut.write(sendWord);
			}
			cics_prevWord = currWord;
			if (currWord.last)
			{
				cics_wordCount = 0;
				if (!sendWord.last)
				{
					cics_state = LAST;
				}
			}
		}
		break;
	case LAST:
		if(!dataOut.full())
		{
			// Send remaining Word
			sendWord.data.range(15, 0) = cics_prevWord.data(63, 48);
			sendWord.keep.range(1, 0) = cics_prevWord.keep(7, 6);
			sendWord.keep.range(7, 2) = 0;
			sendWord.last = 1;
			dataOut.write(sendWord);
			cics_state = PACKET;
		}
		break;
	} //switch
}

void iph_check_ip_checksum(	stream<subSums>&		iph_subSumsFifoOut,
							stream<bool>&			iph_validFifoOut)
{
#pragma HlS INLINE off
#pragma HLS PIPELINE II=1 enable_flush

	if (!iph_subSumsFifoOut.empty()) {
		subSums icic_ip_sums = iph_subSumsFifoOut.read();
		icic_ip_sums.sum0 += icic_ip_sums.sum2;
		icic_ip_sums.sum1 += icic_ip_sums.sum3;
		icic_ip_sums.sum0 = (icic_ip_sums.sum0 + (icic_ip_sums.sum0 >> 16)) & 0xFFFF;
		icic_ip_sums.sum1 = (icic_ip_sums.sum1 + (icic_ip_sums.sum1 >> 16)) & 0xFFFF;
		icic_ip_sums.sum0 += icic_ip_sums.sum1;
		icic_ip_sums.sum0 = (icic_ip_sums.sum0 + (icic_ip_sums.sum0 >> 16)) & 0xFFFF;
		icic_ip_sums.sum0 = ~icic_ip_sums.sum0;
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
void ip_invalid_dropper(stream<axiWord>&		dataIn,
						stream<bool>&			ipValidFifoIn,
						stream<axiWord>&		dataOut)
{
#pragma HLS INLINE off
#pragma HLS pipeline II=1

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
				iid_state = FWD;
			}
			else
			{
				iid_state = DROP;
			}
		}
		break;
	case FWD:
		if(!dataIn.empty() && !dataOut.full())
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
void cut_length(stream<axiWord> &dataIn, stream<axiWord> &dataOut)
{
#pragma HLS INLINE off
#pragma HLS pipeline II=1

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
void detect_ip_protocol(stream<axiWord> &dataIn,
						stream<axiWord> &ICMPdataOut,
						stream<axiWord> &UDPdataOut,
						stream<axiWord> &TCPdataOut)
{
#pragma HLS inline off
#pragma HLS pipeline II=1

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
		if (!dataIn.empty() && !ICMPdataOut.full() && !UDPdataOut.full() && !TCPdataOut.full())
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

/** @ingroup ip_handler
 *  @param[in]		s_axis_raw, incoming data stream
 *  @param[in]		myIpAddress, our IP address
 *  @param[out]		m_axis_ARP, outgoing ARP data stream
 *  @param[out]		m_axis_ICMP, outgoing ICMP (Ping) data stream
 *  @param[out]		m_axis_UDP, outgoing UDP data stream
 *  @param[out]		m_axis_TCP, outgoing TCP data stream
 */
void ip_handler(stream<axiWord>&		s_axis_raw,
				stream<axiWord>&		m_axis_ARP,
				stream<axiWord>&		m_axis_ICMP,
				stream<axiWord>&		m_axis_UDP,
				stream<axiWord>&		m_axis_TCP,
				ap_uint<32>				myIpAddress)
{
#pragma HLS DATAFLOW
#pragma HLS INTERFACE ap_ctrl_none register port=return
#pragma HLS INLINE off

	#pragma HLS resource core=AXI4Stream variable=s_axis_raw metadata="-bus_bundle s_axis_raw"
	#pragma HLS resource core=AXI4Stream variable=m_axis_ARP metadata="-bus_bundle m_axis_ARP"
	#pragma HLS resource core=AXI4Stream variable=m_axis_ICMP metadata="-bus_bundle m_axis_ICMP"
	#pragma HLS resource core=AXI4Stream variable=m_axis_UDP metadata="-bus_bundle m_axis_UDP"
	#pragma HLS resource core=AXI4Stream variable=m_axis_TCP metadata="-bus_bundle m_axis_TCP"

	//New pragmas are not used yet, due to a lackt of testing
	/*#pragma HLS INTERFACE axis port=s_axis_raw
	#pragma HLS INTERFACE axis port=m_axis_ARP
	#pragma HLS INTERFACE axis port=m_axis_ICMP
	#pragma HLS INTERFACE axis port=m_axis_UDP
	#pragma HLS INTERFACE axis port=m_axis_TCP*/

	#pragma HLS INTERFACE ap_stable register port=myIpAddress

	static stream<axiWord>		ipDataFifo("ipDataFifo");
	static stream<axiWord>		ipDataCheckFifo("ipDataCheckFifo");
	static stream<axiWord>		ipDataDropFifo("ipDataDropFifo");
	static stream<axiWord>		ipDataCutFifo("ipDataCutFifo");
	static stream<subSums>		iph_subSumsFifoOut("iph_subSumsFifoOut");
	static stream<bool>			ipValidFifo("ipValidFifo");
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

	detect_mac_protocol(s_axis_raw, m_axis_ARP, ipDataFifo);

	check_ip_checksum(ipDataFifo, myIpAddress, ipDataCheckFifo, iph_subSumsFifoOut);

	iph_check_ip_checksum(iph_subSumsFifoOut, ipValidFifo);

	ip_invalid_dropper(ipDataCheckFifo, ipValidFifo, ipDataDropFifo);

	cut_length(ipDataDropFifo, ipDataCutFifo);

	detect_ip_protocol(ipDataCutFifo, m_axis_ICMP, m_axis_UDP, m_axis_TCP);
}
