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
#include "icmp_server.hpp"

ap_uint<16> byteSwap16(ap_uint<16> inputVector) {
	return (inputVector.range(7,0), inputVector(15, 8));
}

/** @ingroup icmp_server
 *  No MAC Header, already shaved off
 *  Assumption no options in IP header
 *  @param[in]		dataIn
 *  @param[out]		dataOut
 *  @param[out]		icmpValidFifoOut
 *  @param[out]		checksumFifoOut
 */
void check_icmp_checksum(	stream<axiWord>& dataIn,
							stream<axiWord>& dataOut,
							stream<bool>& ValidFifoOut,
							stream<ap_uint<16> >& checksumFifoOut) {
#pragma HLS INLINE off
#pragma HLS pipeline II=1

	static ap_uint<17> cics_sums[4] = {0, 0, 0, 0};
	static ap_uint<16> cics_wordCount = 0;
	static axiWord cics_prevWord;
	static bool cics_writeLastOne = false;
	static bool cics_computeCs = false;
	static ap_uint<2> cics_state = 0;

	static ap_uint<8>	newTTL = 0x40;
	static ap_uint<17>	icmpChecksum = 0;
	static ap_uint<8>	icmpType;
	static ap_uint<8>	icmpCode;

	axiWord currWord;
	axiWord sendWord;

	currWord.last = 0;
	if (cics_writeLastOne) {
		dataOut.write(cics_prevWord);
		cics_writeLastOne = false;
	}
	else if (cics_computeCs) {
		switch (cics_state) {
		case 0:
			cics_sums[0] += cics_sums[2];
			cics_sums[0] = (cics_sums[0] + (cics_sums[0] >> 16)) & 0xFFFF;
			cics_sums[1] += cics_sums[3];
			cics_sums[1] = (cics_sums[1] + (cics_sums[1] >> 16)) & 0xFFFF;
			icmpChecksum = ~icmpChecksum;
			break;
		case 1:
			cics_sums[0] += cics_sums[1];
			cics_sums[0] = (cics_sums[0] + (cics_sums[0] >> 16)) & 0xFFFF;
			icmpChecksum -= ECHO_REQUEST;
			icmpChecksum = (icmpChecksum - (icmpChecksum >> 16)) & 0xFFFF;
			break;
		case 2:
			cics_sums[0] = ~cics_sums[0];
			icmpChecksum = ~icmpChecksum;
			break;
		case 3:
			// Check for 0
			if ((cics_sums[0](15, 0) == 0) && (icmpType == ECHO_REQUEST) && (icmpCode == 0)) {
				ValidFifoOut.write(true);
				checksumFifoOut.write(icmpChecksum);
			}
			else
				ValidFifoOut.write(false);
			cics_computeCs = false;
			break;
		}
		cics_state++;
	}
	else if (!dataIn.empty()) {
		dataIn.read(currWord);
		switch (cics_wordCount) {
		case WORD_0:
			cics_sums[0] = 0;
			cics_sums[1] = 0;
			cics_sums[2] = 0;
			cics_sums[3] = 0;
			break;
		case WORD_1:
			sendWord = cics_prevWord;
			dataOut.write(sendWord); //1
			// Contains Src IP address
			break;
		case WORD_2:
			// Contains Dest IP address
			sendWord.data(31, 0) = cics_prevWord.data(31, 0);
			sendWord.data(63, 32) = currWord.data(31, 0);
			icmpType = currWord.data(39, 32);
			icmpCode = currWord.data(47, 40);
			icmpChecksum(15, 0) = currWord.data(63, 48);
			icmpChecksum[16] = 1;
			sendWord.keep = 0xFF;
			sendWord.last = 0;
			dataOut.write(sendWord);
			for (int i = 2; i < 4; i++) {
	#pragma HLS UNROLL
				ap_uint<16> temp;
					temp(7, 0) = currWord.data.range(i*16+15, i*16+8);
					temp(15, 8) = currWord.data.range(i*16+7, i*16);
					cics_sums[i] += temp;
					cics_sums[i] = (cics_sums[i] + (cics_sums[i] >> 16)) & 0xFFFF;
			}
			currWord.data(31, 0) = cics_prevWord.data(63, 32);
			currWord.data.range(39, 32) = ECHO_REPLY;
			break;
		default:
			for (int i = 0; i < 4; i++) {
	#pragma HLS UNROLL
				ap_uint<16> temp;
				if (currWord.keep.range(i*2+1, i*2) == 0x3) {
					temp(7, 0) = currWord.data.range(i*16+15, i*16+8);
					temp(15, 8) = currWord.data.range(i*16+7, i*16);
					cics_sums[i] += temp;
					cics_sums[i] = (cics_sums[i] + (cics_sums[i] >> 16)) & 0xFFFF;
				}
				else if (currWord.keep[i*2] == 0x1) {
					temp(7, 0) = 0;
					temp(15, 8) = currWord.data.range(i*16+7, i*16);
					cics_sums[i] += temp;
					cics_sums[i] = (cics_sums[i] + (cics_sums[i] >> 16)) & 0xFFFF;
				}
			}
			sendWord = cics_prevWord;
			dataOut.write(sendWord);
			break;
		} // switch
		cics_prevWord = currWord;
		cics_wordCount++;
		if (currWord.last == 1) {
			cics_wordCount = 0;
			cics_writeLastOne = true;
			cics_computeCs = true;
		}
	}
}

