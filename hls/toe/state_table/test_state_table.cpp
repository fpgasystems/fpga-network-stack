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

#include "state_table.hpp"


using namespace hls;

struct readVerify
{
	int id;
	char fifo;
	int exp;
};

void emptyFifos(std::ofstream& out, stream<sessionState>& rxFifoOut, stream<sessionState>& appFifoOut, stream<sessionState>& app2FifoOut, stream<ap_uint<16> >& slupOut, int iter)
{
	sessionState outData;
	ap_uint<16> outDataId;
	while (!(rxFifoOut.empty()))
	{
		rxFifoOut.read(outData);
		out << "Step " << std::dec << iter << ": RX Fifo\t\t";
		out << outData << std::endl;
	}

	while (!(appFifoOut.empty()))
	{
		appFifoOut.read(outData);
		out << "Step " << std::dec << iter << ": App Fifo\t\t";
		out << outData << std::endl;
	}

	while (!(app2FifoOut.empty()))
	{
		app2FifoOut.read(outData);
		out << "Step " << std::dec << iter << ": App2 Fifo\t\t";
		out << outData << std::endl;
	}

	while (!(slupOut.empty()))
	{
		slupOut.read(outDataId);
		out << "Step " << std::dec << iter << ": Slup Fifo\t\t";
		out << std::hex;
		out << std::setfill('0');
		out << std::setw(4) << outDataId;
		out << std::endl;
	}
}

