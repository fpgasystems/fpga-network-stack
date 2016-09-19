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

const uint8_t 	noOfArpTableEntries	= 8;

struct axiWord {
	ap_uint<64>		data;
	ap_uint<8>		keep;
	ap_uint<1>		last;
};

struct arpTableReply
{
	ap_uint<48> macAddress;
	bool		hit;
	arpTableReply() {}
	arpTableReply(ap_uint<48> macAdd, bool hit)
			:macAddress(macAdd), hit(hit) {}
};

struct arpTableEntry {
	ap_uint<48>	macAddress;
	ap_uint<32> ipAddress;
	ap_uint<1>	valid;
	arpTableEntry() {}
	arpTableEntry(ap_uint<48> newMac, ap_uint<32> newIp, ap_uint<1> newValid)
				 : macAddress(newMac), ipAddress(newIp), valid(newValid) {}
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

struct rtlMacLookupRequest {
	ap_uint<1>			source;
	ap_uint<32>			key;
	rtlMacLookupRequest() {}
	rtlMacLookupRequest(ap_uint<32> searchKey)
				:key(searchKey), source(0) {}
};

struct rtlMacUpdateRequest {
	ap_uint<1>			source;
	ap_uint<1>			op;
	ap_uint<48>			value;
	ap_uint<32>			key;

	rtlMacUpdateRequest() {}
	rtlMacUpdateRequest(ap_uint<32> key, ap_uint<48> value, ap_uint<1> op)
			:key(key), value(value), op(op), source(0) {}
};

struct rtlMacLookupReply {

	ap_uint<1>			hit;
	ap_uint<48>			value;
	rtlMacLookupReply() {}
	rtlMacLookupReply(bool hit, ap_uint<48> returnValue)
			:hit(hit), value(returnValue) {}
};

struct rtlMacUpdateReply {
	ap_uint<1>			source;
	ap_uint<1>			op;
	ap_uint<48>			value;

	rtlMacUpdateReply() {}
	rtlMacUpdateReply(ap_uint<1> op)
			:op(op), source(0) {}
	rtlMacUpdateReply(ap_uint<8> id, ap_uint<1> op)
			:value(id), op(op), source(0) {}
};

void arp_server(  stream<axiWord>&          	arpDataIn,
                  stream<ap_uint<32> >&     	macIpEncode_req,
				  stream<axiWord>&          	arpDataOut,
				  stream<arpTableReply>&    	macIpEncode_rsp,
				  stream<rtlMacLookupRequest>	&macLookup_req,
				  stream<rtlMacLookupReply>		&macLookup_resp,
				  stream<rtlMacUpdateRequest>	&macUpdate_req,
				  stream<rtlMacUpdateReply>		&macUpdate_resp,
				  ap_uint<32>	      			myIpAddress,
				  ap_uint<48>					myMacAddress);
