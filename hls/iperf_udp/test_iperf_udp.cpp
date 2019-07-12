/*
 * Copyright (c) 2018, Systems Group, ETH Zurich
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
#include "iperf_udp_config.hpp"
#include "iperf_udp.hpp"
#include <iostream>

int main()
{
	hls::stream<ipUdpMeta>				rxMetaData("rxMetaData");
	hls::stream<net_axis<DATA_WIDTH> >	rxData("rxData");
	hls::stream<ipUdpMeta>				txMetaData("txMetaData");
	hls::stream<net_axis<DATA_WIDTH> >	txData("txData");

	ap_uint<1>	runExperiment;
	ap_uint<32>	targetIpAddress = 0x0A010101;
	ap_uint<8>	pkgWordCount = 10;
	ap_uint<8>	packetGap = 0;

	int count = 0;
	while (count < 100000)
	{
		runExperiment = 0;
		if (count == 20)
		{
			runExperiment = 1;
		}
		iperf_udp(	rxMetaData,
					rxData,
					txMetaData,
					txData,
					runExperiment,
					pkgWordCount,
					packetGap,
					targetIpAddress);

		if (!txMetaData.empty())
		{
			ipUdpMeta meta = txMetaData.read();

			std::cout << "Destination IP" << std::hex << meta.their_address << std::dec << ", port: " << meta.their_port << std::endl;
			std::cout << "Length: " << std::dec << meta.length << std::endl;
		}

		while (!txData.empty())
		{
			net_axis<DATA_WIDTH> currWord = txData.read();
			printLE(std::cout, currWord);
			std::cout << std::endl;
			//Check for last message
			if (currWord.data(7,0) == 0xFF)
			{
				rxMetaData.write(ipUdpMeta(targetIpAddress, 5001, MY_PORT, 0));
			}
		}
		count++;
	}
	return 0;
}
