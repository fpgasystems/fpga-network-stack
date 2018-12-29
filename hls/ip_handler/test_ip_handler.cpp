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
#include "ip_handler.hpp"


using namespace hls;
using namespace std;

int main(int argc, char* argv[]) {
	stream<axiWord> inFIFO("inFIFO");
	stream<axiWord> outFifoARP("outFifoARP");
	stream<axiWord> outFifoTCP("outFifoTCP");
	stream<axiWord> outFifoUDP("outFifoUDP");
	stream<axiWord> outFifoICMP("outFifoICMP");
	stream<axiWord> outFifoICMPexp("outFifoICMPexp");
	stream<axiWord> outFifoICMPv6("outFifoICMPv6");
	stream<axiWord> outFifoIPv6UDP("outFifoIPv6UDP");

	std::ifstream inputFile;
	std::ifstream goldenFile;
	std::ofstream outputFile;

	ap_uint<32> ipAddress 	= 0x01010101;
    int 		errCount 	= 0;

	/*inputFile.open("../../../../in.dat");
	if (!inputFile) {
		cout << "Error: could not open test input file." << endl;
		return -1;
	}
	outputFile.open("../../../../out.dat");
	if (!outputFile) {
		cout << "Error: could not open test output file." << endl;
		return -1;
	}
	goldenFile.open("../../../../out.gold");
	if (!goldenFile) {
		cerr << " Error opening golden output file!" << endl;
		return -1;
	}*/
	if (argc < 3)
	{
		std::cout << "[ERROR] missing arguments." << std::endl;
		return -1;
	}

	inputFile.open(argv[1]);
	if (!inputFile)
	{
		std::cout << "[ERROR] could not open test rx input file." << std::endl;
		return -1;
	}

	outputFile.open(argv[2]);
	if (!outputFile)
	{
		std::cout << "[ERROR] could not open test rx output file." << std::endl;
		return -1;
	}

	axiWord inWord;
	int count = 0;
	while (scan(inputFile, inWord)) {
		inFIFO.write(inWord);
		ip_handler(inFIFO, outFifoARP, outFifoICMPv6, outFifoIPv6UDP, outFifoICMP, outFifoUDP, outFifoTCP, ipAddress);
	}
	while (count < 30000)	{
		ip_handler(inFIFO, outFifoARP, outFifoICMPv6, outFifoIPv6UDP, outFifoICMP, outFifoUDP, outFifoTCP, ipAddress);
		count++;
	}

	axiWord outWord;
	while (!outFifoICMPv6.empty())
	{
		outFifoICMPv6.read(outWord);
		print(outputFile, outWord);
		outputFile << std::endl;
		/*outputFile << std::setw(8) << ((uint32_t) outWord.data(63, 32));
		outputFile << std::setw(8) << ((uint32_t) outWord.data(31, 0));
		outputFile << " " << std::setw(2) << ((uint32_t) outWord.keep) << " ";
		outputFile << std::setw(1) << ((uint32_t) outWord.last) << std::endl;*/
	}

	/*outputFile << std::hex << std::noshowbase;
	outputFile << std::setfill('0');
	while (!(outFifoARP.empty())) {
		outFifoARP.read(outData);
		outputFile << std::setw(8) << ((uint32_t) outData.data(63, 32));
		outputFile << std::setw(8) << ((uint32_t) outData.data(31, 0));
		outputFile << " " << std::setw(2) << ((uint32_t) outData.keep) << " ";
		outputFile << std::setw(1) << ((uint32_t) outData.last) << std::endl;
		goldenFile >> std::hex >> dataTemp >> keepTemp >> lastTemp;
		if (outData.data != dataTemp || outData.keep != keepTemp ||
			outData.last != lastTemp) {
			errCount++;
			cerr << "X";
		} else {
			cerr << ".";
		}
	}
	while (!(outFifoICMP.empty())) {
		outFifoICMP.read(outData);
		outputFile << std::setw(8) << ((uint32_t) outData.data(63, 32));
		outputFile << std::setw(8) << ((uint32_t) outData.data(31, 0));
		outputFile << " " << std::setw(2) << ((uint32_t) outData.keep) << " ";
		outputFile << std::setw(1) << ((uint32_t) outData.last) << std::endl;
		goldenFile >> std::hex >> dataTemp >> keepTemp >> lastTemp;
		if (outData.data != dataTemp || outData.keep != keepTemp ||
			outData.last != lastTemp) {
			errCount++;
			cerr << "X";
		} else {
			cerr << ".";
		}
	}
	while (!(outFifoUDP.empty())) {
		outFifoUDP.read(outData);
		outputFile << std::setw(8) << ((uint32_t) outData.data(63, 32));
		outputFile << std::setw(8) << ((uint32_t) outData.data(31, 0));
		outputFile << " " << std::setw(2) << ((uint32_t) outData.keep) << " ";
		outputFile << std::setw(1) << ((uint32_t) outData.last) << std::endl;
		goldenFile >> std::hex >> dataTemp >> keepTemp >> lastTemp;
		if (outData.data != dataTemp || outData.keep != keepTemp ||
			outData.last != lastTemp) {
			errCount++;
			cerr << "X";
		} else {
			cerr << ".";
		}
	}
	while (!(outFifoTCP.empty())) {
		outFifoTCP.read(outData);
		outputFile << std::setw(8) << ((uint32_t) outData.data(63, 32));
		outputFile << std::setw(8) << ((uint32_t) outData.data(31, 0));
		outputFile << " " << std::setw(2) << ((uint32_t) outData.keep) << " ";
		outputFile << std::setw(1) << ((uint32_t) outData.last) << std::endl;
		goldenFile >> std::hex >> dataTemp >> keepTemp >> lastTemp;
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
	inputFile.close();
	outputFile.close();
	goldenFile.close();*/

	return 0;
}

