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
static const ap_uint<32> TIME_12ms		= 10;
static const ap_uint<32> TIME_64ms		= 10;
#else
static const ap_uint<32> TIME_10us		= (10.0/0.0064/MAX_QPS) + 1;
static const ap_uint<32> TIME_50us		= (50.0/0.0064/MAX_QPS) + 1;
static const ap_uint<32> TIME_100us		= (100.0/0.0064/MAX_QPS) + 1;
static const ap_uint<32> TIME_250us		= (250.0/0.0064/MAX_QPS) + 1;
static const ap_uint<32> TIME_500us		= (500.0/0.0064/MAX_QPS) + 1;
static const ap_uint<32> TIME_1ms		= (1000.0/0.0064/MAX_QPS) + 1;
static const ap_uint<32> TIME_5ms		= (5000.0/0.0064/MAX_QPS) + 1;
static const ap_uint<32> TIME_12ms		= (12000.0/0.0064/MAX_QPS) + 1;
static const ap_uint<32> TIME_64ms		= (64000.0/0.0064/MAX_QPS) + 1;
#endif

struct event;
struct retransmission;

struct transportTimerEntry
{
	ap_uint<32> time;
	ap_uint<4>	retries;
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

/*
 * page 352
 * One timer per QP
 * Timer gets started when a WRITE (with AckReqBit) or a READ_REQ is send
 * Timer gets reset everytime a valid ACK is received
 * Timer is stopped when there are no more outstanding requests
 * Maintain a 3-bit retry counter, decrement on timeout, clear on new ACK
 */
template <int INSTID = 0>
void transport_timer(	
    stream<rxTimerUpdate>&	rxClearTimer_req,
	stream<ap_uint<24> >&	txSetTimer_req,
	stream<retransmission>&	timer2retrans_req
) {
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	static transportTimerEntry transportTimerTable[MAX_QPS];
#if defined( __VITIS_HLS__)
	#pragma HLS bind_storage variable=transportTimerTable type=RAM_T2P impl=BRAM
	#pragma HLS aggregate  variable=transportTimerTable compact=bit
#else
	#pragma HLS RESOURCE variable=transportTimerTable core=RAM_T2P_BRAM
	#pragma HLS DATA_PACK variable=transportTimerTable
#endif

	#pragma HLS DEPENDENCE variable=transportTimerTable inter false

	static ap_uint<16>			tt_currPosition = 0;
	static bool tt_WaitForWrite = false;


	ap_uint<16> checkQP;
	static rxTimerUpdate tt_update;
	ap_uint<24> setQP;
	transportTimerEntry entry;
	ap_uint<1> operationSwitch = 0;


	if (!rxClearTimer_req.empty())
	{
		rxClearTimer_req.read(tt_update);
		if (!tt_update.stop)
		{
			transportTimerTable[tt_update.qpn].time = TIME_1ms;
		}
		else
		{
			transportTimerTable[tt_update.qpn].time = 0;
			transportTimerTable[tt_update.qpn].active = false;
		}
		transportTimerTable[tt_update.qpn].retries = 0;
	}
	else if (!txSetTimer_req.empty())
    {
        // update transportTimerTable with new (timeout) time
        txSetTimer_req.read(setQP);
        if ((setQP - 3 < tt_currPosition) && (tt_currPosition <= setQP))
        {
            tt_currPosition += 5;
        }
		entry = transportTimerTable[setQP];
        if (!entry.active)
        {
            if(entry.retries < RETRANS_S1) {
                entry.time = TIME_1ms;
            }
            else if(entry.retries < RETRANS_S2) {
                entry.time = TIME_5ms;
            }
            else if(entry.retries < RETRANS_S3) {
                entry.time = TIME_12ms;
            }
            else {
                entry.time = TIME_64ms;
            }
        }
        entry.active = true;
		transportTimerTable[setQP] = entry;
    }
    else
	{
        // perform round robin to check whether it has timed out
		checkQP = tt_currPosition;
        tt_currPosition++;
        if (tt_currPosition >= MAX_QPS)
        {
            tt_currPosition = 0;
        }

		//Get entry from table
		entry = transportTimerTable[checkQP];

        if (entry.active)
        {
            if (entry.time > 0 )
            {
                entry.time--;
            }
            else if (!timer2retrans_req.full())
            {
                entry.time = 0;
                entry.active = false;

                if (entry.retries < RETRANS_RETRY_CNT)
                {
                    entry.retries++;
                    //TODO shut QP down if too many retries
                    timer2retrans_req.write(retransmission(checkQP));
                }
            }
        }

		//write entry back
		transportTimerTable[checkQP] = entry;
	}
}
