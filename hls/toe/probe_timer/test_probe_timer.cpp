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
#include <iostream>


using namespace hls;

int main()
{

	static	stream<ap_uint<16> >		clearTimerFifo;
	static	stream<ap_uint<16> >		setTimerFifo;
	static stream<event>				eventFifo;

	event ev;

	int count = 0;

	//for (int i=0; i < 10; i++)
	//{
		setTimerFifo.write(7);
	//}

	while (count < 50000)
	{
		/*if (count < 100)
		{
			setTimerFifo.write(count);
			std::cout << "set Timer for: " << count << std::endl;
		}*/
		if (count == 9 || count == 12)
		{
			//for (int i=0; i < 10; i++)
			//{
				setTimerFifo.write(7); //try 33
			//}
		}
		if (count == 21)
		{
			clearTimerFifo.write(22);
			setTimerFifo.write(22);
		}
		probe_timer(clearTimerFifo, setTimerFifo, eventFifo);
		if (!eventFifo.empty())
		{
			eventFifo.read(ev);
			std::cout << "ev happened, ID: " << ev.sessionID;// << std::endl;
			std::cout << "\t\tcount: " << count << std::endl;
		}
		count++;
	}

	return 0;
}
