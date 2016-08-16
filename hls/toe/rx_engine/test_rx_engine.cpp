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
#include "rx_engine.hpp"
#include <iostream>


using namespace hls;

void simSlookup(stream<sessionLookupQuery>&	req, stream<sessionLookupReply>& rsp)
{
	if (!req.empty())
	{
		req.read();
		rsp.write(sessionLookupReply(1, true));
	}
}

void simPortTable(stream<ap_uint<16> >& req, stream<bool>& rsp)
{
	if (!req.empty())
	{
		req.read();
		rsp.write(true);
	}
}

static sessionState currentState = CLOSED;
void simStateTable(stream<stateQuery>& req, stream<sessionState>& rsp)
{
	stateQuery query;
	if (!req.empty())
	{
		req.read(query);
		if (query.write)
		{
			currentState = query.state;
		}
		else
		{
			rsp.write(currentState);
		}
	}

}

static rxSarEntry currRxEntry;
void simRxSar(stream<rxSarRecvd>& req, stream<rxSarEntry>& rsp)
{
	rxSarRecvd query;
	if (!req.empty())
	{
		req.read(query);
		if (query.write)
		{
			currRxEntry.recvd = query.recvd;
		}
		else
		{
			rsp.write(currRxEntry);
		}
	}

}

static txSarEntry currTxEntry;
void simTxSar(stream<rxTxSarQuery>& req, stream<ap_uint<32> >& rsp)
{
	rxTxSarQuery query;
	if (!req.empty())
	{
		req.read(query);
		if (query.write)
		{
			currTxEntry.ackd = query.ackd;
			currTxEntry.recv_window = query.recv_window;
		}
		else
		{
			rsp.write(currTxEntry.not_ackd);
		}
	}

}

int main(int argc, char* argv[])
{

	stream<axiWord>						ipRxData;
	stream<sessionLookupReply>			sLookup2rxEng_rsp;
	stream<sessionState>				stateTable2rxEng_upd_rsp("stateTable2rxEng_upd_rsp");
	stream<bool>						portTable2rxEng_rsp("portTable2rxEng_rsp");
	stream<rxSarEntry>					rxSar2rxEng_upd_rsp;
	stream<ap_uint<32> >				txSar2rxEng_upd_rsp;
	stream<mmStatus>					rxBufferWriteStatus;
	stream<axiWord>						rxBufferWriteData;
	stream<sessionLookupQuery>			rxEng2sLookup_req;
	stream<stateQuery>					rxEng2stateTable_upd_req("rxEng2stateTable_upd_req");
	stream<ap_uint<16> >				rxEng2portTable_req("rxEng2portTable_req");
	stream<rxSarRecvd>					rxEng2rxSar_upd_req;
	stream<rxTxSarQuery>				rxEng2txSar_upd_req;
	stream<rxRetransmitTimerUpdate>		rxEng2timer_clearRetransmitTimer;
	stream<ap_uint<16> >				rxEng2timer_setCloseTimer;
	stream<openStatus>					openConStatusOut; //TODO remove
	stream<extendedEvent>				rxEng2eventEng_setEvent("rxEng2eventEng_setEvent");
	stream<mmCmd>						rxBufferWriteCmd;
	stream<appNotification>				rxEng2rxApp_notification;

	std::ifstream inputFile;
	std::ofstream outputFile;

	axiWord inData;
	axiWord outData;

	if (argc < 3)
	{
		std::cout << "[ERROR] missing arguments." << std::endl;
		return -1;
	}
	//std::cout << argc << " " << argv[1] << std::endl;
	inputFile.open(argv[1]);

	if (!inputFile)
	{
		std::cout << "Error: could not open test input file." << std::endl;
		return -1;
	}
	//std::cout << argc << " " << argv[2] << std::endl;
	outputFile.open(argv[2]);
	if (!outputFile)
	{
		std::cout << "Error: could not open test output file." << std::endl;
		return -1;
	}

	uint16_t strbTemp;
	uint64_t dataTemp;
	uint16_t lastTemp;

	while(inputFile >> std::hex >> dataTemp >> strbTemp >> lastTemp)
	{
		inData.data = dataTemp;
		inData.keep = strbTemp;
		inData.last = lastTemp;
		ipRxData.write(inData);
		rx_engine(	ipRxData,
					sLookup2rxEng_rsp,
					stateTable2rxEng_upd_rsp,
					portTable2rxEng_rsp,
					rxSar2rxEng_upd_rsp,
					txSar2rxEng_upd_rsp,
					rxBufferWriteStatus,
					rxBufferWriteData,
					rxEng2sLookup_req,
					rxEng2stateTable_upd_req,
					rxEng2portTable_req,
					rxEng2rxSar_upd_req,
					rxEng2txSar_upd_req,
					rxEng2timer_clearRetransmitTimer,
					rxEng2timer_setCloseTimer,
					openConStatusOut, //TODO remove
					rxEng2eventEng_setEvent,
					rxBufferWriteCmd,
					rxEng2rxApp_notification);
		simPortTable(rxEng2portTable_req, portTable2rxEng_rsp);
		simSlookup(rxEng2sLookup_req, sLookup2rxEng_rsp);
		simStateTable(rxEng2stateTable_upd_req, stateTable2rxEng_upd_rsp);
		simRxSar(rxEng2rxSar_upd_req, rxSar2rxEng_upd_rsp);
		simTxSar(rxEng2txSar_upd_req, txSar2rxEng_upd_rsp);
	}


	int count = 0;
	while (count < 100)
	{
		rx_engine(	ipRxData,
					sLookup2rxEng_rsp,
					stateTable2rxEng_upd_rsp,
					portTable2rxEng_rsp,
					rxSar2rxEng_upd_rsp,
					txSar2rxEng_upd_rsp,
					rxBufferWriteStatus,
					rxBufferWriteData,
					rxEng2sLookup_req,
					rxEng2stateTable_upd_req,
					rxEng2portTable_req,
					rxEng2rxSar_upd_req,
					rxEng2txSar_upd_req,
					rxEng2timer_clearRetransmitTimer,
					rxEng2timer_setCloseTimer,
					openConStatusOut, //TODO remove
					rxEng2eventEng_setEvent,
					rxBufferWriteCmd,
					rxEng2rxApp_notification);
		simPortTable(rxEng2portTable_req, portTable2rxEng_rsp);
		simSlookup(rxEng2sLookup_req, sLookup2rxEng_rsp);
		simStateTable(rxEng2stateTable_upd_req, stateTable2rxEng_upd_rsp);
		simRxSar(rxEng2rxSar_upd_req, rxSar2rxEng_upd_rsp);
		simTxSar(rxEng2txSar_upd_req, txSar2rxEng_upd_rsp);
		count++;
	}

	while (!(rxBufferWriteData.empty()))
	{
		rxBufferWriteData.read(outData);
		outputFile << std::hex << std::noshowbase;
		outputFile << std::setfill('0');
		outputFile << std::setw(8) << ((uint32_t) outData.data(63, 32));
		outputFile << std::setw(8) << ((uint32_t) outData.data(31, 0));
		outputFile << " " << std::setw(2) << ((uint32_t) outData.keep) << " ";
		outputFile << std::setw(1) << ((uint32_t) outData.last);
		outputFile << std::endl;
	}

	extendedEvent ev;
	while (!rxEng2eventEng_setEvent.empty())
	{
		rxEng2eventEng_setEvent.read(ev);
		std::cout << ev.sessionID << "\t" << ev.type << std::endl;
	}

	return 0;
}
