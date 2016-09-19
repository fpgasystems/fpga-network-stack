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

#include "arp_server_subnet.hpp"
//#include <iostream>


int main()
{
	stream<axiWord> 			inFIFO("inFIFO");
	stream<axiWord> 			outFIFO("outFIFO");
	stream<ap_uint<32> > 		ipFIFO("ipFIFO");
	stream<arpTableReply> 		macFIFO("macFIFO");

	axiWord inData;
	axiWord outData;
	arpTableReply reply;

	std::ifstream inputFile;
	std::ofstream outputFile;

	static ap_uint<32> ipAddress = 0x01010101;
	static ap_uint<48> macAddress = 0xE59D02350A00;

	//inputFile.open("/home/dsidler/workspace/toe/hls/arp_server/strange_arp.in");
	//inputFile.open("/home/dasidler/toe/hls/arp_server_subnet/in.dat");
	inputFile.open("/home/dasidler/toe/hls/arp_server_subnet/queryReply.dat");
	if (!inputFile)
	{
		std::cout << "Error: could not open test input file." << std::endl;
	}

	outputFile.open("/home/dasidler/toe/hls/arp_server_subnet/out.dat");
	if (!outputFile)
	{
		std::cout << "Error: could not open test output file." << std::endl;
	}


	uint16_t strbTemp;
	uint64_t dataTemp;
	uint16_t lastTemp;
	while (inputFile >> std::hex >> dataTemp >> strbTemp >> lastTemp)
	{
		inData.data = dataTemp;
		inData.keep = strbTemp;
		inData.last = lastTemp;
		inFIFO.write(inData);
	}

	int count = 0;
	while (count < 250)
	{
		arp_server_subnet(inFIFO, ipFIFO, outFIFO, macFIFO, macAddress, ipAddress);

		if (count == 50)
			ipFIFO.write(0x0a010101);
		count++;
	}

	while (!(outFIFO.empty()))
	{
		outFIFO.read(outData);
		outputFile << std::hex;
		outputFile << std::setfill('0');
		outputFile << std::setw(16) << outData.data << " " << std::setw(2) << outData.keep << " ";
		outputFile << std::setw(1) << outData.last << std::endl;
		//outputFile << "TUSER: " << outData.user.range(31,24) << std::endl;
	}

	while (!macFIFO.empty())
	{
		macFIFO.read(reply);
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
