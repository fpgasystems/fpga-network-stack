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
#include "ethernet_frame_padding.hpp"

void ethernet_frame_padding(hls::stream<net_axis<64> >&			dataIn,
							hls::stream<net_axis<64> >&			dataOut)
{
#pragma HLS PIPELINE II=1
#pragma HLS INTERFACE ap_ctrl_none port=return

#pragma HLS INTERFACE axis register port=dataIn name=s_axis
#pragma HLS INTERFACE axis register port=dataOut name=m_axis


	static ap_uint<1> state = 0;
	static ap_uint<8> wordCounter = 0;
	net_axis<64> currWord;
	net_axis<64> sendWord;

	switch(state)
	{
	case 0:
		if (!dataIn.empty())
		{
			dataIn.read(currWord);
			sendWord.data = currWord.data;
			sendWord.keep = currWord.keep;
			sendWord.last = currWord.last;

			wordCounter++;
			if (currWord.last)
			{
				if (wordCounter < 8)
				{
					sendWord.keep = 0xFF;
					sendWord.last = 0;
					state = 1;
				}
				else
				{
					if (wordCounter == 8 && sendWord.keep < 0xF)
					{
						sendWord.keep = 0x0F;
					}
					wordCounter = 0;
				}
			}
			dataOut.write(sendWord);
		}
		break;
	case 1:
		sendWord.data = 0;
		sendWord.keep = 0xFF;
		sendWord.last = 0x0;
		wordCounter++;
		if (wordCounter == 8)
		{
			sendWord.keep = 0x0F;
			sendWord.last = 0x1;
			wordCounter = 0;
			state = 0;
		}
		dataOut.write(sendWord);
		break;
	} //switch
}