void udpPortUnreachable(stream<axiWord> &udpIn, stream<axiWord> &ttlIn, stream<axiWord> &udpPort2addIpHeader_data, 
						stream<ap_uint<64> > &udpPort2addIpHeader_header, stream<ap_uint<16> > &udpPort2insertChecksum_value) {
	#pragma HLS INLINE off
	#pragma HLS pipeline II=1
	static enum uState{UDP_IDLE, UDP_IP, UDP_STREAM, UDP_CS} udpState;
	static ap_uint<20> 	udpChecksum 	= 0;
	static ap_uint<3>	ipWordCounter	= 0;
	static ap_uint<1>	streamSource	= 0; // 0 is UDP, 1 is IP Handler (Destination unreachable & TTL exceeded respectively)
	ap_uint<1>			udpInEmpty		= 0;
	ap_uint<1>			ttlInEmpty		= 0;
	
	udpInEmpty = udpIn.empty();
	ttlInEmpty = ttlIn.empty();
	switch(udpState) {
		case UDP_IDLE:
			if ((udpInEmpty == 0 || ttlInEmpty == 0) && !udpPort2addIpHeader_data.full()) { // If there are data in the queue, don't read them in but start assembling the ICMP header
				ipWordCounter = 0;
				axiWord tempWord = {0, 0xFF, 0};
				if (udpInEmpty == 0) {
					tempWord.data = 0x0000000000000303;
					streamSource = 0;
				}
				else if (ttlInEmpty == 0) {
					tempWord.data = 0x000000000000000B;
					streamSource = 1;
				}
				udpChecksum = (((tempWord.data.range(63, 48) + tempWord.data.range(47, 32)) + tempWord.data.range(31, 16)) + tempWord.data.range(15, 0));
				udpPort2addIpHeader_data.write(tempWord);
				udpState = UDP_IP;
			}
			break;
		case UDP_IP:
			if (((streamSource == 0 && udpInEmpty == 0) || (streamSource == 1 && ttlInEmpty == 0)) && !udpPort2addIpHeader_data.full() && !udpPort2addIpHeader_header.full()) { // If there are data in the queue start reading them
				axiWord tempWord = {0, 0, 0};
				if (streamSource == 0)
					tempWord = udpIn.read();
				else if (streamSource == 1)
					tempWord = ttlIn.read();
				udpChecksum = (udpChecksum + ((tempWord.data.range(63, 48) + tempWord.data.range(47, 32)) + tempWord.data.range(31, 16) + tempWord.data.range(15, 0)));
				udpPort2addIpHeader_data.write(tempWord);
				udpPort2addIpHeader_header.write(tempWord.data);
				if (ipWordCounter == 2)
					udpState = UDP_STREAM;
				else 
					ipWordCounter++;
			}
			break;	
		case UDP_STREAM:
			if (((streamSource == 0 && udpInEmpty == 0) || (streamSource == 1 && ttlInEmpty == 0)) && !udpPort2addIpHeader_data.full()) { // If there are data in the queue start reading them
				axiWord tempWord = {0, 0, 0};
				if (streamSource == 0)
					tempWord = udpIn.read();
				else if (streamSource == 1)
					tempWord = ttlIn.read();
				udpChecksum = (udpChecksum + ((tempWord.data.range(63, 48) + tempWord.data.range(47, 32)) + tempWord.data.range(31, 16) + tempWord.data.range(15, 0)));
				udpPort2addIpHeader_data.write(tempWord);
				if (tempWord.last == 1)
					udpState = UDP_CS;
			}
			break;
		case UDP_CS:
			if (!udpPort2insertChecksum_value.full()) {
				udpChecksum = (udpChecksum & 0xFFFF) + (udpChecksum >> 16);
				udpChecksum = (udpChecksum & 0xFFFF) + (udpChecksum >> 16);
				udpChecksum = ~udpChecksum;			// Reverse the bits of the result
				udpPort2insertChecksum_value.write(udpChecksum.range(15, 0));	// and write it into the output
				udpState = UDP_IDLE;		// When all is complete, move to idle
			}
			break;
	}
}

