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

#include "mac_ip_encode.hpp"

/** @ingroup mac_ip_encode
 *
 */
void compute_ip_checksum(stream<axiWord>&				dataIn,
						stream<axiWord>&			dataOut,
						stream<ap_uint<16> >&		checksumFiFoOut)
{
#pragma HLS INLINE off
#pragma HLS pipeline II=1

	static ap_uint<17> cics_ip_sums[4] = {0, 0, 0, 0};
	static ap_uint<4> cics_ipHeaderLen = 0;
	static bool cics_checksumWritten = true;
	static ap_uint<3> cics_wordCount = 0;
	static ap_uint<2> cics_state = 0;

	axiWord currWord;
	ap_uint<16> temp;

	currWord.last = 0;
	if(!cics_checksumWritten)
	{
		switch (cics_state)
		{
		case 0:
			cics_ip_sums[0] += cics_ip_sums[2];
			cics_ip_sums[1] += cics_ip_sums[3];
			cics_ip_sums[0] = (cics_ip_sums[0] + (cics_ip_sums[0] >> 16)) & 0xFFFF;
			cics_ip_sums[1] = (cics_ip_sums[1] + (cics_ip_sums[1] >> 16)) & 0xFFFF;
			cics_state++;
			break;
		case 1:
			cics_ip_sums[0] += cics_ip_sums[1];
			cics_ip_sums[0] = (cics_ip_sums[0] + (cics_ip_sums[0] >> 16)) & 0xFFFF;
			cics_state++;
			break;
		case 2:
			cics_ip_sums[0] = ~cics_ip_sums[0];
			checksumFiFoOut.write(cics_ip_sums[0](15, 0));
			cics_state++;
			break;
		case 3:
			cics_ip_sums[0] = 0;
			cics_ip_sums[1] = 0;
			cics_ip_sums[2] = 0;
			cics_ip_sums[3] = 0;
			cics_checksumWritten = true;
			cics_state = 0;
			break;
		}
	}
	else if (!dataIn.empty() && cics_checksumWritten)
	{
		dataIn.read(currWord);
		switch (cics_wordCount)
		{
		case 0:
			cics_ipHeaderLen = currWord.data.range(3, 0);
			for (int i = 0; i < 4; i++)
			{
			#pragma HLS unroll
				temp(7, 0) = currWord.data.range(i*16+15, i*16+8);
				temp(15, 8) = currWord.data.range(i*16+7, i*16);
				cics_ip_sums[i] += temp;
				cics_ip_sums[i] = (cics_ip_sums[i] + (cics_ip_sums[i] >> 16)) & 0xFFFF;
			}
			cics_wordCount++;
			break;
		case 1:
			// we skip the 2nd 16bits because it is the ip checksum
			for (int i = 0; i < 4; i++)
			{
			#pragma HLS unroll
				if (i != 1)
				{
					temp(7, 0) = currWord.data.range(i*16+15, i*16+8);
					temp(15, 8) = currWord.data.range(i*16+7, i*16);
					cics_ip_sums[i] += temp;
					cics_ip_sums[i] = (cics_ip_sums[i] + (cics_ip_sums[i] >> 16)) & 0xFFFF;
				}
			}
			cics_ipHeaderLen -= 2;
			cics_wordCount++;
			break;
		default:
			switch (cics_ipHeaderLen)
			{
			case 0:
				break;
			case 3:
				for (int i = 0; i < 2; i++)
				{
				#pragma HLS unroll
					temp(7, 0) = currWord.data.range(i*16+15, i*16+8);
					temp(15, 8) = currWord.data.range(i*16+7, i*16);
					cics_ip_sums[i] += temp;
					cics_ip_sums[i] = (cics_ip_sums[i] + (cics_ip_sums[i] >> 16)) & 0xFFFF;
				}
				cics_ipHeaderLen = 0;
				cics_checksumWritten = false;
				break;
			case 4:
				for (int i = 0; i < 4; i++)
				{
				#pragma HLS unroll
					temp(7, 0) = currWord.data.range(i*16+15, i*16+8);
					temp(15, 8) = currWord.data.range(i*16+7, i*16);
					cics_ip_sums[i] += temp;
					cics_ip_sums[i] = (cics_ip_sums[i] + (cics_ip_sums[i] >> 16)) & 0xFFFF;
				}
				cics_ipHeaderLen = 0;
				cics_checksumWritten = false;
				break;
			default:
				// Sum up everything
				for (int i = 0; i < 4; i++)
				{
				#pragma HLS unroll
					temp(7, 0) = currWord.data.range(i*16+15, i*16+8);
					temp(15, 8) = currWord.data.range(i*16+7, i*16);
					cics_ip_sums[i] += temp;
					cics_ip_sums[i] = (cics_ip_sums[i] + (cics_ip_sums[i] >> 16)) & 0xFFFF;
				}
				cics_ipHeaderLen -= 2;
				break;
			} // switch ipHeaderLen
			break;
		} // switch WORD_N
		dataOut.write(currWord);
		if (currWord.last)
		{
			cics_wordCount = 0;
		}
	}
}

