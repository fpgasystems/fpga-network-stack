/*
 * Copyright (c) 2018, Systems Group, ETH Zurich
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
#include "iperf_udp_client.hpp"
#include <iostream>

void client(hls::stream<ipUdpMeta>&	txMetaData,
			hls::stream<axiWord>& txData,
			hls::stream<bool>&			startSignal,
			hls::stream<bool>&			stopSignal,
			hls::stream<bool>&			doneSignalFifo,
			ap_uint<1>		runExperiment,
			ap_uint<128>	regTargetIpAddress,
			ap_uint<8>		regPacketGap)
{
#pragma HLS PIPELINE II=1

	enum iperfFsmStateType {IDLE, START, START_PKG, START_PKG2, START_PKG3, WRITE_PKG, CHECK_TIME, WAIT_RESPONSE, PKG_GAP};
	static iperfFsmStateType iperfFsmState = IDLE;
	static bool timeOver = false;
	static bool lastPkg = false;
	static bool receivedResponse = false;
	static ap_uint<8> wordCount = 0;
	static ap_uint<32> seqNumber = 0;
	static ap_uint<32> microsecondCounter = 0;
	static ap_uint<32> secondCounter = 0;
	static ap_uint<32> cycleCounter = 0;
	static ap_uint<32> waitCounter = 0;
	static ap_uint<8> packetGapCounter = 0;

	const ap_uint<16> theirPort = 0x1389;
	axiWord currWord;

	/*
	 * CLIENT FSM
	 */
	cycleCounter++;
	if (cycleCounter == 156250)
	{
		microsecondCounter++;
	}
	if (cycleCounter == 156250000)
	{
		secondCounter++;
	}


	switch (iperfFsmState)
	{
	case IDLE:
		//done do nothing
		timeOver = false;
		lastPkg = false;
		receivedResponse = false;
		if (runExperiment)
		{
			seqNumber = 0;
			iperfFsmState = START;
		}
		break;
	case START:
		{
			startSignal.write(true);
			secondCounter = 0;
			microsecondCounter = 0;
			iperfFsmState = START_PKG;
		}
		break;
	case START_PKG:
		txMetaData.write(ipUdpMeta(regTargetIpAddress, theirPort, MY_PORT, PKG_SIZE));
		if (!lastPkg)
		{
			currWord.data(7,0) = seqNumber(31, 24);
			currWord.data(15,8) = seqNumber(23, 16);
			currWord.data(23,16) = seqNumber(15, 8);
			currWord.data(31,24) = seqNumber(7, 0);
			seqNumber++;
		}
		else
		{
			currWord.data[7] = 1;
			currWord.data(6,0) = seqNumber(30, 24);
			currWord.data(15,8) = seqNumber(23, 16);
			currWord.data(23,16) = seqNumber(15, 8);
			currWord.data(31,24) = seqNumber(7, 0);
		}
		currWord.data(39,32) = secondCounter(31, 24);
		currWord.data(47,40) = secondCounter(23, 16);
		currWord.data(55,48) = secondCounter(15, 8);
		currWord.data(63,56) = secondCounter(7, 0);
		currWord.keep = 0xff;
		currWord.last = 0;
		txData.write(currWord);
		wordCount = 1;
		iperfFsmState = START_PKG2;
		break;
	case START_PKG2:
		currWord.data(7,0) = microsecondCounter(31, 24);
		currWord.data(15,8) = microsecondCounter(23, 16);
		currWord.data(23,16) = microsecondCounter(15, 8);
		currWord.data(31,24) = microsecondCounter(7, 0);
		currWord.data(63, 32) = 0x35343332;
		currWord.keep = 0xff;
		currWord.last = 0;
		txData.write(currWord);
		wordCount++;
		iperfFsmState = START_PKG3;
		break;
	case START_PKG3:
		currWord.data(31,0) = 0;
		currWord.data(63, 32) = 0x33323130;
		currWord.keep = 0xff;
		currWord.last = 0;
		txData.write(currWord);
		wordCount++;
		iperfFsmState = WRITE_PKG;
		break;
	case WRITE_PKG:
		if (wordCount < PKG_WORDS)
		{
			wordCount++;
			currWord.data = 0x3736353433323130;
			currWord.keep = 0xff;
			currWord.last = (wordCount == (PKG_WORDS));
			txData.write(currWord);
			if (currWord.last)
			{
				if (lastPkg)
				{
					std::cout << "microseconds: " << std::dec << microsecondCounter << std::endl;
					std::cout << "cycles: " << std::dec << cycleCounter << std::endl;
					if (receivedResponse)
					{
						iperfFsmState = IDLE;
					}
					else
					{
						iperfFsmState = WAIT_RESPONSE;
					}
				}
				else
				{
					iperfFsmState = CHECK_TIME;
				}
			}
		}
		break;
	case CHECK_TIME:
		if (timeOver)
		{
			lastPkg = true;
		}
		//else
		{
			wordCount = 0;
			if (regPacketGap != 0)
			{
				iperfFsmState = PKG_GAP;
			}
			else
			{
				iperfFsmState = START_PKG;
			}
		}
		break;
	case WAIT_RESPONSE:
		waitCounter++;
		if (waitCounter == END_TIME_100ms)
		{
			waitCounter = 0;
			iperfFsmState = START_PKG;
		}
		break;
	case PKG_GAP:
		wordCount++;
		if(wordCount == PKG_WORDS)
		{
			wordCount = 0;
			packetGapCounter++;
			if (packetGapCounter == regPacketGap)
			{
				packetGapCounter = 0;
				iperfFsmState = START_PKG;
			}

		}
		break;
	}

	//clock
	if (!stopSignal.empty())
	{
		stopSignal.read(timeOver);
	}
	if (!doneSignalFifo.empty())
	{
		doneSignalFifo.read(receivedResponse);
	}
}

