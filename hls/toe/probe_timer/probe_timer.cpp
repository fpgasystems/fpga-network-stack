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

#include "probe_timer.hpp"

using namespace hls;

/** @ingroup probe_timer
 *  Reads in the Session-ID and activates. a timer with an interval of 5 seconds. When the timer times out
 *  a RT Event is fired to the @ref tx_engine. In case of a zero-window (or too small window) an RT Event
 *  will generate a packet without payload which is the same as a probing packet.
 *	@param[in]		txEng2timer_setProbeTimer
 *	@param[out]		probeTimer2eventEng_setEvent
 */
void probe_timer(	stream<ap_uint<16> >&		rxEng2timer_clearProbeTimer,
					stream<ap_uint<16> >&		txEng2timer_setProbeTimer,
					stream<event>&				probeTimer2eventEng_setEvent)
{
#pragma HLS aggregate  variable=txEng2timer_setProbeTimer compact=bit
#pragma HLS aggregate  variable=probeTimer2eventEng_setEvent compact=bit

#pragma HLS PIPELINE II=1

	static probe_timer_entry probeTimerTable[MAX_SESSIONS];
	#pragma HLS bind_storage variable=probeTimerTable type=RAM_T2P impl=BRAM
	#pragma HLS aggregate  variable=probeTimerTable compact=bit
	#pragma HLS DEPENDENCE variable=probeTimerTable inter false

	static ap_uint<16>		pt_currSessionID = 0;
	static ap_uint<16>		pt_updSessionID = 0;
	static ap_uint<16>		pt_prevSessionID = 0;
	static bool				pt_WaitForWrite = false;

	ap_uint<16> checkID;
	bool		fastResume = false;

	if (pt_WaitForWrite)
	{
		if (pt_updSessionID != pt_prevSessionID)
		{
			probeTimerTable[pt_updSessionID].time = TIME_50ms;
			probeTimerTable[pt_updSessionID].active = true;
			pt_WaitForWrite = false;
		}
		pt_prevSessionID--;
	}
	else if (!txEng2timer_setProbeTimer.empty())
	{
		txEng2timer_setProbeTimer.read(pt_updSessionID);
		pt_WaitForWrite = true;
	}
	// too restrictive FIXME
	else// if (!probeTimer2eventEng_setEvent.full()) this leads to II=2
	{
		//pt_prevSessionID = pt_currSessionID;
		checkID = pt_currSessionID;

		if (!rxEng2timer_clearProbeTimer.empty())
		{
			rxEng2timer_clearProbeTimer.read(checkID);
			fastResume = true;
		}
		else
		{
			pt_currSessionID++;
			if (pt_currSessionID == MAX_SESSIONS)
			{
				pt_currSessionID = 0;
			}
		}

		// Check if 0, otherwise decrement
		if (probeTimerTable[checkID].active && !probeTimer2eventEng_setEvent.full())
		{
			if (probeTimerTable[checkID].time == 0 || fastResume)
			{
				probeTimerTable[checkID].time = 0;
				probeTimerTable[checkID].active = false;
				// It's not an RT, we want to resume TX
#if !(TCP_NODELAY)
				probeTimer2eventEng_setEvent.write(event(TX, checkID));
#else
				probeTimer2eventEng_setEvent.write(event(RT, checkID));
#endif
				fastResume = false;
			}
			else
			{
				probeTimerTable[checkID].time -= 1;
			}
		}
		pt_prevSessionID = checkID;
	}
}
