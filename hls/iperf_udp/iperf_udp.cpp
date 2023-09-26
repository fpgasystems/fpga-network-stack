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
#include "iperf_udp_config.hpp"
#include "iperf_udp.hpp"
#include <iostream>

template <int WIDTH>
void client(hls::stream<ipUdpMeta>&	txMetaData,
			hls::stream<net_axis<WIDTH> >& txData,
			hls::stream<bool>&			startSignal,
			hls::stream<bool>&			stopSignal,
			hls::stream<bool>&			doneSignalFifo,
			ap_uint<1>		runExperiment,
			ap_uint<8>		pkgWordCount,
			ap_uint<8>		packetGap,
			ap_uint<32>		targetIpAddress)
{
#pragma HLS PIPELINE II=1

	enum iperfFsmStateType {IDLE, START, CONSTRUCT_HEADER, HEADER, WRITE_PKG, WAIT_RESPONSE, PKG_GAP};
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
	static iperfHeader<WIDTH> header; //size 192bit
	static bool metaWritten = false;

	const ap_uint<16> theirPort = 0x1389;
	net_axis<WIDTH> currWord;

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
			iperfFsmState = CONSTRUCT_HEADER;
		}
		break;
	case CONSTRUCT_HEADER:
		header.clear();
		header.setSeqNumber(seqNumber, lastPkg);
		header.setSeconds(secondCounter);
		header.setMicroSeconds(microsecondCounter);
		seqNumber++;
		iperfFsmState = HEADER;
		break;
	case HEADER:
		if (!metaWritten)
		{
			txMetaData.write(ipUdpMeta(targetIpAddress, theirPort, MY_PORT, pkgWordCount*(WIDTH/8)));
			metaWritten = true;
		}

		if (header.consumeWord(currWord.data) < (WIDTH/8))
		{
			metaWritten = false;
			iperfFsmState = WRITE_PKG;
		}
		wordCount++;
		currWord.keep = ~0;
		//TODO handle 128 properly
      if (WIDTH == 128)
      {
			for (int i = 1; i < (WIDTH/64); i++)
         {
				#pragma HLS UNROLL
				currWord.data(i*64+63, i*64) = 0x3736353433323130;
				currWord.keep(i*8+7, i*8) = 0xff;
			}
      }
		if (WIDTH > 128)
		{
			for (int i = 3; i < (WIDTH/64); i++)
         {
				#pragma HLS UNROLL
				currWord.data(i*64+63, i*64) = 0x3736353433323130;
				currWord.keep(i*8+7, i*8) = 0xff;
			}
		}
		currWord.last = 0; //TODO handle what no more words??
		txData.write(currWord);
		break;
	case WRITE_PKG:
		if (wordCount < pkgWordCount)
		{
			wordCount++;
			for (int i = 0; i < (WIDTH/64); i++)
         {
				#pragma HLS UNROLL
				currWord.data(i*64+63, i*64) = 0x3736353433323130;
				currWord.keep(i*8+7, i*8) = 0xff;
			}
			currWord.last = (wordCount == (pkgWordCount));
			txData.write(currWord);
			if (currWord.last)
			{
				wordCount = 0;
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
					if (timeOver)
					{
						lastPkg = true;
					}
					if (packetGap != 0)
					{
						iperfFsmState = PKG_GAP;
					}
					else
					{
						iperfFsmState = CONSTRUCT_HEADER;
					}
				}
			}
		}
		break;
	case WAIT_RESPONSE:
		waitCounter++;
		if (waitCounter == END_TIME_100ms)
		{
			waitCounter = 0;
			iperfFsmState = IDLE;
		}
		break;
	case PKG_GAP:
		packetGapCounter++;
		if (packetGapCounter == packetGap)
		{
			packetGapCounter = 0;
			iperfFsmState = CONSTRUCT_HEADER;
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

template <int WIDTH>
void check_for_response(hls::stream<ipUdpMeta>&	rxMetaData,
						hls::stream<net_axis<WIDTH> >&	rxData,
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


void iperf_udp(	hls::stream<ipUdpMeta>&	rxMetaData,
				hls::stream<net_axis<DATA_WIDTH> >&	rxData,
				hls::stream<ipUdpMeta>&	txMetaData,
				hls::stream<net_axis<DATA_WIDTH> >&	txData,
				ap_uint<1>		runExperiment,
				ap_uint<8>		pkgWordCount,
				ap_uint<8>		packetGap,
				ap_uint<32>		targetIpAddress)
{
	#pragma HLS DATAFLOW disable_start_propagation
	#pragma HLS INTERFACE ap_ctrl_none port=return

	#pragma HLS INTERFACE axis register port=rxMetaData name=s_axis_rx_metadata
	#pragma HLS INTERFACE axis register port=rxData name=s_axis_rx_data
	#pragma HLS INTERFACE axis register port=txMetaData name=m_axis_tx_metadata
	#pragma HLS INTERFACE axis register port=txData name=m_axis_tx_data
	#pragma HLS aggregate  variable=rxMetaData compact=bit
	#pragma HLS aggregate  variable=txMetaData compact=bit

	#pragma HLS INTERFACE ap_none register port=runExperiment
	#pragma HLS INTERFACE ap_none register port=pkgWordCount
	#pragma HLS INTERFACE ap_none register port=packetGap
	#pragma HLS INTERFACE ap_none register port=targetIpAddress


	static hls::stream<bool>		startSignalFifo("startSignalFifo");
	static hls::stream<bool>		stopSignalFifo("stopSignalFifo");
	static hls::stream<bool>		doneSignalFifo("doneSignalFifo");
	#pragma HLS STREAM variable=startSignalFifo depth=2
	#pragma HLS STREAM variable=stopSignalFifo depth=2
	#pragma HLS STREAM variable=doneSignalFifo depth=2

	/*
	 * Client
	 */
	client<DATA_WIDTH>(	txMetaData,
			txData,
			startSignalFifo,
			stopSignalFifo,
			doneSignalFifo,
			runExperiment,
			pkgWordCount,
			packetGap,
			//regMyIpAddress,
			targetIpAddress);

	/*
	 * Check Response
	 */
	check_for_response<DATA_WIDTH>(rxMetaData, rxData, doneSignalFifo);

	/*
	 * Clock
	 */
	clock( startSignalFifo,
			stopSignalFifo);
}

/*void iperf_udp_top(	hls::stream<ipUdpMeta>&	rxMetaData,
					hls::stream<net_axis<DATA_WIDTH> >&	rxData,
					hls::stream<ipUdpMeta>&	txMetaData,
					hls::stream<net_axis<DATA_WIDTH> >&	txData,
					ap_uint<1>		runExperiment,
					ap_uint<8>		pkgWordCount,
					ap_uint<8>		packetGap,
					ap_uint<32>		targetIpAddress)
{
	#pragma HLS DATAFLOW disable_start_propagation
	#pragma HLS INTERFACE ap_ctrl_none port=return

	#pragma HLS INTERFACE axis_register port=rxMetaData name=s_axis_rx_metadata
	#pragma HLS INTERFACE axis_register port=rxData name=s_axis_rx_data
	#pragma HLS INTERFACE axis_register port=txMetaData name=m_axis_tx_metadata
	#pragma HLS INTERFACE axis_register port=txData name=m_axis_tx_data
	#pragma HLS resource core=AXI4Stream variable=rxMetaData metadata="-bus_bundle s_axis_rx_metadata"
	#pragma HLS resource core=AXI4Stream variable=rxData metadata="-bus_bundle s_axis_rx_data"
	#pragma HLS resource core=AXI4Stream variable=txMetaData metadata="-bus_bundle m_axis_tx_metadata"
	#pragma HLS resource core=AXI4Stream variable=txData metadata="-bus_bundle m_axis_tx_data"
	#pragma HLS aggregate  variable=rxMetaData compact=bit
	#pragma HLS aggregate  variable=txMetaData compact=bit

	#pragma HLS INTERFACE ap_none register port=runExperiment
	#pragma HLS INTERFACE ap_none register port=pkgWordCount
	#pragma HLS INTERFACE ap_none register port=packetGap
	#pragma HLS INTERFACE ap_none register port=targetIpAddress

	iperf_udp<DATA_WIDTH>(rxMetaData,
							rxData,
							txMetaData,
							txData,
							runExperiment,
							pkgWordCount,
							packetGap,
							targetIpAddress);
}*/
