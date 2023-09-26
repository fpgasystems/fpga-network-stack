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
#include "ethernet_frame_padding_512.hpp"

void ethernet_frame_padding_512(	hls::stream<ap_axiu<512, 0, 0, 0> >&			dataIn,
				hls::stream<ap_axiu<512, 0, 0, 0> >&			dataOut)
{
#pragma HLS PIPELINE II=1
#pragma HLS INTERFACE ap_ctrl_none port=return

#pragma HLS INTERFACE axis register port=dataIn name=s_axis
#pragma HLS INTERFACE axis register port=dataOut name=m_axis

   /*static ap_uint<1> firstWord = 1;


   if (!dataIn.empty())
   {
      ap_axiu<512, 0, 0, 0> currWord = dataIn.read();

      for (int i = 0; i < 64; i++)
      {
         #pragma HLS UNROLL
         if (currWord.keep[0] == 0)
         {
            currWord.keep[i] = firstWord;
            currWord.data(i*8+7, i*8) = 0;
         }
      }

      dataOut.write(currWord);
      firstWord = 0;
      if (currWord.last)
      {
         firstWord = 1;
      }
   }*/


	static ap_uint<1> state = 0;

	switch(state)
	{
	case 0:
		if (!dataIn.empty())
		{
			ap_axiu<512, 0, 0, 0> currWord = dataIn.read();
         if (currWord.last)
         {
            for (int i = 0; i < 64; ++i)
            {
               #pragma HLS UNROLL
               if (currWord.keep[i] == 0)
               {
                  currWord.keep[i] = 1;
                  currWord.data(i*8+7, i*8) = 0;
               }
            }
         }
         else
         {
            state = 1;
         }
			dataOut.write(currWord);
		}
		break;
	case 1:
      if (!dataIn.empty())
      {
         ap_axiu<512, 0, 0, 0> currWord = dataIn.read();
         dataOut.write(currWord);

         if (currWord.last)
         {
            state = 0;
         }
      }
		break;
	} //switch
}
