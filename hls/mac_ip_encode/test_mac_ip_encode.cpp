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

#include "mac_ip_encode.hpp"

using namespace hls;

int main()
{
#pragma HLS inline region off
	axiWord inData;
	axiWord outData;
	stream<axiWord> arpFIFO;
	stream<axiWord> icmpFIFO;
	stream<axiWord> udpFIFO;
	stream<axiWord> tcpFIFO;
	stream<axiWord> outFIFO;
	//stream<ap_uint<16> > checksumFIFO;
	stream<ap_uint<32> > arpTableIn;
	stream<arpTableReply> arpTableOut;
	ap_uint<32>					regSubNetMask;
	ap_uint<32>					regDefaultGateway;
	ap_uint<48>					regMacAddress;


	regMacAddress = 0xE59D02350A00;
	regSubNetMask = 0x00FFFFFF;
	regDefaultGateway = 0x01010101;

	//ap_uint<32> ipAddress = 0x0a010101;
	ap_uint<32> requestIpAddress;
	//ap_uint<48> macAddress = 0x699a45dd6000;

	//std::ifstream icmpFile;
	std::ifstream tcpFile;
	std::ofstream outputFile;

	/*icmpFile.open("/home/dsidler/workspace/toe/hls/mac_ip_encode/icmp.in");
	if (!icmpFile)
	{
		std::cout << "Error: could not open icmp input file." << std::endl;
		return -1;
	}*/

	tcpFile.open("/home/dasidler/toe/hls/mac_ip_encode/tcp.in");
	if (!tcpFile)
	{
		std::cout << "Error: could not open tcp input file." << std::endl;
		return -1;
	}

	outputFile.open("/home/dasidler/toe/hls/mac_ip_encode/tcp.out");
	if (!outputFile)
	{
		std::cout << "Error: could not open test output file." << std::endl;
	}

	uint16_t strbTemp;
	uint64_t dataTemp;
	uint16_t lastTemp;
	int count = 0;

	/*while (icmpFile >> std::hex >> dataTemp >> strbTemp >> lastTemp)
	{
		inData.data = dataTemp;
		inData.keep = strbTemp;
		inData.last = lastTemp;
		icmpFIFO.write(inData);
	}*/
	while (tcpFile >> std::hex >> dataTemp >> strbTemp >> lastTemp)
	{
		inData.data = dataTemp;
		inData.keep = strbTemp;
		inData.last = lastTemp;
		tcpFIFO.write(inData);
	}

	int numbLookup = 0;
	while (count < 250)
	{
		mac_ip_encode(tcpFIFO, arpTableOut, outFIFO, arpTableIn, regMacAddress, regSubNetMask, regDefaultGateway);
		if (!arpTableIn.empty())
		{
			// Make sure ARP request goes out
			int countArp = 0;
			while (countArp < 50)
			{
				mac_ip_encode(tcpFIFO, arpTableOut, outFIFO, arpTableIn, regMacAddress, regSubNetMask, regDefaultGateway);
				countArp++;
			}
			arpTableIn.read(requestIpAddress);
			if (requestIpAddress == 0x0a010101)
			{
				//arpTableOut.write(arpTableReply(0x699a45dd6000, (numbLookup != 0)));
				arpTableOut.write(arpTableReply(0x699a45dd6000, true));
			}
			else if (requestIpAddress == 0x01010101)
			{
				arpTableOut.write(arpTableReply(0xab8967452301, true));
			}
			numbLookup++;
		}
		count++;
	}


	while (!(outFIFO.empty()))
	{
		outFIFO.read(outData);
		outputFile << std::hex << std::noshowbase;
		outputFile << std::setfill('0');
		outputFile << std::setw(8) << ((uint32_t) outData.data(63, 32));
		outputFile << std::setw(8) << ((uint32_t) outData.data(31, 0));
		outputFile << " " << std::setw(2) << ((uint32_t) outData.keep) << " ";
		outputFile << std::setw(1) << ((uint32_t) outData.last) << std::endl;
	}

	return 0;
}
