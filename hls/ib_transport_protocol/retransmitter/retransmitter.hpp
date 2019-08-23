/************************************************
Copyright (c) 2019, Systems Group, ETH Zurich.
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
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
************************************************/
#pragma once

#include "../../axi_utils.hpp"
#include "../ib_transport_protocol.hpp"
using namespace hls;

const uint32_t META_TABLE_SIZE = 2000;

struct retransEvent;

struct retransRelease
{
	ap_uint<16> qpn;
	ap_uint<24> latest_acked_req; //TODO rename?
	retransRelease() {};
	retransRelease(ap_uint<16> qpn, ap_uint<24> psn)
		:qpn(qpn), latest_acked_req(psn) {}
};

struct retransmission
{
	ap_uint<16> qpn;
	ap_uint<24> psn;
	//bool		implicit; //TODO or remove
	retransmission() {}
	retransmission(ap_uint<16> qpn)
		:qpn(qpn), psn(0) {}
	retransmission(ap_uint<16> qpn, ap_uint<24> psn)
		:qpn(qpn), psn(psn) {}
};

struct retransMeta
{
	ap_uint<16> qpn;
	ap_uint<24> psn;
	ibOpCode	opCode;
	retransMeta() {}
	retransMeta(ap_uint<16> qpn, ap_uint<24> psn, ibOpCode op)
		:qpn(qpn), psn(psn), opCode(op){}
};

struct retransAddrLen
{
	ap_uint<48> localAddr;
	ap_uint<48> remoteAddr;
	ap_uint<32> length;
	retransAddrLen() {}
	retransAddrLen(ap_uint<48> laddr, ap_uint<48> raddr, ap_uint<32> len)
		:localAddr(laddr), remoteAddr(raddr), length(len) {}
};

struct retransEntry
{
	ap_uint<16> qpn;
	ap_uint<24> psn;
	ibOpCode	opCode;
	ap_uint<48> localAddr;
	ap_uint<48> remoteAddr;
	ap_uint<32> length;
	retransEntry() {}
	retransEntry(ap_uint<16> qpn, ap_uint<24> psn, ibOpCode op, ap_uint<64> laddr, ap_uint<64> raddr, ap_uint<32> len)
		:qpn(qpn), psn(psn), opCode(op), localAddr(laddr), remoteAddr(raddr), length(len) {}
	retransEntry(retransMeta meta, retransAddrLen addrlen)
		:qpn(meta.qpn), psn(meta.psn), opCode(meta.opCode), localAddr(addrlen.localAddr), remoteAddr(addrlen.remoteAddr), length(addrlen.length) {}
};

struct retransPointerEntry
{
	ap_uint<16>	head;
	ap_uint<16> tail;
	//ap_uint<16> oldest_read_req; //TODO maybe move to fsm
	bool valid;
};

typedef enum {INSERT, RELEASE, RX_RETRANS, TIMER_RETRANS} retransOperation ;

struct pointerMeta
{
	retransOperation	op;
	//ap_uint<16>			qpn;
	ap_uint<24>			psn;
	retransPointerEntry entry;
	pointerMeta() {}
	pointerMeta(retransOperation op, ap_uint<24> psn, retransPointerEntry e)
		:op(op), psn(psn), entry(e) {}
};

struct pointerUpdate
{
	ap_uint<16>	qpn;
	retransPointerEntry entry;
	pointerUpdate() {}
	pointerUpdate(ap_uint<16> qpn, retransPointerEntry e)
		:qpn(qpn), entry(e) {}
};

struct retransMetaEntry
{
	ap_uint<24> psn;
	ap_uint<16> next;
	ibOpCode	opCode;
	ap_uint<48> localAddr;
	ap_uint<48> remoteAddr;
	ap_uint<32> length;
	bool valid;
	bool isTail;
	retransMetaEntry() {}
	retransMetaEntry(retransEntry& e)
		:psn(e.psn), next(0), opCode(e.opCode), localAddr(e.localAddr), remoteAddr(e.remoteAddr), length(e.length), valid(true), isTail(true) {}
	/*retransMetaEntry(ap_uint<24> psn)
		:psn(psn) {}*/
	retransMetaEntry(ap_uint<16> next)
		:next(next) {}
};

struct retransMetaReq
{
	ap_uint<16> idx;
	retransMetaEntry entry;
	bool write;
	bool append;
	retransMetaReq() {}
	retransMetaReq(ap_uint<16> idx)
		:idx(idx), write(false),  append(false) {}
	retransMetaReq(ap_uint<16> idx, ap_uint<16> next)
		:idx(idx), entry(retransMetaEntry(next)), write(false), append(true) {}
	retransMetaReq(ap_uint<16> idx, retransMetaEntry e)
		:idx(idx), entry(e), write(true), append(false) {}
};

/*struct retransMetaRsp
{
	retransMetaEntry	entry;
	//bool			 	last;
	retransMetaRsp() {}
	retransMetaRsp(retransMetaEntry e)
		:entry(e) {} //, last(false) {}
	retransMetaRsp(retransMetaEntry e, bool l)
		:entry(e) {} //, last(l) {}
};*/

struct pointerReq
{
	ap_uint<16>	qpn;
	bool		lock;
	//bool		write;
	pointerReq() {}
	pointerReq(ap_uint<16> qpn)
		:qpn(qpn), lock(false) {}
	pointerReq(ap_uint<16> qpn, bool l)
		:qpn(qpn), lock(l) {}
};

void retransmitter(	stream<retransRelease>&	rx2retrans_release_upd,
					stream<retransmission>& rx2retrans,
					stream<retransmission>& timer2retrans,
					stream<retransEntry>&	tx2retrans_insertRequest,
					stream<retransEvent>&	retrans2event);