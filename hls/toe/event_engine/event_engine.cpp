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
#include "event_engine.hpp"

using namespace hls;

/** @ingroup event_engine
 *  Arbitrates between the different event source FIFOs and forwards the event to the \ref tx_engine
 *  @param[in]		txApp2eventEng_setEvent
 *  @param[in]		rxEng2eventEng_setEvent
 *  @param[in]		timer2eventEng_setEvent
 *  @param[out]		eventEng2txEng_event
 */
void event_engine(	stream<event>&				txApp2eventEng_setEvent,
					stream<extendedEvent>&		rxEng2eventEng_setEvent,
					stream<event>&				timer2eventEng_setEvent,
					stream<extendedEvent>&		eventEng2txEng_event,
					stream<ap_uint<1> >&		ackDelayFifoReadCount,
					stream<ap_uint<1> >&		ackDelayFifoWriteCount,
					stream<ap_uint<1> >&		txEngFifoReadCount) {
#pragma HLS PIPELINE II=1

	static ap_uint<1> eventEnginePriority = 0;
	static ap_uint<8> ee_writeCounter = 0;
	static ap_uint<8> ee_adReadCounter = 0; //depends on FIFO depth
	static ap_uint<8> ee_adWriteCounter = 0; //depends on FIFO depth
	static ap_uint<8> ee_txEngReadCounter = 0; //depends on FIFO depth
	extendedEvent ev;

	/*switch (eventEnginePriority)
	{
	case 0:
		if (!txApp2eventEng_setEvent.empty())
		{
			txApp2eventEng_setEvent.read(ev);
			eventEng2txEng_event.write(ev);
		}
		else if (!rxEng2eventEng_setEvent.empty())
		{
			rxEng2eventEng_setEvent.read(ev);
			eventEng2txEng_event.write(ev);
		}
		else if (!timer2eventEng_setEvent.empty())
		{
			timer2eventEng_setEvent.read(ev);
			eventEng2txEng_event.write(ev);
		}
		break;
	case 1:*/
		if (!rxEng2eventEng_setEvent.empty() && !eventEng2txEng_event.full())
		{
			rxEng2eventEng_setEvent.read(ev);
			eventEng2txEng_event.write(ev);
			std::cout<<"rxEng2eventEng_setEvent:"<<ev.type<<std::endl;
			ee_writeCounter++;
		}
		else if (ee_writeCounter == ee_adReadCounter && ee_adWriteCounter == ee_txEngReadCounter)
		{
			// rtTimer and probeTimer events have priority
			if (!timer2eventEng_setEvent.empty())
			{
				timer2eventEng_setEvent.read(ev);
				eventEng2txEng_event.write(ev);
				std::cout<<"timer2eventEng_setEvent:"<<ev.type<<std::endl;
				ee_writeCounter++;
			}
			else if (!txApp2eventEng_setEvent.empty())
			{
				txApp2eventEng_setEvent.read(ev);
				eventEng2txEng_event.write(ev);
				std::cout<<"txApp2eventEng_setEvent:"<<ev.type<<std::endl;
				ee_writeCounter++;
			}
		}
		//break;
	//} //switch
	//eventEnginePriority++;
	if (!ackDelayFifoReadCount.empty())
	{
		ackDelayFifoReadCount.read();
		ee_adReadCounter++;
	}
	if (!ackDelayFifoWriteCount.empty())
	{
		ee_adWriteCounter++;
		ackDelayFifoWriteCount.read();
	}
	if (!txEngFifoReadCount.empty())
	{
		ee_txEngReadCounter++;
		txEngFifoReadCount.read();
	}
}
