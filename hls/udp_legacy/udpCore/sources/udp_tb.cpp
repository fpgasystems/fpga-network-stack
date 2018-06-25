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
#include "udp.h"

using namespace std;

#define maxOverflowValue 2000

uint32_t clockCounter = 0;

ap_uint<64>	convertString(std::string stringToConvert) {
	ap_uint<64>	tempOutput = 0;
	unsigned short int tempValue = 16;
	static const char* const lut = "0123456789ABCDEF";
	
	for (unsigned short int i = 0; i<stringToConvert.size();++i) {
		tempValue = 16;
		for (unsigned short int j = 0;j<16;++j) {
			if (lut[j] == stringToConvert[i]) {
				tempValue = j;
				break;
			}
		}
		if (tempValue != 16) {
			for (short int k = 3;k>=0;--k) {
				if (tempValue >= pow(2.0, k)) {
					tempOutput.bit(63-(4*i+(3-k))) = 1;
					tempValue -= static_cast <unsigned short int>(pow(2.0, k));
				}
			}
		}
		else return -1;
	}
	return tempOutput;
}

std::vector<std::string> parseLine(std::string stringBuffer)
{
	std::vector<std::string> tempBuffer;
	bool found = false;

	while (stringBuffer.find(" ") != std::string::npos)	// Search for spaces delimiting the different data words
	{
		std::string temp = stringBuffer.substr(0, stringBuffer.find(" "));							// Split the the string
		stringBuffer = stringBuffer.substr(stringBuffer.find(" ")+1, stringBuffer.length());	// into two
		tempBuffer.push_back(temp);		// and store the new part into the vector. Continue searching until no more spaces are found.
	}
	tempBuffer.push_back(stringBuffer);	// and finally push the final part of the string into the vector when no more spaces are present.
	return tempBuffer;
}

string decodeApUint64(ap_uint<64> inputNumber) {
	std::string 				outputString	= "0000000000000000";
	unsigned short int			tempValue 		= 16;
	static const char* const	lut 			= "0123456789ABCDEF";
	for (int i = 15;i>=0;--i) {
	tempValue = 0;
	for (unsigned short int k = 0;k<4;++k) {
		if (inputNumber.bit((i+1)*4-k-1) == 1)
			tempValue += static_cast <unsigned short int>(pow(2.0, 3-k));
		}
		outputString[15-i] = lut[tempValue];
	}
	return outputString;
}

string decodeApUint32(ap_uint<32> inputNumber) {
	std::string 				outputString	= "00000000";
	unsigned short int			tempValue 		= 16;
	static const char* const	lut 			= "0123456789ABCDEF";
	for (int i = 7;i>=0;--i) {
	tempValue = 0;
	for (unsigned short int k = 0;k<4;++k) {
		if (inputNumber.bit((i+1)*4-k-1) == 1)
			tempValue += static_cast <unsigned short int>(pow(2.0, 3-k));
		}
		outputString[7-i] = lut[tempValue];
	}
	return outputString;
}

string decodeApUint16(ap_uint<16> inputNumber) {
	std::string 				outputString	= "0000";
	unsigned short int			tempValue 		= 16;
	static const char* const	lut 			= "0123456789ABCDEF";
	for (int i = 3;i>=0;--i) {
	tempValue = 0;
	for (unsigned short int k = 0;k<4;++k) {
		if (inputNumber.bit((i+1)*4-k-1) == 1)
			tempValue += static_cast <unsigned short int>(pow(2.0, 3-k));
		}
		outputString[4-i] = lut[tempValue];
	}
	return outputString;
}

string decodeApUint8(ap_uint<8> inputNumber) {
	string 						outputString	= "00";
	unsigned short int			tempValue 		= 16;
	static const char* const	lut 			= "0123456789ABCDEF";
	for (int i = 1;i>=0;--i) {
	tempValue = 0;
	for (unsigned short int k = 0;k<4;++k) {
		if (inputNumber.bit((i+1)*4-k-1) == 1)
			tempValue += static_cast <unsigned short int>(pow(2.0, 3-k));
		}
		outputString[1-i] = lut[tempValue];
	}
	return outputString;
}

