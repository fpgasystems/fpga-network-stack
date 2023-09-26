/*
 * Copyright (c) 2019, Systems Group, ETH Zurich
 * Copyright (c) 2016 Xilinx, Inc.
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
#include "arp_server_subnet_config.hpp"
#include "arp_server_subnet.hpp"

template <int WIDTH>
void process_arp_pkg(	hls::stream<net_axis<WIDTH> >&		dataIn,
						hls::stream<arpReplyMeta>&	arpReplyMetaFifo,
						hls::stream<arpTableEntry>& arpTableInsertFifo,
                      	ap_uint<32>             myIpAddress,
						ap_uint<16>&			regRequestCount,
						ap_uint<16>&			regReplyCount)
{
	#pragma HLS PIPELINE II=1
	#pragma HLS INLINE off

	static arpHeader<WIDTH> header;
	static ap_uint<16> pag_requestCounter = 0;
	static ap_uint<16> pag_replyCounter = 0;

	if (!dataIn.empty())
	{
		net_axis<WIDTH> word = dataIn.read();
		header.parseWord(word.data);

		if (word.last)
		{
			if (header.getTargetProtoAddr() == myIpAddress)
			{
				if (header.isRequest())
				{
					// Trigger ARP reply
					arpReplyMetaFifo.write(arpReplyMeta(header.getSrcMacAddr(), header.getSenderHwAddr(), header.getSenderProtoAddr()));
					pag_requestCounter++;
					regRequestCount = pag_requestCounter;
				}
				else if (header.isReply())
				{
					arpTableInsertFifo.write(arpTableEntry(header.getSenderProtoAddr(), header.getSenderHwAddr(), true));
					pag_replyCounter++;
					regReplyCount = pag_replyCounter;
				}
			}
			header.clear();
		}
	}
}

template <int WIDTH>
void generate_arp_pkg(	hls::stream<arpReplyMeta>&     arpReplyMetaFifo,
						hls::stream<ap_uint<32> >&     arpRequestMetaFifo,
						hls::stream<net_axis<WIDTH> >&          dataOut,
						ap_uint<48>				  myMacAddress,
						ap_uint<32>               myIpAddress)
{
	#pragma HLS PIPELINE II=1
	#pragma HLS INLINE off

	enum arpSendStateType {IDLE, HEADER, PARTIAL_HEADER};
	static arpSendStateType gap_state = IDLE;
	static arpHeader<WIDTH> header;
	static ap_uint<4>		headerCount = 0;
	static arpReplyMeta		replyMeta;
	static ap_uint<32>		inputIP;
	static ap_uint<8> 		remainingLength;

	switch (gap_state)
	{
	case IDLE:
		header.clear();
		if (!arpReplyMetaFifo.empty())
		{
			arpReplyMetaFifo.read(replyMeta);
			header.setDstMacAddr(replyMeta.srcMacAddr);
			header.setSrcMacAddr(myMacAddress);
			header.setReply();
			header.setSenderHwAddr(myMacAddress);
			header.setSenderProtoAddr(myIpAddress);
			header.setTargetHwAddr(replyMeta.senderHwAddr);
			header.setTargetProtoAddr(replyMeta.senderProtoAddr);
			std::cout << "reply" << std::endl;
			gap_state = HEADER;
		}
		else if (!arpRequestMetaFifo.empty())
		{
			arpRequestMetaFifo.read(inputIP);
			header.setDstMacAddr(BROADCAST_MAC);
			header.setSrcMacAddr(myMacAddress);
			header.setRequest();
			header.setSenderHwAddr(myMacAddress);
			header.setSenderProtoAddr(myIpAddress);
			header.setTargetHwAddr(0);
			header.setTargetProtoAddr(inputIP);
			std::cout << "reqest" << std::endl;
			gap_state = HEADER;
		}
		break;
	case HEADER:
	{
		net_axis<WIDTH> sendWord;
		#ifndef __SYNTHESIS___
			sendWord.data = 0;
		#endif
		remainingLength = header.consumeWord(sendWord.data);

      if (WIDTH > ARP_HEADER_SIZE)
      {
		   sendWord.keep = lenToKeep((ARP_HEADER_SIZE/8));
		   sendWord.last = 1;
			gap_state = IDLE;
      }
		else
		{
		   sendWord.keep = ~0;
		   sendWord.last = 0;
			if (remainingLength < (WIDTH/8))
			{
				gap_state = PARTIAL_HEADER;
			}
		}
		
		dataOut.write(sendWord);
		std::cout << std::dec << "remaining lengt: " << remainingLength << std::endl;
		break;
	}
	case PARTIAL_HEADER:
	{
		net_axis<WIDTH> sendWord;
		#ifndef __SYNTHESIS___
			sendWord.data = 0;
		#endif
		header.consumeWord(sendWord.data);
		sendWord.keep = lenToKeep(remainingLength);
		sendWord.last = 1;
		dataOut.write(sendWord);
		gap_state = IDLE;
		break;
	}
  } //switch
}

void arp_table( stream<arpTableEntry>&    	arpTableInsertFifo,
                stream<ap_uint<32> >&     	macIpEncode_req,
                stream<ap_uint<32> >&     	hostIpEncode_req,
                stream<arpTableReply>&		macIpEncode_rsp,
                stream<arpTableReply>&		hostIpEncode_rsp,
                stream<ap_uint<32> >&		arpRequestMetaFifo)
{
#pragma HLS PIPELINE II=1

	static arpTableEntry		arpTable[256];
	#pragma HLS bind_storage variable=arpTable type=RAM_1P impl=BRAM
	#pragma HLS DEPENDENCE variable=arpTable inter false

	ap_uint<32>			query_ip;
	arpTableEntry		currEntry;


	if (!arpTableInsertFifo.empty())
	{
		arpTableInsertFifo.read(currEntry);
		arpTable[currEntry.ipAddress(31,24)] = currEntry;
	}
	else if (!macIpEncode_req.empty())
	{
		macIpEncode_req.read(query_ip);
		currEntry = arpTable[query_ip(31,24)];
		if (!currEntry.valid)
		{
			// send ARP request
			arpRequestMetaFifo.write(query_ip);
		}
		macIpEncode_rsp.write(arpTableReply(currEntry.macAddress, currEntry.valid));
	}
	else if (!hostIpEncode_req.empty())
	{
		hostIpEncode_req.read(query_ip);
		currEntry = arpTable[query_ip(31,24)];
		if (!currEntry.valid)
		{
			// send ARP request
			arpRequestMetaFifo.write(query_ip);
		}
		hostIpEncode_rsp.write(arpTableReply(currEntry.macAddress, currEntry.valid));
	}

}

template<int WIDTH>
void arp_server_subnet(	hls::stream<net_axis<WIDTH> >&          arpDataIn,
                  	  	hls::stream<ap_uint<32> >&     macIpEncode_req,
                  	  	hls::stream<ap_uint<32> >&     hostIpEncode_req,
				        hls::stream<net_axis<WIDTH> >&          arpDataOut,
				        hls::stream<arpTableReply>&    macIpEncode_rsp,
				        hls::stream<arpTableReply>&    hostIpEncode_rsp,
				        ap_uint<48>				myMacAddress,
				        ap_uint<32>				myIpAddress,
						ap_uint<16>&			regRequestCount,
						ap_uint<16>&			regReplyCount)
{
	#pragma HLS INLINE

	static hls::stream<arpReplyMeta>     arpReplyMetaFifo("arpReplyMetaFifo");
	#pragma HLS STREAM variable=arpReplyMetaFifo depth=4
	#pragma HLS aggregate variable=arpReplyMetaFifo compact=bit 

	static hls::stream<ap_uint<32> >   arpRequestMetaFifo("arpRequestMetaFifo");
	#pragma HLS STREAM variable=arpRequestMetaFifo depth=4
	//#pragma HLS aggregate variable=arpRequestMetaFifo compact=bit 

	static hls::stream<arpTableEntry>    arpTableInsertFifo("arpTableInsertFifo");
	#pragma HLS STREAM variable=arpTableInsertFifo depth=4
	#pragma HLS aggregate variable=arpTableInsertFifo compact=bit 

	process_arp_pkg<WIDTH>(arpDataIn,
  					arpReplyMetaFifo,
					arpTableInsertFifo,
					myIpAddress,
					regRequestCount,
					regReplyCount);

  	generate_arp_pkg<WIDTH>(	arpReplyMetaFifo,
						arpRequestMetaFifo,
						arpDataOut,
						myMacAddress,
						myIpAddress);

  	arp_table(	arpTableInsertFifo,
				macIpEncode_req,
				hostIpEncode_req,
				macIpEncode_rsp,
				hostIpEncode_rsp,
				arpRequestMetaFifo);
}

void arp_server_subnet_top(	hls::stream<ap_axiu<DATA_WIDTH, 0, 0, 0> >&          arpDataIn,
                  	  	hls::stream<ap_uint<32> >&     macIpEncode_req,
                  	  	hls::stream<ap_uint<32> >&     hostIpEncode_req,
				        hls::stream<ap_axiu<DATA_WIDTH, 0, 0, 0> >&          arpDataOut,
				        hls::stream<arpTableReply>&    macIpEncode_rsp,
				        hls::stream<arpTableReply>&    hostIpEncode_rsp,
				        ap_uint<48>				myMacAddress,
				        ap_uint<32>				myIpAddress,
						ap_uint<16>&			regRequestCount,
						ap_uint<16>&			regReplyCount)
{
	#pragma HLS INTERFACE ap_ctrl_none port=return
	#pragma HLS DATAFLOW disable_start_propagation

	#pragma HLS INTERFACE axis register port=arpDataIn name=s_axis
	#pragma HLS INTERFACE axis register port=arpDataOut name=m_axis
	#pragma HLS INTERFACE axis register port=macIpEncode_req name=s_axis_arp_lookup_request
	#pragma HLS INTERFACE axis register port=macIpEncode_rsp name=m_axis_arp_lookup_reply
	#pragma HLS INTERFACE axis register port=hostIpEncode_req name=s_axis_host_arp_lookup_request
	#pragma HLS INTERFACE axis register port=hostIpEncode_rsp name=m_axis_host_arp_lookup_reply
	#pragma HLS aggregate variable=macIpEncode_rsp compact=bit
	#pragma HLS aggregate variable=hostIpEncode_rsp compact=bit
    #pragma HLS INTERFACE ap_none register port=myMacAddress
	#pragma HLS INTERFACE ap_none register port=myIpAddress

	static hls::stream<net_axis<DATA_WIDTH> > arpDataIn_internal;
	#pragma HLS STREAM depth=2 variable=arpDataIn_internal
	static hls::stream<net_axis<DATA_WIDTH> > arpDataOut_internal;
	#pragma HLS STREAM depth=2 variable=arpDataOut_internal

	convert_axis_to_net_axis<DATA_WIDTH>(arpDataIn, 
							arpDataIn_internal);

	convert_net_axis_to_axis<DATA_WIDTH>(arpDataOut_internal, 
							arpDataOut);

   	arp_server_subnet<DATA_WIDTH>(arpDataIn_internal,
                                 macIpEncode_req,
                                 hostIpEncode_req,
                                 arpDataOut_internal,
                                 macIpEncode_rsp,
                                 hostIpEncode_rsp,
                                 myMacAddress,
                                 myIpAddress,
                                 regRequestCount,
                                 regReplyCount);
}
