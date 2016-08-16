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
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <math.h>
#include <hls_stream.h>
#include "ap_int.h"
#include <stdint.h>

using namespace hls;

const uint16_t 		REQUEST 		= 0x0100;
const uint16_t 		REPLY 			= 0x0200;
const ap_uint<32>	replyTimeOut 	= 65536;

const ap_uint<48> MY_MAC_ADDR 	= 0xE59D02350A00; 	// LSB first, 00:0A:35:02:9D:E5
const ap_uint<48> BROADCAST_MAC	= 0xFFFFFFFFFFFF;	// Broadcast MAC Address

const uint8_t 	noOfArpTableEntries	= 8;


struct axiWord {
	ap_uint<64>		data;
	ap_uint<8>		keep;
	ap_uint<1>		last;
};

struct sockaddr_in {
    ap_uint<16>     port;   /* port in network byte order */
    ap_uint<32>		addr;   /* internet address */
};

struct metadata {
	sockaddr_in sourceSocket;
	sockaddr_in destinationSocket;
};

void udpLoopback(stream<axiWord>&      	rxDataIn,
                 stream<metadata>&     	rxMetadataIn,
				 stream<ap_uint<16> >&  requestPortOpenOut,
				 stream<bool>&			portOpenReplyIn,
				 stream<axiWord> 		&txDataOut,
				 stream<metadata> 		&txMetadataOut,
				 stream<ap_uint<16> > 	&txLengthOut);
