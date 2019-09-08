/*
 * Copyright (c) 2018, Systems Group, ETH Zurich
 * Copyright (c) 2016 Xilinx, Inc.
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
#ifndef ARP_SERVER_SUBNET
#define ARP_SERVER_SUBNET

#include "../axi_utils.hpp"
#include "../packet.hpp"

#define ARP_HEADER_SIZE 336

const uint16_t 		ARP_OP_REQUEST	= 0x0100;
const uint16_t 		ARP_OP_REPLY 	= 0x0200;
const ap_uint<48> BROADCAST_MAC	= 0xFFFFFFFFFFFF;	// Broadcast MAC Address

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
	ap_uint<48>   srcMacAddr;
	ap_uint<48>   senderHwAddr;
	ap_uint<32>   senderProtoAddr;
	arpReplyMeta() {}
	arpReplyMeta(ap_uint<48> srcMacAddr, ap_uint<48> senderHwAddr, ap_uint<32> senderProtoAddr)
		:srcMacAddr(srcMacAddr), senderHwAddr(senderHwAddr), senderProtoAddr(senderProtoAddr) {}
};

/* *
 * [47:0] MAC destination address
 * [95:48] MAC source address
 * [111:96] Ethernet type
 * [127:112] Hardware type
 * [143:128] Protocol type
 * [151:144] Hardware length
 * [159:152] Protocol length
 * [175:160] Operation [REQUEST, REQPLY]
 * [223:176] Sender hardware address
 * [255:224] Sender protocol address
 * [303:256] Target hardware address
 * [335:304] Target protocol address
 */
template <int W>
class arpHeader : public packetHeader<W, ARP_HEADER_SIZE> {
	using packetHeader<W, ARP_HEADER_SIZE>::header;

public:
	arpHeader()
	{
		header(111,96) = 0x0608; // Ethernet type
		header(127,112) = 0x0100; //Hardware type
		header(143, 128) = 0x0008; //Protocol type
		header(151,144) = 6; //Hardware length
		header(158,152) = 4; //Protocol length
	}
	void setDstMacAddr(const ap_uint<48>& addr)
	{
		header(47, 0)  = addr;
	}
	ap_uint<48> getDstMacAddr()
	{
		return header(47, 0);
	}
	void setSrcMacAddr(const ap_uint<48>& addr)
	{
		header(95, 48)  = addr;
	}
	ap_uint<48> getSrcMacAddr()
	{
		return header(95, 48);
	}
	void setSenderHwAddr(const ap_uint<48>& addr)
	{
		header(223,176)  = addr;
	}
	ap_uint<48> getSenderHwAddr()
	{
		return header(223,176);
	}
	void setSenderProtoAddr(const ap_uint<32>& addr)
	{
		header(255, 224)  = addr;
	}
	ap_uint<32> getSenderProtoAddr()
	{
		return header(255, 224);
	}
	void setTargetHwAddr(const ap_uint<48>& addr)
	{
		header(303, 256)  = addr;
	}
	ap_uint<48> getTargetHwAddr()
	{
		return header(303, 256);
	}
	void setTargetProtoAddr(const ap_uint<32>& addr)
	{
		header(335, 304)  = addr;
	}
	ap_uint<32> getTargetProtoAddr()
	{
		return header(335, 304);
	}
	void setRequest()
	{
		header(175,160) = ARP_OP_REQUEST;
	}
	bool isRequest()
	{
		return (header(175,160) == ARP_OP_REQUEST);
	}
	void setReply()
	{
		header(175,160) = ARP_OP_REPLY;
	}
	bool isReply()
	{
		return (header(175,160) == ARP_OP_REPLY);
	}
};

template <int WIDTH>
void arp_server_subnet(	hls::stream<net_axis<WIDTH> >&          arpDataIn,
                  	  	hls::stream<ap_uint<32> >&     macIpEncode_req,
                  	  	hls::stream<ap_uint<32> >&     hostIpEncode_req,
				        hls::stream<net_axis<WIDTH> >&          arpDataOut,
				        hls::stream<arpTableReply>&    macIpEncode_rsp,
				        hls::stream<arpTableReply>&    hostIpEncode_rsp,
							ap_uint<48> myMacAddress,
							ap_uint<32> myIpAddress,
							ap_uint<16>&			regRequestCount,
							ap_uint<16>&			regReplyCount);

#endif