void udpAddIpHeader(stream<axiWord> &udpPort2addIpHeader_data, stream<ap_uint<64> > &udpPort2addIpHeader_header, 
					stream<axiWord> &addIpHeader2insertChecksum) {
	#pragma HLS INLINE off
#pragma HLS pipeline II=1

	static enum aState{AIP_IDLE, AIP_IP, AIP_MERGE, AIP_STREAM, AIP_RESIDUE} addIpState;
	static axiWord tempWord 	= {0, 0, 0};
	static ap_int<32> sourceIP	= 0;
	
	switch(addIpState) {
		case AIP_IDLE:
			if (!udpPort2addIpHeader_header.empty() && !addIpHeader2insertChecksum.full()) { // If there are data in the queue, don't read them in but start assembling the ICMP header
				tempWord.data = udpPort2addIpHeader_header.read();
				ap_uint<16> tempLength = byteSwap16(tempWord.data.range(31, 16));
				tempWord.data.range(31, 16) = byteSwap16(tempLength + 28);
				tempWord.keep = 0xFF;
				tempWord.last = 0;
				addIpHeader2insertChecksum.write(tempWord);
				addIpState = AIP_IP;
			}
			break;
		case AIP_IP:
			if (!udpPort2addIpHeader_header.empty() && !addIpHeader2insertChecksum.full()) { // If there are data in the queue, don't read them in but start assembling the ICMP header
				tempWord.data 	= udpPort2addIpHeader_header.read();
				tempWord.data.range(7, 0) = 0x80;								// Set the TTL to 128
				tempWord.data.range(15, 8) = 0x01;								// Swap the protocol from whatever it was to ICMP
				sourceIP		= tempWord.data.range(63, 32);
				tempWord.data.range(63, 32)	=	0x01010101;
				tempWord.keep 	= 0xFF;
				tempWord.last 	= 0;
				addIpHeader2insertChecksum.write(tempWord);
				addIpState 		= AIP_MERGE;
			}
			break;
		case AIP_MERGE:
			if (!udpPort2addIpHeader_header.empty() && !udpPort2addIpHeader_data.empty() && !addIpHeader2insertChecksum.full()) {
				udpPort2addIpHeader_header.read();
				ap_uint<64> tempData = sourceIP;
				tempWord = udpPort2addIpHeader_data.read();
				axiWord outputWord = {0, 0xFF, 0};
				outputWord.data = tempData;
				outputWord.data.range(63, 32) = tempWord.data.range(31,  0);
				addIpHeader2insertChecksum.write(outputWord);
				addIpState = AIP_STREAM;
			}
			break;
		case AIP_STREAM:
			if (!udpPort2addIpHeader_data.empty() && !addIpHeader2insertChecksum.full()) {
				axiWord outputWord = {0, 0xFF, 0};
				outputWord.data.range(31, 0) = tempWord.data.range(63,  32);
				tempWord = udpPort2addIpHeader_data.read();
				outputWord.data.range(63, 32) = tempWord.data.range(31,  0);
				if (tempWord.last == 1) {
					if (tempWord.keep.range(7, 4) == 0) {
						outputWord.last = 1;
						outputWord.keep.range(7, 4) = tempWord.keep.range(3,0);
						addIpState = AIP_IDLE;
					}
					else
						addIpState = AIP_RESIDUE;
				}
				addIpHeader2insertChecksum.write(outputWord);
			}
			break;
		case AIP_RESIDUE:
			if (!addIpHeader2insertChecksum.full()) {
				axiWord outputWord 				= {0, 0, 1};
				outputWord.data.range(31, 0) 	= tempWord.data.range(63, 32);
				outputWord.keep.range(3, 0)		= tempWord.keep.range(7, 4);
				addIpHeader2insertChecksum.write(outputWord);
				addIpState = AIP_IDLE;
			}
			break;
	}
}

/** @ingroup icmp_server
 *  Reads valid bit from validBufffer, if package is invalid it is dropped otherwise it is forwarded
 *  @param[in]		dataIn
 *  @param[in]		validFifoIn
 *  @param[out]		dataOut
 */
void dropper(stream<axiWord>& dataIn, stream<bool>& validFifoIn, stream<axiWord>& dataOut) {
	static bool d_isFirstWord 	= true;
	static bool d_drop 			= false;
	bool d_valid;
	axiWord currWord;

	if (!dataIn.empty()) {
		if(d_isFirstWord) {
			if (!validFifoIn.empty()) {
				dataIn.read(currWord);
				validFifoIn.read(d_valid);
				if(d_valid)
					dataOut.write(currWord);
				else
					d_drop = true;
				d_isFirstWord = false;
			}
		}
		else if (d_drop)
			dataIn.read(currWord);
		else {
			dataIn.read(currWord);
			dataOut.write(currWord);
		}
		if (currWord.last == 1) {
			d_drop = false;
			d_isFirstWord = true;
		}
	}
}

