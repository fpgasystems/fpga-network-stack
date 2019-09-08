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
#include "toe_config.hpp"
#include "toe.hpp"
#include "dummy_memory.hpp"
#include "session_lookup_controller/session_lookup_controller.hpp"
#include <map>
#include <string>

#define noOfTxSessions 0 // Number of Tx Sessions to open for testing
#define totalSimCycles 2500000

using namespace std;

uint32_t cycleCounter;
unsigned int	simCycleCounter		= 0;

void sessionLookupStub(stream<rtlSessionLookupRequest>& lup_req, stream<rtlSessionLookupReply>& lup_rsp,
						stream<rtlSessionUpdateRequest>& upd_req, stream<rtlSessionUpdateReply>& upd_rsp) {
						//stream<ap_uint<14> >& new_id, stream<ap_uint<14> >& fin_id)
	static map<threeTupleInternal, ap_uint<14> > lookupTable;

	rtlSessionLookupRequest request;
	rtlSessionUpdateRequest update;

	map<threeTupleInternal, ap_uint<14> >::const_iterator findPos;

	if (!lup_req.empty()) {
		lup_req.read(request);
		findPos = lookupTable.find(request.key);
		if (findPos != lookupTable.end()) //hit
			lup_rsp.write(rtlSessionLookupReply(true, findPos->second, request.source));
		else
			lup_rsp.write(rtlSessionLookupReply(false, request.source));
	}

	if (!upd_req.empty()) {	//TODO what if element does not exist
		upd_req.read(update);
		if (update.op == INSERT) {	//Is there a check if it already exists?
			// Read free id
			//new_id.read(update.value);
			lookupTable[update.key] = update.value;
			upd_rsp.write(rtlSessionUpdateReply(update.value, INSERT, update.source));

		}
		else {	// DELETE
			//fin_id.write(update.value);
			lookupTable.erase(update.key);
			upd_rsp.write(rtlSessionUpdateReply(update.value, DELETE, update.source));
		}
	}
}

// Use Dummy Memory
template <int WIDTH>
void simulateRx(dummyMemory<WIDTH>* memory, stream<mmCmd>& WriteCmdFifo,  stream<mmStatus>& WriteStatusFifo, stream<mmCmd>& ReadCmdFifo,
					stream<net_axis<WIDTH> >& BufferIn, stream<net_axis<WIDTH> >& BufferOut) {
	mmCmd cmd;
	mmStatus status;
	net_axis<WIDTH> inWord = net_axis<WIDTH>(0, 0, 0);
	net_axis<WIDTH> outWord = net_axis<WIDTH>(0, 0, 0);
	static uint32_t rxMemCounter = 0;
	static uint32_t rxMemCounterRd = 0;

	static bool stx_write = false;
	static bool stx_read = false;

	static bool stx_readCmd = false;
	static ap_uint<16> wrBufferWriteCounter = 0;
	static ap_uint<16> wrBufferReadCounter = 0;

	if (!WriteCmdFifo.empty() && !stx_write) {
		WriteCmdFifo.read(cmd);
		memory->setWriteCmd(cmd);
		//cerr << dec << "mem write cmd " << cmd.saddr << ", " << cmd.bbt << std::endl;
		wrBufferWriteCounter = cmd.bbt;
		stx_write = true;
	}
	else if (!BufferIn.empty() && stx_write) {
		BufferIn.read(inWord);
		//cerr << dec << rxMemCounter << " - " << hex << inWord.data << " " << inWord.keep << " " << inWord.last << " " << dec << wrBufferWriteCounter << endl;
		//rxMemCounter++;
		memory->writeWord(inWord);
		if (wrBufferWriteCounter < (WIDTH/8)) {
			//fake_txBuffer.write(inWord); // RT hack
			stx_write = false;
			status.okay = 1;
			WriteStatusFifo.write(status);
			//cerr << "write status" << endl;
		}
		else
			wrBufferWriteCounter -= (WIDTH/8);
	}
	if (!ReadCmdFifo.empty() && !stx_read) {
		ReadCmdFifo.read(cmd);
		memory->setReadCmd(cmd);
		wrBufferReadCounter = cmd.bbt;
		stx_read = true;
	}
	else if(stx_read) {
		memory->readWord(outWord);
		BufferOut.write(outWord);
		//cerr << dec << rxMemCounterRd << " - " << hex << outWord.data << " " << outWord.keep << " " << outWord.last << endl;
		rxMemCounterRd++;;
		if (wrBufferReadCounter < (WIDTH/8))
			stx_read = false;
		else
			wrBufferReadCounter -= (WIDTH/8);
	}
}



template <int WIDTH>
void simulateTx(dummyMemory<WIDTH>* memory, stream<mmCmd>& WriteCmdFifo,  stream<mmStatus>& WriteStatusFifo, stream<mmCmd>& ReadCmdFifo,
					stream<net_axis<WIDTH> >& BufferIn, stream<net_axis<WIDTH> >& BufferOut) {
	mmCmd cmd;
	mmStatus status;
	net_axis<WIDTH> inWord;
	net_axis<WIDTH> outWord;

	static bool stx_write = false;
	static bool stx_read = false;

	static bool stx_readCmd = false;

	if (!WriteCmdFifo.empty() && !stx_write) {
		WriteCmdFifo.read(cmd);
		//cerr << "WR: " << dec << cycleCounter << hex << " - " << cmd.saddr << " - " << cmd.bbt << endl;
		memory->setWriteCmd(cmd);
		stx_write = true;
	}
	else if (!BufferIn.empty() && stx_write) {
		BufferIn.read(inWord);
		//cerr << "Data: " << dec << cycleCounter << hex << inWord.data << " - " << inWord.keep << " - " << inWord.last << endl;
		memory->writeWord(inWord);
		if (inWord.last) {
			//fake_txBuffer.write(inWord); // RT hack
			stx_write = false;
			status.okay = 1;
			WriteStatusFifo.write(status);
		}
	}
	if (!ReadCmdFifo.empty() && !stx_read) {
		ReadCmdFifo.read(cmd);
		//cerr << "RD: " << cmd.saddr << " - " << cmd.bbt << endl;
		memory->setReadCmd(cmd);
		stx_read = true;
	}
	else if(stx_read) {
		memory->readWord(outWord);
		//cerr << inWord.data << " " << inWord.last << " - ";
		BufferOut.write(outWord);
		if (outWord.last)
			stx_read = false;
	}
}

