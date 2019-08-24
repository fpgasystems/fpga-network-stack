/*
 * Copyright (c) 2019, Systems Group, ETH Zurich
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
#pragma once

#include "axi_utils.hpp"

typedef enum {
	ROUTE_DMA = 0x0,
	ROUTE_CUSTOM = 0x1,
} axiRoute;

struct dmCmd
{
	ap_uint<23>	bbt;
	ap_uint<1>	type;
	ap_uint<6>	dsa;
	ap_uint<1>	eof;
	ap_uint<1>	drr;
	ap_uint<32>	saddr;
	ap_uint<4>	tag;
	ap_uint<4>	rsvd;
	dmCmd() {}
	//dmCmd(const dmCmd& cmd)
		//:bbt(cmd.bbt), type(cmd.type), dsa(cmd.dsa), eof(cmd.eof), drr(cmd.drr), saddr(cmd.saddr), tag(cmd.tag), rsvd(cmd.rsvd) {}
	dmCmd(ap_uint<32> addr, ap_uint<16> len) //TODO length depends on datamover config
		:bbt(len), type(1), dsa(0), eof(1), drr(0), saddr(addr), tag(0), rsvd(0) {}
	dmCmd(ap_uint<32> addr, ap_uint<16> len, ap_uint<4> tag)
		:bbt(len), type(1), dsa(0), eof(1), drr(0), saddr(addr), tag(tag), rsvd(0) {}
};

struct routedDmCmd
{
	ap_uint<23>	bbt;
	ap_uint<32>	saddr;
	ap_uint<4>	tag;
	ap_uint<1>	dest;
	routedDmCmd() {}
	routedDmCmd(ap_uint<32> addr, ap_uint<16> len, ap_uint<1> dest=0)
		:bbt(len), saddr(addr), tag(0), dest(dest) {}
	routedDmCmd(ap_uint<32> addr, ap_uint<16> len, ap_uint<4> tag, ap_uint<1> dest=0)
		:bbt(len), saddr(addr), tag(tag), dest(dest) {}
};

struct dmStatus
{
	ap_uint<4>	tag;
	ap_uint<1>	interr;
	ap_uint<1>	decerr;
	ap_uint<1>	slverr;
	ap_uint<1>	okay;
	dmStatus() {}
	dmStatus(bool okay)
		:okay(okay) {}
	dmStatus(bool okay, ap_uint<4> tag)
		:okay(okay), tag(tag) {}
};

struct memCmd
{
	ap_uint<64> addr;
	ap_uint<32> len;
	memCmd() {}
	memCmd(ap_uint<64> addr, ap_uint<32> len)
		:addr(addr), len(len) {}
};

struct routedMemCmd
{
	memCmd      data;
	ap_uint<1>	dest;
	routedMemCmd() {}
	routedMemCmd(ap_uint<64> addr, ap_uint<32> len)
		:data(addr, len), dest(ROUTE_DMA) {}
	routedMemCmd(ap_uint<64> addr, ap_uint<32> len, axiRoute route)
		:data(addr, len), dest(route) {}
};
