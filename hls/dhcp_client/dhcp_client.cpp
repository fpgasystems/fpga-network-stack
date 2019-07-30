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
#include "dhcp_client_config.hpp"
#include "dhcp_client.hpp"

using namespace hls;

void open_dhcp_port(stream<ap_uint<16> >&	openPort,
					stream<bool>&			confirmPortStatus,
					stream<ap_uint<1> >		&portOpen)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	static bool odp_listenDone 			= false;
	static bool odp_waitListenStatus 	= false;
	static ap_uint<32> openPortWaitTime = TIME_5S;

	if (openPortWaitTime == 0) {
		if (!odp_listenDone && !odp_waitListenStatus && !openPort.full()) {
			openPort.write(68);
			odp_waitListenStatus = true;
		}
		else if (!confirmPortStatus.empty() && odp_waitListenStatus){
			confirmPortStatus.read(odp_listenDone);
			portOpen.write(1);
		}
	}
	else
		openPortWaitTime--;
}


void receive_message(	stream<udpMetadata>&	dataInMeta,
						stream<net_axis<DATA_WIDTH> >&		dataIn,
						stream<dhcpReplyMeta>&	metaOut,
						ap_uint<48>				myMacAddress) {
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	static bool rm_isReply 			= false;
	static bool rm_correctMac 		= true;
	static bool rm_isDHCP 			= false;
	static ap_uint<6> rm_wordCount 	= 0;
	static dhcpReplyMeta meta;

	net_axis<DATA_WIDTH> currWord;

	if (!dataIn.empty()) {
		dataIn.read(currWord);
		//std::cout << std::hex << currWord.data << " " << currWord.last << std::endl;
		switch (rm_wordCount) {
		case 0: // Type, HWTYPE, HWLEN, HOPS, ID
			rm_isReply = currWord.data(7, 0) == 0x2;
			meta.identifier = currWord.data(63, 32);
			rm_correctMac = true;
			break;
		case 1: //SECS, FLAGS, ClientIp
			// Do nothing
			//currWord.data[32] == flag
			break;
		case 2: //YourIP, NextServer IP
			meta.assignedIpAddress = currWord.data(31, 0);
			meta.serverAddress = currWord.data(63, 32); //TODO maybe not necessary
			break;
		case 3: //Relay Agent, Client MAC part1
			rm_correctMac = (rm_correctMac && (myMacAddress(31, 0) == currWord.data(63, 32)));
			break;
		case 4:
			// Client Mac Part 2
			rm_correctMac = (rm_correctMac && (myMacAddress(47, 32) == currWord.data(15, 0)));
			break;
		/*case 5: //client mac padding
		case 7:
			//legacy BOOTP
			break;*/
		case 29:
			// Assumption, no Option Overload and DHCP Message Type is first option
			//MAGIC COOKIE
			rm_isDHCP = (MAGIC_COOKIE == currWord.data(63, 32));
			break;
		case 30:
			//Option 53
			if (currWord.data(15, 0) == 0x0135)
				meta.type = currWord.data(23, 16);
			break;
		default:
			// 5-7 legacy BOOTP stuff
			break;
		} //switch
		rm_wordCount++;
		if (currWord.last) {
			rm_wordCount = 0;
			//s Write out metadata
			if (rm_isReply && rm_correctMac && rm_isDHCP)
				metaOut.write(meta);
		}
	} //if

	if (!dataInMeta.empty())
		dataInMeta.read();
}