void iperf(	stream<ap_uint<16> >& listenPort, stream<bool>& listenPortStatus,
			// This is disabled for the time being, because it adds complexity/potential issues
			//stream<ap_uint<16> >& closePort,
			stream<appNotification>& notifications, stream<appReadRequest>& readRequest,
			stream<ap_uint<16> >& rxMetaData, stream<net_axis<64> >& rxData, stream<net_axis<64> >& rxDataOut,
			stream<ipTuple>& openConnection, stream<openStatus>& openConStatus,
			stream<ap_uint<16> >& closeConnection, vector<ap_uint<16> > &txSessionIDs) {

	static bool listenDone = false;
	static bool runningExperiment = false;
	static ap_uint<1> listenFsm = 0;

	openStatus newConStatus;
	appNotification notification;
	ipTuple tuple;

	// Open Port 5001
	if (!listenDone) {
		switch (listenFsm) {
		case 0:
			listenPort.write(0x57);
			listenFsm++;
			break;
		case 1:
			if (!listenPortStatus.empty()) {
				listenPortStatus.read(listenDone);
				listenFsm++;
			}
			break;
		}
	}

	net_axis<64> transmitWord;
	// In case we are connecting back
	if (!openConStatus.empty()) {
		openStatus tempStatus = openConStatus.read();
		if(tempStatus.success)
			txSessionIDs.push_back(tempStatus.sessionID);
	}

	if (!notifications.empty())	{
		notifications.read(notification);

		if (notification.length != 0)
			readRequest.write(appReadRequest(notification.sessionID, notification.length));
		else // closed
			runningExperiment = false;
	}

	enum consumeFsmStateType {WAIT_PKG, CONSUME, HEADER_2, HEADER_3};
	static consumeFsmStateType  serverFsmState = WAIT_PKG;
	ap_uint<16> sessionID;
	net_axis<64> currWord;
	currWord.last = 0;
	static bool dualTest = false;
	static ap_uint<32> mAmount = 0;

	switch (serverFsmState)	{
	case WAIT_PKG:
		if (!rxMetaData.empty() && !rxData.empty())	{
			rxMetaData.read(sessionID);
			rxData.read(currWord);
			rxDataOut.write(currWord);
			if (!runningExperiment) {
				if (currWord.data(31, 0) == 0x00000080) // Dual test
					dualTest = true;
				else
					dualTest = false;

				runningExperiment = true;
				serverFsmState = HEADER_2;
			}
			else
				serverFsmState = CONSUME;
		}
		break;
	case HEADER_2:
		if (!rxData.empty()) {
			rxData.read(currWord);
			rxDataOut.write(currWord);
			if (dualTest) {
				tuple.ip_address = 0x0a010101;
				tuple.ip_port = currWord.data(31, 16);
				openConnection.write(tuple);
			}
			serverFsmState = HEADER_3;
		}
		break;
	case HEADER_3:
		if (!rxData.empty()) {
			rxData.read(currWord);
			rxDataOut.write(currWord);
			mAmount = currWord.data(63, 32);
			serverFsmState = CONSUME;
		}
		break;
	case CONSUME:
		if (!rxData.empty()) {
			rxData.read(currWord);
			rxDataOut.write(currWord);
		}
		break;
	}
	if (currWord.last == 1)
		serverFsmState = WAIT_PKG;
}

