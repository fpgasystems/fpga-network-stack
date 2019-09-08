/*
 * Copyright (c) 2018, Systems Group, ETH Zurich
 * Copyright (c) 2016 Xilinx, Inc.
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
#include <arp_server_subnet_config.hpp>
#include "arp_server_subnet.hpp"

int main(int argc, char* argv[])
{
	hls::stream<net_axis<64> >          arpDataIn64("arpDataIn64");
	hls::stream<net_axis<DATA_WIDTH> >  arpDataIn("arpDataIn");

	hls::stream<ap_uint<32> >     macIpEncode_req("macIpEncode_req");
	hls::stream<ap_uint<32> >     hostIpEncode_req("hostIpEncode_req");

	hls::stream<net_axis<64> >          arpDataOut64("arpDataOut64");
	hls::stream<net_axis<DATA_WIDTH> >          arpDataOut("arpDataOut");
	hls::stream<arpTableReply>    macIpEncode_rsp("macIpEncode_rsp");
	hls::stream<arpTableReply>    hostIpEncode_rsp("hostIpEncode_rsp");
	ap_uint<16>			regRequestCount;
	ap_uint<16>			regReplyCount;

	std::ifstream inputFile;
	std::ofstream outputFile;

	static ap_uint<32> ipAddress = 0x01010101;
	static ap_uint<48> macAddress = 0xE59D02350A00;

	std::cout << "Running test for " << DATA_WIDTH << "bit width" << std::endl;

	if (argc != 3)
	{
		std::cout << "Not input and output file specified as argument\n";
		return -1;
	}

	inputFile.open(argv[1]);//"../../../../in.dat");
	//inputFile.open("../../../../queryReply.dat");
	if (!inputFile)
	{
		std::cout << "Error: could not open test input file." << std::endl;
	}

	outputFile.open(argv[2]);//"../../../../out.dat");
	if (!outputFile)
	{
		std::cout << "Error: could not open test output file." << std::endl;
	}


	net_axis<64> inWord;
	uint16_t strbTemp;
	uint64_t dataTemp;
	uint16_t lastTemp;
	while (inputFile >> std::hex >> dataTemp >> strbTemp >> lastTemp)
	{
		inWord.data = dataTemp;
		inWord.keep = strbTemp;
		inWord.last = lastTemp;
		arpDataIn64.write(inWord);
	}

	int count = 0;
	while (count < 250)
	{
		arp_server_subnet<DATA_WIDTH>(	arpDataIn,
							macIpEncode_req,
							hostIpEncode_req,
							arpDataOut,
							macIpEncode_rsp,
							hostIpEncode_rsp,
							macAddress,
							ipAddress,
							regRequestCount,
							regReplyCount);

		if (count == 50) {
			macIpEncode_req.write(0x0a010101);
		}

		convertStreamWidth<64, 941>(arpDataIn64, arpDataIn);
		convertStreamWidth<DATA_WIDTH, 951>(arpDataOut, arpDataOut64);

		count++;
	}

	while (!arpDataOut64.empty())
	{
		net_axis<64> outWord = arpDataOut64.read();
		printLE(outputFile, outWord);
		outputFile << std::endl;
	}

	while (!macIpEncode_rsp.empty())
	{
		arpTableReply reply = macIpEncode_rsp.read();
		std::cout << std::hex << reply.macAddress;
		if (reply.hit)
		{
			std::cout << "\t hit";
		}
		else
		{
			std::cout << "\t miss";
		}
		std::cout << std::endl;
	}
	return 0;
}