void send_message(	stream<dhcpRequestMeta>&	metaIn,
					stream<udpMetadata>&		dataOutMeta,
					stream<ap_uint<16> >&		dataOutLength,
					stream<net_axis<DATA_WIDTH> >&			dataOut,
					ap_uint<48> 				myMacAddress)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	static ap_uint<6> sm_wordCount = 0;
	static dhcpRequestMeta meta;
	net_axis<DATA_WIDTH> sendWord;

   sendWord.data = 0;
   sendWord.keep = 0xff;
   sendWord.last = 0; //TODO remove this
	switch (sm_wordCount)
	{
	case 0:
		if (!metaIn.empty())
		{
			metaIn.read(meta);
			sendWord.data(7, 0) = 0x01; //O
			sendWord.data(15, 8) = 0x01; //HTYPE
			sendWord.data(23, 16) = 0x06; //HLEN
			sendWord.data(31, 24) = 0x00; //HOPS
			// identifier
			sendWord.data(63, 32) = meta.identifier;
			dataOut.write(sendWord);
			dataOutMeta.write(udpMetadata(sockaddr_in(0x00000000, 68), sockaddr_in(0xffffffff, 67)));
			dataOutLength.write(300); // 37*8 + 4
			sm_wordCount++;
		}
		break;
	case 1: //secs, flags, CIADDR
		sendWord.data(22, 0) = 0;
		sendWord.data[23] = 0x1; // Broadcast flag
		sendWord.data(63, 24) = 0;
		dataOut.write(sendWord);
		sm_wordCount++;
		break;
	//case 2: //YIADDR, SIADDR
	//case 5: //CHADDR padding + servername
	default: //BOOTP legacy
	//case 5:
	//case 6:
		dataOut.write(sendWord);
		sm_wordCount++;
		break;
	case 3:
		// GIADDR, CHADDR
		sendWord.data(31, 0) = 0;
		sendWord.data(63, 32) = myMacAddress(31, 0);
		dataOut.write(sendWord);
		sm_wordCount++;
		break;
	case 4:
		sendWord.data(15, 0) = myMacAddress(47, 32);
		sendWord.data(63, 16) = 0;
		dataOut.write(sendWord);
		sm_wordCount++;
		break;
	case 29:
		sendWord.data(31, 0) = 0;
		// Magic cookie
		sendWord.data(63, 32) = MAGIC_COOKIE;
		dataOut.write(sendWord);
		sm_wordCount++;
		break;
	case 30:
		// DHCP option 53
		sendWord.data(15, 0) = 0x0135;
		sendWord.data(23, 16) = meta.type;
		if (meta.type == DISCOVER)
		{
			// We are done
			sendWord.data(31, 24) = 0xff;
			sendWord.data(63, 32) = 0;
		}
		else
		{
			// Add DHCP Option 50, len 4, ip add
			sendWord.data(31, 24) = 0x32; //50
			sendWord.data(39, 32) = 0x04;
			sendWord.data(63, 40) = meta.requestedIpAddress(23, 0);
		}
		sm_wordCount++;
		dataOut.write(sendWord);
		break;
	case 31:
		// DHCP Option 50, len 4, ip add
		if (meta.type == REQUEST)
			sendWord.data(7, 0) = meta.requestedIpAddress(31, 24);
		else
			sendWord.data = 0;
		sm_wordCount++;
		dataOut.write(sendWord);
		break;
	case 37:
		// Last padding word, after 64bytes
		sendWord.keep = 0x0f;
		sendWord.last = 1;
		dataOut.write(sendWord);
		sm_wordCount = 0;
		break;
	}//switch
}

void dhcp_fsm(	stream<dhcpReplyMeta>& 		replyMetaIn,
				stream<dhcpRequestMeta>&	requestMetaOut,
				stream<ap_uint<1> >			&portOpen,
				ap_uint<32>&				ipAddressOut,
				ap_uint<1>&					dhcpEnable,
				ap_uint<32>&				inputIpAddress) {
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	enum dhcpStateType {PORT_WAIT, INIT, SELECTING, REQUESTING, BOUND};
	static dhcpStateType state = PORT_WAIT;
	static ap_uint<32> randomValue = 0x34aad34b;
	static ap_uint<32> myIdentity = 0;
	static ap_uint<32> myIpAddress = 0;
	static ap_uint<32> IpAddressBuffer = 0;
	static ap_uint<32> time = 100;

	dhcpReplyMeta reply;
	ipAddressOut = myIpAddress;

	switch (state) {
	case PORT_WAIT:
		myIpAddress = inputIpAddress;
		if (!portOpen.empty()) {
			portOpen.read();
			state = INIT;
		}
		break;
	case INIT:
		myIpAddress = inputIpAddress;
		if (dhcpEnable == 1) {
			if (time == 0) {
				myIdentity = randomValue;
				requestMetaOut.write(dhcpRequestMeta(randomValue, DISCOVER));
				randomValue = (randomValue * 8) xor randomValue; // Update randomValue
				time = TIME_5S;
				state = SELECTING;
			}
			else
				time--;
		}
		break;
	case SELECTING:
		if (!replyMetaIn.empty()) {
			replyMetaIn.read(reply);
			if (reply.identifier == myIdentity) {
				if (reply.type == OFFER) {
					IpAddressBuffer = reply.assignedIpAddress;
					requestMetaOut.write(dhcpRequestMeta(myIdentity, REQUEST, reply.assignedIpAddress));
					time = TIME_5S;
					state = REQUESTING;
				}
				else
					state = INIT;
			}
		}
		else { //else we are waiting
			if (time == 0)
				state = INIT;
			else // Decrease time-out value
				time--;
		}
		break;
	case REQUESTING:
		if (!replyMetaIn.empty()) {
			replyMetaIn.read(reply);
			if (reply.identifier == myIdentity) {
				if (reply.type == ACK) {  //TODO check if IP address correct
					myIpAddress = IpAddressBuffer;
					state = BOUND;
				}
				else
					state = INIT;
			}
			//else omit
		}
		else {
			if (time == 0)
				state = INIT;
			else // Decrease time-out value
				time--;
		}
		break;
	case BOUND:
		ipAddressOut = myIpAddress;
		if (!replyMetaIn.empty())
			replyMetaIn.read();
		if (dhcpEnable == 0)
			state = INIT;
		break;
	}
	randomValue++; //Make sure it doesn't get zero
}