/** @ingroup mac_ip_encode
 *
 */
void ip_checksum_insert(stream<axiWord>&		dataIn,
						stream<ap_uint<16> >&	checksumFifoIn,
						stream<axiWord>&		dataOut)
{
#pragma HLS INLINE off
#pragma HLS pipeline II=1

	static ap_uint<4> ici_wordCount = 0;

	axiWord currWord;
	ap_uint<16> checksum;

	currWord.last = 0;
	switch (ici_wordCount)
	{
	case 0:
		if (!dataIn.empty())
		{
			dataIn.read(currWord);
			dataOut.write(currWord);
			ici_wordCount++;
		}
		break;
	case 1:
		if (!dataIn.empty() && !checksumFifoIn.empty())
		{
			dataIn.read(currWord);
			checksumFifoIn.read(checksum);
			currWord.data(23, 16) = checksum(15, 8);
			currWord.data(31, 24) = checksum(7, 0);
			dataOut.write(currWord);
			ici_wordCount++;
		}
		break;
	default:
		if (!dataIn.empty())
		{
			dataIn.read(currWord);
			dataOut.write(currWord);
		}
		break;
	}
	if (currWord.last)
	{
		ici_wordCount = 0;
	}

}

/** @ingroup mac_ip_encode
 *
 */
void extract_ip_address(stream<axiWord>&			dataIn,
						stream<axiWord>&			dataOut,
						stream<ap_uint<32> >&		arpTableOut,
						ap_uint<32>					regSubNetMask,
						ap_uint<32>					regDefaultGateway)
{
#pragma HLS INLINE off
#pragma HLS pipeline II=1

	static ap_uint<2> eia_wordCount=0;
	axiWord currWord;
	ap_uint<32> dstIpAddress;

	if (!dataIn.empty())
	{
		dataIn.read(currWord);
		switch (eia_wordCount)
		{
		case 0:
			eia_wordCount++;
			break;
		case 1:
			eia_wordCount++;
			break;
		case 2:
			// Extract destination IP address
			dstIpAddress = currWord.data(31, 0);
			if ((dstIpAddress & regSubNetMask) == (regDefaultGateway & regSubNetMask))
			{
				arpTableOut.write(dstIpAddress);
			}
			else
			{
				arpTableOut.write(regDefaultGateway);
			}
			eia_wordCount++;
			break;
		default:
			if (currWord.last)
			{
				eia_wordCount = 0;
			}
			break;
		} // switch;
		dataOut.write(currWord);
	}
}

/** @ingroup mac_ip_encode
 *
 */