string decodeApUint64(ap_uint<64> inputNumber) {
	string 				outputString	= "0000000000000000";
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

	for (int i = 0; i < 64/8; i++)
	{
		uint16_t temp;
		std::stringstream parser(dataString.substr(i*2, 2));
		parser >> std::hex >> temp;
		tempOutput(63-(i*8), 56-(i*8)) = temp;
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

ap_uint<16> checksumComputation(deque<net_axis<64> >	pseudoHeader) {
	ap_uint<32> tcpChecksum = 0;
	for (uint8_t i=0;i<pseudoHeader.size();++i) {
		ap_uint<64> tempInput = (pseudoHeader[i].data.range(7, 0), pseudoHeader[i].data.range(15, 8), pseudoHeader[i].data.range(23, 16), pseudoHeader[i].data.range(31, 24), pseudoHeader[i].data.range(39, 32), pseudoHeader[i].data.range(47, 40), pseudoHeader[i].data.range(55, 48), pseudoHeader[i].data.range(63, 56));
		//cerr << hex << tempInput << " " << pseudoHeader[i].data << endl;
		tcpChecksum = ((((tcpChecksum + tempInput.range(63, 48)) + tempInput.range(47, 32)) + tempInput.range(31, 16)) + tempInput.range(15, 0));
		tcpChecksum = (tcpChecksum & 0xFFFF) + (tcpChecksum >> 16);
		tcpChecksum = (tcpChecksum & 0xFFFF) + (tcpChecksum >> 16);
	}
//	tcpChecksum = tcpChecksum.range(15, 0) + tcpChecksum.range(19, 16);
	tcpChecksum = ~tcpChecksum;			// Reverse the bits of the result
	return tcpChecksum.range(15, 0);	// and write it into the output
}


//// This version does not work for TCP segments that are too long... overflow happens
//ap_uint<16> checksumComputation(deque<net_axis<64> >	pseudoHeader) {
//	ap_uint<20> tcpChecksum = 0;
//	for (uint8_t i=0;i<pseudoHeader.size();++i) {
//		ap_uint<64> tempInput = (pseudoHeader[i].data.range(7, 0), pseudoHeader[i].data.range(15, 8), pseudoHeader[i].data.range(23, 16), pseudoHeader[i].data.range(31, 24), pseudoHeader[i].data.range(39, 32), pseudoHeader[i].data.range(47, 40), pseudoHeader[i].data.range(55, 48), pseudoHeader[i].data.range(63, 56));
//		//cerr << hex << tempInput << " " << pseudoHeader[i].data << endl;
//		tcpChecksum = ((((tcpChecksum + tempInput.range(63, 48)) + tempInput.range(47, 32)) + tempInput.range(31, 16)) + tempInput.range(15, 0));
//	}
//	tcpChecksum = tcpChecksum.range(15, 0) + tcpChecksum.range(19, 16);
//	tcpChecksum = ~tcpChecksum;			// Reverse the bits of the result
//	return tcpChecksum.range(15, 0);	// and write it into the output
//}

ap_uint<16> recalculateChecksum(deque<net_axis<64> > inputPacketizer) {
	ap_uint<16> newChecksum = 0;
	// Create the pseudo-header
	ap_uint<16> tcpLength 					= (inputPacketizer[0].data.range(23, 16), inputPacketizer[0].data.range(31, 24)) - 20;
	inputPacketizer[0].data					= (inputPacketizer[2].data.range(31, 0), inputPacketizer[1].data.range(63, 32));
	inputPacketizer[1].data.range(15, 0)	= 0x0600;
	inputPacketizer[1].data.range(31, 16)	= (tcpLength.range(7, 0), tcpLength(15, 8));
	inputPacketizer[4].data.range(47, 32)	= 0x0;
	//ap_uint<32> temp	= (tcpLength, 0x0600);
	inputPacketizer[1].data.range(63, 32) 	= inputPacketizer[2].data.range(63, 32);
	for (uint8_t i=2;i<inputPacketizer.size() -1;++i)
		inputPacketizer[i]= inputPacketizer[i+1];
	inputPacketizer.pop_back();
	//for (uint8_t i=0;i<inputPacketizer.size();++i)
	//	cerr << hex << inputPacketizer[i].data << " " << inputPacketizer[i].keep << endl;
	//newChecksum = checksumComputation(inputPacketizer); 		// Calculate the checksum
	//return newChecksum;
	return checksumComputation(inputPacketizer);
}

short int injectAckNumber(deque<net_axis<64> > &inputPacketizer, map<fourTuple, ap_uint<32> > &sessionList) {
	fourTuple newTuple = fourTuple(inputPacketizer[1].data.range(63, 32), inputPacketizer[2].data.range(31, 0), inputPacketizer[2].data.range(47, 32), inputPacketizer[2].data.range(63, 48));
	//cerr << hex << inputPacketizer[4].data << endl;
	if (inputPacketizer[4].data.bit(9))	{													// If this packet is a SYN there's no need to inject anything
		//cerr << " Open Tuple: " << hex << inputPacketizer[1].data.range(63, 32) << " - " << inputPacketizer[2].data.range(31, 0) << " - " << inputPacketizer[2].data.range(47, 32) << " - " << inputPacketizer[2].data.range(63, 48) << endl;
		if (sessionList.find(newTuple) != sessionList.end()) {
			cerr << "WARNING: Trying to open an existing session! - " << simCycleCounter << endl;
			return -1;
		}
		else {
			sessionList[newTuple] = 0;
			cerr << "INFO: Opened new session - " << simCycleCounter << endl;
			return 0;
		}
	}
	else {																					// if it is any other packet
		if (sessionList.find(newTuple) != sessionList.end()) {
			inputPacketizer[3].data.range(63, 32) 	= sessionList[newTuple];				// inject the oldest acknowldgement number in the ack number deque
			//cerr << hex << "Input: " << inputPacketizer[3].data.range(63, 32) << endl;
			ap_uint<16> tempChecksum = recalculateChecksum(inputPacketizer);
			//inputPacketizer[4].data.range(47, 32) 	= recalculateChecksum(inputPacketizer);	// and recalculate the checksum
			inputPacketizer[4].data.range(47, 32) = (tempChecksum.range(7, 0), tempChecksum(15, 8));
			//inputPacketizer[4].data.range(47, 32) 	= 0xB13D;
			//for (uint8_t i=0;i<inputPacketizer.size();++i)
			//	cerr << hex << inputPacketizer[i].data << endl;
			return 1;
		}
		else {
			cerr << "WARNING: Trying to send data to a non-existing session!" << endl;
			return -1;
		}
	}
}

bool parseOutputPacket(deque<net_axis<64> > &outputPacketizer, map<fourTuple, ap_uint<32> > &sessionList, deque<net_axis<64> > &inputPacketizer) {		// Looks for an ACK packet in the output stream and when found if stores the ackNumber from that packet into
// the seqNumbers deque and clears the deque containing the output packet.
	bool returnValue = false;
	bool finPacket = false;
	static int pOpacketCounter = 0;
	static ap_uint<32> oldSeqNumber = 0;
	if (outputPacketizer[4].data.bit(9) && !outputPacketizer[4].data.bit(12)) { // Check if this is a SYN packet and if so reply with a SYN-ACK
		inputPacketizer.push_back(outputPacketizer[0]);
		ap_uint<32> ipBuffer = outputPacketizer[1].data.range(63, 32);
		outputPacketizer[1].data.range(63, 32) = outputPacketizer[2].data.range(31, 0);
		inputPacketizer.push_back(outputPacketizer[1]);
		outputPacketizer[2].data.range(31, 0) = ipBuffer;
		ap_uint<16> portBuffer = outputPacketizer[2].data.range(47, 32);
		outputPacketizer[2].data.range(47, 32) = outputPacketizer[2].data.range(63, 48);
		outputPacketizer[2].data.range(63, 48) = portBuffer;
		inputPacketizer.push_back(outputPacketizer[2]);
		ap_uint<32> reversedSeqNumber = (outputPacketizer[3].data.range(7,0), outputPacketizer[3].data.range(15, 8), outputPacketizer[3].data.range(23, 16), outputPacketizer[3].data.range(31, 24)) + 1;
		reversedSeqNumber = (reversedSeqNumber.range(7, 0), reversedSeqNumber.range(15, 8), reversedSeqNumber.range(23, 16), reversedSeqNumber.range(31, 24));
		outputPacketizer[3].data.range(31, 0) = outputPacketizer[3].data.range(63, 32);
		outputPacketizer[3].data.range(63, 32) = reversedSeqNumber;
		inputPacketizer.push_back(outputPacketizer[3]);
		outputPacketizer[4].data.bit(12) = 1;												// Set the ACK bit
		ap_uint<16> tempChecksum = recalculateChecksum(outputPacketizer);
		outputPacketizer[4].data.range(47, 32) = (tempChecksum.range(7, 0), tempChecksum(15, 8));
		//inputPacketizer.push_back(outputPacketizer[4]);
		/*cerr << hex << outputPacketizer[0].data << endl;
		cerr << hex << outputPacketizer[1].data << endl;
		cerr << hex << outputPacketizer[2].data << endl;
		cerr << hex << outputPacketizer[3].data << endl;
		cerr << hex << outputPacketizer[4].data << endl;*/
	}
	else if (outputPacketizer[4].data.bit(8) && !outputPacketizer[4].data.bit(12)) 	// If the FIN bit is set but without the ACK bit being set at the same time
		sessionList.erase(fourTuple(outputPacketizer[1].data.range(63, 32), outputPacketizer[2].data.range(31, 0), outputPacketizer[2].data.range(47, 32), outputPacketizer[2].data.range(63, 48))); // Erase the tuple for this session from the map
	else if (outputPacketizer[4].data.bit(12)) { // If the ACK bit is set
		uint16_t packetLength = reverse((ap_uint<16>) outputPacketizer[0].data.range(31, 16));
		ap_uint<32> reversedSeqNumber = (outputPacketizer[3].data.range(7,0), outputPacketizer[3].data.range(15, 8), outputPacketizer[3].data.range(23, 16), outputPacketizer[3].data.range(31, 24));
		if (outputPacketizer[4].data.bit(9) || outputPacketizer[4].data.bit(8))
			reversedSeqNumber++;
		if (packetLength >= 40) {
			packetLength -= 40;
			reversedSeqNumber += packetLength;
		}
		reversedSeqNumber = (reversedSeqNumber.range(7, 0), reversedSeqNumber.range(15, 8), reversedSeqNumber.range(23, 16), reversedSeqNumber.range(31, 24));
		fourTuple packetTuple = fourTuple(outputPacketizer[2].data.range(31, 0), outputPacketizer[1].data.range(63, 32), outputPacketizer[2].data.range(63, 48), outputPacketizer[2].data.range(47, 32));
		sessionList[packetTuple] = reversedSeqNumber;
		returnValue = true;
		if (outputPacketizer[4].data.bit(8)) {	// This might be a FIN segment at the same time. In this case erase the session from the list
			uint8_t itemsErased = sessionList.erase(fourTuple(outputPacketizer[2].data.range(31, 0), outputPacketizer[1].data.range(63, 32), outputPacketizer[2].data.range(63, 48), outputPacketizer[2].data.range(47, 32))); // Erase the tuple for this session from the map
			finPacket = true;
			//cerr << "Close Tuple: " << hex << outputPacketizer[2].data.range(31, 0) << " - " << outputPacketizer[1].data.range(63, 32) << " - " << inputPacketizer[2].data.range(63, 48) << " - " << outputPacketizer[2].data.range(47, 32) << endl;
			if (itemsErased != 1)
				cerr << "WARNING: Received FIN segment for a non-existing session - " << simCycleCounter << endl;
			else
				cerr << "INFO: Session closed successfully - " << simCycleCounter << endl;
		}
		// Check if the ACK packet also constains data. If it does generate an ACK for. Look into the IP header length for this.
		if (packetLength > 0 || finPacket == true) { // 20B of IP Header & 20B of TCP Header since we never generate options
			finPacket = false;
			outputPacketizer[0].data.range(31, 16) = 0x2800;
			inputPacketizer.push_back(outputPacketizer[0]);
			ap_uint<32> ipBuffer = outputPacketizer[1].data.range(63, 32);
			outputPacketizer[1].data.range(63, 32) = outputPacketizer[2].data.range(31, 0);
			inputPacketizer.push_back(outputPacketizer[1]);
			outputPacketizer[2].data.range(31, 0) = ipBuffer;
			ap_uint<16> portBuffer = outputPacketizer[2].data.range(47, 32);
			outputPacketizer[2].data.range(47, 32) = outputPacketizer[2].data.range(63, 48);
			outputPacketizer[2].data.range(63, 48) = portBuffer;
			inputPacketizer.push_back(outputPacketizer[2]);
			//ap_uint<32> seqNumber = outputPacketizer[3].data.range(31, 0);
			outputPacketizer[3].data.range(31, 0) = outputPacketizer[3].data.range(63, 32);
			outputPacketizer[3].data.range(63, 32) = reversedSeqNumber;
			//cerr << hex << outputPacketizer[3].data.range(63, 32) << " - " << reversedSeqNumber << endl;
			inputPacketizer.push_back(outputPacketizer[3]);
			outputPacketizer[4].data.bit(12) = 1;												// Set the ACK bit
			outputPacketizer[4].data.bit(8) = 0;												// Unset the FIN bit
			ap_uint<16> tempChecksum = recalculateChecksum(outputPacketizer);
			outputPacketizer[4].data.range(47, 32) = (tempChecksum.range(7, 0), tempChecksum(15, 8));
			outputPacketizer[4].keep = 0x3F;
			outputPacketizer[4].last = 1;
			inputPacketizer.push_back(outputPacketizer[4]);
			/*cerr << std::hex << outputPacketizer[0].data << " - " << outputPacketizer[0].keep << " - " << outputPacketizer[0].last << endl;
			cerr << std::hex << outputPacketizer[1].data << " - " << outputPacketizer[1].keep << " - " << outputPacketizer[1].last << endl;
			cerr << std::hex << outputPacketizer[2].data << " - " << outputPacketizer[2].keep << " - " << outputPacketizer[2].last << endl;
			cerr << std::hex << outputPacketizer[3].data << " - " << outputPacketizer[3].keep << " - " << outputPacketizer[3].last << endl;
			cerr << std::hex << outputPacketizer[4].data << " - " << outputPacketizer[4].keep << " - " << outputPacketizer[4].last << endl;*/
			if (oldSeqNumber != reversedSeqNumber) {
				pOpacketCounter++;
				//cerr << "ACK cnt: " << dec << pOpacketCounter << hex << " - " << outputPacketizer[3].data.range(63, 32) << endl;
			}
			oldSeqNumber = reversedSeqNumber;
		}
		//cerr << hex << "Output: " << outputPacketizer[3].data.range(31, 0) << endl;
	}
	outputPacketizer.clear();
	return returnValue;
}

void flushInputPacketizer(deque<net_axis<64> > &inputPacketizer, stream<net_axis<64> > &ipRxData, map<fourTuple, ap_uint<32> > &sessionList) {
	//std::cout << "input packetizer: " << inputPacketizer.size() << std::endl;
	if (inputPacketizer.size() != 0) {
		injectAckNumber(inputPacketizer, sessionList);
		uint8_t inputPacketizerSize = inputPacketizer.size();
		for (uint8_t i=0;i<inputPacketizerSize;++i) {
			net_axis<64> temp = inputPacketizer.front();
			ipRxData.write(temp);
			inputPacketizer.pop_front();
		}
	}
	//std::cout << "after input packetizer: " << inputPacketizer.size() << std::endl;
}

vector<string> parseLine(string stringBuffer) {
	vector<string> tempBuffer;
	bool found = false;

	while (stringBuffer.find(" ") != string::npos)	// Search for spaces delimiting the different data words
	{
		string temp = stringBuffer.substr(0, stringBuffer.find(" "));							// Split the the string
		stringBuffer = stringBuffer.substr(stringBuffer.find(" ")+1, stringBuffer.length());	// into two
		tempBuffer.push_back(temp);		// and store the new part into the vector. Continue searching until no more spaces are found.
	}
	tempBuffer.push_back(stringBuffer);	// and finally push the final part of the string into the vector when no more spaces are present.
	return tempBuffer;
}

template <int W, int X>
void convertPacketWidth(hls::stream<net_axis<W> >&	input, hls::stream<net_axis<X> >& output)
{
	while(!input.empty())
	{
		convertStreamWidth<W, 24>(input, output);
	}
}

int main(int argc, char *argv[]) {
	stream<net_axis<DATA_WIDTH> >						ipRxData("ipRxData");
	stream<mmStatus>					rxBufferWriteStatus("rxBufferWriteStatus");
	stream<mmStatus>					txBufferWriteStatus("txBufferWriteStatus");
	stream<net_axis<DATA_WIDTH> >						rxBufferReadData("rxBufferReadData");
	stream<net_axis<DATA_WIDTH> >						txBufferReadData("txBufferReadData");
	stream<net_axis<DATA_WIDTH> >						ipTxData("ipTxData");
	stream<mmCmd>						rxBufferWriteCmd("rxBufferWriteCmd");
	stream<mmCmd>						rxBufferReadCmd("rxBufferReadCmd");
	stream<mmCmd>						txBufferWriteCmd("txBufferWriteCmd");
	stream<mmCmd>						txBufferReadCmd("txBufferReadCmd");
	stream<net_axis<DATA_WIDTH> >						rxBufferWriteData("rxBufferWriteData");
	stream<net_axis<DATA_WIDTH> >						txBufferWriteData("txBufferWriteData");
	stream<rtlSessionLookupReply>		sessionLookup_rsp("sessionLookup_rsp");
	stream<rtlSessionUpdateReply>		sessionUpdate_rsp("sessionUpdate_rsp");
	stream<rtlSessionLookupRequest>		sessionLookup_req("sessionLookup_req");
	stream<rtlSessionUpdateRequest>		sessionUpdate_req("sessionUpdate_req");
//	stream<rtlSessionUpdateRequest>		sessionUpdate_req;
	stream<ap_uint<16> >				listenPortReq("listenPortReq");
	stream<appReadRequest>				rxDataReq("rxDataReq");
	stream<ipTuple>						openConnReq("openConnReq");
	stream<ap_uint<16> >				closeConnReq("closeConnReq");
	stream<appTxMeta>					txDataReqMeta("txDataReqMeta");
	stream<net_axis<DATA_WIDTH> >						txDataReq("txDataReq");
	stream<bool>						listenPortRsp("listenPortRsp");
	stream<appNotification>				notification("notification");
	stream<ap_uint<16> >				rxDataRspMeta("rxDataRspMeta");
	stream<net_axis<DATA_WIDTH> >						rxDataRsp("rxDataRsp");
	stream<openStatus>					openConnRsp("openConnRsp");
	stream<appTxRsp>					txDataRsp("txDataRsp");
	ap_uint<16>							regSessionCount;
	//ap_uint<16>							relSessionCount;
	stream<net_axis<64> >						ipRxData64;
	stream<net_axis<64> >						rxDataRsp64;
	stream<net_axis<64> >						ipTxData64;
	stream<net_axis<64> >						txDataReq64;
	net_axis<64>								ipTxDataOut_Data;
	net_axis<64>								ipRxData_Data;
	net_axis<64>								ipTxDataIn_Data;
	stream<net_axis<64> > 					rxDataOut("rxDataOut");						// This stream contains the data output from the Rx App I/F
	net_axis<64>								rxDataOut_Data;								// This variable is where the data read from the stream above is temporarily stored before output

	dummyMemory<DATA_WIDTH> rxMemory;
	dummyMemory<DATA_WIDTH> txMemory;
	
	map<fourTuple, ap_uint<32> > sessionList;

	deque<net_axis<64> >	inputPacketizer;
	deque<net_axis<64> >	outputPacketizer;						// This deque collects the output data word until a whole packet is accumulated.

	ifstream	rxInputFile;
	ifstream	txInputFile;
	ofstream	rxOutput;
	ofstream	txOutput;
	ifstream	rxGold;
	ifstream	txGold;
	unsigned int	skipCounter 		= 0;
	cycleCounter = 0;
	simCycleCounter		= 0;
	unsigned int	myCounter		= 0;
	bool		idleCycle		= 0;
	string		dataString, keepString;
	vector<string> 	stringVector;
	vector<string>	txStringVector;
	bool 		firstWordFlag		= true;
	int 		rxGoldCompare		= 0;
	int			txGoldCompare		= 0; // not used yet
	int			returnValue		= 0;
	uint16_t	txWordCounter 		= 0;
	uint16_t	rxPayloadCounter	= 0; 	// This variable counts the number of packets output during the simulation on the Rx side
	uint16_t	txPacketCounter		= 0;	// This variable countrs the number of packet send from the Tx side (total number of packets, all kinds, from all sessions).
	bool		testRxPath		= true;	// This variable indicates if the Rx path is to be tested, thus it remains true in case of Rx only or bidirectional testing
	bool		testTxPath		= true;	// Same but for the Tx path.
	vector<ap_uint<16> > txSessionIDs;		// This vector holds the session IDs of the session to which data are transmitted.
	uint16_t	currentTxSessionID	= 0;	// Holds the session ID which was last used to send data into the Tx path.
	char 		mode 		= *argv[1];

	if (argc > 7 || argc < 4) {
		cout << "Expected format: mode input_file rx_output_file tx_output_file (rx_gold_output)" << endl;
//		cout << "You need to provide three parameters (the input file name followed by the output file name for the Rx Data and the one for the Tx Data)!" << endl;
		return -1;
	}
	if (mode == '0')  { // Test the Rx side only
		rxInputFile.open(argv[2]);
		if (!rxInputFile) {
			cout << " Error opening Rx Input file!" << endl;
			return -1;
		}
		rxOutput.open(argv[3]);
		if (!rxOutput) {
			cout << " Error opening Rx Output file!" << endl;
			return -1;
		}
		txOutput.open(argv[4]);
		if (!txOutput) {
			cout << " Error opening Tx Output file!" << endl;
			return -1;
		}
		if(argc == 6) {
			rxGold.open(argv[5]);
			if (!rxGold) {
				cout << " Error accessing Gold Rx file!" << endl;
				return -1;
			}
		}
		testTxPath = false;
	}
	else if (mode == '1') { // Test the Tx side only
		txInputFile.open(argv[2]);
		if (!txInputFile) {
			cout << " Error opening Tx Input file!" << endl;
			return -1;
		}
		txOutput.open(argv[3]);
		if (!txOutput) {
			cout << " Error opening Tx Output file!" << endl;
			return -1;
		}
		if(argc == 5){
			txGold.open(argv[4]);
			if (!txGold) {
				cout << " Error accessing Gold Tx file!" << endl;
				return -1;
			}
		}
		testRxPath = false;
	}
	else if (mode == '2') { // Test the Rx & Tx side together
		rxInputFile.open(argv[2]);
		if (!rxInputFile) {
			cout << " Error opening Rx Input file!" << endl;
			return -1;
		}
		txInputFile.open(argv[3]);
		if (!txInputFile) {
			cout << " Error opening Tx Input file!" << endl;
			return -1;
		}
		rxOutput.open(argv[4]);
		if (!rxOutput) {
			cout << " Error opening Rx Output file!" << endl;
			return -1;
		}
		txOutput.open(argv[5]);
		if (!txOutput) {
			cout << " Error opening Tx Output file!" << endl;
			return -1;
		}
		if(argc == 7){
			rxGold.open(argv[6]);
			if (!rxGold) {
				cout << " Error accessing Gold Rx file!" << endl;
				return -1;
			}
		}
		else if (argc == 6 ) {
			cout << "Error! Bi-directional testing requires two golden output files to be passed!" << endl;
			return -1;
		}
	}
	else { // Other cases are not allowed, exit.
		cout << "First argument can be: 0 - Rx path testing only, 1 - Tx path testing only, 2 - Bi-directional testing." << endl;
		return -1;
	}
	if (testTxPath == true) { // If the Tx Path will be tested then open a session for testing.
		for (uint8_t i=0;i<noOfTxSessions;++i) { 
			ipTuple newTuple = {150*(i+65355), 10*(i+65355)}; 		// IP address and port to open
			openConnReq.write(newTuple); 	// Write into TOE Tx I/F queue
		}
	}
//	while (simCycleCounter < totalSimCycles) {
	do {
		if (idleCycle == true) {
			if (cycleCounter >= skipCounter) {
				cycleCounter = 0;
				idleCycle = false;
			}
			else
				cycleCounter++;
		}
		else if (idleCycle == false) {
			unsigned short int temp;
			string stringBuffer;
			string txStringBuffer;
			flushInputPacketizer(inputPacketizer, ipRxData64, sessionList);			// Before processing the input file data words, write any packets generated from the B itself
			convertPacketWidth(ipRxData64, ipRxData);
			if (testRxPath == true) {
				//flushInputPacketizer(inputPacketizer, ipRxData, sessionList);			// Before processing the input file data words, write any packets generated from the B itself
				getline(rxInputFile, stringBuffer);
				stringVector = parseLine(stringBuffer);
				if (stringVector[0] == "W") {	// Take into account wait cycles in the Rx input files. If they coincide with wait cycles in the Tx input files (only in case of idirectional testing, then the Tx side ones takes precedence.)
					skipCounter = atoi(stringVector[1].c_str());
					idleCycle = true;
				}
				else if(rxInputFile.fail() == 1 || stringBuffer.empty()){
				//break;
				}
				else {
					do {
						if (firstWordFlag == false) {
							getline(rxInputFile, stringBuffer);
							stringVector = parseLine(stringBuffer);
						}
						firstWordFlag = false;
						string tempString = "0000000000000000";
						ipRxData_Data = net_axis<64>(encodeApUint64(stringVector[0]), encodeApUint8(stringVector[2]), atoi(stringVector[1].c_str()));
						inputPacketizer.push_back(ipRxData_Data);
					} while (ipRxData_Data.last != 1);
					firstWordFlag = true;
					flushInputPacketizer(inputPacketizer, ipRxData64, sessionList);
					convertPacketWidth(ipRxData64, ipRxData);
				}
			}
			if (testTxPath == true && txSessionIDs.size() > 0) {
				//flushInputPacketizer(inputPacketizer, ipRxData, sessionList);			// Before processing the input file data words, write any packets generated from the B itself
				getline(txInputFile, txStringBuffer);
				txStringVector = parseLine(txStringBuffer);
				if (txStringVector[0] == "W") {	// Only the Rx Path testing can 
					skipCounter = atoi(txStringVector[1].c_str());
					idleCycle = true;
				}
				else if(txInputFile.fail() || txStringBuffer.empty()){
					//break;
					//return 0;
				}
				else { // Send data only after a session has been opened on the Tx Side
					do {
						if (firstWordFlag == false) { // If this isn't the first word of the packet then data have to be read in from the file
							getline(txInputFile, txStringBuffer);
							txStringVector = parseLine(txStringBuffer);
						}
						else { // If this is the first word of the data then the request has to be written into the Tx App I/F
							txDataReqMeta.write(appTxMeta(txSessionIDs[currentTxSessionID], 1460)); //TODO fix this
							currentTxSessionID == noOfTxSessions - 1 ? currentTxSessionID = 0 : currentTxSessionID++;
						}
						firstWordFlag = false;
						string tempString = "0000000000000000";
						ipTxDataIn_Data = net_axis<64>(encodeApUint64(txStringVector[0]), encodeApUint8(txStringVector[2]), atoi(txStringVector[1].c_str()));
						txDataReq64.write(ipTxDataIn_Data);
					} while (ipTxDataIn_Data.last != 1);
					convertPacketWidth(txDataReq64, txDataReq);
					firstWordFlag = true;
				}
			}
//			}
		}
		toe(ipRxData, rxBufferWriteStatus, txBufferWriteStatus, rxBufferReadData, txBufferReadData, ipTxData, rxBufferWriteCmd,
			rxBufferReadCmd, txBufferWriteCmd, txBufferReadCmd, rxBufferWriteData, txBufferWriteData, sessionLookup_rsp, sessionUpdate_rsp,
			sessionLookup_req, sessionUpdate_req, listenPortReq, rxDataReq, openConnReq, closeConnReq, txDataReqMeta, txDataReq,
			//listenPortRsp, notification, rxDataRspMeta, rxDataRsp, openConnRsp, txDataRsp);
			//relSessionCount, regSessionCount);
			listenPortRsp, notification, rxDataRspMeta, rxDataRsp, openConnRsp, txDataRsp, 0x01010101, /*relSessionCount,*/ regSessionCount);

		//TODO use actual iperf
		convertStreamWidth<DATA_WIDTH, 22>(rxDataRsp, rxDataRsp64);
		iperf(listenPortReq, listenPortRsp, notification, rxDataReq,
			  rxDataRspMeta, rxDataRsp64, rxDataOut, openConnReq, openConnRsp,
			  closeConnReq, txSessionIDs);

		simulateRx<DATA_WIDTH>(&rxMemory, rxBufferWriteCmd, rxBufferWriteStatus, rxBufferReadCmd, rxBufferWriteData, rxBufferReadData);
		simulateTx<DATA_WIDTH>(&txMemory, txBufferWriteCmd, txBufferWriteStatus, txBufferReadCmd, txBufferWriteData, txBufferReadData);
		  		   sessionLookupStub(sessionLookup_req, sessionLookup_rsp,	sessionUpdate_req, sessionUpdate_rsp);

		convertStreamWidth<DATA_WIDTH,23>(ipTxData, ipTxData64);
		if (!ipTxData64.empty()) {
			ipTxData64.read(ipTxDataOut_Data);
			string dataOutput = decodeApUint64(ipTxDataOut_Data.data);
			string keepOutput = decodeApUint8(ipTxDataOut_Data.keep);
			txOutput << dataOutput << " " << ipTxDataOut_Data.last << " " << keepOutput << endl;
			outputPacketizer.push_back(ipTxDataOut_Data);
			if (ipTxDataOut_Data.last == 1) {				// If the whole packet has been written into the deque
				parseOutputPacket(outputPacketizer, sessionList, inputPacketizer);	// then parse it.
				txWordCounter = 0;
				txPacketCounter++;
				//cerr << dec << txPacketCounter << ", " << endl << hex;
			}
			else
				txWordCounter++;
		}
		if (!rxDataOut.empty()) {
			rxDataOut.read(rxDataOut_Data);
			string dataOutput = decodeApUint64(rxDataOut_Data.data);
			string keepOutput = decodeApUint8(rxDataOut_Data.keep);
//			cout << rxDataOut_Data.keep << endl;
			for (unsigned short int i = 0; i<8; ++i) { // delete the data not to be kept by "keep" - for Golden comparison
				if(rxDataOut_Data.keep[7-i] == 0){
//					cout << "rxDataOut_Data.keep[" << i << "] = " << rxDataOut_Data.keep[i] << endl;
//					cout << "dataOutput " << dataOutput[i*2] << dataOutput[(i*2)+1] << " is replaced by 00" << endl;
					dataOutput.replace(i*2,2,"00");
				}
			}
			if (rxDataOut_Data.last == 1)
				rxPayloadCounter++;
			rxOutput << dataOutput << " " << rxDataOut_Data.last << " " << keepOutput << endl;
		}
		if (!txDataRsp.empty()) {
			appTxRsp tempResp = txDataRsp.read();
			if (tempResp.error != 0)
				cerr << endl << "Warning: Attempt to write data into the Tx App I/F of the TOE was unsuccesfull. Returned error code: " << tempResp.error << endl;
		}
		simCycleCounter++;
		//cout << dec << simCycleCounter << endl;
		//cerr << simCycleCounter << " - Number of Sessions opened: " <<  dec << regSessionCount << endl << "Number of Sessions closed: " << relSessionCount << endl;
	} while (simCycleCounter < totalSimCycles);
//	while (!ipTxData.empty() || !ipRxData.empty() || !rxDataOut.empty());
	/*while (!txBufferWriteCmd.empty()) {
		mmCmd tempMemCmd = txBufferWriteCmd.read();
		std::cerr <<  "Left-over Cmd: " << std::hex << tempMemCmd.saddr << " - " << tempMemCmd.bbt << std::endl;
	}*/
	// Compare the results file with the golden results
	// only RX Gold supported for now
	float rxDividedPacketCounter = static_cast <float>(rxPayloadCounter) / 2;
	unsigned int roundedRxPayloadCounter = rxPayloadCounter / 2;
	if (rxDividedPacketCounter > roundedRxPayloadCounter)
		roundedRxPayloadCounter++;
	if (roundedRxPayloadCounter != (txPacketCounter - 1))
		cout << "WARNING: Number of received packets (" << rxPayloadCounter << ") is not equal to the number of Tx Packets (" << txPacketCounter << ")!" << endl;
	// Output Number of Sessions
	//cerr << "Number of Sessions opened: " <<  dec << regSessionCount << endl << "Number of Sessions closed: " << relSessionCount << endl;
	// Convert command line arguments to strings
	if(argc == 5) {
		vector<string> args(argc);
		for (int i=1; i<argc; ++i)
			args[i] = argv[i];

		rxGoldCompare = system(("diff --brief -w "+args[2]+" " + args[4]+" ").c_str());
		//	txGoldCompare = system(("diff --brief -w "+args[3]+" " + args[5]+" ").c_str()); // uncomment when TX Golden comparison is supported

		if (rxGoldCompare != 0){
			cout << "RX Output != Golden RX Output. Simulation FAILED." << endl;
			returnValue = 0;
		}
		else cout << "RX Output == Golden RX Output" << endl;

		//	if (txGoldCompare != 0){
		//		cout << "TX Output != Golden TX Output" << endl;
		//		returnValue = 1;
		//	}
		//	else cout << "TX Output == Golden TX Output" << endl;

		if(rxGoldCompare == 0 && txGoldCompare == 0){
			cout << "Test Passed! (Both Golden files match)" << endl;
			returnValue = 0; // Return 0 if the test passes
		}
	}
	
	if (mode == '0') { // Rx side testing only
		rxInputFile.close();
		rxOutput.close();
		txOutput.close();
		if(argc == 6)
			rxGold.close();
	}
	else if (mode == '1') { // Tx side testing only
		txInputFile.close();
		txOutput.close();
		if(argc == 5)
			txGold.close();
	}
	else if (mode == '2') { // Bi-directional testing
		rxInputFile.close();
		txInputFile.close();
		rxOutput.close();
		txOutput.close();
		if(argc == 7){
			rxGold.close();
			txGold.close();
		}
	}
	return returnValue;
}
