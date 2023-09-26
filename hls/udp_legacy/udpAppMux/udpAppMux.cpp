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
#include "udpAppMux.hpp"
	   
void appMuxRxPath(stream<axiWord> 			&rxDataIn, stream<metadata>&     	rxMetadataIn, 
		    stream<axiWord> 			&rxDataOutDhcp, stream<metadata>&     	rxMetadataOutDhcp,
		    stream<axiWord> 			&rxDataOutApp, 	stream<metadata>&     	rxMetadataOutApp) {
#pragma HLS PIPELINE II=1
	static enum sState {LB_IDLE = 0, LB_FIRST, LB_ACC} shimState;
	static ap_uint<1> streamSourceRx = 0;

	switch(shimState) {
		case LB_IDLE:
		{
			if (!rxMetadataIn.empty() && !rxMetadataOutDhcp.full()) {
				metadata tempMetadata = rxMetadataIn.read();
				if (tempMetadata.destinationSocket.port == 0x0044) {
					rxMetadataOutDhcp.write(tempMetadata);
					streamSourceRx = 0;
				}
				else {
					rxMetadataOutApp.write(tempMetadata);
					streamSourceRx = 1;
				}
				shimState = LB_FIRST;
			}
		}
		break;
		case LB_FIRST:
		{
			if (!rxDataIn.empty()) {
				if ( streamSourceRx == 0 && !rxDataOutDhcp.full()) {
					axiWord outputWord 		= rxDataIn.read();
					rxDataOutDhcp.write(outputWord);
					if (!outputWord.last)
						shimState = LB_ACC;
					else if (outputWord.last)
						shimState = LB_IDLE;
				}
				else if (streamSourceRx == 1 && !rxDataOutApp.full()) {
					axiWord outputWord 		= rxDataIn.read();
					rxDataOutApp.write(outputWord);
					if (!outputWord.last)
						shimState = LB_ACC;
					else if (outputWord.last)
						shimState = LB_IDLE;
				}
			}
		}
		break;
		case LB_ACC:
		{
			if (!rxDataIn.empty()) {
				if (streamSourceRx == 0&& !rxDataOutDhcp.full()) {
					axiWord outputWord = rxDataIn.read();
					rxDataOutDhcp.write(outputWord);
					if (outputWord.last)
						shimState = LB_IDLE;
				}
				else if (streamSourceRx == 1 && !rxDataOutApp.full()) {
					axiWord outputWord = rxDataIn.read();
					rxDataOutApp.write(outputWord);
					if (outputWord.last)
						shimState = LB_IDLE;
				}
			}
		}
		break;
	}
}

void appMuxTxPath(stream<axiWord> 		&txDataInDhcp, 	stream<metadata> 	&txMetadataInDhcp, 	stream<ap_uint<16> > 	&txLengthInDhcp,
			   stream<axiWord> 		&txDataInApp, 	stream<metadata> 	&txMetadataInApp, 	stream<ap_uint<16> > 	&txLengthInApp,
			   stream<axiWord> 		&txDataOut, 	stream<metadata> 	&txMetadataOut, 	stream<ap_uint<16> > 	&txLengthOut) {
#pragma HLS PIPELINE II=1
	static enum txsState {SHIM_IDLE = 0, SHIM_STREAM} shimState_tx;
	static ap_uint<1>	streamSource = 0;
	
	switch(shimState_tx) {
		case SHIM_IDLE:
			if (!txDataOut.full() && !txMetadataOut.full() && !txLengthOut.full()) {
				if (!txDataInDhcp.empty() && !txMetadataInDhcp.empty() && !txLengthInDhcp.empty()) {
					streamSource = 0;
					axiWord outputWord = txDataInDhcp.read();
					txDataOut.write(outputWord);
					txMetadataOut.write(txMetadataInDhcp.read());
					txLengthOut.write(txLengthInDhcp.read());
					if (outputWord.last == 0)
						shimState_tx = SHIM_STREAM;
				}
				else if (!txDataInApp.empty() && !txMetadataInApp.empty() && !txLengthInApp.empty()) {
					streamSource = 1;
					axiWord outputWord = txDataInApp.read();
					txDataOut.write(outputWord);
					txMetadataOut.write(txMetadataInApp.read());
					txLengthOut.write(txLengthInApp.read());
					if (outputWord.last == 0)
						shimState_tx = SHIM_STREAM;
				}
			}
			break;
		case SHIM_STREAM:
			if (!txDataOut.full()) {
				if (streamSource == 0 && !txDataInDhcp.empty()) {
					axiWord outputWord = txDataInDhcp.read();
					txDataOut.write(outputWord);
					if (outputWord.last == 1)
						shimState_tx = SHIM_IDLE;
				}
				else if (streamSource == 1 && !txDataInApp.empty()) {
					axiWord outputWord = txDataInApp.read();
					txDataOut.write(outputWord);
					if (outputWord.last == 1)
						shimState_tx = SHIM_IDLE;
				}
			}
			break;
	}
}

