/*
 * Copyright (c) 2019, Systems Group, ETH Zurich
 * Copyright (c) 2016, Xilinx, Inc.
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
#include "mac_ip_encode_config.hpp"
#include "mac_ip_encode.hpp"

int main(int argc, char* argv[])
{
	net_axis<64> inData;
	net_axis<64> outData;
	hls::stream<net_axis<DATA_WIDTH> >		arpFIFO;
	hls::stream<net_axis<64> >	icmpFifo64("icmpFifo64");
	hls::stream<net_axis<DATA_WIDTH> >		icmpFifo("icmpFifo");
	hls::stream<net_axis<DATA_WIDTH> > 		udpFIFO;
	hls::stream<net_axis<64> >	tcpFifo64("tcpFifo64");
	hls::stream<net_axis<DATA_WIDTH> >			tcpFifo("tcpFifo");

	hls::stream<net_axis<64> >	outFifo64("outFifo");
	hls::stream<net_axis<DATA_WIDTH> >			outFifo("outFifo");
	//hls::stream<ap_uint<16> > checksumFIFO;
	hls::stream<ap_uint<32> >	arpTableIn;
	hls::stream<arpTableReply>	arpTableOut;
	ap_uint<32>					regSubNetMask;
	ap_uint<32>					regDefaultGateway;
	ap_uint<48>					regMacAddress;


	regMacAddress = 0xE59D02350A00;
	regSubNetMask = 0x00FFFFFF;
	regDefaultGateway = 0x01010101;

	//ap_uint<32> ipAddress = 0x0a010101;
	ap_uint<32> requestIpAddress;
	//ap_uint<48> macAddress = 0x699a45dd6000;

	std::ifstream icmpFile;
	std::ifstream tcpFile;
	std::ofstream outputFile;

	/*icmpFile.open("../../../../icmp.in");
	if (!icmpFile)
	{
		std::cout << "Error: could not open icmp input file." << std::endl;
		return -1;
	}*/

	if (argc < 3)
	{
		std::cerr << "[ERRO] missing arguments." << std::endl;
		return -1;
	}

	//tcpFile.open("../../../../in.dat");
	tcpFile.open(argv[1]);
	if (!tcpFile)
	{
		std::cout << "Error: could not open tcp input file." << std::endl;
		return -1;
	}

	//outputFile.open("../../../../tcp.out");
	outputFile.open(argv[2]);
	if (!outputFile)
	{
		std::cout << "Error: could not open test output file." << std::endl;
	}

	uint16_t strbTemp;
	uint64_t dataTemp;
	uint16_t lastTemp;
	int count = 0;

	while (icmpFile >> std::hex >> dataTemp >> strbTemp >> lastTemp)
	{
		inData.data = dataTemp;
		inData.keep = strbTemp;
		inData.last = lastTemp;
		icmpFifo64.write(inData);
	}
	while (tcpFile >> std::hex >> dataTemp >> strbTemp >> lastTemp)
	{
		inData.data = dataTemp;
		inData.keep = strbTemp;
		inData.last = lastTemp;
		tcpFifo64.write(inData);
	}

	int numbLookup = 0;
	while (count < 250)
	{

		mac_ip_encode(tcpFifo, arpTableOut, outFifo, arpTableIn, regMacAddress, regSubNetMask, regDefaultGateway);
		if (!arpTableIn.empty())
		{
			// Make sure ARP request goes out
			int countArp = 0;
			while (countArp < 50)
			{
				mac_ip_encode(tcpFifo, arpTableOut, outFifo, arpTableIn, regMacAddress, regSubNetMask, regDefaultGateway);
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

		convertStreamWidth<64, 23>(tcpFifo64, tcpFifo);
		convertStreamWidth<64, 24>(icmpFifo64, icmpFifo);
		convertStreamWidth<DATA_WIDTH, 25>(outFifo, outFifo64);
		count++;
	}


	while (!(outFifo64.empty()))
	{
		outFifo64.read(outData);
		outputFile << std::hex << std::noshowbase;
		outputFile << std::setfill('0');
		outputFile << std::setw(8) << ((uint32_t) outData.data(63, 32));
		outputFile << std::setw(8) << ((uint32_t) outData.data(31, 0));
		outputFile << " " << std::setw(2) << ((uint32_t) outData.keep) << " ";
		outputFile << std::setw(1) << ((uint32_t) outData.last) << std::endl;
	}

	return 0;
}