/** @ingroup icmp_server
 *  Inserts IP & ICMP checksum at the correct position
 *  @param[in]		dataIn
 *  @param[in]		checksumFifoIn
 *  @param[out]		dataOut
 */
 
 void insertChecksum(stream<axiWord> inputStreams[2], stream<ap_uint<16> > checksumStreams[2], stream<axiWord> &outputStream) {
#pragma HLS INLINE off
#pragma HLS pipeline II=1
 
	axiWord 			inputWord 		= {0, 0, 0};
	ap_uint<16> 		icmpChecksum	= 0;
	static ap_uint<16> 	ic_wordCount 	= 0;
    static ap_uint<1>	streamSource    = 0; 

    switch(ic_wordCount) {
        case 0:
            bool stream_empty[2]; // store all input stream empty states
            for (uint8_t i=0;i<2;++i) 
                stream_empty[i] = inputStreams[i].empty();
          
            for (uint8_t i=0;i<2;++i) {
                if(!stream_empty[i]) {
                    streamSource = i;
                    inputWord = inputStreams[i].read();
                    outputStream.write(inputWord);
                    ic_wordCount++;
                    break;
                }
            }
            break;
        case 2:
            if (!inputStreams[streamSource].empty() && !checksumStreams[streamSource].empty()) {
                inputStreams[streamSource].read(inputWord);
				icmpChecksum = checksumStreams[streamSource].read();
				inputWord.data(63, 48) = icmpChecksum;
                outputStream.write(inputWord);
                ic_wordCount++;
            }
            break;
		default:
			if (!inputStreams[streamSource].empty()) {
				inputStreams[streamSource].read(inputWord);
				outputStream.write(inputWord);
				if (inputWord.last == 1)
					ic_wordCount = 0;
				else
					ic_wordCount++;
			}
			break;
    }
}
/** @ingroup icmp_server
 *  Main function
 *  @param[in]		dataIn
 *  @param[out]		dataOut
 */
void icmp_server(stream<axiWord>&	dataIn,
				 stream<axiWord>&	udpIn,
				 stream<axiWord>&	ttlIn,
				 stream<axiWord>&	dataOut) {
	#pragma HLS DATAFLOW disable_start_propagation
	#pragma HLS INTERFACE ap_ctrl_none port=return

	#pragma HLS INTERFACE axis register port=dataIn name=s_axis
	#pragma HLS INTERFACE axis register port=udpIn name=udpIn
	#pragma HLS INTERFACE axis register port=ttlIn name=ttlIn
	#pragma HLS INTERFACE axis register port=dataOut name=m_axis

	static stream<axiWord>			packageBuffer1("packageBuffer1");
	static stream<axiWord>			udpPort2insertChecksum("udpPort2insertChecksum");
	static stream<bool>				validFifo("validFifo");
	static stream<ap_uint<64> > 	udpPort2addIpHeader_header("udpPort2addIpHeader");
	static stream<axiWord>			udpPort2addIpHeader_data("udpPort2addIpHeader_data");

	static stream<axiWord> dataStreams[2];
	#pragma HLS STREAM variable=dataStreams depth=16
	static stream<ap_uint<16> > checksumStreams[2];
	#pragma HLS STREAM variable=checksumStreams depth=16

	#pragma HLS stream 		variable=packageBuffer1 			depth=64 //TODO change this one is crucial
	#pragma HLS stream 		variable=udpPort2insertChecksum 	depth=8
	#pragma HLS stream 		variable=udpPort2addIpHeader_data 	depth=192
	#pragma HLS stream 		variable=validFifo 					depth=8
	#pragma HLS stream 		variable=udpPort2addIpHeader_header	depth=64
	#pragma HLS DATA_PACK 	variable=packageBuffer1
	#pragma HLS DATA_PACK 	variable=udpPort2insertChecksum

	check_icmp_checksum(dataIn, packageBuffer1, validFifo, checksumStreams[0]);
	udpPortUnreachable(udpIn, ttlIn, udpPort2addIpHeader_data, udpPort2addIpHeader_header, checksumStreams[1]);
	udpAddIpHeader(udpPort2addIpHeader_data, udpPort2addIpHeader_header, dataStreams[1]);
	dropper(packageBuffer1, validFifo, dataStreams[0]);
	insertChecksum(dataStreams, checksumStreams, dataOut);
}