void appMuxPortPath(stream<ap_uint<16> >&  requestPortOpenOut, 		stream<bool >& portOpenReplyIn,
			  stream<ap_uint<16> >&  requestPortOpenInDhcp, 	stream<bool >& portOpenReplyOutDhcp,
			  stream<ap_uint<16> >&  requestPortOpenInApp, 		stream<bool >& portOpenReplyOutApp) {
#pragma HLS PIPELINE II=1
	static enum portState {PORT_IDLE = 0, PORT_STREAM} shimStatePort;
	static ap_uint<1>	streamSourcePort = 0;
	
	switch(shimStatePort) {
		case PORT_IDLE:
			if (!requestPortOpenOut.full()) {
				if (!requestPortOpenInDhcp.empty()) {
					requestPortOpenOut.write(requestPortOpenInDhcp.read());
					streamSourcePort = 0;
					shimStatePort = PORT_STREAM;
				}
				else if (!requestPortOpenInApp.empty()) {
					requestPortOpenOut.write(requestPortOpenInApp.read());
					streamSourcePort = 1;
					shimStatePort = PORT_STREAM;
				}
			}
			break;
		case PORT_STREAM:
			if (!portOpenReplyOutDhcp.full()) {
				if (streamSourcePort == 0 && !portOpenReplyIn.empty()) {
					portOpenReplyOutDhcp.write(portOpenReplyIn.read());
					shimStatePort = PORT_IDLE;
				}
				else if (streamSourcePort == 1 && !portOpenReplyIn.empty()) {
					portOpenReplyOutApp.write(portOpenReplyIn.read());
					shimStatePort = PORT_IDLE;
				}
			}
			break;
	}
}
			 
