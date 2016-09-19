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

#include "ack_delay.hpp"

using namespace hls;

void ack_delay(	stream<extendedEvent>&	input,
				stream<extendedEvent>&	output,
				stream<ap_uint<1> >&	readCountFifo,
				stream<ap_uint<1> >&	writeCountFifo)
{
#pragma HLS PIPELINE II=1

	static ap_uint<12> ack_table[MAX_SESSIONS]; //TODO why is it 12
	#pragma HLS RESOURCE variable=ack_table core=RAM_2P_BRAM
	#pragma HLS DEPENDENCE variable=ack_table inter false
	static ap_uint<16>	ad_pointer = 0;
	//static ap_uint<4>	ad_readCounter = 0;
	extendedEvent ev;

	if (!input.empty())
	{
		input.read(ev);
		readCountFifo.write(1);
		// Check if there is a delayed ACK
		if (ev.type == ACK && ack_table[ev.sessionID] == 0)
		{
			ack_table[ev.sessionID] = TIME_64us;
		}
		else
		{
			// Assumption no SYN/RST
			ack_table[ev.sessionID] = 0;
			output.write(ev);
			writeCountFifo.write(1);
		}
	}
	else
	{
		if (ack_table[ad_pointer] > 0 && !output.full())
		{
			if (ack_table[ad_pointer] == 1)
			{
				output.write(event(ACK, ad_pointer));
				writeCountFifo.write(1);
			}
			// Decrease value
			ack_table[ad_pointer] -= 1;
		}
		ad_pointer++;
		if (ad_pointer == MAX_SESSIONS)
		{
			ad_pointer = 0;
		}
	}
}
