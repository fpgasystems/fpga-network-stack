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
#include <iostream>

using namespace hls;

int main()
{
	stream<event>				txApp2eventEng_setEvent;
	stream<extendedEvent>		rxEng2eventEng_setEvent;
	stream<event>				timer2eventEng_setEvent;
	stream<extendedEvent>		eventEng2txEng_event;

	extendedEvent ev;
	int count=0;
	while (count < 50)
	{
		event_engine(	txApp2eventEng_setEvent,
						rxEng2eventEng_setEvent,
						timer2eventEng_setEvent,
						eventEng2txEng_event);

		if (count == 20)
		{
			fourTuple tuple;
			tuple.srcIp = 0x0101010a;
			tuple.srcPort = 12;
			tuple.dstIp = 0x0101010b;
			tuple.dstPort = 788789;
			txApp2eventEng_setEvent.write(event(TX, 23));
			rxEng2eventEng_setEvent.write(extendedEvent(rstEvent(0x8293479023), tuple));
			timer2eventEng_setEvent.write(event(RT, 22));
		}
		if (!eventEng2txEng_event.empty())
		{
			eventEng2txEng_event.read(ev);
			std::cout << ev.type << std::endl;
		}
		count++;
	}
	return 0;
}
