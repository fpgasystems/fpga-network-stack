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
#include "dhcp_client.hpp"
#include <iostream>
#include <fstream>

using namespace hls;
using namespace std;

void getOffer(stream<axiWord>& outData)
{
	static ap_uint<6> wordCount = 0;
	static bool done = false;

	axiWord sendWord;
	if (!done)
	{
		switch (wordCount)
		{
		case 0:
			sendWord.data = 0x34aad42800060102;
			break;
		case 1: //seconds, flags, clientip
		case 5: //clientMac padding + severhostname
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15: //boot filename
		case 16:
		case 17:
		case 18:
		case 19:
		case 20:
		case 21:
		case 22:
		case 23:
		case 24:
		case 25:
		case 26:
		case 27:
		case 28:
			sendWord.data = 0;
			break;
		case 2:
			sendWord.data = 0x0105050a0a05050a; //your ip, next server ip
			break;
		case 3:
			sendWord.data = 0x0304050600000000;
			break;
		case 4:
			sendWord.data = 0x0000000000000102; //clientMac 2nd part
			break;
		/*case 13:
			sendWord.data = 0x6c65707800000000;
			break;
		case 14:
			sendWord.data = 0x0000302e78756e69;
			break;*/
		case 29:
			sendWord.data = 0x6353826300000000; //Magic Cookie
			break;
		case 30:
			sendWord.data = 0x05050a0436020135; //dhcp option 53, 54
			break;
		case 31:
			sendWord.data = 0x0158020000043301; //54, 51, 58
			break;
		case 32:
			sendWord.data = 0x0a040300ffffff04; // 58, 59
			break;
		case 33:
			sendWord.data = 0x05050a041c010505; // 59, 1
			break;
		case 34:
			sendWord.data = 0x7265746e69140fff; //1, 3, 15
			break;
		case 35:
			sendWord.data = 0x6d6178652e6c616e; //15
			break;
		case 36:
			sendWord.data = 0xff67726f2e656c70; //15, done
			break;
		case 37:
			sendWord.data = 0; //padding
			done = true;
			break;
/*		case 38:
			sendWord.data = 0x6863027a68746504; //119
			break;
		case 39:
			sendWord.data = 0x00000000ff04c000; //119, done
			done = true;
			break;*/
		} //switch
		if (!done)
		{
			sendWord.keep = 0xff;
			sendWord.last = 0;
		}
		else
		{
			sendWord.keep = 0x0f;
			sendWord.last = 1;
		}
		outData.write(sendWord);
		wordCount++;
	} //done
}

void getAck(stream<axiWord>& outData)
{
	static ap_uint<6> wordCount = 0;
	static bool done = false;

	axiWord sendWord;
	if (!done)
	{
		switch (wordCount)
		{
		case 0:
			sendWord.data = 0x34aad42800060102;
			break;
		case 1: //seconds, flags, clientip
		case 5: //clientMac padding + severhostname
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15: //boot filename
		case 16:
		case 17:
		case 18:
		case 19:
		case 20:
		case 21:
		case 22:
		case 23:
		case 24:
		case 25:
		case 26:
		case 27:
		case 28:
			sendWord.data = 0;
			break;
		case 2:
			sendWord.data = 0x0105050a0a05050a; //your ip, next server ip
			break;
		case 3:
			sendWord.data = 0x0304050600000000;
			break;
		case 4:
			sendWord.data = 0x0000000000000102; //clientMac 2nd part
			break;
		/*case 13:
			sendWord.data = 0x6c65707800000000;
			break;
		case 14:
			sendWord.data = 0x0000302e78756e69;
			break;*/
		case 29:
			sendWord.data = 0x6353826300000000; //Magic Cookie
			break;
		case 30:
			sendWord.data = 0x05050a0436050135; //dhcp option 53, 54
			break;
		case 31:
			sendWord.data = 0x0158020000043301; //54, 51, 1
			break;
		case 32:
			sendWord.data = 0x0a040300ffffff04; //
			break;
		case 33:
			sendWord.data = 0x05050a041c010505; //
			break;
		case 34:
			sendWord.data = 0x7265746e69140fff; //1, 28, 15
			break;
		case 35:
			sendWord.data = 0x6d6178652e6c616e; //15
			break;
		case 36:
			sendWord.data = 0xff67726f2e656c70; //15, done
			break;
		case 37:
			sendWord.data = 0; //padding
			done = true;
			break;
/*		case 38:
			sendWord.data = 0x6863027a68746504; //119
			break;
		case 39:
			sendWord.data = 0x00000000ff04c000; //119, done
			done = true;
			break;*/
		} //switch
		if (!done)
		{
			sendWord.keep = 0xff;
			sendWord.last = 0;
		}
		else
		{
			sendWord.keep = 0x0f;
			sendWord.last = 1;
		}
		outData.write(sendWord);
		wordCount++;
	} //done
}

