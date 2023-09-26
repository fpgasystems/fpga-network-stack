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
#include "transport_timer.hpp"
#include <rocev2_config.hpp> //defines MAX_QPS

/*
 * page 352
 * One timer per QP
 * Timer gets started when a WRITE (with AckReqBit) or a READ_REQ is send
 * Timer gets reset everytime a valid ACK is received
 * Timer is stopped when there are no more outstanding requests
 * Maintain a 3-bit retry counter, decrement on timeout, clear on new ACK
 */
void transport_timer(	stream<rxTimerUpdate>&	rxClearTimer_req,
						stream<ap_uint<24> >&	txSetTimer_req,
						stream<retransmission>&	timer2retrans_req)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	static transportTimerEntry transportTimerTable[MAX_QPS];
	#pragma HLS bind_storage variable=transportTimerTable type=RAM_T2P impl=BRAM
	#pragma HLS aggregate  variable=transportTimerTable compact=bit
	#pragma HLS DEPENDENCE variable=transportTimerTable inter false

	static ap_uint<16>			tt_currPosition = 0;
	static ap_uint<16> 			tt_prevPosition = 0;
	static bool tt_WaitForWrite = false;


	ap_uint<16> checkQP;
	static rxTimerUpdate tt_update;
	ap_uint<24> setQP;
	transportTimerEntry entry;
	ap_uint<1> operationSwitch = 0;


	if (tt_WaitForWrite)
	{
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
		tt_WaitForWrite = false;
	}
	else if (!rxClearTimer_req.empty())
	{
		rxClearTimer_req.read(tt_update);
		tt_WaitForWrite = true;
	}
	else
	{
		checkQP = tt_currPosition;
		if (!txSetTimer_req.empty())
		{
			txSetTimer_req.read(setQP);
			checkQP = setQP;
			operationSwitch = 1;
			if ((setQP - 3 < tt_currPosition) && (tt_currPosition <= setQP))
			{
				tt_currPosition += 5;
			}
		}
		else
		{
			tt_currPosition++;
			if (tt_currPosition >= MAX_QPS)
			{
				tt_currPosition = 0;
			}
			operationSwitch = 0;
		}

		//Get entry from table
		entry = transportTimerTable[checkQP];

		switch (operationSwitch)
		{
		case 1:
			if (!entry.active)
			{
				switch (entry.retries)
				{
				case 0:
					entry.time = TIME_1ms;
					break;
				case 1:
					entry.time = TIME_5ms;
					break;
				case 2:
					entry.time = TIME_10ms;
					break;
				default:
					entry.time = TIME_10ms;
					break;
				}
			}
			entry.active = true;
			break;
		case 0:
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

					if (entry.retries < 4)
					{
						entry.retries++;
						//TODO shut QP down if too many retries
						timer2retrans_req.write(retransmission(checkQP));
					}
				}
			}//if active
			break;
		}//switch
		//write entry back
		transportTimerTable[checkQP] = entry;
		tt_prevPosition = checkQP;
	}
}
