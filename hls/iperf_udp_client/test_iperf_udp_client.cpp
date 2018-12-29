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
#include "iperf_udp_client.hpp"
#include <iostream>

int main()
{
	hls::stream<ipUdpMeta>	rxMetaData("rxMetaData");
	hls::stream<axiWord>		rxData("rxData");
	hls::stream<ipUdpMeta>	txMetaData("txMetaData");
	hls::stream<axiWord>		txData("txData");
	ap_uint<1> runExperiment;
	//ap_uint<32> myIpAddress = 0x01010101;
	ap_uint<128> targetIpAddress = 0x0A010101;
	targetIpAddress(63, 32) = 0x0A010101;
	targetIpAddress(95, 64) = 0x0A010101;
	targetIpAddress(127, 96) = 0x0A010101;

	ap_uint<16> sessionID;
	axiWord currWord;
	int count = 0;
	ap_uint<8>		packetGap = 1;

	while (count < 10000)
	{
		runExperiment = 0;
		if (count == 20)
		{
			runExperiment = 1;
		}
		iperf_udp_client(	rxMetaData,
							rxData,
							txMetaData,
							txData,
							runExperiment,
							targetIpAddress,
							packetGap);

		ipUdpMeta meta;
		if (!txMetaData.empty())
		{
			txMetaData.read(meta);

			std::cout << "Destination IP" << std::hex << meta.their_address << std::dec << ", port: " << meta.their_port << std::endl;
			std::cout << "Length: " << std::dec << meta.length << std::endl;
		}

		while (!txData.empty())
		{
			txData.read(currWord);
			std::cout << std::hex << std::noshowbase;
			std::cout << std::setfill('0');
			std::cout << std::setw(8) << ((uint32_t) currWord.data(63, 32));
			std::cout << std::setw(8) << ((uint32_t) currWord.data(31, 0));
			std::cout << " " << std::setw(2) << ((uint32_t) currWord.keep) << " ";
			std::cout << std::setw(1) << ((uint32_t) currWord.last) << std::endl;
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