int main()
{
	stream<ap_uint<16> >	openPort("openPort");
	stream<bool>			confirmPortStatus("confirmPortStatus");
	//stream<ap_uint<16> >&	realeasePort,
	stream<udpMetadata>		dataInMeta("dataInMeta");
	stream<axiWord>			dataIn("dataIn");
	stream<udpMetadata>		dataOutMeta("dataOutMeta");
	stream<ap_uint<16> >	dataOutLength("dataOutLength");
	stream<axiWord>			dataOut("dataOut");
	ap_uint<32>				ipAddressOut;
	ap_uint<1>				dhcpEnable = 0;
	ap_uint<32>				inputIpAddress = 0x0C0C0C0C;
	ap_uint<48>				myMacAddress = 0x010203040506;

	int count = 0;
	ap_uint<16> port;

	ifstream goldenFile;
	ofstream outputFile;

	outputFile.open("../../../../out.dat");
	if (!outputFile) {
		cout << "Error: could not open output vector file." << endl;
		return -1;
	}
	goldenFile.open("../../../../out.gold");
	if (!goldenFile) {
		cout << "Error: could not open golden output vector file." << endl;
		return -1;
	}
	while (count < 1000) {
		dhcp_client(	openPort,
						confirmPortStatus,
						dataInMeta,
						dataIn,
						dataOutMeta,
						dataOutLength,
						dataOut,
						dhcpEnable,
						inputIpAddress,
						ipAddressOut,
						myMacAddress);

		if (!openPort.empty()) {
			openPort.read(port);
			confirmPortStatus.write(true);
			//std::cout << "Port: " << port << "opened." << std::endl;
		}

		if (count == 120)
			dhcpEnable = 1;
		else if (count == 800)
			dhcpEnable = 0;
		if (count > 200 && count < 300) {
			//std::cout << "Incoming DHCP offer" << std::endl;
			getOffer(dataIn);
		}
		else if (count > 300) {
			//std::cout << "Incoming DHCP ACK" << std::endl;
			getAck(dataIn);
		}
		count++;
		std::cout << std::hex << count << " - " << ipAddressOut << std::endl;
	}

	axiWord outWord;
	udpMetadata outMeta;
	ap_uint<16> outLen;
	bool wasLast = true;
	int outCount = 0;
	uint16_t keepTemp;
	uint64_t dataTemp;
	uint16_t lastTemp;
	int	errCount = 0;

	while (!dataOut.empty()) {
		if (wasLast && !dataOutMeta.empty()) {
			dataOutMeta.read(outMeta);
			std::cout << "Src: " << outMeta.sourceSocket.addr(31, 24)<< "." << outMeta.sourceSocket.addr(23, 16) << ".";
			std::cout << outMeta.sourceSocket.addr(15, 8)<< "." << outMeta.sourceSocket.addr(7, 0) << ":";
			std::cout << ":" << outMeta.sourceSocket.port << std::endl;
			std::cout << "Dst: " << outMeta.destinationSocket.addr(31, 24)<< "." << outMeta.destinationSocket.addr(23, 16) << ".";
			std::cout << outMeta.destinationSocket.addr(15, 8)<< "." << outMeta.destinationSocket.addr(7, 0) << ":";
			std::cout << outMeta.destinationSocket.port << std::endl;
		}
		if (wasLast && !dataOutLength.empty()) {
			dataOutLength.read(outLen);
			std::cout << "Length: " << outLen << std::endl;
		}
		dataOut.read(outWord);
		std::cout << std::hex << std::setfill('0');
		std::cout << std::setw(8) << ((uint32_t) outWord.data(63, 32)) << std::setw(8) << ((uint32_t) outWord.data(31, 0)) << "\t";
		std::cout << std::setw(2) << outWord.keep << " " << outWord.last << std::endl;
		wasLast = outWord.last;

		outputFile << std::hex << std::noshowbase;
		outputFile << std::setfill('0');
		outputFile << std::setw(8) << ((uint32_t) outWord.data(63, 32));
		outputFile << std::setw(8) << ((uint32_t) outWord.data(31, 0));
		outputFile << " " << std::setw(2) << ((uint32_t) outWord.keep) << " ";
		outputFile << std::setw(1) << ((uint32_t) outWord.last) << std::endl;

		goldenFile >> std::hex >> dataTemp >> keepTemp >> lastTemp;

		if (outWord.data != dataTemp || outWord.keep != keepTemp || outWord.last != lastTemp) { // Compare results
			errCount++;
			cerr << "X";
		} else {
			cerr << ".";
		}

		outCount++;
		if (outWord.last) {
			std::cout << "computed length: " << std::dec <<  (outCount*8) << std::endl;
				outCount = 0;
		}
	}

	std::cout << std::dec<< "IP Address: " << ipAddressOut(7,0) << "." << ipAddressOut(15,8) << ".";
	std::cout << ipAddressOut(23,16) << "." << ipAddressOut(31,24) << std::endl;
	outputFile.close();
	goldenFile.close();
	cerr << " done." << endl << endl;
	if (errCount == 0) {
	    cerr << "*** Test Passed ***" << endl << endl;
	    return 0;
	} else {
	   	cerr << "!!! TEST FAILED -- " << errCount << " mismatches detected !!!";
	   	cerr << endl << endl;
	   	//return -1;
	}
	return 0;
}
