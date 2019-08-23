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

#include <iostream>
#include "../axi_utils.hpp"
#include "ib_transport_protocol.hpp"
#include "retransmitter/retransmitter.hpp"
#include <rocev2_config.hpp> //defines MAX_QPS


using namespace hls;

#ifndef __SYNTHESIS__
static const ap_uint<32> TIME_10us		= 5;
static const ap_uint<32> TIME_50us		= 5;
static const ap_uint<32> TIME_100us		= 5;
static const ap_uint<32> TIME_250us		= 10;
static const ap_uint<32> TIME_500us		= 10;
static const ap_uint<32> TIME_1ms		= 10;
static const ap_uint<32> TIME_5ms		= 10;
static const ap_uint<32> TIME_10ms		= 10;
#else
static const ap_uint<32> TIME_10us		= (10.0/0.0064/MAX_QPS) + 1;
static const ap_uint<32> TIME_50us		= (50.0/0.0064/MAX_QPS) + 1;
static const ap_uint<32> TIME_100us		= (100.0/0.0064/MAX_QPS) + 1;
static const ap_uint<32> TIME_250us		= (250.0/0.0064/MAX_QPS) + 1;
static const ap_uint<32> TIME_500us		= (500.0/0.0064/MAX_QPS) + 1;
static const ap_uint<32> TIME_1ms		= (1000.0/0.0064/MAX_QPS) + 1;
static const ap_uint<32> TIME_5ms		= (5000.0/0.0064/MAX_QPS) + 1;
static const ap_uint<32> TIME_10ms		= (10000.0/0.0064/MAX_QPS) + 1;
#endif

struct event;
struct retransmission;

struct transportTimerEntry
{
	ap_uint<32> time;
	ap_uint<3>	retries;
	bool		active;
};

struct rxTimerUpdate
{
	ap_uint<16> qpn;
	bool stop;
	rxTimerUpdate() {}
	rxTimerUpdate(ap_uint<16> qpn, bool stop)
		:qpn(qpn), stop(stop) {}
};

/**
 * page 352
 */
void transport_timer(	stream<rxTimerUpdate>&	rxClearTimer_req,
						stream<ap_uint<24> >&	txSetTimer_req,
						stream<retransmission>&	timer2retrans_req);