void dhcp_client(	stream<ap_uint<16> >&	openPort,
					stream<bool>&			confirmPortStatus,
					//stream<ap_uint<16> >&	realeasePort,
					stream<udpMetadata>&	dataInMeta,
					stream<net_axis<DATA_WIDTH> >&		dataIn,
					stream<udpMetadata>&	dataOutMeta,
					stream<ap_uint<16> >&	dataOutLength,
					stream<net_axis<DATA_WIDTH> >&		dataOut,
					ap_uint<1>&				dhcpEnable,
					ap_uint<32>&			inputIpAddress,
					ap_uint<32>&			dhcpIpAddressOut,
					ap_uint<48>&			myMacAddress) {
#pragma HLS DATAFLOW disable_start_propagation
#pragma HLS INTERFACE ap_ctrl_none port=return

	#pragma HLS INTERFACE axis register port=openPort name=m_axis_open_port
	#pragma HLS INTERFACE axis register port=confirmPortStatus name=s_axis_open_port_status
	#pragma HLS INTERFACE axis register port=dataInMeta name=s_axis_rx_metadata
	#pragma HLS INTERFACE axis register port=dataIn name=s_axis_rx_data
	#pragma HLS INTERFACE axis register port=dataOutMeta name=m_axis_tx_metadata
	#pragma HLS INTERFACE axis register port=dataOutLength name=m_axis_tx_length
	#pragma HLS INTERFACE axis register port=dataOut name=m_axis_tx_data
	#pragma HLS DATA_PACK variable=dataOutMeta

	#pragma HLS INTERFACE ap_stable register	port=dhcpEnable
	#pragma HLS INTERFACE ap_stable register	port=dhcpIpAddressOut
	#pragma HLS INTERFACE ap_stable			port=myMacAddress
	#pragma HLS INTERFACE ap_stable register	port=inputIpAddress

	static stream<dhcpReplyMeta>		dhcp_replyMetaFifo("dhcp_replyMetaFifo");
	static stream<dhcpRequestMeta>		dhcp_requestMetaFifo("dhcp_requestMetaFifo");
	#pragma HLS stream variable=dhcp_replyMetaFifo depth=4
	#pragma HLS stream variable=dhcp_requestMetaFifo depth=4
	#pragma HLS DATA_PACK variable=dhcp_replyMetaFifo
	#pragma HLS DATA_PACK variable=dhcp_requestMetaFifo
	static stream<ap_uint<1> >		portOpen("portOpen");

	open_dhcp_port(openPort, confirmPortStatus, portOpen);

	receive_message(dataInMeta, dataIn, dhcp_replyMetaFifo, myMacAddress);

	dhcp_fsm(dhcp_replyMetaFifo, dhcp_requestMetaFifo, portOpen, dhcpIpAddressOut, dhcpEnable, inputIpAddress);

	send_message(	dhcp_requestMetaFifo,
					dataOutMeta,
					dataOutLength,
					dataOut, myMacAddress);
}
