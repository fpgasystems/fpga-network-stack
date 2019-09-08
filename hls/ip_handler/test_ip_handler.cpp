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
#include "ip_handler_config.hpp"
#include "ip_handler.hpp"

int main(int argc, char* argv[]) {
	hls::stream<net_axis<64> > inFifo64("inFIFO");
	hls::stream<net_axis<DATA_WIDTH> > inFifo("inFIFO");

	hls::stream<net_axis<64> > outFifoARP64("outFifoARP64");
	hls::stream<net_axis<DATA_WIDTH> > outFifoARP("outFifoARP");

	hls::stream<net_axis<64> > outFifoTCP64("outFifoTCP64");
	hls::stream<net_axis<DATA_WIDTH> > outFifoTCP("outFifoTCP");

	hls::stream<net_axis<64> > outFifoUDP64("outFifoUDP64");
	hls::stream<net_axis<DATA_WIDTH> > outFifoUDP("outFifoUDP");

	hls::stream<net_axis<64> > outFifoROCE64("outFifoROCE64");
	hls::stream<net_axis<DATA_WIDTH> > outFifoROCE("outFifoROCE");

	hls::stream<net_axis<64> > outFifoICMP64("outFifoICMP64");
	hls::stream<net_axis<DATA_WIDTH> > outFifoICMP("outFifoICMP");


	hls::stream<net_axis<64> > outFifoICMPv6_64("outFifoICMPv6_64");
	hls::stream<net_axis<DATA_WIDTH> > outFifoICMPv6("outFifoICMPv6");

	hls::stream<net_axis<64> > outFifoIPv6UDP64("outFifoIPv6UDP64");
	hls::stream<net_axis<DATA_WIDTH> > outFifoIPv6UDP("outFifoIPv6UDP");

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
		ip_handler_top(inFifo, outFifoARP, outFifoICMPv6, outFifoIPv6UDP, outFifoICMP, outFifoUDP, outFifoTCP, outFifoROCE, ipAddress);

		convertStreamWidth<64, 14>(inFifo64, inFifo);

		convertStreamWidth<DATA_WIDTH, 15>(outFifoARP,outFifoARP64);
		convertStreamWidth<DATA_WIDTH, 16>(outFifoICMPv6,outFifoICMPv6_64);
		convertStreamWidth<DATA_WIDTH, 17>(outFifoIPv6UDP,outFifoIPv6UDP64);
		convertStreamWidth<DATA_WIDTH, 18>(outFifoICMP,outFifoICMP64);
		convertStreamWidth<DATA_WIDTH, 19>(outFifoUDP,outFifoUDP64);
		convertStreamWidth<DATA_WIDTH, 20>(outFifoTCP,outFifoTCP64);
		convertStreamWidth<DATA_WIDTH, 21>(outFifoROCE, outFifoROCE64);

	}
	while (count < 30000)	{
		ip_handler_top(inFifo, outFifoARP, outFifoICMPv6, outFifoIPv6UDP, outFifoICMP, outFifoUDP, outFifoTCP, outFifoROCE, ipAddress);

		convertStreamWidth<64, 22>(inFifo64, inFifo);

		convertStreamWidth<DATA_WIDTH, 23>(outFifoARP,outFifoARP64);
		convertStreamWidth<DATA_WIDTH, 24>(outFifoICMPv6,outFifoICMPv6_64);
		convertStreamWidth<DATA_WIDTH, 25>(outFifoIPv6UDP,outFifoIPv6UDP64);
		convertStreamWidth<DATA_WIDTH, 26>(outFifoICMP,outFifoICMP64);
		convertStreamWidth<DATA_WIDTH, 27>(outFifoUDP,outFifoUDP64);
		convertStreamWidth<DATA_WIDTH, 28>(outFifoTCP,outFifoTCP64);
		convertStreamWidth<DATA_WIDTH, 29>(outFifoROCE, outFifoROCE64);


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
			}
	outputFile << "ICMP" << std::endl;
	while (!(outFifoICMP64.empty())) {
		outFifoICMP64.read(outWord);
		print(outputFile, outWord);
		outputFile << std::endl;
			}
	outputFile << "UDP" << std::endl;
	while (!(outFifoUDP64.empty())) {
		outFifoUDP64.read(outWord);
		print(outputFile, outWord);
		outputFile << std::endl;
			}
	outputFile << "TCP" << std::endl;
	while (!(outFifoTCP64.empty())) {
		outFifoTCP64.read(outWord);
		print(outputFile, outWord);
		outputFile << std::endl;
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