ap_uint<64> encodeApUint64(string dataString){
	ap_uint<64> tempOutput = 0;
	unsigned short int	tempValue = 16;
	static const char* const	lut = "0123456789ABCDEF";

	for (unsigned short int i = 0; i<dataString.size();++i) {
		for (unsigned short int j = 0;j<16;++j) {
			if (lut[j] == dataString[i]) {
				tempValue = j;
				break;
			}
		}
		if (tempValue != 16) {
			for (short int k = 3;k>=0;--k) {
				if (tempValue >= pow(2.0, k)) {
					tempOutput.bit(63-(4*i+(3-k))) = 1;
					tempValue -= static_cast <unsigned short int>(pow(2.0, k));
				}
			}
		}
	}
	return tempOutput;
}

ap_uint<32> encodeApUint32(string parseString){
	ap_uint<32> tempOutput = 0;
	unsigned short int	tempValue = 16;
	static const char* const	lut = "0123456789ABCDEF";

	for (unsigned short int i = 0; i<8;++i) {
		for (unsigned short int j = 0;j<16;++j) {
			if (lut[j] == parseString[i]) {
				tempValue = j;
				break;
			}
		}
		if (tempValue != 16) {
			for (short int k = 3;k>=0;--k) {
				if (tempValue >= pow(2.0, k)) {
					tempOutput.bit(31-(4*i+(3-k))) = 1;
					tempValue -= static_cast <unsigned short int>(pow(2.0, k));
				}
			}
		}
	}
	return tempOutput;
}

ap_uint<8> encodeApUint8(string keepString){
	ap_uint<8> tempOutput = 0;
	unsigned short int	tempValue = 16;
	static const char* const	lut = "0123456789ABCDEF";

	for (unsigned short int i = 0; i<2;++i) {
		for (unsigned short int j = 0;j<16;++j) {
			if (lut[j] == keepString[i]) {
				tempValue = j;
				break;
			}
		}
		if (tempValue != 16) {
			for (short int k = 3;k>=0;--k) {
				if (tempValue >= pow(2.0, k)) {
					tempOutput.bit(7-(4*i+(3-k))) = 1;
					tempValue -= static_cast <unsigned short int>(pow(2.0, k));
				}
			}
		}
	}
	return tempOutput;
}

ap_uint<16> encodeApUint16(string parseString){
	ap_uint<16> tempOutput = 0;
	unsigned short int	tempValue = 16;
	static const char* const	lut = "0123456789ABCDEF";

	for (unsigned short int i = 0; i<4;++i) {
		for (unsigned short int j = 0;j<16;++j) {
			if (lut[j] == parseString[i]) {
				tempValue = j;
				break;
			}
		}
		if (tempValue != 16) {
			for (short int k = 3;k>=0;--k) {
				if (tempValue >= pow(2.0, k)) {
					tempOutput.bit(15-(4*i+(3-k))) = 1;
					tempValue -= static_cast <unsigned short int>(pow(2.0, k));
				}
			}
		}
	}
	return tempOutput;
}

