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


int main(int argc, char* argv[]) {
	hls::stream<net_axis<64> > inFifo64("inFIFO");
	hls::stream<net_axis<128> > inFifo128("inFIFO");
	hls::stream<net_axis<256> > inFifo256("inFIFO");
	hls::stream<axiWord> inFifo("inFIFO");

	hls::stream<net_axis<64> > outFifoARP64("outFifoARP64");
	hls::stream<net_axis<128> > outFifoARP128("outFifoARP128");
	hls::stream<net_axis<256> > outFifoARP256("outFifoARP256");
	hls::stream<axiWord> outFifoARP("outFifoARP");

	hls::stream<net_axis<64> > outFifoTCP64("outFifoTCP64");
	hls::stream<net_axis<128> > outFifoTCP128("outFifoTCP128");
	hls::stream<net_axis<256> > outFifoTCP256("outFifoTCP256");
	hls::stream<axiWord> outFifoTCP("outFifoTCP");

	hls::stream<net_axis<64> > outFifoUDP64("outFifoUDP64");
	hls::stream<net_axis<128> > outFifoUDP128("outFifoUDP128");
	hls::stream<net_axis<256> > outFifoUDP256("outFifoUDP256");
	hls::stream<axiWord> outFifoUDP("outFifoUDP");

	hls::stream<net_axis<64> > outFifoICMP64("outFifoICMP64");
	hls::stream<net_axis<128> > outFifoICMP128("outFifoICMP128");
	hls::stream<net_axis<256> > outFifoICMP256("outFifoICMP256");
	hls::stream<axiWord> outFifoICMP("outFifoICMP");

	hls::stream<axiWord> outFifoICMPexp("outFifoICMPexp");

	hls::stream<net_axis<64> > outFifoICMPv6_64("outFifoICMPv6_64");
	hls::stream<net_axis<128> > outFifoICMPv6_128("outFifoICMPv6_128");
	hls::stream<net_axis<256> > outFifoICMPv6_256("outFifoICMPv6_256");
	hls::stream<axiWord> outFifoICMPv6("outFifoICMPv6");

	hls::stream<net_axis<64> > outFifoIPv6UDP64("outFifoIPv6UDP64");
	hls::stream<net_axis<128> > outFifoIPv6UDP128("outFifoIPv6UDP128");
	hls::stream<net_axis<256> > outFifoIPv6UDP256("outFifoIPv6UDP256");
	hls::stream<axiWord> outFifoIPv6UDP("outFifoIPv6UDP");

	std::ifstream inputFile;
	std::ifstream goldenFile;
	std::ofstream outputFile;

	ap_uint<32> ipAddress 	= 0x0a010101;
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

	net_axis<64> inWord;
	int count = 0;
	while (scanLE(inputFile, inWord)) {
		inFifo64.write(inWord);
		ip_handler(inFifo, outFifoARP, outFifoICMPv6, outFifoIPv6UDP, outFifoICMP, outFifoUDP, outFifoTCP, ipAddress);

#if (AXI_WIDTH == 512)
		convertStreamToDoubleWidth(inFifo64, inFifo128);
		convertStreamToDoubleWidth(inFifo128, inFifo256);
		convertStreamToDoubleWidth(inFifo256, inFifo);
		convertStreamToHalfWidth<512, 951>(outFifoTCP, outFifoTCP256);
		convertStreamToHalfWidth<256, 952>(outFifoTCP256, outFifoTCP128);
		convertStreamToHalfWidth<128, 953>(outFifoTCP128, outFifoTCP64);
#else
		if (!inFifo64.empty()) {
			inFifo.write(inFifo64.read());
		}
		if (!outFifoTCP.empty()) {
			outFifoTCP64.write(outFifoTCP.read());
		}
#endif

	}
	while (count < 30000)	{
		ip_handler(inFifo, outFifoARP, outFifoICMPv6, outFifoIPv6UDP, outFifoICMP, outFifoUDP, outFifoTCP, ipAddress);

#if (AXI_WIDTH == 512)
		convertStreamToDoubleWidth(inFifo64, inFifo128);
		convertStreamToDoubleWidth(inFifo128, inFifo256);
		convertStreamToDoubleWidth(inFifo256, inFifo);
		convertStreamToHalfWidth<512, 951>(outFifoTCP, outFifoTCP256);
		convertStreamToHalfWidth<256, 952>(outFifoTCP256, outFifoTCP128);
		convertStreamToHalfWidth<128, 953>(outFifoTCP128, outFifoTCP64);
#else
		if (!inFifo64.empty()) {
			inFifo.write(inFifo64.read());
		}
		if (!outFifoTCP.empty()) {
			outFifoTCP64.write(outFifoTCP.read());
		}
#endif

		count++;
	}

	net_axis<64> outWord;
	outputFile << "ICMPv6" << std::endl;
	while (!outFifoICMPv6_64.empty())
	{
		outFifoICMPv6_64.read(outWord);
		print(outputFile, outWord);
		outputFile << std::endl;
	}

	outputFile << "IPv6 UDP" << std::endl;
	while (!outFifoIPv6UDP64.empty())
	{
		outFifoIPv6UDP64.read(outWord);
		print(outputFile, outWord);
		outputFile << std::endl;
	}

	outputFile << "ARP" << std::endl;
	while (!(outFifoARP64.empty())) {
		outFifoARP64.read(outWord);
		print(outputFile, outWord);
		outputFile << std::endl;
		/*outputFile << std::setw(8) << ((uint32_t) outData.data(63, 32));
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
		}*/
	}
	outputFile << "ICMP" << std::endl;
	while (!(outFifoICMP64.empty())) {
		outFifoICMP64.read(outWord);
		print(outputFile, outWord);
		outputFile << std::endl;
		/*outputFile << std::setw(8) << ((uint32_t) outData.data(63, 32));
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
		}*/
	}
	outputFile << "UDP" << std::endl;
	while (!(outFifoUDP64.empty())) {
		outFifoUDP64.read(outWord);
		print(outputFile, outWord);
		outputFile << std::endl;
		/*outputFile << std::setw(8) << ((uint32_t) outData.data(63, 32));
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
		}*/
	}
	outputFile << "TCP" << std::endl;
	while (!(outFifoTCP64.empty())) {
		outFifoTCP64.read(outWord);
		print(outputFile, outWord);
		outputFile << std::endl;
		/*outputFile << std::setw(8) << ((uint32_t) outData.data(63, 32));
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
		}*/
	}
	/*cerr << " done." << endl << endl;
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