void udpAppMux(stream<axiWord>			&rxDataIn, 		stream<metadata>&     	rxMetadataIn,
			   stream<axiWord> 			&rxDataOutDhcp, stream<metadata>&     	rxMetadataOutDhcp,
			   stream<axiWord> 			&rxDataOutApp, 	stream<metadata>&     	rxMetadataOutApp,
			   
			   stream<ap_uint<16> >&  requestPortOpenOut, 		stream<bool >& portOpenReplyIn,
			   stream<ap_uint<16> >&  requestPortOpenInDhcp, 	stream<bool >& portOpenReplyOutDhcp,
			   stream<ap_uint<16> >&  requestPortOpenInApp, 	stream<bool >& portOpenReplyOutApp,
			   
			   stream<axiWord> 		&txDataInDhcp, 	stream<metadata> 	&txMetadataInDhcp, 	stream<ap_uint<16> > 	&txLengthInDhcp,
			   stream<axiWord> 		&txDataInApp, 	stream<metadata> 	&txMetadataInApp, 	stream<ap_uint<16> > 	&txLengthInApp,
			   stream<axiWord> 		&txDataOut, 	stream<metadata> 	&txMetadataOut, 	stream<ap_uint<16> > 	&txLengthOut) {
	#pragma HLS INTERFACE ap_ctrl_none port=return
	#pragma HLS DATAFLOW interval=1
	
	#pragma HLS resource core=AXI4Stream variable=rxDataIn 				metadata="-bus_bundle rxDataIn"
	#pragma HLS resource core=AXI4Stream variable=rxMetadataIn 			metadata="-bus_bundle rxMetadataIn"
	#pragma HLS resource core=AXI4Stream variable=requestPortOpenOut 	metadata="-bus_bundle requestPortOpenOut"
	#pragma HLS resource core=AXI4Stream variable=portOpenReplyIn 		metadata="-bus_bundle portOpenReplyIn"
	#pragma HLS resource core=AXI4Stream variable=txDataOut 			metadata="-bus_bundle txDataOut"
	#pragma HLS resource core=AXI4Stream variable=txMetadataOut 		metadata="-bus_bundle txMetadataOut"
	#pragma HLS resource core=AXI4Stream variable=txLengthOut 			metadata="-bus_bundle txLengthOut"
	
	#pragma HLS resource core=AXI4Stream variable=rxDataOutDhcp 		metadata="-bus_bundle rxDataOutDhcp"
	#pragma HLS resource core=AXI4Stream variable=rxMetadataOutDhcp 	metadata="-bus_bundle rxMetadataOutDhcp"
	#pragma HLS resource core=AXI4Stream variable=requestPortOpenInDhcp	metadata="-bus_bundle requestPortOpenInDhcp"
	#pragma HLS resource core=AXI4Stream variable=portOpenReplyOutDhcp 	metadata="-bus_bundle portOpenReplyOutDhcp"
	#pragma HLS resource core=AXI4Stream variable=txDataInDhcp 			metadata="-bus_bundle txDataInDhcp"
	#pragma HLS resource core=AXI4Stream variable=txMetadataInDhcp 		metadata="-bus_bundle txMetadataInDhcp"
	#pragma HLS resource core=AXI4Stream variable=txLengthInDhcp 		metadata="-bus_bundle txLengthInDhcp"
	
	#pragma HLS resource core=AXI4Stream variable=rxDataOutApp 			metadata="-bus_bundle rxDataOutApp"
	#pragma HLS resource core=AXI4Stream variable=rxMetadataOutApp 		metadata="-bus_bundle rxMetadataOutApp"
	#pragma HLS resource core=AXI4Stream variable=requestPortOpenInApp	metadata="-bus_bundle requestPortOpenInApp"
	#pragma HLS resource core=AXI4Stream variable=portOpenReplyOutApp 	metadata="-bus_bundle portOpenReplyOutApp"
	#pragma HLS resource core=AXI4Stream variable=txDataInApp 			metadata="-bus_bundle txDataInApp"
	#pragma HLS resource core=AXI4Stream variable=txMetadataInApp 		metadata="-bus_bundle txMetadataInApp"
	#pragma HLS resource core=AXI4Stream variable=txLengthInApp 		metadata="-bus_bundle txLengthInApp"

  	#pragma HLS aggregate  variable=rxMetadataIn compact=bit
  	#pragma HLS aggregate  variable=rxMetadataOutDhcp compact=bit
  	#pragma HLS aggregate  variable=rxMetadataOutApp compact=bit
  	#pragma HLS aggregate  variable=txMetadataOut compact=bit
  	#pragma HLS aggregate  variable=txMetadataInDhcp compact=bit
  	#pragma HLS aggregate  variable=txMetadataInApp compact=bit

	appMuxRxPath(rxDataIn, rxMetadataIn, 
		   rxDataOutDhcp, rxMetadataOutDhcp,
		   rxDataOutApp, rxMetadataOutApp);
	appMuxPortPath(requestPortOpenOut, portOpenReplyIn,
			 requestPortOpenInDhcp, portOpenReplyOutDhcp,
			 requestPortOpenInApp, portOpenReplyOutApp);
	appMuxTxPath(txDataInDhcp, txMetadataInDhcp, txLengthInDhcp,
		   txDataInApp, txMetadataInApp, txLengthInApp, 
		   txDataOut, txMetadataOut, txLengthOut);
}