void clock (hls::stream<bool>&	startSignal,
			hls::stream<bool>&	stopSignal)
{
#pragma HLS PIPELINE II=1

	static bool startClock = false;
	static ap_uint<40> time;

	if (startClock)
	{
		if (time < END_TIME_5s)
		{
			time++;
		}
		else
		{
			startClock = false;
			stopSignal.write(true);
		}
	}
	else
	{
		time = 0;
	}

	if (!startSignal.empty())
	{
		startSignal.read(startClock);
	}
}

void check_for_response(hls::stream<ipUdpMeta>&	rxMetaData,
						hls::stream<axiWord>&	rxData,
						hls::stream<bool>&		doneSignalFifo)
{
	ipUdpMeta meta;

	if (!rxMetaData.empty())
	{
		rxMetaData.read(meta);
		if (meta.my_port == MY_PORT)
		{
			doneSignalFifo.write(true);
		}
	}
	if (!rxData.empty())
	{
		rxData.read();
	}
}


void iperf_udp_client(	hls::stream<ipUdpMeta>&	rxMetaData,
						hls::stream<axiWord>&	rxData,
						hls::stream<ipUdpMeta>&	txMetaData,
						hls::stream<axiWord>&	txData,
						ap_uint<1>		runExperiment,
						ap_uint<128>	regTargetIpAddress,
						ap_uint<8>		regPacketGap)
{
	#pragma HLS DATAFLOW
	#pragma HLS INTERFACE ap_ctrl_none port=return

	#pragma HLS resource core=AXI4Stream variable=rxMetaData metadata="-bus_bundle s_axis_rx_metadata"
	#pragma HLS resource core=AXI4Stream variable=rxData metadata="-bus_bundle s_axis_rx_data"
	#pragma HLS resource core=AXI4Stream variable=txMetaData metadata="-bus_bundle m_axis_tx_metadata"
	#pragma HLS resource core=AXI4Stream variable=txData metadata="-bus_bundle m_axis_tx_data"
	#pragma HLS DATA_PACK variable=rxMetaData
	#pragma HLS DATA_PACK variable=txMetaData

	#pragma HLS INTERFACE ap_stable register port=runExperiment
	//#pragma HLS INTERFACE ap_none register port=regMyIpAddress
	#pragma HLS INTERFACE ap_stable register port=regTargetIpAddress
	#pragma HLS INTERFACE ap_stable register port=regPacketGap


	static hls::stream<bool>		startSignalFifo("startSignalFifo");
	static hls::stream<bool>		stopSignalFifo("stopSignalFifo");
	static hls::stream<bool>		doneSignalFifo("doneSignalFifo");
	#pragma HLS STREAM variable=startSignalFifo depth=2
	#pragma HLS STREAM variable=stopSignalFifo depth=2
	#pragma HLS STREAM variable=doneSignalFifo depth=2

	/*
	 * Client
	 */
	client(	txMetaData,
			txData,
			startSignalFifo,
			stopSignalFifo,
			doneSignalFifo,
			runExperiment,
			//regMyIpAddress,
			regTargetIpAddress,
			regPacketGap);

	/*
	 * Check Response
	 */
	check_for_response(rxMetaData, rxData, doneSignalFifo);

	/*
	 * Clock
	 */
	clock( startSignalFifo,
			stopSignalFifo);
}