int main()
{
	stream<stateQuery> rxIn;
	stream<sessionState> rxOut;
	stream<stateQuery> txAppIn;
	stream<sessionState> txAppOut;
	stream<stateQuery> txApp2In;
	stream<sessionState> txApp2Out;
	stream<ap_uint<16> > timerIn;
	stream<ap_uint<16> > slupOut;

	stateQuery inData;
	sessionState outData;

	std::ifstream inputFile;
	std::ofstream outputFile;

	/*inputFile.open("/home/dasidler/toe/hls/toe/state_table/in.dat");
	if (!inputFile)
	{
		std::cout << "Error: could not open test input file." << std::endl;
		return -1;
	}*/
	outputFile.open("/home/dasidler/toe/hls/toe/state_table/out.dat");
	if (!outputFile)
	{
		std::cout << "Error: could not open test output file." << std::endl;
	}

	int count = 0;

	/*
	 * Test 1: rx(x, ESTA); timer(x), rx(x, FIN_WAIT)
	 */
	//ap_uint<16> id = rand() % 100;
	ap_uint<16> id = 0x57;
	outputFile << "Test 1" << std::endl;
	outputFile << "ID: " << id << std::endl;
	while (count < 20)
	{
		switch (count)
		{
		case 1:
			rxIn.write(stateQuery(id, ESTABLISHED, 1));
			break;
		case 2:
			timerIn.write(id);
			break;
		case 3:
			rxIn.write(stateQuery(id, FIN_WAIT_1, 1));
			break;
		default:
			break;

		}
		state_table(rxIn, txAppIn, txApp2In, timerIn, rxOut, txAppOut, txApp2Out, slupOut);
		emptyFifos(outputFile, rxOut, txAppOut, txApp2Out, slupOut, count);
		count++;
	}
	outputFile << "------------------------------------------------" << std::endl;

	//BREAK
	count = 0;
	while (count < 200)
	{
		state_table(rxIn, txAppIn, txApp2In, timerIn, rxOut, txAppOut, txApp2Out, slupOut);
		count++;
	}

	/*
	 * Test 2: rx(x, ESTA); rx(y, ESTA); timer(x), time(y)
	 */
	//ap_uint<16> id = rand() % 100;
	count = 0;
	id = 0x6f;
	ap_uint<16> id2 = 0x3a;
	outputFile << "Test 2" << std::endl;
	outputFile << "ID: " << id << " ID2: " << id2 << std::endl;
	while (count < 20)
	{
		switch (count)
		{
		case 1:
			rxIn.write(stateQuery(id, ESTABLISHED, 1));
			break;
		case 2:
			rxIn.write(stateQuery(id2, ESTABLISHED, 1));
			break;
		case 3:
			timerIn.write(id2);
			break;
		case 4:
			timerIn.write(id);
			break;
		case 5:
			timerIn.write(id);
			break;
		default:
			break;

		}
		state_table(rxIn, txAppIn, txApp2In, timerIn, rxOut, txAppOut, txApp2Out, slupOut);
		emptyFifos(outputFile, rxOut, txAppOut, txApp2Out, slupOut, count);
		count++;
	}
	outputFile << "------------------------------------------------" << std::endl;
	//BREAK
	count = 0;
	while (count < 200)
	{
		state_table(rxIn, txAppIn, txApp2In, timerIn, rxOut, txAppOut, txApp2Out, slupOut);
		count++;
	}

	/*
	 * Test 3: rx(x); timer(x), rx(x, ESTABLISHED)
	 * Pause then: rx(y), txApp(y), rx(y, ESTABLISHED), txApp(y, value)
	 */
	//ap_uint<16> id = rand() % 100;
	count = 0;
	id = 0x6f;
	id2 = 0x3a;
	outputFile << "Test 3" << std::endl;
	outputFile << "ID: " << id << " ID2: " << id2 << std::endl;
	while (count < 40)
	{
		switch (count)
		{
		case 1:
			rxIn.write(stateQuery(id));
			break;
		case 2:
			timerIn.write(id);
			break;
		case 5:
			rxIn.write(stateQuery(id, ESTABLISHED, 1));
			break;
		case 8:
			rxIn.write(stateQuery(id));
			break;
		case 10:
			rxIn.write(stateQuery(id2));
			break;
		case 11:
			txAppIn.write(stateQuery(id2));
			break;
		case 14:
			rxIn.write(stateQuery(id2, CLOSING, 1));
			break;
		case 15:
			txAppIn.write(stateQuery(id2, SYN_SENT, 1));
			break;
		case 19:
			txApp2In.write(id2);
			break;
		default:
			break;

		}
		state_table(rxIn, txAppIn, txApp2In, timerIn, rxOut, txAppOut, txApp2Out, slupOut);
		emptyFifos(outputFile, rxOut, txAppOut, txApp2Out, slupOut, count);
		count++;
	}
	outputFile << "------------------------------------------------" << std::endl;
	//BREAK
	count = 0;
	while (count < 200)
	{
		state_table(rxIn, txAppIn, txApp2In, timerIn, rxOut, txAppOut, txApp2Out, slupOut);
		count++;
	}


	/*
	 * Test 4: txApp(x); rx(x), txApp(x, SYN_SENT), rx(x, ESTABLISHED), txApp2(x)
	 */
	//ap_uint<16> id = rand() % 100;
	count = 0;
	id = 0x7f3;
	outputFile << "Test 4" << std::endl;
	outputFile << "ID: " << id << std::endl;
	while (count < 40)
	{
		switch (count)
		{
		case 1:
			txAppIn.write(stateQuery(id));
			break;
		case 2:
			rxIn.write(stateQuery(id));
			break;
		case 5:
			txAppIn.write(stateQuery(id, SYN_SENT, 1));
			break;
		case 7:
			rxIn.write(stateQuery(id, ESTABLISHED, 1));
			break;
		case 19:
			txApp2In.write(id);
			break;
		default:
			break;

		}
		state_table(rxIn, txAppIn, txApp2In, timerIn, rxOut, txAppOut, txApp2Out, slupOut);
		emptyFifos(outputFile, rxOut, txAppOut, txApp2Out, slupOut, count);
		count++;
	}
	outputFile << "------------------------------------------------" << std::endl;
	//BREAK
	count = 0;
	while (count < 200)
	{
		state_table(rxIn, txAppIn, txApp2In, timerIn, rxOut, txAppOut, txApp2Out, slupOut);
		count++;
	}

	/*
	 * Test 5: rxEng(x, ESTABLISED), timer(x), rxEng(x)
	 */
	//ap_uint<16> id = rand() % 100;
	count = 0;
	id = 0x783;
	outputFile << "Test 5" << std::endl;
	outputFile << "ID: " << id << std::endl;
	while (count < 40)
	{
		switch (count)
		{
		case 1:
			rxIn.write(stateQuery(id, ESTABLISHED, 1));
			break;
		case 5:
			timerIn.write(id);
			break;
		case 6:
			rxIn.write(stateQuery(id));
			break;
		default:
			break;

		}
		state_table(rxIn, txAppIn, txApp2In, timerIn, rxOut, txAppOut, txApp2Out, slupOut);
		emptyFifos(outputFile, rxOut, txAppOut, txApp2Out, slupOut, count);
		count++;
	}

	return 0;
}

