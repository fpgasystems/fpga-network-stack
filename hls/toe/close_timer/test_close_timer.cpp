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
#include "close_timer.hpp"

using namespace hls;

int main()
{
#pragma HLS inline region off
	//axiWord inData;
	ap_uint<16> outData;
	stream<ap_uint<16> > timeWaitFifo;
	stream<ap_uint<16> > sessionReleaseFifo;

	//std::ifstream inputFile;
	std::ofstream outputFile;

	/*inputFile.open("/home/dsidler/workspace/toe/retransmit_timer/in.dat");

	if (!inputFile)
	{
		std::cout << "Error: could not open test input file." << std::endl;
		return -1;
	}*/
	outputFile.open("/home/dasidler/toe/hls/toe/close_timer/out.dat");
	if (!outputFile)
	{
		std::cout << "Error: could not open test output file." << std::endl;
	}

	uint32_t count = 0;
	uint32_t setCount = 0;
	timeWaitFifo.write(1);
	timeWaitFifo.write(2);
	while (count < 2147483647)
	{

		if ((count % 1000000000) == 0)
		{
			outputFile << "set new timer, count: " << count << std::endl;
			timeWaitFifo.write(1);
			setCount = count;
		}

		close_timer(timeWaitFifo, sessionReleaseFifo);
		while(!sessionReleaseFifo.empty())
		{
			double dbcount = count - setCount;
			sessionReleaseFifo.read(outData);
			outputFile << "Event fired at count: " << count;
			outputFile << " ID: " << outData;// << std::endl;
			outputFile << " Time[s]: " << ((dbcount * 6.66) /1000000000) << std::endl;
		}
		count++;
	}


	//should return comparison

	return 0;
}
