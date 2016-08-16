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
using namespace std;

int main()
{
#pragma HLS inline region off
	axiWord inData;
	axiWord outData;
	stream<axiWord> inFIFO;
	stream<axiWord> outFIFO;
	//stream<ap_uint<16> > checksumFIFO;
	stream<ap_uint<32> > arpTableIn;
	stream<arpTableReply> arpTableOut;
	ap_uint<32>		regSubNetMask		= 0x00FFFFFF;
	ap_uint<32>		regDefaultGateway	= 0x01010101;
	int				errCount 			= 0;
	unsigned int	idleCounter			= 0;

	//ap_uint<32> ipAddress = 0x0a010101;
	ap_uint<32> requestIpAddress;
	//ap_uint<48> macAddress = 0x699a45dd6000;

	ifstream inputFile;
	ifstream goldenFile;
	ofstream outputFile;

	inputFile.open("../../../../in.dat");
	if (!inputFile)	{
		cout << "Error: could not open input vector file." << endl;
		return -1;
	}
	outputFile.open("../../../../out.dat");
	if (!outputFile) {
		cout << "Error: could not open output vector file." << endl;
	}
	goldenFile.open("../../../../out.gold");
	if (!goldenFile) {
		cout << "Error: could not open golden output vector file." << endl;
	}

	uint16_t keepTemp;
	uint64_t dataTemp;
	uint16_t lastTemp;
	int count = 0;

	while (inputFile >> hex >> dataTemp >> keepTemp >> lastTemp) {
		inData.data = dataTemp;
		inData.keep = keepTemp;
		inData.last = lastTemp;
		inFIFO.write(inData);
	}

	int numbLookup = 0;
	while (count < 250) {
		if (idleCounter == 0) {
			mac_ip_encode(inFIFO, arpTableOut, outFIFO, arpTableIn, regSubNetMask, regDefaultGateway, 0xE59D02350A00);
			if (!arpTableIn.empty()) {
				idleCounter = 10;
				arpTableIn.read(requestIpAddress);
				if (requestIpAddress == 0x0a010101) {
					arpTableOut.write(arpTableReply(0x699a45dd6000, true));
				}
				else if (requestIpAddress == 0x01010101)
					arpTableOut.write(arpTableReply(0xab8967452301, true));
				numbLookup++;
			}
		}
		else
			idleCounter--;
		count++;
	}

	while (!outFIFO.empty()) {
		outFIFO.read(outData);
		outputFile << std::hex << std::noshowbase;
		outputFile << std::setfill('0');
		outputFile << std::setw(8) << ((uint32_t) outData.data(63, 32));
		outputFile << std::setw(8) << ((uint32_t) outData.data(31, 0));
		outputFile << " " << std::setw(2) << ((uint32_t) outData.keep) << " ";
		outputFile << std::setw(1) << ((uint32_t) outData.last) << std::endl;
		goldenFile >> std::hex >> dataTemp >> keepTemp >> lastTemp;

		if (outData.data != dataTemp || outData.keep != keepTemp || outData.last != lastTemp) { // Compare results
			errCount++;
			cerr << "X";
		} else {
			cerr << ".";
		}
	}
	inputFile.close();
	outputFile.close();
	goldenFile.close();
	cerr << " done." << endl << endl;
	if (errCount == 0) {
	    cerr << "*** Test Passed ***" << endl << endl;
	    return 0;
	} else {
	   	cerr << "!!! TEST FAILED -- " << errCount << " mismatches detected !!!";
	   	cerr << endl << endl;
	   	return -1;
	}
	return 0;
}
