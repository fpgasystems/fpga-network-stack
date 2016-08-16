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
#include "icmp_server.hpp"

using namespace hls;
using namespace std;

int main() {
#pragma HLS inline region off
	axiWord inData;
	axiWord outData;
	stream<axiWord> 		inFIFO("inFIFO");
	stream<axiWord> 		outFIFO("outFIFO");
	stream<axiWord> 		udpInFIFO("udpInFIFO");
	stream<axiWord> 		ttlInFIFO("ttlInFIFO");
	stream<ap_uint<16> > 	checksumFIFO;
	int 					errCount 					= 0;

	std::ifstream inputFile;
	std::ifstream goldenFile;
	std::ofstream outputFile;

	inputFile.open("../../../../in.dat");

	if (!inputFile)	{
		std::cout << "Error: could not open test input file." << std::endl;
		return -1;
	}
	outputFile.open("../../../../out.dat");
	if (!outputFile) {
		std::cout << "Error: could not open test output file." << std::endl;
		return -1;
	}
	goldenFile.open("../../../../out.gold");
	if (!outputFile) {
		std::cout << "Error: could not open golden output file." << std::endl;
		return -1;
	}
	uint16_t keepTemp;
	uint64_t dataTemp;
	uint16_t lastTemp;
	int count = 0;
	int count2 = 0;
	while (count < 11) {
		inputFile >> std::hex >> dataTemp >> keepTemp >> lastTemp;
		inData.data = dataTemp;
		inData.keep = keepTemp;
		inData.last = lastTemp;
		inFIFO.write(inData);
		count++;
	}
	while (count < 100) {
		icmp_server(inFIFO, udpInFIFO, ttlInFIFO, outFIFO);
		count++;
	}
	while (inputFile >> std::hex >> dataTemp >> keepTemp >> lastTemp) {	
		inData.data = dataTemp;
		inData.keep = keepTemp;
		inData.last = lastTemp;
		ttlInFIFO.write(inData);
		count++;
	}
	while (count < 200) {
		icmp_server(inFIFO, udpInFIFO, ttlInFIFO, outFIFO);
		count++;
	}
	while (!(outFIFO.empty())) {
		outFIFO.read(outData);
		outputFile << std::hex << std::noshowbase;
		outputFile << std::setfill('0');
		outputFile << std::setw(8) << ((uint32_t) outData.data(63, 32));
		outputFile << std::setw(8) << ((uint32_t) outData.data(31, 0));
		outputFile << " " << std::setw(2) << ((uint32_t) outData.keep) << " ";
		outputFile << std::setw(1) << ((uint32_t) outData.last) << std::endl;
		goldenFile >> std::hex >> dataTemp >> keepTemp >> lastTemp;
		// Compare results
		if (outData.data != dataTemp || outData.keep != keepTemp ||
			outData.last != lastTemp) {
			errCount++;
			cerr << "X";
		} else {
			cerr << ".";
		}
	}
	cerr << " done." << endl << endl;
    if (errCount == 0) {
    	cerr << "*** Test Passed ***" << endl << endl;
    	return 0;
    } else {
    	cerr << "!!! TEST FAILED -- " << errCount << " mismatches detected !!!";
    	cerr << endl << endl;
    	return -1;
    }
	//should return comparison btw out.dat and out.gold.dat
	return 0;
}
