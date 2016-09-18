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

const ap_uint<48> BROADCAST_MAC	= 0xFFFFFFFFFFFF;	// Broadcast MAC Address

struct axiWord
{
	ap_uint<64>		data;
	ap_uint<8>		keep;
	ap_uint<1>		last;
};

/** @ingroup arp_server
 *
 */
struct arpTableEntry
{
	ap_uint<32> ipAddress;
	ap_uint<48> macAddress;
	bool		valid;
	arpTableEntry() {}
	arpTableEntry(ap_uint<32> protoAdd, ap_uint<48> hwAdd, bool valid)
		:ipAddress(protoAdd), macAddress(hwAdd), valid(valid) {}
};

struct arpTableReply
{
	ap_uint<48> macAddress;
	bool		hit;
	arpTableReply() {}
	arpTableReply(ap_uint<48> macAdd, bool hit)
			:macAddress(macAdd), hit(hit) {}
};

struct arpReplyMeta
{
  ap_uint<48>   srcMac; //rename
  ap_uint<16>   ethType;
  ap_uint<16>   hwType;
  ap_uint<16>   protoType;
  ap_uint<8>    hwLen;
  ap_uint<8>    protoLen;
  ap_uint<48>   hwAddrSrc;
  ap_uint<32>   protoAddrSrc;
  arpReplyMeta() {}
};

/** @defgroup arp_server ARP Server
 *
 */
void arp_server_subnet(	stream<axiWord> &inData,
						stream<ap_uint<32> > &queryIP,
						stream<axiWord> &outData,
						stream<arpTableReply> &returnMAC,
						ap_uint<48> myMacAddress,
						ap_uint<32> myIpAddress);
