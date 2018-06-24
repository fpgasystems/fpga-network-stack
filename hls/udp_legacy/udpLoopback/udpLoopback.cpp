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
#include "udpLoopback.hpp"

void rxPath(stream<axiWord>&       lbRxDataIn,
        	stream<metadata>&      lbRxMetadataIn,
        	stream<ap_uint<16> >&  lbRequestPortOpenOut,
        	stream<bool >&  		lbPortOpenReplyIn,
        	stream<axiWord> 	   &lb_packetBuffer,
        	stream<ap_uint<16> >   &lb_lengthBuffer,
        	stream<metadata>	   &lb_metadataBuffer) {
#pragma HLS PIPELINE II=1

	static enum sState {LB_IDLE = 0, LB_W8FORPORT, LB_ACC_FIRST, LB_ACC} sinkState;
	static uint16_t 			lbPacketLength 		= 0;
	//static ap_uint<32> 			openPortWaitTime 	= TIME_5S;
	static ap_uint<32> 			openPortWaitTime 	= 100;
	
	switch(sinkState) {
		case LB_IDLE:
			if(!lbRequestPortOpenOut.full() && openPortWaitTime == 0) {
				lbRequestPortOpenOut.write(0x0280);
				sinkState = LB_W8FORPORT;
			}
			else 
				openPortWaitTime--;
			break;
		case LB_W8FORPORT:
			if(!lbPortOpenReplyIn.empty()) {
				bool openPort = lbPortOpenReplyIn.read();
				sinkState = LB_ACC_FIRST;
			}
			break;
		case LB_ACC_FIRST:
			if (!lbRxDataIn.empty() && !lb_packetBuffer.full() && !lbRxMetadataIn.empty() && !lb_metadataBuffer.full()) {
				metadata tempMetadata = lbRxMetadataIn.read();
				sockaddr_in tempSocket = tempMetadata.sourceSocket ;
				tempMetadata.sourceSocket = tempMetadata.destinationSocket;
				tempMetadata.destinationSocket = tempSocket;
				lb_metadataBuffer.write(tempMetadata);
				axiWord tempWord = lbRxDataIn.read();
				lb_packetBuffer.write(tempWord);
				ap_uint<4> counter = 0;
				for (uint8_t i=0;i<8;++i) {
					if (tempWord.keep.bit(i) == 1)
						counter++;
				}
				/*switch(tempWord.keep) {
					case 0x01:
						counter = 1;
						break;
					case 0x03:
						counter = 2;
						break;
					case 0x07:
						counter = 3;
						break;
					case 0x0F:
						counter = 4;
						break;
					case 0x1F:
						counter = 5;
						break;
					case 0x3F:
						counter = 6;
						break;
					case 0x7F:
						counter = 7;
						break;
					case 0xFF:
						counter = 8;
						break;
				}*/
				lbPacketLength += counter;
				if (tempWord.last) {
					lb_lengthBuffer.write(lbPacketLength);
					lbPacketLength = 0;
				}
				else
					sinkState = LB_ACC;
			}
			break;
		case LB_ACC:
			if (!lbRxDataIn.empty() && !lb_packetBuffer.full()) {
				axiWord tempWord = lbRxDataIn.read();
				lb_packetBuffer.write(tempWord);
				ap_uint<4> counter = 0;
				for (uint8_t i=0;i<8;++i) {
					if (tempWord.keep.bit(i) == 1)
						counter++;
				}
				/*switch(tempWord.keep) {
					case 0x01:
						counter = 1;
						break;
					case 0x03:
						counter = 2;
						break;
					case 0x07:
						counter = 3;
						break;
					case 0x0F:
						counter = 4;
						break;
					case 0x1F:
						counter = 5;
						break;
					case 0x3F:
						counter = 6;
						break;
					case 0x7F:
						counter = 7;
						break;
					case 0xFF:
						counter = 8;
						break;
				}*/
				lbPacketLength += counter;
				if (tempWord.last) {
					lb_lengthBuffer.write(lbPacketLength);
					lbPacketLength = 0;
					sinkState = LB_ACC_FIRST;
				}
			}
			break;
	}
}
void txPath(stream<axiWord> 	   &lb_packetBuffer,
    		stream<ap_uint<16> >   &lb_lengthBuffer,
    		stream<metadata>	   &lb_metadataBuffer,
    		stream<axiWord> 	   &lbTxDataOut,
		 	stream<metadata> 	   &lbTxMetadataOut,
		 	stream<ap_uint<16> >   &lbTxLengthOut) {
#pragma HLS PIPELINE II=1
	if (!lb_packetBuffer.empty() && !lbTxDataOut.full())
		lbTxDataOut.write(lb_packetBuffer.read());
	if (!lb_metadataBuffer.empty()  && !lbTxMetadataOut.full())
		lbTxMetadataOut.write(lb_metadataBuffer.read());
	if (!lb_lengthBuffer.empty() && !lbTxLengthOut.full())
		lbTxLengthOut.write(lb_lengthBuffer.read());
}