void handle_arp_reply(	stream<axiWord>&		dataIn,
						stream<arpTableReply>&	arpTableIn,
						stream<axiWord>&		dataOut,
						ap_uint<48>				myMacAddress)
{
#pragma HLS INLINE off
#pragma HLS pipeline II=1

	enum mwState {WAIT_LOOKUP, DROP, WRITE_FIRST, WRITE};
	static mwState mw_state = WAIT_LOOKUP;
	static axiWord mw_prevWord;
	static bool mw_writeLast = false;
	axiWord sendWord, currWord;
	arpTableReply reply;

	currWord.last = 0;
	if (mw_writeLast)
	{
		sendWord.data(47, 0) = mw_prevWord.data(63, 16);
		sendWord.data(63, 48) = 0;
		sendWord.keep(5, 0) = mw_prevWord.keep(7, 2);
		sendWord.keep(7, 6) = 0;
		sendWord.last = 1;
		dataOut.write(sendWord);
		mw_writeLast = false;
	}
	else// if (!dataIn.empty())
	{
		switch (mw_state)
		{
		case WAIT_LOOKUP:
			if (!arpTableIn.empty())
			{
				arpTableIn.read(reply);
				if (reply.hit)
				{
					sendWord.data(47, 0) =  reply.macAddress;
					sendWord.data(63, 48) = myMacAddress(15, 0);
					sendWord.keep = 0xff;
					sendWord.last = 0;
					dataOut.write(sendWord);
					mw_state = WRITE_FIRST;
				}
				else //drop it all, wait for RT
				{
					mw_state = DROP;
				}
			}
			break;
		case WRITE_FIRST:
			if (!dataIn.empty())
			{
				dataIn.read(currWord);
				sendWord.data.range(31, 0) = myMacAddress(47,16);
				sendWord.data(47, 32) = 0x0008;
				sendWord.data(63, 48) = currWord.data(15, 0);
				sendWord.keep = 0xff;
				sendWord.last = 0;
				dataOut.write(sendWord);
				mw_prevWord = currWord;
				mw_state = WRITE;
			}
			break;
		case WRITE:
			if (!dataIn.empty())
			{
				dataIn.read(currWord);
				sendWord.data(47, 0) = mw_prevWord.data(63, 16);
				sendWord.data(63, 48) = currWord.data(15, 0);
				sendWord.keep(5, 0) = mw_prevWord.keep(7, 2);
				sendWord.keep(7, 6) = currWord.keep(1, 0);
				sendWord.last = (currWord.keep[2] == 0);
				dataOut.write(sendWord);
				mw_prevWord = currWord;
			}
			break;
		case DROP:
			if (!dataIn.empty())
			{
				dataIn.read(currWord);
				sendWord.last = 1;
			}
			break;
		} // switch
		if (currWord.last)
		{
			mw_state = WAIT_LOOKUP;
			mw_writeLast = !sendWord.last;
		}
	} //else
}

/** @ingroup mac_ip_encode
 *  No more merging functionality only contains the mac_wrapper module, which lookups the MAC addresses.
 */
void mac_ip_encode( stream<axiWord>&			dataIn,
					stream<arpTableReply>&		arpTableIn,
					stream<axiWord>&			dataOut,
					stream<ap_uint<32> >&		arpTableOut,
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
	static stream<axiWord> dataStreamBuffer0("dataStreamBuffer0");
	static stream<axiWord> dataStreamBuffer1("dataStreamBuffer1");
	static stream<axiWord> dataStreamBuffer2("dataStreamBuffer2");
	#pragma HLS stream variable=dataStreamBuffer0 depth=16 //must hold ip header for checksum computation
	#pragma HLS stream variable=dataStreamBuffer1 depth=16
	#pragma HLS stream variable=dataStreamBuffer2 depth=16
	#pragma HLS DATA_PACK variable=dataStreamBuffer0
	#pragma HLS DATA_PACK variable=dataStreamBuffer1
	#pragma HLS DATA_PACK variable=dataStreamBuffer2

	static stream<ap_uint<16> > checksumFifo("checksumFifo");
	#pragma HLS stream variable=checksumFifo depth=16

	compute_ip_checksum(dataIn, dataStreamBuffer0, checksumFifo);

	ip_checksum_insert(dataStreamBuffer0, checksumFifo, dataStreamBuffer1);

	extract_ip_address(dataStreamBuffer1, dataStreamBuffer2, arpTableOut, regSubNetMask, regDefaultGateway);

	handle_arp_reply(dataStreamBuffer2, arpTableIn, dataOut, myMacAddress);
}
