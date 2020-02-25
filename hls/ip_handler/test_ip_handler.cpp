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
#include <algorithm>


void writeToOutputFile(std::ofstream& outputFile, std::string name, hls::stream<net_axis<64> >& outFifo)
{
		net_axis<64> outWord;
		if (!outFifo.empty())
		{
			outputFile << name << std::endl;
		}
		while (!outFifo.empty())
		{
			outFifo.read(outWord);
			print(outputFile, outWord);
			outputFile << std::endl;
		}
}

bool compareFiles(const std::string& filename1, const std::string& filename2)
{
    std::ifstream file1(filename1, std::ifstream::ate | std::ifstream::binary); //open file at the end
    std::ifstream file2(filename2, std::ifstream::ate | std::ifstream::binary); //open file at the end
    const std::ifstream::pos_type fileSize = file1.tellg();

    if (fileSize != file2.tellg()) {
        return false; //different file size
    }

    file1.seekg(0); //rewind
    file2.seekg(0); //rewind

    std::istreambuf_iterator<char> begin1(file1);
    std::istreambuf_iterator<char> begin2(file2);

    return std::equal(begin1,std::istreambuf_iterator<char>(),begin2); //Second argument is end-of-range iterator
}

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


	ap_uint<32> ipAddress 	= 0x0a010101;

	net_axis<64> inWord;
	int count = 0;
	size_t numFiles = 2;
	std::string inputFiles[numFiles] = {"tcp", "udp"};
	for (int i = 0; i < numFiles; i++)
	{
		std::ifstream inputFile;
		std::ofstream outputFile;

		inputFile.open(currentDirectory+"/"+inputFiles[i]+".in");
		if (!inputFile)
		{
			std::cout << "[ERROR] could not open test input file: " << inputFiles[i] << std::endl;
			return -1;
		}
		outputFile.open(currentDirectory+"/"+inputFiles[i]+".out");
		if (!outputFile)
		{
			std::cout << "[ERROR] could not open test rx output file." << std::endl;
			return -1;
		}

		std::cout << "Using input file: " << inputFiles[i] << ".in" << std::endl;

		while (scanLE(inputFile, inWord))
		{
			inFifo64.write(inWord);
			std::cout << "INWORD: ";
			printLE(std::cout, inWord);
			std::cout << std::endl;
			ip_handler<DATA_WIDTH>(inFifo, outFifoARP, outFifoICMPv6, outFifoIPv6UDP, outFifoICMP, outFifoUDP, outFifoTCP, outFifoROCE, ipAddress);

			convertStreamWidth<64, 14>(inFifo64, inFifo);

			convertStreamWidth<DATA_WIDTH, 15>(outFifoARP,outFifoARP64);
			convertStreamWidth<DATA_WIDTH, 16>(outFifoICMPv6,outFifoICMPv6_64);
			convertStreamWidth<DATA_WIDTH, 17>(outFifoIPv6UDP,outFifoIPv6UDP64);
			convertStreamWidth<DATA_WIDTH, 18>(outFifoICMP,outFifoICMP64);
			convertStreamWidth<DATA_WIDTH, 19>(outFifoUDP,outFifoUDP64);
			convertStreamWidth<DATA_WIDTH, 20>(outFifoTCP,outFifoTCP64);
			convertStreamWidth<DATA_WIDTH, 21>(outFifoROCE, outFifoROCE64);

		}
		count = 0;
		while (count < 30000)	{
			ip_handler<DATA_WIDTH>(inFifo, outFifoARP, outFifoICMPv6, outFifoIPv6UDP, outFifoICMP, outFifoUDP, outFifoTCP, outFifoROCE, ipAddress);

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

		writeToOutputFile(outputFile, "ICMPv6", outFifoICMPv6_64);
		writeToOutputFile(outputFile, "IPv6 UDP", outFifoIPv6UDP64);
		writeToOutputFile(outputFile, "ARP", outFifoARP64);
		writeToOutputFile(outputFile, "ICMP", outFifoICMP64);
		writeToOutputFile(outputFile, "UDP", outFifoUDP64);
		writeToOutputFile(outputFile, "TCP", outFifoTCP64);
		writeToOutputFile(outputFile, "ICMPv6", outFifoICMPv6_64);

		inputFile.close();
		outputFile.close();
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
	outputFile.close();
	goldenFile.close();*/

	//Compare output files with golden files
	int errorCount = 0;
	for (int i = 0; i < numFiles; i++)
	{
		/*std::ifstream outputFile;
		std::ifstream goldenFile;
		outputFile.open(currentDirectory+"/"+inputFiles[i]+".out");*/
		std::string outputFileName = currentDirectory+"/"+inputFiles[i]+".out";
		std::string goldenFileName = currentDirectory+"/"+inputFiles[i]+".gold";

		/*if (!outputFile)
		{
			std::cout << "[ERROR] could not open test input file: " << inputFiles[i] << std::endl;
			return -1;
		}
		goldenFile.open(currentDirectory+"/"+inputFiles[i]+".gold");
		if (!outputFile)
		{
			std::cout << "[ERROR] could not open test rx output file." << std::endl;
			return -1;
		}*/

		bool equal = compareFiles(outputFileName, goldenFileName);
		if (!equal)
		{
			std::cout << "[ERROR] files do not match: " << inputFiles[i] << std::endl;
			errorCount++;
		}

		//outputFile.close();
		//goldenFile.close();
	}

	return errorCount;
}