void udpLoopback(stream<axiWord>&       lbRxDataIn,
                 stream<metadata>&     	lbRxMetadataIn,
				 stream<ap_uint<16> >&  lbRequestPortOpenOut,
				 stream<bool >&			lbPortOpenReplyIn,
				 stream<axiWord> 		&lbTxDataOut,
				 stream<metadata> 		&lbTxMetadataOut,
				 stream<ap_uint<16> > 	&lbTxLengthOut) {
	#pragma HLS INTERFACE ap_ctrl_none port=return
	#pragma HLS DATAFLOW

	/*#pragma HLS INTERFACE port=lbRxDataIn 			axis
	#pragma HLS INTERFACE port=lbRxMetadataIn 		axis
	#pragma HLS INTERFACE port=lbRequestPortOpenOut 	axis
	#pragma HLS INTERFACE port=lbPortOpenReplyIn 	axis
	#pragma HLS INTERFACE port=lbTxDataOut 			axis
	#pragma HLS INTERFACE port=lbTxMetadataOut	 	axis
	#pragma HLS INTERFACE port=lbTxLengthOut	 		axis*/
	
	#pragma HLS resource core=AXI4Stream variable=lbRxDataIn 				metadata="-bus_bundle lbRxDataIn"
	#pragma HLS resource core=AXI4Stream variable=lbRxMetadataIn 			metadata="-bus_bundle lbRxMetadataIn"
	#pragma HLS resource core=AXI4Stream variable=lbRequestPortOpenOut 	metadata="-bus_bundle lbRequestPortOpenOut"
	#pragma HLS resource core=AXI4Stream variable=lbPortOpenReplyIn 	metadata="-bus_bundle lbPortOpenReplyIn"
	#pragma HLS resource core=AXI4Stream variable=lbTxDataOut 			metadata="-bus_bundle lbTxDataOut"
	#pragma HLS resource core=AXI4Stream variable=lbTxMetadataOut 		metadata="-bus_bundle lbTxMetadataOut"
	#pragma HLS resource core=AXI4Stream variable=lbTxLengthOut 			metadata="-bus_bundle lbTxLengthOut"

  	#pragma HLS DATA_PACK variable=lbRxMetadataIn
  	#pragma HLS DATA_PACK variable=lbTxMetadataOut

	static stream<axiWord> 		lb_packetBuffer("lb_packetBuffer");
	static stream<ap_uint<16> > lb_lengthBuffer("lb_lengthBuffer");
	static stream<metadata>		lb_metadataBuffer("lb_metadataBuffer");
	#pragma HLS DATA_PACK variable 	= lb_packetBuffer
	#pragma HLS STREAM variable 	= lb_packetBuffer	depth = 1024

	rxPath(lbRxDataIn, lbRxMetadataIn, lbRequestPortOpenOut, lbPortOpenReplyIn, lb_packetBuffer, lb_lengthBuffer, lb_metadataBuffer);
	txPath(lb_packetBuffer, lb_lengthBuffer, lb_metadataBuffer, lbTxDataOut, lbTxMetadataOut, lbTxLengthOut);
}