/*int main()
{
	stream<stateQuery> rxIn;
	stream<sessionState> rxOut;
	stream<stateQuery> txAppIn;
	stream<sessionState> txAppOut;
	stream<stateQuery> txApp2In;
	stream<sessionState> txApp2Out;
	stream<ap_uint<16> > timerIn;
	stream<ap_uint<16> > slupOut;

	stateQuery inData;
	sessionState outData;

	std::ifstream inputFile;
	std::ofstream outputFile;

	inputFile.open("/home/dasidler/toe/hls/toe/state_table/in.dat");
	if (!inputFile)
	{
		std::cout << "Error: could not open test input file." << std::endl;
		return -1;
	}
	outputFile.open("/home/dasidler/toe/hls/toe/state_table/out.dat");
	if (!outputFile)
	{
		std::cout << "Error: could not open test output file." << std::endl;
	}




	std::string fifoTemp;
	int sessionIDTemp;
	char opTemp;
	int valueTemp;
	int exp_valueTemp;
	sessionState response;
	int errCount = 0;

	std::vector<readVerify> reads;

	int count = 0;
	while (count < 500)
	{
		if ((count % 10) == 0)
		{
			if (inputFile >> fifoTemp >> sessionIDTemp >> opTemp >> valueTemp >> exp_valueTemp)
			{
				inData.sessionID = sessionIDTemp;
				inData.state = (sessionState)valueTemp;
				inData.write = (opTemp == 'W');
				if (fifoTemp == "TX")
				{
					txAppIn.write(inData);
					if (opTemp == 'R')
					{
						reads.push_back( (readVerify){sessionIDTemp, fifoTemp[0], exp_valueTemp});
					}
				}
				else // RX
				{
					rxIn.write(inData);
					if (opTemp == 'R')
					{
						reads.push_back((readVerify) {sessionIDTemp, fifoTemp[0], exp_valueTemp});
					}
				}
			}
		}
		state_table(rxIn, txAppIn, txApp2In, timerIn, rxOut, txAppOut, txApp2Out, slupOut);
		count++;
	}

	std::vector<readVerify>::const_iterator it;
	it = reads.begin();
	bool readData = false;
	while (it != reads.end())
	{
		readData = false;
		if (it->fifo == 'T')
		{
			if (!txAppOut.empty())
			{
				txAppOut.read(response);
				readData = true;
			}
		}
		else
		{
			if (!rxOut.empty())
			{
				rxOut.read(response);
				readData = true;
			}
		}

		if (readData)
		{
			if (((int)response) != it->exp)
			{
				outputFile << "Error at ID " << it->id << " on fifo ";
				outputFile << it->fifo << " response value was " << response << " instead of " << it->exp << std::endl;
				errCount ++;
			}
			else
			{
				outputFile << "Success at ID " << it->id << " on fifo ";
				outputFile << it->fifo << " response value was " << response << " instead of " << it->exp << std::endl;
			}
			it++;
		}
	}

	if (errCount == 0)
	{
		outputFile << "No errors coccured." << std::endl;
	}
	else
	{
		outputFile << errCount << " errors coccured." << std::endl;
	}
}*/
