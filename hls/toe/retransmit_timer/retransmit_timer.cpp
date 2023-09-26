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

#include "retransmit_timer.hpp"

using namespace hls;

/** @ingroup retransmit_timer
 *  The @ref tx_engine sends the Session-ID and Eventy type through the @param txRetransmitTimerFifoIn.
 *  If the timer is unactivated for this session it is activated, the time-out interval is set depending on
 *  how often the session already time-outed.
 *  The @ref rx_engine indicates when a timer for a specific session has to be stopped.
 *	If a timer times-out the corresponding EVent is fired back to the @ref tx_engine. If a session times-out more than 4 times
 *	in a row, it is aborted. The session is released through @param retransmitTimerReleaseFifoOut and the application is
 *	notified through @param timerNotificationFifoOut.
 *	Currently the II is 2 which means that the interval take twice as much time as defined.
 *  @param[in]		rxEng2timer_clearRetransmitTimer
 *  @param[in]		txEng2timer_setRetransmitTimer
 *  @param[out]		rtTimer2eventEng_setEvent
 *  @param[out]		rtTimer2stateTable_releaseState
 *  @param[out]		rtTimer2rxApp_notification
 */
void retransmit_timer(	stream<rxRetransmitTimerUpdate>&	rxEng2timer_clearRetransmitTimer,
						stream<txRetransmitTimerSet>&		txEng2timer_setRetransmitTimer,
						stream<event>&						rtTimer2eventEng_setEvent,
						stream<ap_uint<16> >&				rtTimer2stateTable_releaseState,
						stream<appNotification>&			rtTimer2rxApp_notification,
						stream<openStatus>&					rtTimer2txApp_notification)
{
#pragma HLS PIPELINE II=1
//#pragma HLS INLINE

#pragma HLS aggregate  variable=rxEng2timer_clearRetransmitTimer compact=bit
#pragma HLS aggregate  variable=txEng2timer_setRetransmitTimer compact=bit
#pragma HLS aggregate  variable=rtTimer2eventEng_setEvent compact=bit
#pragma HLS aggregate  variable=rtTimer2stateTable_releaseState compact=bit
#pragma HLS aggregate  variable=rtTimer2rxApp_notification compact=bit

	static retransmitTimerEntry retransmitTimerTable[MAX_SESSIONS];
	#pragma HLS bind_storage variable=retransmitTimerTable type=RAM_T2P impl=BRAM
	#pragma HLS aggregate  variable=retransmitTimerTable compact=bit
	#pragma HLS DEPENDENCE variable=retransmitTimerTable inter false

	static ap_uint<16>			rt_position = 0;
	static bool							rt_waitForWrite = false;
	static ap_uint<16>					rt_prevPosition = 0;
	static rxRetransmitTimerUpdate	rt_update;

	retransmitTimerEntry	currEntry;
	ap_uint<1> operationSwitch = 0;
	txRetransmitTimerSet	set;
	ap_uint<16>				currID;

	if (rt_waitForWrite && rt_update.sessionID != rt_prevPosition) //TODO maybe prevprev too
	{
		if (!rt_update.stop)
		{
			retransmitTimerTable[rt_update.sessionID].time = TIME_1s;
		}
		else
		{
			retransmitTimerTable[rt_update.sessionID].time = 0;
			retransmitTimerTable[rt_update.sessionID].active = false;
		}
		retransmitTimerTable[rt_update.sessionID].retries = 0;
		rt_waitForWrite = false;
	}
	else if (!rxEng2timer_clearRetransmitTimer.empty() && !rt_waitForWrite) //FIXME rx path has priority over tx path
	{
		rxEng2timer_clearRetransmitTimer.read(rt_update);
		rt_waitForWrite = true;
	}
	else
	{
		currID = rt_position;

		if (!txEng2timer_setRetransmitTimer.empty())
		{
			txEng2timer_setRetransmitTimer.read(set);
			currID = set.sessionID;
			operationSwitch = 1;
			if (set.sessionID-3 < rt_position && rt_position <= set.sessionID)
			{
				rt_position += 5;
			}
		}
		else
		{
			// increment position
			rt_position++;
			if (rt_position >= MAX_SESSIONS)
			{
				rt_position = 0;
			}
			operationSwitch = 0;
		}

		// Get entry from table
		currEntry = retransmitTimerTable[currID];
		switch (operationSwitch)
		{
		case 1:
			currEntry.type = set.type;
			if (!currEntry.active)
			{
				switch(currEntry.retries)
				{
				case 0:
					currEntry.time = TIME_1s; //TIME_5s;
					break;
				case 1:
					currEntry.time = TIME_5s; //TIME_7s;
					break;
				case 2:
					currEntry.time = TIME_10s; //TIME_15s;
					break;
				case 3:
					currEntry.time = TIME_15s; //TIME_20s;
					break;
				default:
					currEntry.time = TIME_30s; //TIME_30s;
					break;
				}
			}
			currEntry.active = true;
			break;
		case 0:
			if (currEntry.active)
			{
				if (currEntry.time > 0)
				{
					currEntry.time--;
				}
				// We need to check if we can generate another event, otherwise we might end up in a Deadlock,
				// since the TX Engine will not be able to set new retransmit timers
				else if (!rtTimer2eventEng_setEvent.full())
				{
					currEntry.time = 0;
					currEntry.active = false;
					if (currEntry.retries < 4)
					{
						currEntry.retries++;
						rtTimer2eventEng_setEvent.write(event(currEntry.type, currID, currEntry.retries));
					}
					else
					{
						currEntry.retries = 0;
						rtTimer2stateTable_releaseState.write(currID);
						if (currEntry.type == SYN)
						{
							rtTimer2txApp_notification.write(openStatus(currID, false));
						}
						else
						{
							rtTimer2rxApp_notification.write(appNotification(currID, true)); //TIME_OUT
						}
					}
				}
			} //if active
			break;
		} //switch
		// write entry back
		retransmitTimerTable[currID] = currEntry;
		rt_prevPosition = currID;
	}
}