int main(int argc, char *argv[]) {

	stream<axiWord> 		inputPathInDataFIFO("inputPathInDataFIFO");
	stream<axiWord> 		inputPathOutDataFIFO("inputPathOutDataFIFO");
	stream<ap_uint<16> >	openPortFIFO("openPortFIFO");
	stream<bool>		 	confirmPortStatusFIFO("confirmPortStatusFIFO");
	stream<ap_uint<16> > 	inputPathPortRealeaseFIFO("inputPathPortRealeaseFIFO");
	stream<metadata> 		inputPathOutputMetadataFIFO("inputPathOutputMetadataFIFO");
	stream<axiWord> 		outputPathInDataFIFO("outputPathInDataFIFO");
	stream<axiWord> 		outputPathOutDataFIFO("outputPathOutDataFIFO");
	stream<metadata> 		outputPathInMetadataFIFO("outputPathInMetadataFIFO");
	stream<ap_uint<16> > 	outputpathInLengthFIFO("outputpathInLengthFIFO");
	stream<ipTuple> 		outIPaddressesFIFO("outIPaddressesFIFO");
	stream<axiWord>			outPortUnreachableFIFO("outPortUnreachableFIFO");

	axiWord					inputPathInputData 			= axiWord(0, 0, 0);
	ipTuple					incomingPacketIPData		= ipTuple(0, 0);
	metadata				inputPathInputMetadata		= metadata(sockaddr_in(0, 0), sockaddr_in(0, 0));
	axiWord					inputPathOutputData			= axiWord(0, 0, 0);
	ap_uint<16>				openPortData				= 0;
	bool					portStatusData				= false;
	metadata				inputPathOutputMetadata 	= metadata(sockaddr_in(0, 0), sockaddr_in(0, 0));
	axiWord					outputPathInData			= axiWord(0, 0, 0);
	axiWord					outputPathOutData			= axiWord(0, 0, 0);
	ap_uint<16> 			outputPathInputLength		= 0;
	metadata				outputPathInputMetadata		= metadata(sockaddr_in(0, 0), sockaddr_in(0, 0));
	ipTuple					outputPathOutputIPAddresses = ipTuple(0, 0);			// Source & Destination IP for Tx packet to be used by the lower layer
	/// Input File Temp Variables ///
	uint16_t				inputSourceIP				= 0;
	uint16_t				inputDestinationIP			= 0;
	//uint64_t 				dataString					= 0;
	std::string 			dataString;
	ipTuple 				inputIP 					= ipTuple(0, 0);
	axiWord					inputData					= axiWord(0, 0, 0);
	uint16_t				sop 						= 0;
	uint16_t				eop							= 0;
	uint16_t				mod							= 0;
	uint16_t				valid						= 0;
	uint32_t				errCount					= 0;

	ifstream rxInput;
	ifstream txInput;
	ofstream rxOutput;
	ofstream txOutput;
	ofstream portStatus;
	ifstream goldenOutputRx;
	ifstream goldenOutputTx;

	uint32_t				overflowCounter				= 0;
	errCount = 0;
	int counter = 0;
	if (argc != 3) {
		std::cout << "You need to provide at least one parameter (the input file name)!" << std::endl;
		return -1;
	}
	rxInput.open(argv[1]);
	if (!rxInput)	{
		std::cout << " Error opening Rx input file!" << std::endl;
		return -1;
	}
	txInput.open(argv[2]);
	if (!txInput) {
		std::cout << " Error opening Tx input file!" << std::endl;
		return -1;
	}
	rxOutput.open("rxOutput.dat");
	if (!rxOutput) {
		std::cout << " Error opening output file!" << std::endl;
		return -1;
	}
	portStatus.open("portStatus.dat");
	if (!portStatus) {
		std::cout << " Error opening port status file!" << std::endl;
		return -1;
	}
	txOutput.open("txOutput.dat");
	if (!txOutput) {
		cout << "Error openingn Tx Output file!" << endl;
		return -1;
	}
	goldenOutputRx.open("../../../../sources/goldenOutput/rxGoldenOutput.short.dat");
	if (!goldenOutputRx) {
		cout << "Error opening golden output file for the Rx side. Check that the correct path is provided!" << endl;
		return -1;
	}
	goldenOutputTx.open("../../../../sources/goldenOutput/txGoldenOutput.short.dat");
	if (!goldenOutputTx) {
		cout << "Error opening golden output file for the Tx side. Check that the correct path is provided!" << endl;
		return -1;
	}
	std::cerr << "Input File: " << argv[1] << std::endl << std::endl;
	// Test 1: Attempt to close a port that isn't open. Expected response is: Nothing
	inputPathPortRealeaseFIFO.write(0x1532);
	for (short int i= 0;i<10;++i)
		udp(inputPathInDataFIFO, inputPathOutDataFIFO, openPortFIFO, confirmPortStatusFIFO,				// Input Path Streams
			inputPathOutputMetadataFIFO, inputPathPortRealeaseFIFO, outputPathInDataFIFO, outputPathOutDataFIFO, outputPathInMetadataFIFO, outputpathInLengthFIFO, outPortUnreachableFIFO);		// Output Path Streams
		//std::cerr << ".";
	// Test 2: Atempt to open a new port. Expected response is: Port opened successfully
	bool temp = 0;
	openPortFIFO.write(0x80);
	for (short int i= 0;i<3;++i)
		udp(inputPathInDataFIFO, inputPathOutDataFIFO, openPortFIFO, confirmPortStatusFIFO,				// Input Path Streams
			inputPathOutputMetadataFIFO, inputPathPortRealeaseFIFO, outputPathInDataFIFO, outputPathOutDataFIFO, outputPathInMetadataFIFO, outputpathInLengthFIFO, outPortUnreachableFIFO);		// Output Path Streams
		//std::cerr << ".";
	if (!confirmPortStatusFIFO.empty()) {
		temp = confirmPortStatusFIFO.read();
		std::cerr << "Port opened successfully!" << std::endl;
	}
	else {
		std::cerr << "Error, port not opened successfully." << std::endl;
		return -1;
	}
	//Test 3: Close an already open port
	inputPathPortRealeaseFIFO.write(temp);
	for (short int i= 0;i<10;++i)
		udp(inputPathInDataFIFO, inputPathOutDataFIFO, openPortFIFO, confirmPortStatusFIFO,				// Input Path Streams
			inputPathOutputMetadataFIFO, inputPathPortRealeaseFIFO, outputPathInDataFIFO, outputPathOutDataFIFO, outputPathInMetadataFIFO, outputpathInLengthFIFO, outPortUnreachableFIFO);		// Output Path Streams
		//std::cerr << ".";
	//Test 4: Read in the test input data for the Rx side without opening the port.
	uint32_t noOfLines = 0;
	while (!rxInput.eof()) {
		std::string stringBuffer;
		getline(rxInput, stringBuffer);
		std::vector<std::string> stringVector = parseLine(stringBuffer);
		string dataString = stringVector[0];
		string keepString = stringVector[2];
		inputPathInputData.data = encodeApUint64(dataString);
		inputPathInputData.keep = encodeApUint8(keepString);
		inputPathInputData.last = atoi(stringVector[1].c_str());
		inputPathInDataFIFO.write(inputPathInputData);
		//noOfLines++;
	}
	//noOfLines = 0;
	while (!inputPathInDataFIFO.empty() || overflowCounter < maxOverflowValue) {
		udp(inputPathInDataFIFO, inputPathOutDataFIFO, openPortFIFO, confirmPortStatusFIFO,	inputPathOutputMetadataFIFO,			// Input Path Streams
			inputPathPortRealeaseFIFO, outputPathInDataFIFO, outputPathOutDataFIFO, outputPathInMetadataFIFO, outputpathInLengthFIFO, outPortUnreachableFIFO);		// Output Path Streams
		//std::cerr << ".";
		//noOfLines++;
		if (inputPathInDataFIFO.empty())
			overflowCounter++;
	}
	rxInput.close();
	rxInput.open(argv[1]);
	//Test 6: Read in the test input data for the Rx side. First re-open the port to which data is to be sent.
	cerr << endl << "Test 4: Exercising the Rx Path" << endl;
	openPortFIFO.write(0x80);
	for (short int i= 0;i<3;++i)
		udp(inputPathInDataFIFO, inputPathOutDataFIFO, openPortFIFO, confirmPortStatusFIFO,				// Input Path Streams
			inputPathOutputMetadataFIFO, inputPathPortRealeaseFIFO, outputPathInDataFIFO, outputPathOutDataFIFO, outputPathInMetadataFIFO, outputpathInLengthFIFO, outPortUnreachableFIFO);		// Output Path Streams
		//std::cerr << ".";
	if (!confirmPortStatusFIFO.empty()) {
		temp = confirmPortStatusFIFO.read();
		std::cerr << endl << "Port opened successfully!" << std::endl;
	}
	else {
		std::cerr << endl << "Error, port not opened successfully." << std::endl;
		return -1;
	}
	// And then send the data in
	//uint32_t noOfLines = 0;
	while (!rxInput.eof()) {
		std::string stringBuffer;
		getline(rxInput, stringBuffer);
		std::vector<std::string> stringVector = parseLine(stringBuffer);
		string dataString = stringVector[0];
		string keepString = stringVector[2];
		inputPathInputData.data = encodeApUint64(dataString);
		inputPathInputData.keep = encodeApUint8(keepString);

		inputPathInputData.last = atoi(stringVector[1].c_str());
		inputPathInDataFIFO.write(inputPathInputData);
		//noOfLines++;
	}
	//noOfLines = 0;
	while (!inputPathInDataFIFO.empty() || overflowCounter < maxOverflowValue) {
		udp(inputPathInDataFIFO, inputPathOutDataFIFO, openPortFIFO, confirmPortStatusFIFO,	inputPathOutputMetadataFIFO,			// Input Path Streams
			inputPathPortRealeaseFIFO, outputPathInDataFIFO, outputPathOutDataFIFO, outputPathInMetadataFIFO, outputpathInLengthFIFO, outPortUnreachableFIFO);		// Output Path Streams
		//std::cerr << ".";
		//noOfLines++;
		if (inputPathInDataFIFO.empty())
			overflowCounter++;
	}
	//noOfLines = 0;
	cerr << endl << "Rx test complete." << endl;
	overflowCounter = 0;
	// Test 5: Tx path
	cerr << endl << "Test 5: Exercising the Tx Path" << endl;
	noOfLines = 0;
	overflowCounter = 0;
	while (!txInput.eof()) {
		std::string stringBuffer;
		getline(txInput, stringBuffer);
		std::vector<std::string> stringVector = parseLine(stringBuffer);
		string sourcePort 		= stringVector[0];
		string destinationPort 	= stringVector[1];
		string sourceIP			= stringVector[2];
		string destinationIP	= stringVector[3];
		string payloadLength	= stringVector[4];
		string dataString 		= stringVector[5];
		string keepString 		= stringVector[6];
		outputPathInData.data 	= encodeApUint64(dataString);
		outputPathInData.keep	= encodeApUint8(keepString);
		outputPathInData.last 	= atoi(stringVector[7].c_str());
		outputPathInputMetadata.sourceSocket.port		= encodeApUint16(sourcePort);
		outputPathInputMetadata.sourceSocket.addr		= encodeApUint32(sourceIP);
		outputPathInputMetadata.destinationSocket.port	= encodeApUint16(destinationPort);
		outputPathInputMetadata.destinationSocket.addr 	= encodeApUint32(destinationIP);
		outputPathInputLength	= encodeApUint16(payloadLength);
		outputPathInDataFIFO.write(outputPathInData);
		if (outputPathInputLength != 0) {
			outputPathInMetadataFIFO.write(outputPathInputMetadata);
			outputpathInLengthFIFO.write(outputPathInputLength);
		}
		noOfLines++;
	}
	noOfLines = 0;
	while (!outputPathInDataFIFO.empty() || overflowCounter < maxOverflowValue) {
		udp(inputPathInDataFIFO, inputPathOutDataFIFO, openPortFIFO, confirmPortStatusFIFO,	inputPathOutputMetadataFIFO,			// Input Path Streams
			inputPathPortRealeaseFIFO, outputPathInDataFIFO, outputPathOutDataFIFO, outputPathInMetadataFIFO, outputpathInLengthFIFO, outPortUnreachableFIFO);		// Output Path Streams
		//std::cerr << ".";
		noOfLines++;
		if (outputPathInDataFIFO.empty())
			overflowCounter++;
	}
	cerr << endl;
	overflowCounter = 0;
	cerr << endl << "Tx test complete. Verifying result." << endl;
	axiWord txCompareOutput = axiWord(0, 0, 0);
	while (!outputPathOutDataFIFO.empty()) {
		axiWord tempOutput = outputPathOutDataFIFO.read();
		goldenOutputTx >> hex >> txCompareOutput.data >> txCompareOutput.last >> txCompareOutput.keep;
		if (txCompareOutput.data != tempOutput.data || txCompareOutput.last != tempOutput.last || txCompareOutput.keep != tempOutput.keep) {
			errCount++;
			cerr << "X";
		}
		else
			cerr << ".";
		string dataOutput = decodeApUint64(tempOutput.data);
		string keepOutput = decodeApUint8(tempOutput.keep);
		txOutput << dataOutput << " " << tempOutput.last << " " << keepOutput << endl;
	}
	/*if (errCount != 0) {
		cerr << endl << "Errors during Tx testing. Check the output file." << endl;
	}
	else
		cerr << endl << "Tx Tests passed succesfully." << endl;*/
	axiWord compareWord = axiWord(0, 0, 0);
	while (!inputPathOutDataFIFO.empty() && !goldenOutputRx.eof()) {
		//noOfLines++;
		//counter++;
		//cerr << counter << " ";
		axiWord tempOutput = inputPathOutDataFIFO.read();
		goldenOutputRx >> hex >> compareWord.data >> compareWord.last;
		if (compareWord.data != tempOutput.data || compareWord.last != tempOutput.last) {
			errCount++;
			//cerr << "X";
		}
		//else
			//cerr << ".";
		string dataOutput = decodeApUint64(tempOutput.data);
		rxOutput << dataOutput << " " << tempOutput.last << endl;
	}
	if (errCount != 0) {
		cerr << endl << "Errors during testing. Check the output file." << endl;
	}
	else
		cerr << endl << "All Tests passed succesfully." << endl;
	errCount = 0;
	while(!outPortUnreachableFIFO.empty())
		outPortUnreachableFIFO.read();
	while(!inputPathOutputMetadataFIFO.empty())
		inputPathOutputMetadataFIFO.read();
	while(!inputPathOutDataFIFO.empty())
		inputPathOutDataFIFO.read();

	rxInput.close();
	rxOutput.close();
	txInput.close();
	txOutput.close();
	portStatus.close();
	return 0;
}
