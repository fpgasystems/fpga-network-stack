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
#include "tx_sar_table.hpp"


using namespace hls;

struct readVerify
{
	int id;
	char fifo;
	int exp;
};

int main()
{
	stream<txAppTxSarQuery> txAppReqFifo;
	stream<txAppTxSarReply> txAppRspFifo;
	stream<txAppTxSarPush> txAppPushFifo;
	stream<txTxSarQuery> txReqFifo;
	stream<txSarEntry> txRspFifo;
	stream<rxTxSarQuery> rxQueryFifo;
	stream<ap_uint<32> > rxRespFifo;

	txTxSarQuery txInData;
	rxTxSarQuery rxInData;
	sessionState outData;

	std::ifstream inputFile;
	std::ofstream outputFile;

	/*inputFile.open("/home/dasidler/toe/hls/toe/tx_sar_table/in.dat");
	if (!inputFile)
	{
		std::cout << "Error: could not open test input file." << std::endl;
		return -1;
	}*/
	outputFile.open("/home/dasidler/toe/hls/toe/tx_sar_table/out.dat");
	if (!outputFile)
	{
		std::cout << "Error: could not open test output file." << std::endl;
	}

	std::string fifoTemp;
	int sessionIDTemp;
	char opTemp;
	int valueTemp;
	int exp_valueTemp;
	int errCount = 0;

	std::vector<readVerify> reads;

	int count = 0;
	while (count < 500)
	{
		if ((count % 10) == 0)
		{
			if (inputFile >> fifoTemp >> sessionIDTemp >> opTemp >> valueTemp >> exp_valueTemp)
			{
				if (fifoTemp == "TX")
				{
					txInData.sessionID = sessionIDTemp;
					//txInData.recv_window = valueTemp;
					txInData.lock = 1;
					txInData.write = (opTemp == 'W');;
					txReqFifo.write(txInData);
					if (opTemp == 'R')
					{
						reads.push_back( (readVerify){sessionIDTemp, fifoTemp[0], exp_valueTemp});
					}
				}
				else // RX
				{
					rxInData.sessionID = sessionIDTemp;
					rxInData.recv_window = valueTemp;
					rxInData.ackd = 0;
					rxQueryFifo.write(rxInData);
					/*if (opTemp == 'R')
					{
						reads.push_back((readVerify) {sessionIDTemp, fifoTemp[0], exp_valueTemp});
					}*/
				}
			}
		}
		//tx_sar_table(txReqFifo, txRspFifo, txAppReqFifo, txAppRspFifo, rxQueryFifo);
		tx_sar_table(rxQueryFifo, txAppReqFifo, txReqFifo, txAppPushFifo, rxRespFifo, txAppRspFifo, txRspFifo);
		count++;
	}

	std::vector<readVerify>::const_iterator it;
	it = reads.begin();
	bool readData = false;
	txSarEntry response;
	while (it != reads.end())
	{
		readData = false;
		if (it->fifo == 'T')
		{
			if (!txRspFifo.empty())
			{
				txRspFifo.read(response);
				readData = true;
			}
		}

		if (readData)
		{
			if (((int)response.recv_window) != it->exp)
			{
				outputFile << "Error at ID " << it->id << " on fifo ";
				outputFile << it->fifo << " response value was " << response.recv_window << " instead of " << it->exp << std::endl;
				errCount ++;
			}
			/*else
			{
				outputFile << "Success at ID " << it->id << " on fifo ";
				outputFile << it->fifo << " response value was " << response.recv_window << " as expected." << std::endl;
			}*/
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
}
