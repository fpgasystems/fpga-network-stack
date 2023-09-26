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
#include "../axi_utils.hpp"

using namespace hls;

enum dhcpMessageType {DISCOVER=0x01, OFFER=0x02, REQUEST=0x03, ACK=0x05};

struct dhcpReplyMeta
{
	ap_uint<32>		identifier;
	ap_uint<32>		assignedIpAddress;
	ap_uint<32>		serverAddress;
	ap_uint<8>		type;
};

struct dhcpRequestMeta
{
	ap_uint<32>		identifier;
	ap_uint<8>		type;
	ap_uint<32>		requestedIpAddress;
	dhcpRequestMeta() {}
	dhcpRequestMeta(ap_uint<32> i, ap_uint<8> type)
		:identifier(i), type(type), requestedIpAddress(0) {}
	dhcpRequestMeta(ap_uint<32> i, ap_uint<8> type, ap_uint<32> ip)
		:identifier(i), type(type), requestedIpAddress(ip) {}
};

static const ap_uint<32> MAGIC_COOKIE		= 0x63538263;

#ifndef __SYNTHESIS__
static const ap_uint<32> TIME_US = 200;
static const ap_uint<32> TIME_5S = 100;
static const ap_uint<32> TIME_30S = 300;
#else
//static const ap_uint<32> TIME_US = 200;
//static const ap_uint<32> TIME_5S = 100;
//static const ap_uint<32> TIME_30S = 300;
static const ap_uint<32> TIME_US = 20000;
static const ap_uint<32> TIME_5S = 750750750;
static const ap_uint<32> TIME_30S = 0xFFFFFFFF;
#endif
//Copied from udp.hpp
struct sockaddr_in {
    ap_uint<16>     port;   /* port in network byte order */
    ap_uint<32>		addr;   /* internet address */
    sockaddr_in () {}
    sockaddr_in (ap_uint<32> addr, ap_uint<16> port)
    	:addr(addr), port(port) {}
};

struct udpMetadata {
	sockaddr_in sourceSocket;
	sockaddr_in destinationSocket;
	udpMetadata() {}
	udpMetadata(sockaddr_in src, sockaddr_in dst)
		:sourceSocket(src), destinationSocket(dst) {}
};

template <int WIDTH>
void dhcp_client(	stream<ap_uint<16> >&	openPort,
					stream<bool>&			confirmPortStatus,
					//stream<ap_uint<16> >&	realeasePort,
					stream<udpMetadata>&	dataInMeta,
					stream<ap_axiu<DATA_WIDTH, 0, 0, 0> >&		dataIn,
					stream<udpMetadata>&	dataOutMeta,
					stream<ap_uint<16> >&	dataOutLength,
					stream<ap_axiu<DATA_WIDTH, 0, 0, 0> >&		dataOut,
					ap_uint<1>&				dhcpEnable,
					ap_uint<32>&			inputIpAddress,
					ap_uint<32>&			dhcpIpAddressOut,
					ap_uint<48>&			myMacAddress);
