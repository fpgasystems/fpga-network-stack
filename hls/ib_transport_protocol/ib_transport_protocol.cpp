/************************************************
Copyright (c) 2019, Systems Group, ETH Zurich.
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
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
************************************************/

#include "ib_transport_protocol.hpp"
#include <rocev2_config.hpp>
#include "conn_table.hpp"
#include "state_table.hpp"
#include "msn_table.hpp"
#include "transport_timer.hpp"
#include "retransmitter/retransmitter.hpp"
#include "read_req_table.hpp"
#include "multi_queue/multi_queue.hpp"

// ------------------------------------------------------------------------------------------------
// RX path - process incoming RDMA packets with IBH header (a.k.a. RDMA packets)
// ------------------------------------------------------------------------------------------------

/** 
 * RX process IBH - first point of contact for incoming RDMA packets, dissects the Meta-information from the incoming packets
 */
template <int WIDTH, int INSTID = 0>
void rx_process_ibh(	
#ifdef DBG_IBV_IBH_PROCESS
	stream<psnPkg>& m_axis_dbg,
#endif
	// Input stream for incoming RDMA-packets
	stream<net_axis<WIDTH> >& input,
	// First output stream for meta information - contains opcode, partition key, destination qp, psn, validPSN and packet number
	stream<ibhMeta>& metaOut,
	// Second output stream for meta information - only the opcode 
	stream<ibOpCode>& metaOut2,
	// Output for packets for further processing 
	stream<net_axis<WIDTH> >& output,
	// Counter for packets? - Investigate! 
	ap_uint<32>&		regIbvCountRx
) {
//
#pragma HLS inline off
#pragma HLS pipeline II=1
	// Create BaseTransportHeader, metaWritten-bool
	static BaseTransportHeader<WIDTH> bth;
	static bool metaWritten = false;
	static ap_uint<32> validRx = 0;

	net_axis<WIDTH> currWord; // data type of incoming stream

	if (!input.empty()) {
		// Read current input into the word-variable
		input.read(currWord);

		// parse word to infiniband header
		bth.parseWord(currWord.data);

		// Increment the word counter and pass it to Ibv-Counter
		validRx++;
		regIbvCountRx = validRx;

		// If read word is ready, write it to output and write output
		if (bth.isReady()) {
			output.write(currWord);
			
			// Write the output as required 
			if (!metaWritten) {
				std::cout << "[RX PROCESS IBH " << INSTID << "]: input psn " << std::hex << bth.getPsn() << std::endl;
            	metaOut.write(ibhMeta(bth.getOpCode(), bth.getPartitionKey(), bth.getDstQP(), bth.getPsn(), true));
				metaOut2.write(bth.getOpCode());
				metaWritten = true;

				std::cout << "[RX PROCESS IBH " << INSTID << "]: process IBH opcode: " << bth.getOpCode() << std::endl;
			}
		}
		
		// If the last-bit is set, reset everything again. 
		if (currWord.last) {
			bth.clear();
			metaWritten = false;

#ifdef DBG_IBV_IBH_PROCESS
		m_axis_dbg.write(psnPkg(bth.getPsn(), currWord.last));
#endif
		}
	}
}

/**
 * RX process EXH - Handles the extended header (either ACK or WRITE, follows the BTH)
 */
template <int WIDTH, int INSTID = 0>
void rx_process_exh(
	stream<net_axis<WIDTH> >& input, // Input for the incoming packets
	stream<ibOpCode>& metaIn, // Input for Meta - only the Opcodes
	stream<exhMeta>& exhMetaFifo, // Meta-Output: bit for ACK / NACK, packet number
 	stream<ExHeader<WIDTH> >& metaOut, // Dissected Extended Header
	stream<net_axis<WIDTH> >& output // Pass on of the original input 
) {
#pragma HLS inline off
#pragma HLS pipeline II=1

	// Definition of the State Machine for processing different kinds of Extended Headers
	enum fsmStateType {META, ACK_HEADER, RETH_HEADER, NO_HEADER};
	static fsmStateType state = META;

	// RDMA-Extended Header (for READ, WRITE etc.)
	static RdmaExHeader<WIDTH> rdmaHeader;

	// ACK-Extended Header
	static AckExHeader<WIDTH> ackHeader;

	// Check whether meta-information was already written 
	static bool metaWritten = false;

	// Input-word as read from the stream 
	net_axis<WIDTH> currWord;
	static ibOpCode opCode;

	// State-Machine for handling the various different types of Extended Headers 
	switch (state)
	{
	// Idle-state for reading Input and deciding on the type of packet for further processing
	case META:
		if (!metaIn.empty())
		{
			// Read only the opcode 
			metaIn.read(opCode);
			metaWritten = false;

			// Decide with opcode whether it's an ACK, RDMA or none, switch state accordingly
			if (checkIfAethHeader(opCode))
			{
				state = ACK_HEADER;
			}
			else if (checkIfRethHeader(opCode))
			{
				state = RETH_HEADER;
			}
			else
			{
				state = NO_HEADER;
			}
		}
		break;
	
	// Deal with a recognized ACK Ex-Header
	case ACK_HEADER:
		// Read the packet from input if available 
		if (!input.empty())
		{
			input.read(currWord);

			// Get the ACK-Ex-Header from the input-word
			ackHeader.parseWord(currWord.data);

			// Check if the header is ready
			if (ackHeader.isReady())
			{
				// Sent out for further processing only if it's not an ACK (but a Read Response)
				if (opCode != RC_ACK)
				{
					output.write(currWord);
				}

				// Meta not written: Write to the output-meta-queues
				if (!metaWritten)
				{
					// FIFO: Write out single bit whether the ACK is a NAK
					exhMetaFifo.write(exhMeta(ackHeader.isNAK()));
					// Write out the dissected ACK-Ex Header
					metaOut.write(ExHeader<WIDTH>(ackHeader));
					metaWritten = true;
				}
			}

			// If last word, clean up everything
			if (currWord.last)
			{
				ackHeader.clear();
				state = META;
			}
		}
		break;

	// Deal with a recognized RDMA Ex-Header
	case RETH_HEADER:
		// Read the packet from input if available
		if (!input.empty())
		{
			input.read(currWord);

			// Parse the RDMA Ex-Header from the input word 
			rdmaHeader.parseWord(currWord.data);

			// Write out word to the output stream if meta has already been processed (by a previous packet from the same stream)
			if (metaWritten && WIDTH <= RETH_SIZE)
			{
				output.write(currWord);
			}

			// If the RDMA-Header is ready, proceed with processing 
			if (rdmaHeader.isReady())
			{
				if (!metaWritten)
				{
					// Decide between READ_REQUEST and WRITEs to write out to the Meta-FIFO
					if (opCode == RC_RDMA_READ_REQUEST)
					{
						// Is not NAK, number of packets derived by length of read access
						exhMetaFifo.write(exhMeta(false, (rdmaHeader.getLength()+(PMTU-1))/PMTU));
					}
					else
					{
						// Is not NAK
						exhMetaFifo.write(exhMeta(false));
					}
					// Write out the dissected RDMA Ex-Header 
					metaOut.write(ExHeader<WIDTH>(rdmaHeader));
					// Set metaWritten true 
					metaWritten = true;
				}

				// If it's a WRITE, pass it on in the queue 
				if (checkIfWrite(opCode) && WIDTH > RETH_SIZE)
				{
					output.write(currWord);
				}
			}

			// Clean up in the end 
			if (currWord.last)
			{
				rdmaHeader.clear();
				state = META;
			}
		}
		break;

	// Treatment if no header is provided 
	case NO_HEADER:
		// If input is available, read it and pass it on. Write empty META (where should it come from if no header is provided?)
		if (!input.empty())
		{
			input.read(currWord);
			//std::cout << "[RX PROCESS EXH " << INSTID << "]: EXH NO HEADER" << std::endl;
#ifdef DBG_FULL
			std::cout << "\t";
			print(std::cout, currWord);
			std::cout << std::endl;
#endif
			output.write(currWord);
			if (!metaWritten)
			{
				exhMetaFifo.write(exhMeta(false));
				metaOut.write(ExHeader<WIDTH>());
				metaWritten = true;
			}
			if (currWord.last)
			{
				state = META;
			}
		}
		break;
	}//switch

}

/**
 * RX IBH fsm - State Machine for further dealing with the information from the InfiniBand Base Transport Header
 * 
 * PSN handling page
 * page 298, responser receiving requests
 * page 346, requester receiving responses
 *
 * NAK processing, NAK has to be in-order
 * Responder: - send NAK do not update PSN
 * page 336   - do not send futher ACKs or NAKs until valid PKG is received
 * Requester: - retransmit data
 * page 349   - do not increment epsn
 */
//TODO check if RC_ACK is a NAK
//TODO validate response is consistent with request
//TODO actually any response in Unack region is valid, not just the next one.
template <int INSTID = 0>
void rx_ibh_fsm(
	// Base Transport Header, received from rx_process_ibh
	stream<ibhMeta>& metaIn,
	// NAK-bit (and pkg-number), received from rx_process_exh
	stream<exhMeta>& exhMetaFifo,
	// Input from the state table: has information on epsn (expected ? psn), oldest outstanding psn, max_forwarding and retryCounter - has to do with handling retransmissions
	stream<rxStateRsp>& stateTable_rsp,
	// Output to the state table: has information on qpn, epsn, retryCounter and bools for isResponse and WRITE
	stream<rxStateReq>& stateTable_upd_req,
	// Passes on the BTH from rx_process_exh to the stream-merge module
	stream<ibhMeta>& metaOut,
	// Output to the stream merge module: Ack-event with information on qpn, psn, valid signal for psn, NAK-bit
	stream<ackEvent>& ibhEventFifo,
	// Output to ipUdpMetahandler: Single Bit to show if packet should be dropped 
	stream<bool>& ibhDropFifo,
	// Output to ipUdpMetahandler: Single bits for drop and ack. 
	stream<fwdPolicy>& ibhDropMetaFifo,
	// Output to rx_exh_fsm: One bit rd (no idea for what), qpn, psn
	stream<ackMeta>& m_axis_rx_ack_meta,
#ifdef RETRANS_EN
	stream<rxTimerUpdate>&	rxClearTimer_req,
	stream<retransUpdate>&	rx2retrans_upd,
#endif
#ifdef DBG_IBV_IBH_FSM
	stream<psnPkg>& m_axis_dbg,
#endif
	// Counter for dropped packets
	ap_uint<32>&		regInvalidPsnDropCount
) {
#pragma HLS inline off
#pragma HLS pipeline II=1

	// Define a 2-state FSM
	enum fsmStateType{LOAD, PROCESS};
	static fsmStateType fsmState = LOAD;

	// Variable for BTH
	static ibhMeta meta;

	// Variable for Extended Header 
	static exhMeta emeta;

	// Single bit for is Response 
	static bool isResponse;
	static ap_uint<32> droppedPackets = 0;

	// State of the QueuePair: Dealing with PSNs, retry counter etc. 
	rxStateRsp qpState;


	switch(fsmState)
	{
	// LOAD: Idle-state to read the BTH, ExH, check if it's a response and write destination qp and response-bit to the state table
	case LOAD:
		if (!metaIn.empty() && !exhMetaFifo.empty())
		{
			metaIn.read(meta);
			exhMetaFifo.read(emeta);
			isResponse = checkIfResponse(meta.op_code);
			stateTable_upd_req.write(rxStateReq(meta.dest_qp, isResponse));
			fsmState = PROCESS;
		}
		break;
	case PROCESS:
		//TODO TIME-WAIT
		//TODO consider opCode??

		// If available, read entry from the State Table 
		if (!stateTable_rsp.empty())
		{
			stateTable_rsp.read(qpState);
			//ap_uint<24> oldest_outstanding_psn = qpState.epsn- 8388608;
			//TODO compute or store oldest outstanding??
			//TODO also increment oldest outstanding
			//Check if in order
			//TODO Update oldest_oustanding_psn
			//TODO this is not working with coalescing ACKs
			std::cout << std::hex << "[RX IBH FSM " << INSTID << "]: epsn: " << qpState.epsn << ", packet psn: " << meta.psn << std::endl;
			// For requests we require total order, for responses, there is potential ACK coalescing, see page 299
			// For requests, max_forward == epsn

			// Easy case: current psn is the expected psn, or packet is an ACK
			if (qpState.epsn == meta.psn || meta.op_code == RC_ACK)
			{

			#ifdef DBG_IBV_IBH_FSM
				m_axis_dbg.write(psnPkg(0, meta.psn, qpState.epsn, meta.op_code));
			#endif

				// Forwarding - Packet won't get dropped 
				if (meta.op_code != RC_ACK && meta.op_code != RC_RDMA_READ_REQUEST) //TODO do length check instead
				{
					ibhDropFifo.write(false);
                }
				ibhDropMetaFifo.write(fwdPolicy(false, false));

				// Write out the BTH 
				metaOut.write(ibhMeta(meta.op_code, meta.partition_key, meta.dest_qp, meta.psn, meta.validPSN));

				// Update psn with the Pkg-number as received from the Extended Header Meta Content. Write it to the State Table. 
				//TODO for last param we need vaddr here!
				if (!emeta.isNak && (meta.op_code != RC_ACK || ((qpState.epsn <= meta.psn && meta.psn <= qpState.max_forward)
					|| ((qpState.epsn <= meta.psn || meta.psn <= qpState.max_forward) && qpState.max_forward < qpState.epsn))))
				{
					// if the next req is invalid, trigger NAK
					stateTable_upd_req.write(rxStateReq(meta.dest_qp, meta.psn+emeta.numPkg, isResponse));
				}
#ifdef RETRANS_EN

				// Update oldest-unacked-reqeust
				if (isResponse && !emeta.isNak)
				{
					std::cout << std::hex <<"[RX IBH FSM " << INSTID << "]: retrans update, psn " << meta.psn << std::endl;

                    // Retrans table update
					rx2retrans_upd.write(retransUpdate(meta.dest_qp, meta.psn, meta.op_code));

                    if (meta.op_code != RC_RDMA_READ_RESP_FIRST && meta.op_code != RC_RDMA_READ_RESP_MIDDLE) {
                        // to flow control
                        m_axis_rx_ack_meta.write(ackMeta(meta.op_code == RC_ACK, meta.dest_qp, meta.psn));
                    }
				}
				// Check if no oustanding requests -> stop timer
				if (isResponse && meta.op_code != RC_RDMA_READ_RESP_MIDDLE)
				{
					rxClearTimer_req.write(rxTimerUpdate(meta.dest_qp, meta.psn == qpState.max_forward));
#ifndef __SYNTHESIS__
					if (meta.psn  == qpState.max_forward)
					{
						std::cout << std::hex << "[RX IBH FSM " << INSTID << "]: clearing transport timer at psn " << meta.psn << std::endl;
					}
#endif
				}
#endif
			}
			// Check for duplicates - something's off with the psns, so that additional checks for duplicates must be executed 
			// For response: epsn = old_unack, old_oustanding = old_valid
			else if ((qpState.oldest_outstanding_psn < qpState.epsn && meta.psn < qpState.epsn && meta.psn >= qpState.oldest_outstanding_psn)
					 || (qpState.oldest_outstanding_psn > qpState.epsn && (meta.psn < qpState.epsn || meta.psn >= qpState.oldest_outstanding_psn)))
			{
				#ifdef DBG_IBV_IBH_FSM
					m_axis_dbg.write(psnPkg(1, meta.psn, qpState.epsn, meta.op_code));
				#endif

				// Read request re-execute
				if (meta.op_code == RC_RDMA_READ_REQUEST)
				{
					std::cout << std::hex << "[RX IBH FSM" << INSTID << "]: duplicate read_req psn " << meta.psn << std::endl;
					//ibhDropFifo.write(false);
					// Don't drop, pass on the BTH to the next module in line 
					ibhDropMetaFifo.write(fwdPolicy(false, false));
					metaOut.write(ibhMeta(meta.op_code, meta.partition_key, meta.dest_qp, meta.psn, meta.validPSN));
					//No release required
					//stateTable_upd_req.write(rxStateReq(meta.dest_qp, meta.psn, meta.partition_key, 0)); //TODO always +1??
				}
				// Write requests acknowledge, see page 313
				else if (checkIfWrite(meta.op_code))
				{
					// Packet needs to be dropped - but why write to the ibhEventFifo then?: Probably to initiate a NAK as response
					ibhEventFifo.write(ackEvent(meta.dest_qp, meta.psn, false)); //TODO do we need PSN???
					std::cout << std::hex << "[RX IBH FSM " << INSTID << "]: dropping duplicate psn " << meta.psn << std::endl;
					// Increment the drop counter 
					droppedPackets++;
					regInvalidPsnDropCount = droppedPackets;

					// Write the Drop-Event to the other queues as well 
					ibhDropFifo.write(true);
					//Meta is required for ACK, TODO no longer
					ibhDropMetaFifo.write(fwdPolicy(false, true));
				}
				//TODO what about duplicate responses
				// Drop them
				else
				{
					// Also get dropped here, but counters are not incremented - why? 
					// Case Requester: Valid ACKs -> reset timer TODO
					// Propagate ACKs for flow control
					if (meta.op_code != RC_ACK) //TODO do length check instead
					{
						ibhDropFifo.write(true);
					} 
					ibhDropMetaFifo.write(fwdPolicy(true, false));
				}
			}
			else // completely invalid - nothing to recover here
			{
				#ifdef DBG_IBV_IBH_FSM
					m_axis_dbg.write(psnPkg(2, meta.psn, qpState.epsn, meta.op_code));
				#endif

				// behavior, see page 313
				std::cout << std::hex << "[RX IBH FSM " << INSTID << "]: dropping invalid psn " << meta.psn << " with retry " << qpState.retryCounter << std::endl;
				// Packets need to get dropped, so increment the drop counter 
				droppedPackets++;
				regInvalidPsnDropCount = droppedPackets;
				ibhDropMetaFifo.write(fwdPolicy(true, false));

				// Issue NAK TODO NAK has to be in sequence
				if (meta.op_code != RC_ACK)
				{
					ibhDropFifo.write(true);
					// Do not generate further ACK/NAKs until we received a valid pkg

					// if retrycounter is set to 0x7, unlimited retries are attempted 
					if (qpState.retryCounter == 0x7)
					{
						if (isResponse)
						{
							std::cout << "[RX IBH FSM " << INSTID << "]: sending NAK on psn " << meta.psn << std::endl;
							ibhEventFifo.write(ackEvent(meta.dest_qp, meta.psn, true));
						}
						else
						{
							std::cout << "[RX IBH FSM " << INSTID << "]: sending NAK on epsn " << qpState.epsn << std::endl;
							// Setting NAK to epsn, since otherwise epsn-1 is used
							ibhEventFifo.write(ackEvent(meta.dest_qp, qpState.epsn, true));
						}
						qpState.retryCounter--;
					}
					// We do not increment PSN
					stateTable_upd_req.write(rxStateReq(meta.dest_qp, qpState.epsn, qpState.retryCounter, isResponse));

				}
			}

			fsmState = LOAD;
		}
		break;
	}
}

/**
 * RX EXH fsm - State Machine for further dealing with the Extended Headers 
 * 
 * For reliable connections, page 246, 266, 269
 * SEND ONLY: PayLd
 * SEND FIRST: PayLd
 * SEND MIDDLE: PayLd
 * SEND LAST: PayLd
 * RDMA WRITE ONLY: RETH, PayLd
 * RDMA WRITE FIRST: RETH, PayLd
 * RDMA WRITE MIDDLE: PayLd
 * RDMA WRITE LAST: PayLd
 * RDMA READ REQUEST: RETH
 * RDMA READ RESPONSE ONLY: AETH, PayLd
 * RDMA READ RESPONSE FIRST: AETH, PayLd
 * RDMA READ RESPONSE MIDDLE: PayLd
 * RDMA READ RESPONSE LAST: AETH, PayLd
 * ACK: AETH
 */
template <int WIDTH, int INSTID = 0>
void rx_exh_fsm(
	// BTH-Input coming from earlier modules
	stream<ibhMeta>& metaIn,
	// 16-Bit Number to indicate a length (probably of the UDP-packet?)
	stream<ap_uint<16> >& udpLengthFifo,
	// Input from the msn-table: Has information on message sequence number, virtual address, dma_length, rkey and last-bit
	stream<dmaState>& msnTable2rxExh_rsp,
#ifdef RETRANS_EN
    //stream<rxReadReqUpdate>& readReqTable_upd_req,
	//stream<rxReadReqRsp>& readReqTable_rsp,
    stream<retransmission>&	rx2retrans_req,
    stream<retransRdInit>&  retrans2rx_init,
#endif
	//stream<ap_uint<64> >& rx_readReqAddr_pop_rsp,
	// Ex-Header Input from the rx_process_exh-module 
	stream<ExHeader<WIDTH> >& headerInput,
	// Write command to memory, includes address, length and bits for control, sync, host, pid, vfid (Identification of vFPGAs I guess?)
	stream<memCmd>& memoryWriteCmd,
	// Read request to handle read request, includes QPN, virtuall address, dma_length, PSN and host-bit
	stream<readRequest>& readRequestFifo,
	//stream<ackMeta>& m_axis_rx_ack_meta,
	// Output to the msn-table, includes qpn, msn, vaddr, dma-length, lst- and write-bit
	stream<rxMsnReq>& rxExh2msnTable_upd_req,
	//stream<mqPopReq>& rx_readReqAddr_pop_req,
	// Output to stream merger, includes QPN, PSN, valid- and NAK-Bit
	stream<ackEvent>& rx_exhEventMetaFifo,
	// Output to rx_exh payload, carries the opcode 
	stream<pkgSplit>& rx_pkgSplitTypeFifo,
	// Output to merge_rx_pckgs, carries QPN and PkgShiftType (Can be either SHIFT_AETH, SHIFT_RETH, SHIFT_NONE)
	stream<pkgShift>& rx_pkgShiftTypeFifo
) {
#pragma HLS inline off
#pragma HLS pipeline II=1

	// State-Machine with three states for META, DMA_META and DATA
	enum pe_fsmStateType {META, DMA_META, DATA};
	static pe_fsmStateType pe_fsmState = META;
	// Variable for storing the BTH
	static ibhMeta meta;
	// Not used here 
	net_axis<WIDTH> currWord;
	// Local Variable for the extended Header 
	static ExHeader<WIDTH> exHeader;
	// Local Variable to read the content from the msn table 
	static dmaState dmaMeta;
	// Local Variable for the incoming UDP-length and the length of the attached payload 
	static ap_uint<16> udpLength;
	ap_uint<32> payLoadLength;
	static bool consumeReadInit;
	//static rxReadReqRsp readReqMeta;
	static retransRdInit readReqInit;


	switch (pe_fsmState)
	{
	// Initial state is META: read the incoming meta-information (headers)
	case META:
		if (!metaIn.empty() && !headerInput.empty())
		{
			// Read both BTH and Extended Header
			metaIn.read(meta);
			headerInput.read(exHeader);

			// Write the destination QP to the msn table
			rxExh2msnTable_upd_req.write(rxMsnReq(meta.dest_qp));
			consumeReadInit = false;

#ifdef RETRANS_EN // ?
			/*if (meta.op_code == RC_ACK)
			{
				readReqTable_upd_req.write(rxReadReqUpdate(meta.dest_qp));
			}*/
#endif		
			// If the opcode is either a single read or the beginning of a longer read, set the consume variable to true
			if (meta.op_code == RC_RDMA_READ_RESP_ONLY || meta.op_code == RC_RDMA_READ_RESP_FIRST)
			{
				consumeReadInit = true;
				//rx_readReqAddr_pop_req.write(mqPopReq(meta.dest_qp));
			}
			pe_fsmState = DMA_META;
		}
		break;

	// Second state for further processing
	case DMA_META:
		// Check if msn-input is ready and read-init was not already consumed or retrans is available
		if (!msnTable2rxExh_rsp.empty() && !udpLengthFifo.empty() && (!consumeReadInit || !retrans2rx_init.empty()))
		{
			// Read both the MSN-Input and the length of the UDP-packet
			msnTable2rxExh_rsp.read(dmaMeta);
			udpLengthFifo.read(udpLength);
#ifdef RETRANS_EN
			/*if (meta.op_code == RC_ACK)
			{
				readReqTable_rsp.read(readReqMeta);
			}*/
#endif		
			if (consumeReadInit)
			{
				retrans2rx_init.read(readReqInit);
			}
			pe_fsmState = DATA;
		}
		break;
	case DATA: // TODO merge with DMA_META
		// Data treatment depends on the opcode of the received packet 
		switch(meta.op_code)
		{
		case RC_SEND_ONLY: 

		// Last part of a SEND message stream
        case RC_SEND_LAST:
		{
            std::cout << "[RX EXH FSM " << INSTID << "]: send opcode: " << std::hex << meta.op_code << std::endl;
			// [BTH][PayLd]
			// Compute payload length
			// Payloadlength is calculated from length of the UDP-frame minus bitfields for UDP header, BTH header and ICRC field
			payLoadLength = udpLength - (8 + 12 + 4); // UDP, BTH, CRC
			// Issue a memory write command based on the SEND-opcode
			memoryWriteCmd.write(memCmd(0, payLoadLength, PKG_F, PKG_INT, meta.dest_qp));
			// Update state - count up the message sequence number for current destination QPN
			rxExh2msnTable_upd_req.write(rxMsnReq(meta.dest_qp, dmaMeta.msn+1));
			// Trigger ACK - requires actions of the stream merger, packet merger and payload manager
			rx_exhEventMetaFifo.write(ackEvent(meta.dest_qp, meta.psn, false));
			rx_pkgSplitTypeFifo.write(pkgSplit(meta.op_code));
			rx_pkgShiftTypeFifo.write(pkgShift(SHIFT_NONE, meta.dest_qp));

			pe_fsmState = META;
			break;
		}
        case RC_SEND_FIRST:
        case RC_SEND_MIDDLE:
        {
			// Equivalent to the treatment for the RC_SEND_LAST

            std::cout << "[RX EXH FSM " << INSTID << "]: send opcode: " << std::hex << meta.op_code << std::endl;
            // [BTH][PayLd]
			// Compute payload length
			payLoadLength = udpLength - (8 + 12 + 4); // UDP, BTH, RETH, CRC
			memoryWriteCmd.write(memCmd(0, payLoadLength, PKG_NF, PKG_INT, meta.dest_qp));
			// Update state
			rxExh2msnTable_upd_req.write(rxMsnReq(meta.dest_qp, dmaMeta.msn+1));
			// Trigger ACK
			rx_exhEventMetaFifo.write(ackEvent(meta.dest_qp, meta.psn, false));
			rx_pkgSplitTypeFifo.write(pkgSplit(meta.op_code));
			rx_pkgShiftTypeFifo.write(pkgShift(SHIFT_NONE, meta.dest_qp));

			pe_fsmState = META;
			break;
        }
		case RC_RDMA_WRITE_ONLY:
		case RC_RDMA_WRITE_FIRST:
		{
			// [BTH][RETH][PayLd]
			RdmaExHeader<WIDTH> rdmaHeader = exHeader.getRdmaHeader();

			if (rdmaHeader.getLength() != 0)
			{
				//Compute payload length
				payLoadLength = udpLength - (8 + 12 + 16 + 4); //UDP, BTH, RETH, CRC
				//compute remaining length

				// For WRITE-messages, the length of the RDMA Extended Header needs to be subtracted from the overall length as well
				ap_uint<32> headerLen = rdmaHeader.getLength();
				ap_uint<32> remainingLength =  headerLen - payLoadLength;

				// Send external request for writing to memory 
				if(meta.op_code == RC_RDMA_WRITE_ONLY)
				{
					memoryWriteCmd.write(memCmd(rdmaHeader.getVirtualAddress(), payLoadLength, PKG_F, PKG_HOST, meta.dest_qp));
				}
				if(meta.op_code == RC_RDMA_WRITE_FIRST) 
				{
					memoryWriteCmd.write(memCmd(rdmaHeader.getVirtualAddress(), payLoadLength, PKG_NF, PKG_HOST, meta.dest_qp));
				}

				// Update state in the msn table 
				//TODO msn, only for ONLY??
				rxExh2msnTable_upd_req.write(rxMsnReq(meta.dest_qp, dmaMeta.msn+1, rdmaHeader.getVirtualAddress()+payLoadLength, remainingLength, 1));
				// Trigger ACK
				rx_exhEventMetaFifo.write(ackEvent(meta.dest_qp, meta.psn, false));
				rx_pkgSplitTypeFifo.write(pkgSplit(meta.op_code));
				rx_pkgShiftTypeFifo.write(pkgShift(SHIFT_RETH, meta.dest_qp));
				pe_fsmState = META;
			}
			break;
		}
		case RC_RDMA_WRITE_MIDDLE:
		case RC_RDMA_WRITE_LAST:
		{
			// [BTH][PayLd]
			//Fwd data words
			payLoadLength = udpLength - (8 + 12 + 4); //UDP, BTH, CRC
			//compute remaining length
			ap_uint<32> remainingLength = dmaMeta.dma_length - payLoadLength;

			// Issue the write command 
			if(meta.op_code == RC_RDMA_WRITE_LAST) 
			{
				memoryWriteCmd.write(memCmd(dmaMeta.vaddr, payLoadLength, PKG_F, PKG_HOST, meta.dest_qp));
			}
			if(meta.op_code == RC_RDMA_WRITE_MIDDLE) 
			{
				memoryWriteCmd.write(memCmd(dmaMeta.vaddr, payLoadLength, PKG_NF, PKG_HOST, meta.dest_qp));
			}

			//TODO msn only on LAST??
			// Update the msn table 
			rxExh2msnTable_upd_req.write(rxMsnReq(meta.dest_qp, dmaMeta.msn+1, dmaMeta.vaddr+payLoadLength, remainingLength, 1));
			// Trigger ACK
			rx_exhEventMetaFifo.write(ackEvent(meta.dest_qp, meta.psn, false));
			rx_pkgSplitTypeFifo.write(pkgSplit(meta.op_code));
			rx_pkgShiftTypeFifo.write(pkgShift(SHIFT_NONE, meta.dest_qp));
			pe_fsmState = META;
			break;
		}
		case RC_RDMA_READ_REQUEST:
		{
			// [BTH][RETH]
			RdmaExHeader<WIDTH> rdmaHeader = exHeader.getRdmaHeader();
			if (rdmaHeader.getLength() != 0)
			{
				readRequestFifo.write(readRequest(meta.dest_qp, rdmaHeader.getVirtualAddress(), rdmaHeader.getLength(), meta.psn));
				rxExh2msnTable_upd_req.write(rxMsnReq(meta.dest_qp, dmaMeta.msn+1));
			}
			pe_fsmState = META;
			break;
		}
		case RC_RDMA_READ_RESP_ONLY:
		case RC_RDMA_READ_RESP_FIRST:
		case RC_RDMA_READ_RESP_LAST:
		{
			// [BTH][AETH][PayLd]
			//AETH for first and last
			AckExHeader<WIDTH> ackHeader = exHeader.getAckHeader();
			/*if(meta.op_code == RC_RDMA_READ_RESP_ONLY || meta.op_code == RC_RDMA_READ_RESP_LAST)
			{
				m_axis_rx_ack_meta.write(ackMeta(RD_ACK, meta.dest_qp(9,0), ackHeader.getSyndrome(), ackHeader.getMsn()));
			}*/

			// Retransmission is disabled, so don't react to a NAK with a retransmission
			if (ackHeader.isNAK())
			{
				//Trigger retransmit
#ifdef RETRANS_EN
				rx2retrans_req.write(retransmission(meta.dest_qp, meta.psn));
#endif
			}
			/*else
			{
				readReqTable_upd_req.write((rxReadReqUpdate(meta.dest_qp, meta.psn)));
			}*/
			//Write out meta
			payLoadLength = udpLength - (8 + 12 + 4 + 4); //UDP, BTH, AETH, CRC
			rx_pkgShiftTypeFifo.write(pkgShift(SHIFT_AETH, meta.dest_qp));
			
			// Based on the received opcode, issue memory commands 
			if (meta.op_code == RC_RDMA_READ_RESP_FIRST) 
			{
				memoryWriteCmd.write(memCmd(readReqInit.laddr, payLoadLength, PKG_NF, PKG_HOST, meta.dest_qp));
				//TODO maybe not the best way to store the vaddr in the msnTable
				rxExh2msnTable_upd_req.write(rxMsnReq(meta.dest_qp, dmaMeta.msn, readReqInit.laddr+payLoadLength, payLoadLength, readReqInit.lst));
                std::cout << "[RX EXH FSM " << INSTID << "]: read resp first: " << std::hex << dmaMeta.vaddr << ", ctl: " << readReqInit.lst << std::endl;
			}
			if (meta.op_code == RC_RDMA_READ_RESP_ONLY) 
			{
				memoryWriteCmd.write(memCmd(readReqInit.laddr, payLoadLength, readReqInit.lst, PKG_HOST, meta.dest_qp));
				//TODO maybe not the best way to store the vaddr in the msnTable
				rxExh2msnTable_upd_req.write(rxMsnReq(meta.dest_qp, dmaMeta.msn, readReqInit.laddr+payLoadLength, payLoadLength, readReqInit.lst));
                std::cout << "[RX EXH FSM " << INSTID << "]: read resp only: " << std::hex << dmaMeta.vaddr << ", ctl: " << dmaMeta.lst << std::endl;
			}
			if (meta.op_code == RC_RDMA_READ_RESP_LAST) 
			{
				memoryWriteCmd.write(memCmd(dmaMeta.vaddr, payLoadLength, dmaMeta.lst, PKG_HOST, meta.dest_qp));
                std::cout << "[RX EXH FSM " << INSTID << "]: read resp last: " << std::hex << dmaMeta.vaddr << ", ctl: " << dmaMeta.lst << std::endl;
			}
			
			rx_pkgSplitTypeFifo.write(pkgSplit(meta.op_code));
			pe_fsmState = META;
			break;
		}
		case RC_RDMA_READ_RESP_MIDDLE:
			// [BTH][PayLd]
			payLoadLength = udpLength - (8 + 12 + 4); //UDP, BTH, CRC
			rx_pkgShiftTypeFifo.write(pkgShift(SHIFT_NONE, meta.dest_qp));
			memoryWriteCmd.write(memCmd(dmaMeta.vaddr, payLoadLength, PKG_NF, PKG_HOST, meta.dest_qp));
            std::cout << "[RX EXH FSM " << INSTID << "]: read resp middle: " << std::hex << dmaMeta.vaddr << std::endl;

			rxExh2msnTable_upd_req.write(rxMsnReq(meta.dest_qp, dmaMeta.msn, dmaMeta.vaddr+payLoadLength, payLoadLength, dmaMeta.lst));
			rx_pkgSplitTypeFifo.write(pkgSplit(meta.op_code));
			pe_fsmState = META;
			break;
		case RC_ACK:
		{
			// For an ACK, don't perform any particular action anymore
			// [BTH][AETH]
			AckExHeader<WIDTH> ackHeader = exHeader.getAckHeader();
			//m_axis_rx_ack_meta.write(ackMeta(WR_ACK, meta.dest_qp(9,0), ackHeader.getSyndrome(), ackHeader.getMsn()));

			std::cout << "[RX EXH FSM " << INSTID << "]: syndrome: " << std::hex << ackHeader.getSyndrome() << std::endl;
#ifdef RETRANS_EN
			if (ackHeader.isNAK())
			{
				//Trigger retransmit
				std::cout << "[RX EXH FSM " << INSTID << "]: receive NAK" << std::endl;
				rx2retrans_req.write(retransmission(meta.dest_qp, meta.psn));
			}
			/*else if (readReqMeta.oldest_outstanding_readreq < meta.psn && readReqMeta.valid)
			{
				//Trigger retransmit
				std::cout << "[RX EXH FSM " << INSTID << "]: retranmission triggered, outstanding req " << std::hex << readReqMeta.oldest_outstanding_readreq << std::endl;
				rx2retrans_req.write(retransmission(meta.dest_qp, readReqMeta.oldest_outstanding_readreq));
			}*/
#endif
			pe_fsmState = META;
			break;
		}
		default:
			break;
		} //switch meta_Opcode
		break;
	} //switch
}

/**
 * Drop out of order
 */
//Currently not used!!
template <int WIDTH, int INSTID = 0>
void drop_ooo_ibh(	
	stream<net_axis<WIDTH> >& input,
	stream<bool>& metaIn,
	stream<net_axis<WIDTH> >& output
) {
#pragma HLS inline off
#pragma HLS pipeline II=1

	enum fsmType {META, FWD, DROP};
	static fsmType state = META;
    static ap_uint<32> cntPacketsFwd = 0;
    static ap_uint<32> cntPacketsDrop = 0;
    static ap_uint<32> cntBeatsFwd = 0;
    static ap_uint<32> cntBeatsDrop = 0;

	bool drop;
	net_axis<WIDTH> currWord;

	switch (state)
	{
		case META:
			if (!metaIn.empty())
			{
				metaIn.read(drop);
                
				if (drop)
				{
					state = DROP;
				}
				else
				{
					state = FWD;
				}
			}
			break;
		case FWD:
			if (!input.empty())
			{
				input.read(currWord);
				output.write(currWord);
                cntBeatsFwd++;
                //m_cnt_dbg_bf = cntBeatsFwd;
				if (currWord.last)
				{
                    cntPacketsFwd++;
                    //m_cnt_dbg_pf = cntPacketsFwd;
					state = META;
				}
			}
			break;
		case DROP:
			if (!input.empty())
			{
				input.read(currWord);
                cntBeatsDrop++;
                //m_cnt_dbg_bd = cntBeatsDrop;
				if (currWord.last)
				{
                    cntPacketsDrop++;
                    //m_cnt_dbg_pd = cntPacketsDrop;
					state = META;
				}
			}
			break;
	} //switch
}

/**
 * RX EXH payload - basically meaningless? Just forwards the packet to the stream merger in one of three forms. 
 */
template <int WIDTH, int INSTID = 0>
void rx_exh_payload(
	// Meta-Input from rx_exh_fsm has the opcode
	stream<pkgSplit>& metaIn,
	// Whole packet as input from rx_process_exh
	stream<net_axis<WIDTH> >& input,
	stream<net_axis<WIDTH> >& rx_exh2rethShiftFifo,
	stream<net_axis<WIDTH> >& rx_exh2aethShiftFifo,
	stream<net_axis<WIDTH> >& rx_exhNoShiftFifo
) {
#pragma HLS inline off
#pragma HLS pipeline II=1

	// 2-state FSM
	enum fsmStateType {META, PKG};
	static fsmStateType rep_state = META;

	// word to read opcode from rx_exh_fsm
	static pkgSplit meta;

	// Word to read packet as input from rx_process_exh
	net_axis<WIDTH> currWord;

	switch (rep_state)
	{
	case META:
		// Read the opcode from the input queue and switch to PKG afterwards 
		if (!metaIn.empty())
		{
			metaIn.read(meta);
			rep_state = PKG;
		}
		break;
	case PKG:
		if (!input.empty())
		{
			// Read the whole packet as received from rx_process_exh
			input.read(currWord); 

			// Depending on the specific Extended Header, send out the word as either Reth, Aeth or no header shift
			if (checkIfRethHeader(meta.op_code))
			{
#ifdef DBG_FULL
				std::cout << "[RX EXH PAYLOAD" << INSTID << "]: EXH payload";
				print(std::cout, currWord);
				std::cout << std::endl;
#endif
				rx_exh2rethShiftFifo.write(currWord);

			}
			else if ((meta.op_code == RC_RDMA_READ_RESP_ONLY) || (meta.op_code == RC_RDMA_READ_RESP_FIRST) ||
					(meta.op_code == RC_RDMA_READ_RESP_LAST))
			{
				rx_exh2aethShiftFifo.write(currWord);
			}
			else
			{
				rx_exhNoShiftFifo.write(currWord);
			}

			if (currWord.last)
			{
				rep_state = META;
			}
		}
		break;
	} //switch
}

/**
 * Handling of the read requests - generates memory commands for reading the requested areas from memory
 */
template <int INSTID = 0>
void handle_read_requests(	
	// Input from rx_exh_fsm, has QPN, vaddr, dma_length, PSN and a single host bit again 
	stream<readRequest>& requestIn,
	// Output to mem_cmd_merger, has all internal required for memory commands
	stream<memCmdInternal>&	memoryReadCmd,
	// Output to meta merger, carries opcode, QPN, virtual address, length, PSN, valid signal and NAK-bit (essential information from the headers)
	stream<event>& readEventFifo
) {
#pragma HLS inline off
#pragma HLS pipeline II=1

	// Two-state FSM 
	enum hrr_fsmStateType {META, GENERATE};
	static hrr_fsmStateType hrr_fsmState = META;

	// Current word for loading the read request
	static readRequest request; //Need QP, dma_length, vaddr

	// Variables to dissect the read request in single fields 
	ibOpCode readOpcode;
	ap_uint<64> readAddr;
	ap_uint<32> readLength;
	ap_uint<32> dmaLength;

	switch (hrr_fsmState)
	{
	case META:
		// If a read request is available, fetch it from the input queue for further processing 
		if (!requestIn.empty())
		{
			requestIn.read(request);

			// Dissect the read request in its separate fields 
			readAddr = request.vaddr;
			readLength = request.dma_length;
			dmaLength = request.dma_length;
			readOpcode = RC_RDMA_READ_RESP_ONLY;

			// If the requested DMA length is longer than the MTU, split workload up and switch to GENERATE
			if (request.dma_length > PMTU)
			{
				// Set read length to MTU, change vaddr and dma_length
				readLength = PMTU;
				request.vaddr += PMTU;
				request.dma_length -= PMTU;
				readOpcode = RC_RDMA_READ_RESP_FIRST;
				hrr_fsmState = GENERATE;
			}

			// Generate the memory Read Command 
			memoryReadCmd.write(memCmdInternal(readOpcode, request.qpn, readAddr, readLength, request.host));
			
			// Send out the event-info to the meta merger
			readEventFifo.write(event(readOpcode, request.qpn, readLength, request.psn));
            std::cout << "[READ_HANDLER " << INSTID << "]: read handler init packet, psn " << request.psn << std::endl;
		}
		break;
	case GENERATE:
		// Second step if requested read length is larger than MTU: Reset vaddr and dma_length 
		readAddr = request.vaddr;
		readLength = request.dma_length;

		// If still too long: Adapt addresses and length, generate the middle read 
		if (request.dma_length > PMTU)
		{
			readLength = PMTU;
			request.vaddr += PMTU;
			request.dma_length -= PMTU;
			readOpcode = RC_RDMA_READ_RESP_MIDDLE;
            std::cout << "[READ_HANDLER " << INSTID << "]: read handler middle packet, psn " << request.psn << std::endl;
		}
		// Generate the last read if length now fits into MTU
		else
		{
			readOpcode = RC_RDMA_READ_RESP_LAST;
			hrr_fsmState = META;
            std::cout << "[READ_HANDLER " << INSTID << "]: read handler last packet, psn " << request.psn << std::endl;
		}
		request.psn++;
		// Send out the memory read command, generate read event 
		memoryReadCmd.write(memCmdInternal(readOpcode, request.qpn, readAddr, readLength, request.host));
		readEventFifo.write(event(readOpcode, request.qpn, readLength, request.psn));

		break;
	}
}

template <int WIDTH, int INSTID = 0>

// STREAM MERGE in the block diagram
void merge_rx_pkgs(	
	// Input queue from rx_exh_fsm, contains the shift type (RETH / AETH / None)
	stream<pkgShift>& rx_pkgShiftTypeFifo,
	// Inputs of the whole packet as received from rx_exh_payload
	stream<net_axis<WIDTH> >& rx_aethSift2mergerFifo,
	stream<net_axis<WIDTH> >& rx_rethSift2mergerFifo,
	stream<net_axis<WIDTH> >& rx_NoSift2mergerFifo,
	// package sent out for further processing 
	stream<net_axis<WIDTH> >& m_axis_mem_write_data
) {
#pragma HLS inline off
#pragma HLS pipeline II=1

	// Four states, three states for the three different package types 
	enum mrpStateType{IDLE, FWD_AETH, FWD_RETH, FWD_NONE};
	static mrpStateType state = IDLE;

	pkgShift shift;

	switch (state)
	{
	case IDLE:
		// Wait until the input-info about the package type becomes available 
		if (!rx_pkgShiftTypeFifo.empty())
		{
			// Read the package-type input and switch to the corresponding following state for further processing
			rx_pkgShiftTypeFifo.read(shift);
			if (shift.type == SHIFT_AETH)
			{
				state = FWD_AETH;
			}
			else if (shift.type == SHIFT_RETH)
			{
				state = FWD_RETH;
			}
			else
			{
				state = FWD_NONE;
			}
		}
		break;
	case FWD_AETH:
		// Process Acks
		if (!rx_aethSift2mergerFifo.empty())
		{
			net_axis<WIDTH> currWord;

			// Read incoming word and forward it to the memory - why though? It's an ACK.
			rx_aethSift2mergerFifo.read(currWord);
			m_axis_mem_write_data.write(currWord);

			if (currWord.last)
			{
				state = IDLE;
			}
		}
		break;
	case FWD_RETH:
		// Process RDMA messages
		if (!rx_rethSift2mergerFifo.empty())
		{
			net_axis<WIDTH> currWord;
			// Same treatment
			rx_rethSift2mergerFifo.read(currWord);
			m_axis_mem_write_data.write(currWord);
			
			if (currWord.last)
			{
				state = IDLE;
			}
		}
		break;
	case FWD_NONE:
		if (!rx_NoSift2mergerFifo.empty())
		{
			// Same treatment
			net_axis<WIDTH> currWord;
			rx_NoSift2mergerFifo.read(currWord);
			m_axis_mem_write_data.write(currWord);
			
			if (currWord.last)
			{
				state = IDLE;
			}
		}
		break;
	}//switch
}

// ------------------------------------------------------------------------------------------------
// TX path
// ------------------------------------------------------------------------------------------------

/**
 * Local request handler - first module after user logic to handle RDMA-requests
 */
template <int INSTID = 0>
void local_req_handler(
	// Incoming requests, holds opcode, qpn, host-bit, lst-bit, offset, parameters with local and remote buffer address
	stream<txMeta>& s_axis_sq_meta,
#ifdef RETRANS_EN
	stream<retransEvent>& retransEventFifo,
	stream<retransAddrLen>& tx2retrans_insertAddrLen,
#endif
	// Output queue to mem_cmd_merger, holds qpn, addr, len, host, sync and offset
	stream<memCmdInternal>&	tx_local_memCmdFifo, //TODO rename
	//stream<mqInsertReq<ap_uint<64> > >&	tx_localReadAddrFifo,
	// Output to meta merger (tx_appMetaFifo), holds opcode, qpn, addr, length, psn, valid- and NAK-bit
	stream<event>&tx_localTxMeta,
	// Retransmitter counter, goes to generate_ibh
    ap_uint<32>& regRetransCount)
{
#pragma HLS inline off
#pragma HLS pipeline II=1

	//enum fsmStateType {META, GENERATE};
	//static fsmStateType lrh_state;

	// Variable to save incoming meta-input
	static txMeta meta;

	// Words for generating the outputs
	event ev;
	retransEvent rev;
    static ap_uint<32> retransCount = 0;

	// Bitfields of requests - local buffer address, remote buffer address, length of request, lst, offset
	ap_uint<64> laddr;
	ap_uint<64> raddr;
	ap_uint<32> length;
    ap_uint<1>  lst;
    ap_uint<4>  offs;

	//switch (lrh_state)
	//{
	//case META:
#ifdef RETRANS_EN
	if (!retransEventFifo.empty())
	{
		retransEventFifo.read(rev);
		tx_localTxMeta.write(event(rev.op_code, rev.qpn, rev.remoteAddr, rev.length, rev.psn));

        retransCount++;
        regRetransCount = retransCount;        

		std::cout << std::dec << "[LOCAL REQ HANDLER " << INSTID << "]: retransmission: length: " << rev.length << ", local addr: " << std::hex << rev.localAddr << ", remote addres: " << rev.remoteAddr << ", psn: " << rev.psn << ", op code: " << rev.op_code << ", qpn: " << rev.qpn << std::endl;
		if (rev.op_code != RC_RDMA_READ_REQUEST)
		{
			std::cout << "[LOCAL REQ HANDLER " << INSTID << "]: retranmission writing into memCmd with lengh " << rev.length << std::endl;
			tx_local_memCmdFifo.write(memCmdInternal(rev.op_code, rev.qpn, rev.localAddr, rev.length, PKG_INT, rev.offs));
		} 

	}
	else if (!s_axis_sq_meta.empty())
#else

	// If external meta request is available, read it 
	if (!s_axis_sq_meta.empty())
#endif
	{
		s_axis_sq_meta.read(meta); // (len-32 | remote-64 | local-64)

		// Dissect the local request in the suitable bitfields 
		laddr = meta.params(63,0);
		raddr = meta.params(127,64);
		length = meta.params(159,128);
        lst = meta.lst;
        offs = meta.offs;

		std::cout << std::dec << "[LOCAL REQ HANDLER " << INSTID << "]: transmission: length: " << std::dec << length << ", local addr: " << std::hex << laddr << ", remote addres: " << raddr << std::endl;

		// In case of a READ_REQUEST: Generate output to the meta merger
		if(meta.op_code == RC_RDMA_READ_REQUEST)
		{
			tx_localTxMeta.write(event(meta.op_code, meta.qpn, raddr, length));
			//tx_localReadAddrFifo.write(mqInsertReq<ap_uint<64> >(meta.qpn, laddr));
		}

		// In case of WRITE or SEND: Generate output to the meta merger and memory command to the mem_cmd_merger (probably to read the memory content that is to be sent to remote)
		if(meta.op_code == RC_RDMA_WRITE_MIDDLE || meta.op_code == RC_RDMA_WRITE_FIRST ||
			meta.op_code == RC_RDMA_WRITE_LAST || meta.op_code == RC_RDMA_WRITE_ONLY || meta.op_code == RC_SEND_ONLY ||
            meta.op_code == RC_SEND_FIRST || meta.op_code == RC_SEND_MIDDLE || meta.op_code == RC_SEND_LAST)
		{
			tx_localTxMeta.write(event(meta.op_code, meta.qpn, raddr, length));
			tx_local_memCmdFifo.write(memCmdInternal(meta.op_code, meta.qpn, laddr, length, meta.host));	
		}
#ifdef RETRANS_EN
		tx2retrans_insertAddrLen.write(retransAddrLen(laddr, raddr, length, lst, offs));
#endif
	}
}

/**
 * Local memory command merger - Handles all read commands to the memory. Either remote read requests, or outgoing write requests that need memory content
 */
template <int WIDTH, int INSTID = 0>
void mem_cmd_merger(
	// Incoming Remote Read Requests from handle read req
	stream<memCmdInternal>& remoteReadRequests,
	// Incoming Local Read Requests from local_req_handler
	stream<memCmdInternal>& localReadRequests,
	// Unified outgoing memory read commands, has information on the corresponding vFPGA etc. 
	stream<memCmd>& out,
	// Output to tx_pkg_arbiter, has type of package (ACK, RDMA, RAW, IMMEDIATE)
	stream<pkgInfo>& pkgInfoFifo
) {
#pragma HLS inline off
#pragma HLS pipeline II=1

	memCmdInternal cmd;

	// Deal with remote read requests and load them into the local variable 
	if (!remoteReadRequests.empty())
	{
        std::cout << "[MEM CMD MERGER " << INSTID << "]: reading remote request" << std::endl;
		remoteReadRequests.read(cmd);

		// Three types of READs (Only / Last, Middle, First) - generate memory commands accordingly and send out ACKs
		if(cmd.op_code == RC_RDMA_READ_RESP_ONLY || cmd.op_code == RC_RDMA_READ_RESP_LAST) 
		{
			out.write(memCmd(cmd.addr, cmd.len, PKG_F, cmd.host, cmd.qpn));
			pkgInfoFifo.write(pkgInfo(AETH, ((cmd.len+(WIDTH/8)-1)/(WIDTH/8))));
		}
		if(cmd.op_code == RC_RDMA_READ_RESP_MIDDLE) 
		{
			out.write(memCmd(cmd.addr, cmd.len, PKG_NF, cmd.host, cmd.qpn));
			pkgInfoFifo.write(pkgInfo(RAW, ((cmd.len+(WIDTH/8)-1)/(WIDTH/8))));
		}
		if(cmd.op_code == RC_RDMA_READ_RESP_FIRST) 
		{
			out.write(memCmd(cmd.addr, cmd.len, PKG_NF, cmd.host, cmd.qpn));
			pkgInfoFifo.write(pkgInfo(AETH, ((cmd.len+(WIDTH/8)-1)/(WIDTH/8))));
		}
	}

	// Deal with local memory requests for SEND / WRITE
	else if (!localReadRequests.empty())
	{
		localReadRequests.read(cmd);

		std::cout << "[MEM CMD MERGER " << INSTID << "]: reading local request, opcode - " << std::hex << cmd.op_code 
            << ", sync - " << cmd.sync << ", offs - " << cmd.offs << ", vaddr - " << cmd.addr << ", len - " << cmd.len << std::endl;

		// Generate memory commands and packet type depending on the processed opcode 
		// To be cleared: What is the SYNC-option for?
		if(cmd.op_code == RC_RDMA_WRITE_ONLY)
		{
			if(cmd.sync)
                out.write(memCmd(cmd.addr, cmd.len, PKG_F, SYNC, cmd.host, cmd.offs, cmd.qpn));
            else    
                out.write(memCmd(cmd.addr, cmd.len, PKG_F, cmd.host, cmd.qpn));
			pkgInfoFifo.write(pkgInfo(RETH, ((cmd.len+(WIDTH/8)-1)/(WIDTH/8))));
		}
		if(cmd.op_code == RC_RDMA_WRITE_FIRST)
		{
            if(cmd.sync)
			    out.write(memCmd(cmd.addr, cmd.len, PKG_NF, SYNC, cmd.host, cmd.offs, cmd.qpn));
            else
                out.write(memCmd(cmd.addr, cmd.len, PKG_NF, cmd.host, cmd.qpn));
			pkgInfoFifo.write(pkgInfo(RETH, ((cmd.len+(WIDTH/8)-1)/(WIDTH/8))));
		}
		if(cmd.op_code == RC_RDMA_WRITE_MIDDLE || cmd.op_code == RC_SEND_FIRST || 
            cmd.op_code == RC_SEND_MIDDLE)
		{
            if(cmd.sync)
			    out.write(memCmd(cmd.addr, cmd.len, PKG_NF, SYNC, cmd.host, cmd.offs, cmd.qpn));
            else    
                out.write(memCmd(cmd.addr, cmd.len, PKG_NF, cmd.host, cmd.qpn));
			pkgInfoFifo.write(pkgInfo(RAW, ((cmd.len+(WIDTH/8)-1)/(WIDTH/8))));
		}
		if(cmd.op_code == RC_RDMA_WRITE_LAST || cmd.op_code == RC_SEND_LAST)
		{
			if(cmd.sync)
                out.write(memCmd(cmd.addr, cmd.len, PKG_F, SYNC, cmd.host, cmd.offs, cmd.qpn));
            else
                out.write(memCmd(cmd.addr, cmd.len, PKG_F, cmd.host, cmd.qpn));
			pkgInfoFifo.write(pkgInfo(RAW, ((cmd.len+(WIDTH/8)-1)/(WIDTH/8))));
		}
		if(cmd.op_code == RC_SEND_ONLY) 
		{
            if(cmd.sync)
                out.write(memCmd(cmd.addr, cmd.len, PKG_F, SYNC, cmd.host, cmd.offs, cmd.qpn));
            else
                out.write(memCmd(cmd.addr, cmd.len, PKG_F, cmd.host, cmd.qpn));
            pkgInfoFifo.write(pkgInfo(RAW, ((cmd.len+(WIDTH/8)-1)/(WIDTH/8))));
		}
	}

}

/**
 * TX pkg arbitration
 */
template <int WIDTH, int INSTID = 0>
void tx_pkg_arbiter(
	// Meta-Information coming from mem_cmd merger 
	stream<pkgInfo>& tx_pkgInfoFifo,
	// Input from the memory, has the read data
	stream<net_axis<WIDTH> >& s_axis_mem_read_data,
	// Output tx_split2aethShift to append_payload - data as read from the memory, used for acking remote read requests
	stream<net_axis<WIDTH> >& remoteReadData,
	// Output tx_rethMerge2rethShift to append_payload - data as read from the memory, used for appending to WRITE or SEND 
	stream<net_axis<WIDTH> >& localReadData,
	// Output as read from memory to append_payload - used for packages of third type
	stream<net_axis<WIDTH> >& rawPayFifo
) {
#pragma HLS inline off
#pragma HLS pipeline II=1

	// FSM with four states for forwarding the different payloads for the different packet types 
	enum mrpStateType{IDLE, FWD_AETH, FWD_RETH, FWD_RAW};
	static mrpStateType state = IDLE;
	// Variable with a word counter 
	static ap_uint<8> wordCounter = 0;

	// Variables for loading meta information and the current word from memory
	static pkgInfo info;
	net_axis<WIDTH> currWord;

	switch (state)
	{
	// Wait until meta-input arrives at the queue, load it and make further processing decision based on the type of packet
	case IDLE:
		if (!tx_pkgInfoFifo.empty())
		{
			tx_pkgInfoFifo.read(info);
			wordCounter = 0;

			// Depending on the loaded header type, move to the correct state for further processing of the packet 
			if (info.type == AETH)
			{
				state = FWD_AETH;
			}
			else if (info.type == RETH)
			{
				state = FWD_RETH;
			}
			else
			{
				state = FWD_RAW;
			}
		}
		break;
	case FWD_AETH:
		// Read memory input when available, increment the word counter, check for last word and pass on the memory word
		if (!s_axis_mem_read_data.empty())
		{
			s_axis_mem_read_data.read(currWord);

			wordCounter++;
			if (currWord.last)
			{
				state = IDLE;
			}
			if (wordCounter == PMTU_WORDS)
			{
				currWord.last = 1;
				state = IDLE;
			}
			remoteReadData.write(currWord);
		}
		break;
	case FWD_RETH:
		// Same as before, but pass on through its own queue 
		if (!s_axis_mem_read_data.empty())
		{
			s_axis_mem_read_data.read(currWord);

			wordCounter++;
			if (currWord.last)
			{
				state = IDLE;
			}
			if (wordCounter == PMTU_WORDS)
			{
				currWord.last = 1;
				state = IDLE;
			}
			localReadData.write(currWord);
		}
		break;
	case FWD_RAW:
		// Same as before, but pass on through its own queue 
		if (!s_axis_mem_read_data.empty())
		{
			s_axis_mem_read_data.read(currWord);
			
			wordCounter++;
			if (currWord.last)
			{
				state = IDLE;
			}
			if (wordCounter == PMTU_WORDS)
			{
				currWord.last = 1;
				state = IDLE;
			}
			rawPayFifo.write(currWord);
		}
		break;
	}//switch

}

/**
 * TX meta merger - three sources of inputs for generating some kind of output: Response 
 * to a remote read request, ACKing another remote message, writing out a local RDMA-packet
 * 
 * rx_ackEventFifo RC_ACK from ibh and exh
 * rx_readEvenFifo READ events from RX side
 * tx_appMetaFifo, retransmission events, WRITEs and READ_REQ only
 */
template <int INSTID = 0>
void meta_merger(	
	// Input from the stream merger, carries QPN, PSN, validPSN and NAK-bit to generate an ACK
	stream<ackEvent>&	rx_ackEventFifo,
	// Input from handle read request, carries opcode, QPN, ADDR, length, PSN, validPSN, NAK-bit to respond to a read request
	stream<event>&		rx_readEvenFifo,
	// Input from the local_req_handler to generate responses to read requests 
	stream<event>&		tx_appMetaFifo,
	//stream<event>&		timer2exhFifo,
	// Number queue
	stream<ap_uint<16> >&	tx_connTable_req,
	// Output of pre-set BTH, goes to generate_ibh
	stream<ibhMeta>&	tx_ibhMetaFifo,
	// Output of pre-set Extended Header, goes to generate_exh
	stream<event>&		tx_exhMetaFifo
) {
#pragma HLS inline off
#pragma HLS pipeline II=1

	ackEvent aev;
	event ev;
	ap_uint<16> key = 0; //TODO hack

	// Check the three sources of incoming meta information for generating an outgoing message

	// Acking a remote message 
	if (!rx_ackEventFifo.empty())
	{
		rx_ackEventFifo.read(aev);

		tx_connTable_req.write(aev.qpn(15, 0));
		// PSN used for read response
		tx_ibhMetaFifo.write(ibhMeta(RC_ACK, key, aev.qpn, aev.psn, aev.validPsn));
		tx_exhMetaFifo.write(event(aev));
		std::cout << "[META MERGER " << INSTID << "]: reading from ack event with qpn " << aev.qpn << ", psn " << aev.psn << std::endl;
	}

	// Answering a remote read request
	else if (!rx_readEvenFifo.empty())
	{
		rx_readEvenFifo.read(ev);
		tx_connTable_req.write(ev.qpn(15, 0));
		// PSN used for read response
		tx_ibhMetaFifo.write(ibhMeta(ev.op_code, key, ev.qpn, ev.psn, ev.validPsn));
		tx_exhMetaFifo.write(ev);
		std::cout << "[META MERGER " << INSTID << "]: reading from read event with qpn " << ev.qpn << ", psn " << ev.psn << std::endl;
	}

	// Generating Meta for a local RDMA application 
	else if (!tx_appMetaFifo.empty()) //TODO rename
	{
		tx_appMetaFifo.read(ev);

		ap_uint<22> numPkg = 1;
		if (ev.op_code == RC_RDMA_READ_REQUEST)
		{
			numPkg = (ev.length+(PMTU-1)) / PMTU;
		}

		tx_connTable_req.write(ev.qpn(15, 0));
		if (ev.validPsn) //retransmit
		{
			tx_ibhMetaFifo.write(ibhMeta(ev.op_code, key, ev.qpn, ev.psn, ev.validPsn));
		}
		else //local
		{
			tx_ibhMetaFifo.write(ibhMeta(ev.op_code, key, ev.qpn, numPkg));
		}
		tx_exhMetaFifo.write(ev);
		std::cout << "[META MERGER " << INSTID << "]: reading from app event with qpn " << ev.qpn << ", psn " << ev.psn << std::endl;
	}
	/*else if (!timer2exhFifo.empty())
	{
		timer2exhFifo.read(ev);

		tx_connTable_req.write(ev.qpn(15, 0));
		// PSN used for retransmission
		tx_ibhMetaFifo.write(ibhMeta(ev.op_code, key, ev.qpn, ev.psn, ev.validPsn));
		tx_exhMetaFifo.write(ev);
	}*/
}

/**
 * Generate IBH
 * 
 * For everything, except READ_RSP, we get PSN from state_table
 */
template <int WIDTH, int INSTID = 0>
void generate_ibh(	
	// Pre-filled BTH coming from the meta merger
	stream<ibhMeta>& metaIn,
	// Destination QP number coming from the connection table 
	stream<ap_uint<24> >& dstQpIn,
	// Input from the state table - comes with all kind of information on PSN
	stream<stateTableEntry>& stateTable2txIbh_rsp,
	// State input to the state table - comes with qpn, psn, write-bit
	stream<txStateReq>&	txIbh2stateTable_upd_req,
#ifdef RETRANS_EN
	stream<retransMeta>& tx2retrans_insertMeta,
#endif
	// Output-Fifo with the generated BTH, goes to prepend_ibh
	stream<BaseTransportHeader<WIDTH> >& tx_ibhHeaderFifo
) {
#pragma HLS inline off
#pragma HLS pipeline II=1

	// Two-state FSM for generating the IBH - start off in META-state
	enum fsmStateType {META, GET_PSN};
	static fsmStateType gi_state = META;

	// Variable to generate the final BTH
	static BaseTransportHeader<WIDTH> header;

	// Storage Variable to read the incoming BTH-meta information 
	static ibhMeta meta;

	net_axis<WIDTH> currWord;

	// Entry to the Qeuepair Table
	stateTableEntry qpState; //TODO what is really required

	// Variable to read the destination QP
	ap_uint<24> dstQp;

	switch(gi_state)
	{
	case META:
		// If BTH and destination QP is available, fetch from the input queues and store
		if (!metaIn.empty() && !dstQpIn.empty())
		{
			metaIn.read(meta);
			dstQpIn.read(dstQp);
			meta.partition_key = 0xFFFF; //TODO this is hard-coded, where does it come from??
			header.clear();

			// Set header-opcode and partition key
			header.setOpCode(meta.op_code);
			header.setPartitionKey(meta.partition_key);

			// Set MigReq-Bit to 1
			header.setMigReq(true);

			//PSN only valid for READ_RSP, otherwise we get it in state GET_PSN
			header.setPsn(meta.psn);

			// Set destination QP in the header
			header.setDstQP(dstQp); //TODO ist meta.dest_qp required??
			if (meta.validPSN)
			{
				// If PSN is already valid, send out the packet to pretend_ibh
				tx_ibhHeaderFifo.write(header);
				std::cout << "[GENERATE IBH " << INSTID << "]: input meta, opcode 0x" << meta.op_code  << " valid psn " << std::hex << meta.psn << std::endl;
			}
			else
			{
				// Otherwise, start a request to the state table to get the PSN
				txIbh2stateTable_upd_req.write(txStateReq(meta.dest_qp)); //TODO this is actually our qp
				gi_state = GET_PSN;
			}
		}
		break;
	case GET_PSN:
		// Wait for the response from the state table 
		if (!stateTable2txIbh_rsp.empty())
		{
			stateTable2txIbh_rsp.read(qpState);

			if (meta.op_code == RC_ACK)
			{
				// If it's an ACK, set the PSN
				header.setPsn(qpState.resp_epsn-1); //TODO -1 necessary??
				std::cout << "[GENERATE IBH " << INSTID << "]: RC_ACK psn " << std::hex << qpState.resp_epsn-1 << std::endl;
			}
			else
			{
				// Else: Set the PSN, request an ACK and update the PSN in the state table. 
				header.setPsn(qpState.req_next_psn);
				header.setAckReq(true);
				//Update PSN
				ap_uint<24> nextPsn = qpState.req_next_psn + meta.numPkg;
				txIbh2stateTable_upd_req.write(txStateReq(meta.dest_qp, nextPsn));

				//Store Packet descirptor in retransmitter table
#ifdef RETRANS_EN
				tx2retrans_insertMeta.write(retransMeta(meta.dest_qp, qpState.req_next_psn, meta.op_code));
#endif
				std::cout << "[GENERATE IBH " << INSTID << "]: generate header opcode 0x" << std::hex << meta.op_code << " psn " << qpState.req_next_psn << std::endl;
			}
			tx_ibhHeaderFifo.write(header);
			gi_state = META;
		}
		break;
	}
}

/**
 * Generate EXH
 * 
 * Types currently supported: DETH, RETH, AETH, ImmDt, IETH
 *
 * For reliable connections, page 246, 266, 269
 * SEND ONLY: PayLd
 * RDMA WRITE ONLY: RETH, PayLd
 * RDMA WRITE FIRST: RETH, PayLd
 * RDMA WRITE MIDDLE: PayLd
 * RDMA WRITE LAST: PayLd
 * RDMA READ REQUEST: RETH
 * RDMA READ RESPONSE ONLY: AETH, PayLd
 * RDMA READ RESPONSE FIRST: AETH, PayLd
 * RDMA READ RESPONSE MIDDLE: PayLd
 * RDMA READ RESPONSE LAST: AETH, PayLd
 * ACK: AETH
 */
template <int WIDTH, int INSTID = 0>
void generate_exh(
	// Meta-Information coming from the meta_merger, including the opcode, QPN, buffer address, requested length, PSN and valid- and NAK-bits.
	stream<event>& metaIn,
	// Input from the MSN-Table: Has the Message Serial Number and r_key
	stream<txMsnRsp>& msnTable2txExh_rsp,
	// QPN sent out to the MSN-Table to query it for r_key and related information 
	stream<ap_uint<16> >& txExh2msnTable_req,
	//stream<txReadReqUpdate>& tx_readReqTable_upd,
	// Length of the packet, sent out to the tx_ipUdpmetaMerger
	stream<ap_uint<16> >& lengthFifo,
	// Forwarded to append_payload to inform about the type of packet that
	stream<txPacketInfo>& packetInfoFifo,
#ifdef RETRANS_EN
	stream<ap_uint<24> >&	txSetTimer_req,
	//stream<retransAddrLen>&		tx2retrans_insertAddrLen,
#endif
	// Output of the completely assembled Extended Header
	stream<net_axis<WIDTH> >& output
) {
#pragma HLS inline off
#pragma HLS pipeline II=1
	// Three-state FSM 
	enum ge_fsmStateType {META, GET_MSN, PROCESS};
	static ge_fsmStateType ge_state = META;

	// Variable to load meta-input into 
	static event meta;

	// Variable to send out the generated Extended Header with 
	net_axis<WIDTH> sendWord;

	// Two options for rdmaHeaders and ackHeaders
	static RdmaExHeader<WIDTH> rdmaHeader;
	static AckExHeader<WIDTH>  ackHeader;

	// Boolean to acknowledge that meta-information was written 
	static bool metaWritten;

	// Variable to store incoming meta-information from the msn-table
	static txMsnRsp msnMeta;

	// Output to the udp-merger with the required length of the packet 
	ap_uint<16> udpLen;

	// Type of the packet
	txPacketInfo info;

	switch(ge_state)
	{
	case META:
		// Read the meta-information from the meta-merger 
		if (!metaIn.empty())
		{
			rdmaHeader.clear();
			ackHeader.clear();

			metaIn.read(meta);
			metaWritten = false;
			//if (meta.op_code == RC_RDMA_READ_RESP_ONLY || meta.op_code == RC_RDMA_READ_RESP_FIRST || meta.op_code == RC_RDMA_READ_RESP_MIDDLE || meta.op_code == RC_RDMA_READ_RESP_LAST || meta.op_code == RC_ACK)
			{
				// Start request to the msn-table with the QPN as received from the meta-input
				txExh2msnTable_req.write(meta.qpn);
				ge_state = GET_MSN;
			}
			//else
			{
				// Start Timer for RDMW_WRITE_* & RDMA_READ_REQUEST

				//txSetTimer_req.write(meta.qpn);
				//ge_state = PROCESS;
			}
#ifdef RETRANS_EN
			//TODO PART HIST
			if (meta.op_code == RC_RDMA_WRITE_ONLY || meta.op_code == RC_RDMA_WRITE_FIRST || meta.op_code == RC_RDMA_WRITE_MIDDLE || meta.op_code == RC_RDMA_WRITE_LAST || meta.op_code == RC_RDMA_READ_REQUEST)
			{
				txSetTimer_req.write(meta.qpn);
			}
#endif
		}
		break;
	case GET_MSN:
		// Wait for the input from the msn-table and store it in a variable for further processing
		if (!msnTable2txExh_rsp.empty())
		{
			msnTable2txExh_rsp.read(msnMeta);
			ge_state = PROCESS;
		}
		break;
	case PROCESS:
		{
			sendWord.last = 0;
			switch(meta.op_code)
			{

			// WRITE_ONLY / FIRST: Extended Header and meta-information to append
			case RC_RDMA_WRITE_ONLY:
			case RC_RDMA_WRITE_FIRST:
			{
				// [BTH][RETH][PayLd]
				// Construct the RDMA Extended Header with the vaddr, the length of the packet and the remote key as read from the msn-table
				rdmaHeader.setVirtualAddress(meta.addr);
				rdmaHeader.setLength(meta.length); //TODO Move up??
				rdmaHeader.setRemoteKey(msnMeta.r_key);
				ap_uint<8> remainingLength = rdmaHeader.consumeWord(sendWord.data);
				sendWord.keep = ~0;
				sendWord.last = (remainingLength == 0);
				std::cout << "[GENERATE EXH " << INSTID << "]: RDMA_WRITE_ONLY/FIRST" << std::endl;
#ifdef DBG_FULL
				print(std::cout, sendWord);
				std::cout << std::endl;
#endif
				output.write(sendWord);
				if (remainingLength == 0)
				{
					//TODO
				}

				// Send out the meta-information to append_payload
				if (!metaWritten) //TODO we are losing 1 cycle here
				{
					info.isAETH = false;
					info.hasHeader = true;
					info.hasPayload = (meta.length != 0); //TODO should be true
					packetInfoFifo.write(info);

					//BTH: 12, RETH: 16, PayLd: x, ICRC: 4
					ap_uint<32> payloadLen = meta.length;
					if (meta.op_code == RC_RDMA_WRITE_FIRST)
					{
						payloadLen = PMTU;
					}
					// Seems as if the 2 trailing padding bytes don't occur here, but somewhere later in the chain
					udpLen = 12+16+payloadLen+4; //TODO dma_len can be much larger, for multiple packets we need to split this into multiple packets
					lengthFifo.write(udpLen);
					//Store meta for retransmit
					metaWritten = true;
				}
				break;
			}

			// WRITE_MIDDLE / LAST / SEND: No Extended Header, only meta-information to append_payload
			case RC_RDMA_WRITE_MIDDLE:
			case RC_RDMA_WRITE_LAST:
			case RC_SEND_ONLY:
            case RC_SEND_FIRST:
            case RC_SEND_MIDDLE:
            case RC_SEND_LAST:
				// [BTH][PayLd]
				info.isAETH = false;
				info.hasHeader = false;
				info.hasPayload = (meta.length != 0); //TODO should be true
				packetInfoFifo.write(info);
				//BTH: 12, PayLd: x, ICRC: 4
				udpLen = 12+meta.length+4;
				lengthFifo.write(udpLen);
				//Store meta for retransmit
				ge_state = META;
				break;
			case RC_RDMA_READ_REQUEST:
			{
				// [BTH][RETH]
				rdmaHeader.setVirtualAddress(meta.addr);
				rdmaHeader.setLength(meta.length); //TODO Move up??
				rdmaHeader.setRemoteKey(msnMeta.r_key);
				ap_uint<8> remainingLength = rdmaHeader.consumeWord(sendWord.data);
				sendWord.keep = ~0;
				sendWord.last = (remainingLength == 0);
				std::cout << "[GENERATE EXH " << INSTID << "]: RDMA_READ_RWQ" << std::endl;
#ifdef DBG_FULL
				print(std::cout, sendWord);
				std::cout << std::endl;
#endif
				output.write(sendWord);
				if (!metaWritten) //TODO we are losing 1 cycle here
				{
					info.isAETH = false;
					info.hasHeader = true;
					info.hasPayload = false; //(meta.length != 0); //TODO should be true
					packetInfoFifo.write(info);

					//BTH: 12, RETH: 16, PayLd: x, ICRC: 4
					udpLen = 12+16+0+4; //TODO dma_len can be much larger, for multiple packets we need to split this into multiple packets
					lengthFifo.write(udpLen);
					//Update Read Req max FWD header, TODO it is not exacly clear if meta.psn or meta.psn+numPkgs should be used
					//TODO i think psn is only used here!!
					//tx_readReqTable_upd.write(txReadReqUpdate(meta.qpn, meta.psn));
					//Store meta for retransmit
					metaWritten = true;
				}
				break;
			}
			case RC_RDMA_READ_RESP_ONLY:
			case RC_RDMA_READ_RESP_FIRST:
			case RC_RDMA_READ_RESP_LAST:
			{
				// [BTH][AETH][PayLd]
				//AETH for first and last
				ackHeader.setSyndrome(0x00);
				ackHeader.setMsn(msnMeta.msn);
				std::cout << "[GENERATE EXH " << INSTID << "]: RDMA_READ_RESP MSN:" << ackHeader.getMsn() << std::endl;
				ackHeader.consumeWord(sendWord.data); //TODO
				{
					info.isAETH = true;
					info.hasHeader = true;
					info.hasPayload = (meta.length != 0); //TODO should be true
					packetInfoFifo.write(info);

					sendWord.keep((AETH_SIZE/8)-1, 0) = 0xFF;
					sendWord.keep(WIDTH/8 -1, (AETH_SIZE/8)) = 0;
					sendWord.last = 1;

#ifdef DBG_FULL
					std::cout << "[GENERATE EXH " << INSTID << "]: RDMA_READ_RESP ";
					print(std::cout, sendWord);
					std::cout << std::endl;
#endif
					output.write(sendWord);

					//BTH: 12, AETH: 4, PayLd: x, ICRC: 4
					udpLen = 12+4+meta.length+4;
					lengthFifo.write(udpLen);
				}
				break;
			}
			case RC_RDMA_READ_RESP_MIDDLE:
				// [BTH][PayLd]
				info.isAETH = true;
				info.hasHeader = false;
				info.hasPayload = (meta.length != 0); //TODO should be true
				packetInfoFifo.write(info);
				//BTH: 12, PayLd: x, ICRC: 4
				udpLen = 12+meta.length+4;
				lengthFifo.write(udpLen);
				ge_state = META;
				break;
			case RC_ACK:
			{
				// [BTH][AETH]
				//Check if ACK or NAK
				if (!meta.isNak)
				{
					ackHeader.setSyndrome(0x00);
				}
				else
				{
					//PSN SEQ error
					std::cout << "[GENERATE EXH " << INSTID << "]: psn seq error" << std::endl;
					ackHeader.setSyndrome(0x60);
				}
				std::cout << "[GENERATE EXH " << INSTID << "]: RC_ACK with qpn " << meta.qpn << ", psn " << meta.psn << std::endl;
				ackHeader.setMsn(msnMeta.msn);
				ackHeader.consumeWord(sendWord.data); //TODO
				{
					info.isAETH = true;
					info.hasHeader = true;
					info.hasPayload = false;
					packetInfoFifo.write(info);
					sendWord.keep(AETH_SIZE/8-1, 0) = 0xFF;
					sendWord.keep(WIDTH/8-1, (AETH_SIZE/8)) = 0;
					sendWord.last = 1;
#ifdef DBG_FULL
					std::cout << "[GENERATE EXH " << INSTID << "]: RC_ACK ";
					print(std::cout, sendWord);
					std::cout << std::endl;
#endif
					output.write(sendWord);
					//BTH: 12, AETH: 4, ICRC: 4
					// Length is correct, the two trailing bytes must be from somewhere else
					lengthFifo.write(12+4+4);
				}
				break;
			}
			default:
				break;
			} //switch
		} //if empty
		if (sendWord.last)
		{
			ge_state = META;
		}
		break;
	}//switch
}

/**
 * Append the payload
 */
template <int WIDTH, int INSTID = 0>
void append_payload(
	// Packet Information coming from generate_exh
	stream<txPacketInfo>&	packetInfoFifo,
	// Extended Header coming from generate_exh
	stream<net_axis<WIDTH> >&	tx_headerFifo,
	// Ack Extended Header coming from generate_exh
	stream<net_axis<WIDTH> >&	tx_aethPayloadFifo,
	// RDMA Extended Header coming from generate_exh
	stream<net_axis<WIDTH> >&	tx_rethPayloadFifo,
	// Raw Extended Header coming from generate_exh
	stream<net_axis<WIDTH> >&	tx_rawPayloadFifo,
	// Final output for the payload with appended Extended Header
	stream<net_axis<WIDTH> >&	tx_packetFifo
) {
#pragma HLS inline off
#pragma HLS pipeline II=1

	enum fsmState {INFO, HEADER, AETH_PAYLOAD, RETH_PAYLOAD, RAW_PAYLOAD};
	static fsmState state = INFO;
	static net_axis<WIDTH> prevWord;
	net_axis<WIDTH> currWord;
	net_axis<WIDTH> sendWord;
	static bool firstPayload = true;

	static txPacketInfo info;

	// TODO align this stuff!!
	switch (state)
	{
	case INFO:
		// Read the packet info and check if there is a header at all for this packet 
		if (!packetInfoFifo.empty())
		{
			firstPayload = true;
			packetInfoFifo.read(info);

			if (info.hasHeader)
			{
				state = HEADER;
				std::cout << "[APPEND PAYLOAD " << INSTID << "]: moving to state HEADER" << std::endl;
			}
			else
			{
				state = RAW_PAYLOAD;
				std::cout << "[APPEND PAYLOAD " << INSTID << "]: moving to state RAW_PAYLOAD" << std::endl;
			}
		}
		break;
	case HEADER:
		// If header is part of this packet, load it from the incoming queue 
		if (!tx_headerFifo.empty())
		{
			tx_headerFifo.read(prevWord);
			//TODO last is not necessary

			// Last does not have payload - just sent out the header
			if (!prevWord.last)
			{
				std::cout << "[APPEND PAYLOAD " << INSTID << "]: writing header" << std::endl;
				tx_packetFifo.write(prevWord);
			}
			else //last
			{
				if (!info.hasPayload)
				{
					state = INFO;
					tx_packetFifo.write(prevWord);
					std::cout << "[APPEND PAYLOAD " << INSTID << "]: moving to state INFO" << std::endl;
				}
				else // hasPayload
				{
					// Switch to states to append either ACK- or RDMA-payload
					prevWord.last = 0;
					if (info.isAETH)
					{
						state = AETH_PAYLOAD;
						std::cout << "[APPEND PAYLOAD " << INSTID << "]: moving to state AETH_PAYLOAD" << std::endl;
					}
					else //RETH
					{
						if (WIDTH <= RETH_SIZE)
						{
							tx_packetFifo.write(prevWord);
						}
						state = RETH_PAYLOAD;
						std::cout << "[APPEND PAYLOAD " << INSTID << "]: moving to state RETH_PAYLOAD" << std::endl;
					}
				}
			}
		}
		break;

	// Both of these cases: Read the data and combine it with the header for sending out
	case AETH_PAYLOAD:
		if (!tx_aethPayloadFifo.empty())
		{
			tx_aethPayloadFifo.read(currWord);
#ifdef DBG_FULL
			std::cout << "[APPEND PAYLOAD" << INSTID << "]: PAYLOAD WORD" << std::endl;
			print(std::cout, currWord);
			std::cout << std::endl;
#endif
			sendWord = currWord;
			if (firstPayload)
			{
				sendWord.data(31, 0) = prevWord.data(31, 0);
				firstPayload = false;
			}
#ifdef DBG_FULL
			std::cout << "[APPEND PAYLOAD" << INSTID << "]: AETH PAYLOAD" << std::endl;
			print(std::cout, sendWord);
			std::cout << std::endl;
#endif
			tx_packetFifo.write(sendWord);
			if (currWord.last)
			{
				state = INFO;
			}
		}
		break;
	case RETH_PAYLOAD:
		if (!tx_rethPayloadFifo.empty())
		{
			std::cout << "[APPEND PAYLOAD " << INSTID << "]: reading rethPayloadFifo" << std::endl;
			tx_rethPayloadFifo.read(currWord);
#ifdef DBG_FULL
			std::cout << "[APPEND PAYLOAD " << INSTID << "]: PAYLOAD WORD" << std::endl;
			print(std::cout, currWord);
			std::cout << std::endl;
#endif
			sendWord = currWord;
			if (firstPayload && WIDTH > RETH_SIZE)
			{
				sendWord.data(127, 0) = prevWord.data(127, 0);
				firstPayload = false;
			}
#ifdef DBG_FULL
			std::cout << "[APPEND PAYLOAD" << INSTID << "]: RETH PAYLOAD" << std::endl;
			print(std::cout, sendWord);
			std::cout << std::endl;
#endif
			tx_packetFifo.write(sendWord);
			if (currWord.last)
			{
				state = INFO;
				std::cout << "[APPEND PAYLOAD " << INSTID << "]: moving state INFO" << std::endl;
			}
		}
		break;
	case RAW_PAYLOAD:
		if (!tx_rawPayloadFifo.empty())
		{
			tx_rawPayloadFifo.read(currWord);
			tx_packetFifo.write(currWord);
			if (currWord.last)
			{
				state = INFO;
			}
		}
		break;
	}
}

/**
 * Prepend the header - add the BTH to the combination of Extended Header and Payload
 */
//TODO this introduces 1 cycle for WIDTH > 64
template <int WIDTH, int INSTID = 0>
void prepend_ibh_header(
	// Incoming BTH from generate_ibh
	stream<BaseTransportHeader<WIDTH> >& tx_ibhHeaderFifo,
	// Payload with Extended Header, coming from append_payload
	stream<net_axis<WIDTH> >& tx_ibhPayloadFifo,
	// Output for the combination of BTH, Extended Header and Payload
	stream<net_axis<WIDTH> >& m_axis_tx_data,
	// Counter for the outgoing packets 
    ap_uint<32>&		regIbvCountTx
) {
#pragma HLS inline off
#pragma HLS pipeline II=1

	enum pihStatea {GET_HEADER, HEADER, PARTIAL_HEADER, BODY};
	static pihStatea state = GET_HEADER;
	static BaseTransportHeader<WIDTH> header;
	static ap_uint<WIDTH> headerData;
	net_axis<WIDTH> currWord;
    static ap_uint<32> validTx = 0;

	switch (state)
	{
	// Fetch the incoming BTH 
	case GET_HEADER:
		if (!tx_ibhHeaderFifo.empty())
		{
			tx_ibhHeaderFifo.read(header);
			// Check if it's a full BTH or not - switch to the next state depending on that
			if (BTH_SIZE >= WIDTH)
			{
				state = HEADER;
				std::cout << "[PREPEND IBH HEADER " << INSTID << "]: going to state HEADER" << std::endl;
			}
			else
			{
				state = PARTIAL_HEADER;
				std::cout << "[PREPEND IBH HEADER " << INSTID << "]: going to state PARTIAL_HEADER" << std::endl;
			}
		}
		break;

	// Case for a full header 
	case HEADER:
	{
		// How much longer than a standard BTH? 
		ap_uint<8> remainingLength = header.consumeWord(currWord.data);
		if (remainingLength < (WIDTH/8))
		{
			state = PARTIAL_HEADER;
		}
		currWord.keep = ~0;
		currWord.last = 0;
		// Send out the read word 
		m_axis_tx_data.write(currWord);

		// Increase the counters for having processed a header
        validTx++;
		regIbvCountTx = validTx;
		break;
	}
		break;

	// Processing a partial header 
	case PARTIAL_HEADER:
		// Load the incoming payload from the FIFO
		if (!tx_ibhPayloadFifo.empty())
		{
			tx_ibhPayloadFifo.read(currWord);
#ifdef DBG_FULL
			std::cout << "[PREPEND IBH HEADER " << INSTID << "]: IBH PARTIAL PAYLOAD" << std::endl;
			print(std::cout, currWord);
			std::cout << std::endl;
#endif		
			// Write out the received data
			header.consumeWord(currWord.data);
			m_axis_tx_data.write(currWord);

			// Increment relevant counters 
            validTx++;
		    regIbvCountTx = validTx;

#ifdef DBG_FULL
			std::cout << "[PREPEND IBH HEADER " << INSTID << "]: IBH PARTIAL HEADER" << std::endl;
			print(std::cout, currWord);
			std::cout << std::endl;
#endif
			// Check whether more payload is to be expected or if this was already the last part 
			state = BODY;
			if (currWord.last)
			{
				state = GET_HEADER;
			}
		}
		break;

	// More payload needs to be processed 
	case BODY:
		if (!tx_ibhPayloadFifo.empty())
		{
			// Read payload from the input FIFO, write it out 
			tx_ibhPayloadFifo.read(currWord);
			m_axis_tx_data.write(currWord);

            validTx++;
		    regIbvCountTx = validTx;

#ifdef DBG_FULL
			std::cout << "[PREPEND IBH HEADER " << INSTID << "]: IBH PAYLOAD WORD" << std::endl;
			print(std::cout, currWord);
			std::cout << std::endl;
#endif
			if (currWord.last)
			{
				state = GET_HEADER;
			}
		}
		break;
	}
}

// ------------------------------------------------------------------------------------------------
// UDP
// ------------------------------------------------------------------------------------------------

/**
 * UDP meta handler
 */
//TODO maybe all ACKS should be triggered by ibhFSM?? what is the guarantee we should/have to give
//TODO this should become a BRAM, storage type of thing
template <int WIDTH, int INSTID = 0>
void ipUdpMetaHandler(	
	// Input from IP / UDP meta information 
	stream<ipUdpMeta>&		input,
	// Input of the Extended Header 
	stream<ExHeader<WIDTH> >& exHeaderInput,
	// Input of the forwarding policy in processed packet
	stream<fwdPolicy>&			dropMetaIn,
	//stream<dstTuple>&		output,
	//stream<ap_uint<16> >&	remcrc_lengthFifo,
	// Length of the Extended Header 
	stream<ap_uint<16> >&	exh_lengthFifo,
	// Output of the Extended Header 
	stream<ExHeader<WIDTH> >& exHeaderOutput
) {
#pragma HLS inline off
#pragma HLS pipeline II=1

	ipUdpMeta meta;
	ExHeader<WIDTH> header;
	fwdPolicy policy;
	bool isDrop;

	if (!input.empty() && !exHeaderInput.empty() && !dropMetaIn.empty())
	{
		input.read(meta);
		exHeaderInput.read(header);
		dropMetaIn.read(policy);

		// Only output the header and length if the packet is not to be dropped and not an ACK
		if (!policy.isDrop) //TODO clean this up
		{
			if (!policy.ackOnly)
			{
				//remcrc_lengthFifo.write(meta.length - (8 + 12 + 4)); //UDP + BTH + CRC
				exh_lengthFifo.write(meta.length);
				exHeaderOutput.write(header);

			}
			//output.write(dstTuple(meta.their_address, meta.their_port));
		}
	}
}

/**
 * UDP meta merger TX - fetches IP Address, RDMA default port, UDP port from the connection table and writes it out as m_axis_tx_meta
 */
template <int INSTID = 0>
void tx_ipUdpMetaMerger(	
	// Input from the connection Table 
	stream<connTableEntry>& tx_connTable2ibh_rsp,
	// Input of the length of the RDMA
	stream<ap_uint<16> >&	tx_lengthFifo,
	// IP / UDP Meta Information Output 
	stream<ipUdpMeta>&		m_axis_tx_meta,
	stream<ap_uint<24> >&	tx_dstQpFifo
) {
#pragma HLS inline off
#pragma HLS pipeline II=1

	connTableEntry connMeta;
	ap_uint<16> len;

	if (!tx_connTable2ibh_rsp.empty() && !tx_lengthFifo.empty())
	{
		// Read from the connection table and fetch required IP / UDP information to generate the IP / UDP header later on
		tx_connTable2ibh_rsp.read(connMeta);
		tx_lengthFifo.read(len);
		std::cout << "[TX IP UDP META MERGER " << INSTID << "]: port " << connMeta.remote_udp_port << std::endl;
		m_axis_tx_meta.write(ipUdpMeta(connMeta.remote_ip_address, RDMA_DEFAULT_PORT, connMeta.remote_udp_port, len));
		tx_dstQpFifo.write(connMeta.remote_qpn);
	}
}

// ------------------------------------------------------------------------------------------------
// QP interface
// ------------------------------------------------------------------------------------------------

/**
 * QP table
 */
template <int INSTID = 0>
void qp_interface(	
	// QP context input: QP State, QPN, RPSN, LPSN, RKEY, VADDR
	stream<qpContext>& 			contextIn,

	// Input from the State Table: Info on PSNs etc. 
	stream<stateTableEntry>&	stateTable2qpi_rsp,
	// Initial access to the State Table: QPN, new state, RPSN, LPSN, Write-Bit
	stream<ifStateReq>&			qpi2stateTable_upd_req,
	// Initial access to the MSN Table: QPN and rkey
	stream<ifMsnReq>&			if2msnTable_init
) {
#pragma HLS inline off
#pragma HLS pipeline II=1

	enum fstStateType{GET_STATE, UPD_STATE};
	static fstStateType qp_fsmState = GET_STATE;
	static qpContext context;
	stateTableEntry state;

	switch(qp_fsmState)
	{
	case GET_STATE:
		// Wait for the incoming context 
		if (!contextIn.empty())
		{
			contextIn.read(context);
			// Update the state table with the QPN
			qpi2stateTable_upd_req.write(context.qp_num);
			qp_fsmState = UPD_STATE;
		}
		break;
	case UPD_STATE:
		// Check the current state of the QP and then update it 
		if (!stateTable2qpi_rsp.empty())
		{
			stateTable2qpi_rsp.read(state);
			//TODO check if valid transition
			// Update the state table with QPN, new state, remote PSN, local PSN
			qpi2stateTable_upd_req.write(ifStateReq(context.qp_num, context.newState, context.remote_psn, context.local_psn));
			// Update the msn table with the QPN and the rkey
			if2msnTable_init.write(ifMsnReq(context.qp_num, context.r_key)); //TODO store virtual address somewhere??
			qp_fsmState = GET_STATE;
		}
		break;
	}
}

// ------------------------------------------------------------------------------------------------
// Merging
// ------------------------------------------------------------------------------------------------

// Receives input from three sources, outputs them to a single output queue 
void three_merger(
	stream<event>& in0, stream<event>& in1, stream<event>& in2, stream<event>& out
) {
#pragma HLS inline off
#pragma HLS pipeline II=1

	if (!in0.empty())
	{
		out.write(in0.read());
	}
	else if (!in1.empty())
	{
		out.write(in1.read());
	}
	else if (!in2.empty())
	{
		out.write(in2.read());
	}
}

void merge_retrans_request(	
	stream<retransMeta>&		tx2retrans_insertMeta,
	stream<retransAddrLen>&		tx2retrans_insertAddrLen,
	stream<retransEntry>&		tx2retrans_insertRequest
) {
#pragma HLS inline off
#pragma HLS pipeline II=1

	retransMeta meta;
	retransAddrLen addrlen;

	if (!tx2retrans_insertMeta.empty() && !tx2retrans_insertAddrLen.empty())
	{
		tx2retrans_insertMeta.read(meta);
		tx2retrans_insertAddrLen.read(addrlen);
		tx2retrans_insertRequest.write(retransEntry(meta, addrlen));
	}
}



// ------------------------------------------------------------------------------------------------
// IB transport protocol
// ------------------------------------------------------------------------------------------------


template <int WIDTH, int INSTID = 0>
void ib_transport_protocol(	
	// RX - net module
	stream<ipUdpMeta>& s_axis_rx_meta,
	stream<net_axis<WIDTH> >& s_axis_rx_data,

	// TX - net module
	stream<ipUdpMeta>& m_axis_tx_meta,
	stream<net_axis<WIDTH> >& m_axis_tx_data,

	// S(R)Q
	stream<txMeta>& s_axis_sq_meta,

	// ACKs
	stream<ackMeta>& m_axis_rx_ack_meta,

	// RDMA
	stream<memCmd>& m_axis_mem_write_cmd,
	stream<memCmd>& m_axis_mem_read_cmd,
	stream<net_axis<WIDTH> >& m_axis_mem_write_data,
	stream<net_axis<WIDTH> >& s_axis_mem_read_data,

	// QP
	stream<qpContext>& s_axis_qp_interface,
	stream<ifConnReq>& s_axis_qp_conn_interface,

	// Debug
#ifdef DBG_IBV
	stream<psnPkg>& m_axis_dbg,
#endif
	ap_uint<32>& regInvalidPsnDropCount,
    ap_uint<32>& regRetransCount,
	ap_uint<32>& regIbvCountRx,
    ap_uint<32>& regIbvCountTx
) {
#pragma HLS INLINE

	static stream<net_axis<WIDTH> > 			rx_ibh2shiftFifo("rx_ibh2shiftFifo");
	static stream<net_axis<WIDTH> > 			rx_shift2exhFifo("rx_shift2exhFifo");
	static stream<net_axis<WIDTH> > 			rx_exh2dropFifo("rx_exh2dropFifo");
	static stream<net_axis<WIDTH> >			rx_ibhDrop2exhFifo("rx_ibhDrop2exhFifo");
	static stream<ibhMeta> 			rx_ibh2fsm_MetaFifo("rx_ibh2fsm_MetaFifo");
	static stream<ibhMeta>			rx_fsm2exh_MetaFifo("rx_fsm2exh_MetaFifo");
	static stream<net_axis<WIDTH> >	rx_exh2rethShiftFifo("rx_exh2rethShiftFifo");
	static stream<net_axis<WIDTH> >			rx_exh2aethShiftFifo("rx_exh2aethShiftFifo");
	static stream<net_axis<WIDTH> >	rx_exhNoShiftFifo("rx_exhNoShiftFifo");
	static stream<net_axis<WIDTH> >	rx_rethSift2mergerFifo("rx_rethSift2mergerFifo");
	static stream<net_axis<WIDTH> >			rx_aethSift2mergerFifo("rx_aethSift2mergerFifo");
	static stream<pkgSplit>		rx_pkgSplitTypeFifo("rx_pkgSplitTypeFifo");
	static stream<pkgShift> 	rx_pkgShiftTypeFifo("rx_pkgShiftTypeFifo");
	#pragma HLS STREAM depth=2 variable=rx_ibh2shiftFifo
	#pragma HLS STREAM depth=2 variable=rx_shift2exhFifo
	#pragma HLS STREAM depth=32 variable=rx_exh2dropFifo
	#pragma HLS STREAM depth=32 variable=rx_ibhDrop2exhFifo
	#pragma HLS STREAM depth=2 variable=rx_ibh2fsm_MetaFifo
	#pragma HLS STREAM depth=2 variable=rx_fsm2exh_MetaFifo
	#pragma HLS STREAM depth=4 variable=rx_exh2rethShiftFifo
	#pragma HLS STREAM depth=4 variable=rx_exh2aethShiftFifo
	#pragma HLS STREAM depth=4 variable=rx_exhNoShiftFifo
	#pragma HLS STREAM depth=4 variable=rx_rethSift2mergerFifo
	#pragma HLS STREAM depth=4 variable=rx_aethSift2mergerFifo
	#pragma HLS STREAM depth=2 variable=rx_pkgSplitTypeFifo
	#pragma HLS STREAM depth=2 variable=rx_pkgShiftTypeFifo
#if defined( __VITIS_HLS__)
	#pragma HLS aggregate  variable=rx_ibh2fsm_MetaFifo compact=bit
	#pragma HLS aggregate  variable=rx_fsm2exh_MetaFifo compact=bit
	#pragma HLS aggregate  variable=rx_pkgSplitTypeFifo compact=bit
	#pragma HLS aggregate  variable=rx_pkgShiftTypeFifo compact=bit
#else 
	#pragma HLS DATA_PACK variable=rx_ibh2fsm_MetaFifo
	#pragma HLS DATA_PACK variable=rx_fsm2exh_MetaFifo
	#pragma HLS DATA_PACK variable=rx_pkgSplitTypeFifo
	#pragma HLS DATA_PACK variable=rx_pkgShiftTypeFifo
#endif 
	static stream<ackEvent>  rx_ibhEventFifo("rx_ibhEventFifo"); //TODO rename
	static stream<ackEvent>  rx_exhEventMetaFifo("rx_exhEventMetaFifo");
	static stream<memCmdInternal> rx_remoteMemCmd("rx_remoteMemCmd");
	#pragma HLS STREAM depth=2 variable=rx_ibhEventFifo
	#pragma HLS STREAM depth=2 variable=rx_exhEventMetaFifo
	#pragma HLS STREAM depth=512 variable=rx_remoteMemCmd
#if defined( __VITIS_HLS__)
	#pragma HLS aggregate  variable=rx_ibhEventFifo compact=bit
	#pragma HLS aggregate  variable=rx_exhEventMetaFifo compact=bit
	#pragma HLS aggregate  variable=rx_remoteMemCmd compact=bit
#else
	#pragma HLS DATA_PACK variable=rx_ibhEventFifo
	#pragma HLS DATA_PACK variable=rx_exhEventMetaFifo
	#pragma HLS DATA_PACK variable=rx_remoteMemCmd
#endif

	static stream<ibhMeta>	tx_ibhMetaFifo("tx_ibhMetaFifo");
	static stream<event>	tx_appMetaFifo("tx_appMetaFifo");
	//static stream<event>	tx_localMetaFifo("tx_localMetaFifo");
	static stream<net_axis<WIDTH> >	tx_appDataFifo("tx_appDataFifo");
	#pragma HLS STREAM depth=8 variable=tx_ibhMetaFifo
	#pragma HLS STREAM depth=32 variable=tx_appMetaFifo
	//#pragma HLS STREAM depth=8 variable=tx_localMetaFifo
	#pragma HLS STREAM depth=8 variable=tx_appDataFifo

	static stream<event> tx_exhMetaFifo("tx_exhMetaFifo");
	static stream<net_axis<WIDTH> > tx_exh2shiftFifo("tx_exh2shiftFifo");
	static stream<net_axis<WIDTH> > tx_shift2ibhFifo("tx_shift2ibhFifo");
	static stream<net_axis<WIDTH> > tx_aethShift2payFifo("tx_aethShift2payFifo");
	static stream<net_axis<WIDTH> > tx_rethShift2payFifo("tx_rethShift2payFifo");
	static stream<net_axis<WIDTH> > tx_rawPayFifo("tx_rawPayFifo");
	static stream<net_axis<WIDTH> > tx_exh2payFifo("tx_exh2payFifo");
	static stream<BaseTransportHeader<WIDTH> > tx_ibhHeaderFifo("tx_ibhHeaderFifo");
	static stream<memCmdInternal>  tx_localMemCmdFifo("tx_localMemCmdFifo");
	#pragma HLS STREAM depth=4 variable=tx_exhMetaFifo
	#pragma HLS STREAM depth=2 variable=tx_exh2shiftFifo
	#pragma HLS STREAM depth=8 variable=tx_shift2ibhFifo
	#pragma HLS STREAM depth=2 variable=tx_aethShift2payFifo
	#pragma HLS STREAM depth=2 variable=tx_rethShift2payFifo
	#pragma HLS STREAM depth=4 variable=tx_rawPayFifo
	#pragma HLS STREAM depth=4 variable=tx_exh2payFifo
	#pragma HLS STREAM depth=2 variable=tx_ibhHeaderFifo
	#pragma HLS STREAM depth=2 variable=tx_localMemCmdFifo
#if defined( __VITIS_HLS__)
	#pragma HLS aggregate  variable=tx_exhMetaFifo compact=bit
	#pragma HLS aggregate  variable=tx_ibhHeaderFifo compact=bit
	#pragma HLS aggregate  variable=tx_localMemCmdFifo compact=bit
#else
	#pragma HLS DATA_PACK variable=tx_exhMetaFifo
	#pragma HLS DATA_PACK variable=tx_ibhHeaderFifo
	#pragma HLS DATA_PACK variable=tx_localMemCmdFifo
#endif

	static stream<txPacketInfo>	tx_packetInfoFifo("tx_packetInfoFifo");
	static stream<ap_uint<16> > tx_lengthFifo("tx_lengthFifo");
	#pragma HLS STREAM depth=2 variable=tx_packetInfoFifo
	#pragma HLS STREAM depth=4 variable=tx_lengthFifo
#if defined( __VITIS_HLS__)
	#pragma HLS aggregate  variable=tx_packetInfoFifo compact=bit
#else
	#pragma HLS DATA_PACK variable=tx_packetInfoFifo
#endif

	static stream<bool> rx_ibhDropFifo("rx_ibhDropFifo");
	static stream<fwdPolicy> rx_ibhDropMetaFifo("rx_ibhDropMetaFifo");
	#pragma HLS STREAM depth=2 variable=rx_ibhDropFifo
	#pragma HLS STREAM depth=2 variable=rx_ibhDropMetaFifo
#if defined( __VITIS_HLS__)
	#pragma HLS aggregate  variable=rx_ibhDropMetaFifo compact=bit
#else
	#pragma HLS DATA_PACK variable=rx_ibhDropMetaFifo
#endif

	//Connection Table
	static stream<ap_uint<16> >	tx_ibhconnTable_req("tx_ibhconnTable_req");
	//static stream<ifConnReq>		qpi2connTable_req("qpi2connTable_req");
	static stream<connTableEntry>	tx_connTable2ibh_rsp("tx_connTable2ibh_rsp");
	//static stream<connTableEntry> connTable2qpi_rsp("connTable2qpi_rsp");
	#pragma HLS STREAM depth=2 variable=tx_ibhconnTable_req
	#pragma HLS STREAM depth=8 variable=tx_connTable2ibh_rsp
#if defined( __VITIS_HLS__)
	//#pragma HLS aggregate  variable=tx_connTable2qpi_rsp compact=bit
#else
	//#pragma HLS DATA_PACK variable=tx_connTable2qpi_rsp
#endif

	//State Table Fifos
	static stream<rxStateReq> rxIbh2stateTable_upd_req("rxIbh2stateTable_upd_req");
	static stream<txStateReq> txIbh2stateTable_upd_req("txIbh2stateTable_upd_req");
	static stream<ifStateReq> qpi2stateTable_upd_req("qpi2stateTable_upd_req");
	static stream<rxStateRsp> stateTable2rxIbh_rsp("stateTable2rxIbh_rsp");
	static stream<stateTableEntry> stateTable2txIbh_rsp("stateTable2txIbh_rsp");
	static stream<stateTableEntry> stateTable2qpi_rsp("stateTable2qpi_rsp");
	#pragma HLS STREAM depth=2 variable=rxIbh2stateTable_upd_req
	#pragma HLS STREAM depth=2 variable=txIbh2stateTable_upd_req
	#pragma HLS STREAM depth=2 variable=qpi2stateTable_upd_req
	#pragma HLS STREAM depth=2 variable=stateTable2rxIbh_rsp
	#pragma HLS STREAM depth=2 variable=stateTable2txIbh_rsp
	#pragma HLS STREAM depth=2 variable=stateTable2qpi_rsp
#if defined( __VITIS_HLS__)
	#pragma HLS aggregate  variable=rxIbh2stateTable_upd_req compact=bit
	#pragma HLS aggregate  variable=txIbh2stateTable_upd_req compact=bit
	#pragma HLS aggregate  variable=qpi2stateTable_upd_req compact=bit
	#pragma HLS aggregate  variable=stateTable2rxIbh_rsp compact=bit
	#pragma HLS aggregate  variable=stateTable2txIbh_rsp compact=bit
	#pragma HLS aggregate  variable=stateTable2qpi_rsp compact=bit
#else
	#pragma HLS DATA_PACK variable=rxIbh2stateTable_upd_req
	#pragma HLS DATA_PACK variable=txIbh2stateTable_upd_req
	#pragma HLS DATA_PACK variable=qpi2stateTable_upd_req
	#pragma HLS DATA_PACK variable=stateTable2rxIbh_rsp
	#pragma HLS DATA_PACK variable=stateTable2txIbh_rsp
	#pragma HLS DATA_PACK variable=stateTable2qpi_rsp
#endif

	// MSN Table
	static stream<rxMsnReq>		rxExh2msnTable_upd_req("rxExh2msnTable_upd_req");
	static stream<ap_uint<16> >	txExh2msnTable_req("txExh2msnTable_req");
	static stream<ifMsnReq>	if2msnTable_init("if2msnTable_init");
	static stream<dmaState>	msnTable2rxExh_rsp("msnTable2rxExh_rsp");
	static stream<txMsnRsp>	msnTable2txExh_rsp("msnTable2txExh_rsp");
	#pragma HLS STREAM depth=2 variable=rxExh2msnTable_upd_req
	#pragma HLS STREAM depth=2 variable=txExh2msnTable_req
	#pragma HLS STREAM depth=2 variable=if2msnTable_init
	#pragma HLS STREAM depth=2 variable=msnTable2rxExh_rsp
	#pragma HLS STREAM depth=2 variable=msnTable2txExh_rsp
#if defined( __VITIS_HLS__)
	#pragma HLS aggregate  variable=rxExh2msnTable_upd_req compact=bit
	#pragma HLS aggregate  variable=if2msnTable_init compact=bit
	#pragma HLS aggregate  variable=msnTable2rxExh_rsp compact=bit
	#pragma HLS aggregate  variable=msnTable2txExh_rsp compact=bit
#else
	#pragma HLS DATA_PACK variable=rxExh2msnTable_upd_req
	#pragma HLS DATA_PACK variable=if2msnTable_init
	#pragma HLS DATA_PACK variable=msnTable2rxExh_rsp
	#pragma HLS DATA_PACK variable=msnTable2txExh_rsp
#endif

	static stream<ap_uint<16> > exh_lengthFifo("exh_lengthFifo");
	static stream<readRequest>	rx_readRequestFifo("rx_readRequestFifo");
	static stream<event>		rx_readEvenFifo("rx_readEvenFifo");
	static stream<ackEvent>		rx_ackEventFifo("rx_ackEventFifo");
	#pragma HLS STREAM depth=4 variable=exh_lengthFifo
	#pragma HLS STREAM depth=8 variable=rx_readRequestFifo
	#pragma HLS STREAM depth=512 variable=rx_readEvenFifo
	#pragma HLS STREAM depth=32 variable=rx_ackEventFifo
#if defined( __VITIS_HLS__)
	#pragma HLS aggregate  variable=rx_readRequestFifo compact=bit
	#pragma HLS aggregate  variable=rx_readEvenFifo compact=bit
	#pragma HLS aggregate  variable=rx_ackEventFifo compact=bit
#else
	#pragma HLS DATA_PACK variable=rx_readRequestFifo
	#pragma HLS DATA_PACK variable=rx_readEvenFifo
	#pragma HLS DATA_PACK variable=rx_ackEventFifo
#endif

	// Read Req Table
    /*
	static stream<txReadReqUpdate>	tx_readReqTable_upd("tx_readReqTable_upd");
	static stream<rxReadReqUpdate>	rx_readReqTable_upd_req("rx_readReqTable_upd_req");
	static stream<rxReadReqRsp>		rx_readReqTable_upd_rsp("rx_readReqTable_upd_rsp");
	#pragma HLS STREAM depth=2 variable=tx_readReqTable_upd
	#pragma HLS STREAM depth=2 variable=rx_readReqTable_upd_req
	#pragma HLS STREAM depth=2 variable=rx_readReqTable_upd_rsp
#if defined( __VITIS_HLS__)
	#pragma HLS aggregate  variable=tx_readReqTable_upd compact=bit
	#pragma HLS aggregate  variable=rx_readReqTable_upd_req compact=bit
	#pragma HLS aggregate  variable=rx_readReqTable_upd_rsp compact=bit
#else
	#pragma HLS DATA_PACK variable=tx_readReqTable_upd
	#pragma HLS DATA_PACK variable=rx_readReqTable_upd_req
	#pragma HLS DATA_PACK variable=rx_readReqTable_upd_rsp
#endif
    */

	// Outstanding Read Req Table
	//TODO merge these two
    /*
	static stream<mqInsertReq<ap_uint<64> > >	tx_readReqAddr_push("tx_readReqAddr_push");
	static stream<mqPopReq> rx_readReqAddr_pop_req("rx_readReqAddr_pop_req");
	static stream<ap_uint<64> > rx_readReqAddr_pop_rsp("rx_readReqAddr_pop_rsp");
	#pragma HLS STREAM depth=2 variable=tx_readReqAddr_push
	#pragma HLS STREAM depth=2 variable=rx_readReqAddr_pop_req
	#pragma HLS STREAM depth=2 variable=rx_readReqAddr_pop_rsp
#if defined( __VITIS_HLS__)
	#pragma HLS aggregate  variable=rx_readReqAddr_pop_req compact=bit
	#pragma HLS aggregate  variable=rx_readReqAddr_pop_rsp compact=bit
#else
	#pragma HLS DATA_PACK variable=rx_readReqAddr_pop_req
	#pragma HLS DATA_PACK variable=rx_readReqAddr_pop_rsp
#endif
    */

	/*
	 * TIMER & RETRANSMITTER
	 */
#ifdef RETRANS_EN
	static stream<rxTimerUpdate> rxClearTimer_req("rxClearTimer_req");
	static stream<ap_uint<24> > txSetTimer_req("txSetTimer_req");
	static stream<retransUpdate> rx2retrans_upd("rx2retrans_upd");
    static stream<retransRdInit> retrans2rx_init("retrans2rx_init");
	static stream<retransmission> rx2retrans_req("rx2retrans_req");
	static stream<retransmission> timer2retrans_req("timer2retrans_req");
	static stream<retransMeta> tx2retrans_insertMeta("tx2retrans_insertMeta");
	static stream<retransAddrLen> tx2retrans_insertAddrLen("tx2retrans_insertAddrLen");
	static stream<retransEntry>	tx2retrans_insertRequest("tx2retrans_insertRequest");
	static stream<retransEvent> retransmitter2exh_eventFifo("retransmitter2exh_eventFifo");
	#pragma HLS STREAM depth=2 variable=rxClearTimer_req
	#pragma HLS STREAM depth=2 variable=txSetTimer_req
	#pragma HLS STREAM depth=2 variable=rx2retrans_upd
    #pragma HLS STREAM depth=16 variable=retrans2rx_init
	#pragma HLS STREAM depth=2 variable=rx2retrans_req
	#pragma HLS STREAM depth=2 variable=timer2retrans_req
	#pragma HLS STREAM depth=2 variable=tx2retrans_insertMeta
	#pragma HLS STREAM depth=8 variable=tx2retrans_insertAddrLen
	#pragma HLS STREAM depth=2 variable=tx2retrans_insertRequest
	#pragma HLS STREAM depth=8 variable=retransmitter2exh_eventFifo
#endif

	//TODO this is a hack
	static stream<ap_uint<24> > tx_dstQpFifo("tx_dstQpFifo");
	#pragma HLS STREAM depth=2 variable=tx_dstQpFifo

	// Interface
	qp_interface<INSTID>(s_axis_qp_interface, stateTable2qpi_rsp, qpi2stateTable_upd_req, if2msnTable_init);


	// ------------------------------------------------------------------------------------------------
	// RX path
	// ------------------------------------------------------------------------------------------------

	static stream<ibOpCode> rx_ibh2exh_MetaFifo("rx_ibh2exh_MetaFifo");
	static stream<ExHeader<WIDTH> > rx_exh2drop_MetaFifo("rx_exh2drop_MetaFifo");
	static stream<ExHeader<WIDTH> > rx_drop2exhFsm_MetaFifo("rx_drop2exhFsm_MetaFifo");
	static stream<exhMeta>	rx_exhMetaFifo("rx_exhMetaFifo");
	#pragma HLS STREAM depth=2 variable=rx_ibh2exh_MetaFifo
	#pragma HLS STREAM depth=8 variable=rx_exh2drop_MetaFifo
	#pragma HLS STREAM depth=2 variable=rx_drop2exhFsm_MetaFifo
	#pragma HLS STREAM depth=2 variable=rx_exhMetaFifo
#if defined( __VITIS_HLS__)
	#pragma HLS aggregate  variable=rx_ibh2exh_MetaFifo compact=bit
	#pragma HLS aggregate  variable=rx_exh2drop_MetaFifo compact=bit
	#pragma HLS aggregate  variable=rx_drop2exhFsm_MetaFifo compact=bit
	#pragma HLS aggregate  variable=rx_exhMetaFifo compact=bit
#else
	#pragma HLS DATA_PACK variable=rx_ibh2exh_MetaFifo
	#pragma HLS DATA_PACK variable=rx_exh2drop_MetaFifo
	#pragma HLS DATA_PACK variable=rx_drop2exhFsm_MetaFifo
	#pragma HLS DATA_PACK variable=rx_exhMetaFifo
#endif

	rx_process_ibh<WIDTH, INSTID>(
#ifdef DBG_IBV_IBH_PROCESS
		m_axis_dbg,
#endif 
		s_axis_rx_data, 
		rx_ibh2fsm_MetaFifo,
		rx_ibh2exh_MetaFifo, 
		rx_ibh2shiftFifo,
		regIbvCountRx
	);

	rshiftWordByOctet<net_axis<WIDTH>, WIDTH,11, INSTID>(((BTH_SIZE%WIDTH)/8), rx_ibh2shiftFifo, rx_shift2exhFifo);

	rx_process_exh<WIDTH, INSTID>(
		rx_shift2exhFifo,
		rx_ibh2exh_MetaFifo,
		rx_exhMetaFifo,
		rx_exh2drop_MetaFifo, //TODO check if this has to be dropped
		rx_exh2dropFifo
	);

	rx_ibh_fsm<INSTID>(	
		rx_ibh2fsm_MetaFifo,
		rx_exhMetaFifo,
		stateTable2rxIbh_rsp,
		rxIbh2stateTable_upd_req,
		rx_fsm2exh_MetaFifo,
		rx_ibhEventFifo,
		rx_ibhDropFifo,
		rx_ibhDropMetaFifo,
		m_axis_rx_ack_meta,
#ifdef RETRANS_EN
		rxClearTimer_req,
		rx2retrans_upd,
#endif
#ifdef DBG_IBV_IBH_FSM
		m_axis_dbg,
#endif		
		regInvalidPsnDropCount
	);

	drop_ooo_ibh<WIDTH, INSTID>(
        rx_exh2dropFifo, rx_ibhDropFifo, rx_ibhDrop2exhFifo);

	//some hack TODO, make this nicer.. not sure what this is still for
	ipUdpMetaHandler<WIDTH, INSTID>(s_axis_rx_meta, rx_exh2drop_MetaFifo, rx_ibhDropMetaFifo, exh_lengthFifo, rx_drop2exhFsm_MetaFifo);

	rx_exh_fsm<WIDTH, INSTID>(	
		rx_fsm2exh_MetaFifo,
		exh_lengthFifo,
		msnTable2rxExh_rsp,
#ifdef RETRANS_EN
		//rx_readReqTable_upd_req,
        //rx_readReqTable_upd_rsp,
        rx2retrans_req,
        retrans2rx_init,
#endif
		//rx_readReqAddr_pop_rsp,
		rx_drop2exhFsm_MetaFifo,
		//rx_ibhDrop2exhFifo,
		m_axis_mem_write_cmd,
		rx_readRequestFifo,
		//m_axis_rx_ack_meta,
		rxExh2msnTable_upd_req,
		//rx_readReqAddr_pop_req,
		rx_exhEventMetaFifo,
		//rx_rethSift2mergerFifo,
		//rx_exh2aethShiftFifo,
		//rx_exhNoShiftFifo,
		rx_pkgSplitTypeFifo,
		rx_pkgShiftTypeFifo
	);

	rx_exh_payload<WIDTH, INSTID>(	
		rx_pkgSplitTypeFifo,
		rx_ibhDrop2exhFifo,
//#if AXI_WIDTH == 64
//		rx_rethSift2mergerFifo,
//#else
		rx_exh2rethShiftFifo,
//#endif
		rx_exh2aethShiftFifo,
		rx_exhNoShiftFifo
	);

	handle_read_requests<INSTID>(	
		rx_readRequestFifo,
		rx_remoteMemCmd,
		rx_readEvenFifo
	);

	//TODO is order important??
	stream_merger<ackEvent>(rx_exhEventMetaFifo, rx_ibhEventFifo, rx_ackEventFifo);

	// RETH: 16 bytes
	//TODO not required for AXI_WIDTH == 64, also this seems to have a bug, this goes together with the hack in process_exh where we don't write the first word out
//#if AXI_WIDTH != 64
	rshiftWordByOctet<net_axis<WIDTH>, WIDTH,12, INSTID>(((RETH_SIZE%WIDTH)/8), rx_exh2rethShiftFifo, rx_rethSift2mergerFifo);
//#endif
	// AETH: 4 bytes
	rshiftWordByOctet<net_axis<WIDTH>, WIDTH,13, INSTID>(((AETH_SIZE%WIDTH)/8), rx_exh2aethShiftFifo, rx_aethSift2mergerFifo);

	merge_rx_pkgs<WIDTH, INSTID>(rx_pkgShiftTypeFifo, rx_aethSift2mergerFifo, rx_rethSift2mergerFifo, rx_exhNoShiftFifo, m_axis_mem_write_data);

	// ------------------------------------------------------------------------------------------------
	// TX path
	// ------------------------------------------------------------------------------------------------

	//application request handler
	static stream<pkgInfo> tx_pkgInfoFifo("tx_pkgInfoFifo");
	//static stream<net_axis<WIDTH> > tx_readDataFifo("tx_readDataFifo");
	static stream<net_axis<WIDTH> > tx_split2aethShift("tx_split2aethShift");
	static stream<net_axis<WIDTH> > tx_split2rethMerge("tx_split2rethMerge");
	static stream<net_axis<WIDTH> > tx_rethMerge2rethShift("tx_rethMerge2rethShift");
	#pragma HLS STREAM depth=128 variable=tx_pkgInfoFifo
	//#pragma HLS STREAM depth=4 variable=tx_readDataFifo
	#pragma HLS STREAM depth=4 variable=tx_split2aethShift
	#pragma HLS STREAM depth=4 variable=tx_split2rethMerge
	#pragma HLS STREAM depth=4 variable=tx_rethMerge2rethShift

	local_req_handler<INSTID>(
		s_axis_sq_meta,
#ifdef RETRANS_EN
		retransmitter2exh_eventFifo,
		tx2retrans_insertAddrLen,
#endif
		tx_localMemCmdFifo,
		//tx_readReqAddr_push,
		tx_appMetaFifo,
        regRetransCount
	);

	tx_pkg_arbiter<WIDTH, INSTID>(	
		tx_pkgInfoFifo,
		s_axis_mem_read_data,
		tx_split2aethShift,
		tx_rethMerge2rethShift,
		tx_rawPayFifo
	);

#ifdef FPGA_STANDALONE
	stream_merger(tx_split2rethMerge, tx_appDataFifo, tx_rethMerge2rethShift);
#endif
	//merges and orders event going to TX path
	meta_merger<INSTID>(rx_ackEventFifo, rx_readEvenFifo, tx_appMetaFifo, tx_ibhconnTable_req, tx_ibhMetaFifo, tx_exhMetaFifo);

	//Shift playload by 4 bytes for AETH (data from memory)
	lshiftWordByOctet<WIDTH,12,INSTID>(((AETH_SIZE%WIDTH)/8), tx_split2aethShift, tx_aethShift2payFifo);
	//Shift payload another 12 bytes for RETH (data from application)
	lshiftWordByOctet<WIDTH,13,INSTID>(((RETH_SIZE%WIDTH)/8), tx_rethMerge2rethShift, tx_rethShift2payFifo);

	//Generate EXH
	generate_exh<WIDTH, INSTID>(	
		tx_exhMetaFifo,
		msnTable2txExh_rsp,
		txExh2msnTable_req,
		//tx_readReqTable_upd,
		tx_lengthFifo,
		tx_packetInfoFifo,
#ifdef RETRANS_EN
		txSetTimer_req,
#endif
		tx_exh2payFifo
	);

	// Append payload to AETH or RETH
	append_payload<WIDTH, INSTID>(tx_packetInfoFifo, tx_exh2payFifo, tx_aethShift2payFifo, tx_rethShift2payFifo, tx_rawPayFifo, tx_exh2shiftFifo);

	// BTH: 12 bytes
	lshiftWordByOctet<WIDTH,11,INSTID>(((BTH_SIZE%WIDTH)/8), tx_exh2shiftFifo, tx_shift2ibhFifo);
	generate_ibh<WIDTH, INSTID>(	
		tx_ibhMetaFifo,
		tx_dstQpFifo,
		stateTable2txIbh_rsp,
		txIbh2stateTable_upd_req,
#ifdef RETRANS_EN
		tx2retrans_insertMeta,
#endif
		tx_ibhHeaderFifo
	);

	//prependt ib header
	prepend_ibh_header<WIDTH, INSTID>(tx_ibhHeaderFifo, tx_shift2ibhFifo, m_axis_tx_data, regIbvCountTx);

	//Get Meta data for UDP & IP layer
	tx_ipUdpMetaMerger(tx_connTable2ibh_rsp, tx_lengthFifo, m_axis_tx_meta, tx_dstQpFifo);

	//merge read requests
	mem_cmd_merger<WIDTH>(rx_remoteMemCmd, tx_localMemCmdFifo, m_axis_mem_read_cmd, tx_pkgInfoFifo);


	// ------------------------------------------------------------------------------------------------
	// Data structures
	// ------------------------------------------------------------------------------------------------

	conn_table<INSTID>(	
		tx_ibhconnTable_req,
		s_axis_qp_conn_interface,
		tx_connTable2ibh_rsp
	);

	state_table<INSTID>(
		rxIbh2stateTable_upd_req,
		txIbh2stateTable_upd_req,
		qpi2stateTable_upd_req,
		stateTable2rxIbh_rsp,
		stateTable2txIbh_rsp,
		stateTable2qpi_rsp
	);

	msn_table<INSTID>(
		rxExh2msnTable_upd_req,
		txExh2msnTable_req,
		if2msnTable_init,
		msnTable2rxExh_rsp,
		msnTable2txExh_rsp
	);
    /*
	read_req_table<INSTID>(	
		tx_readReqTable_upd,
#ifndef RETRANS_EN
		rx_readReqTable_upd_req
	);
#else
		rx_readReqTable_upd_req,
		rx_readReqTable_upd_rsp
	);
#endif
    */

    /*
	multi_queue<ap_uint<64>,MAX_QPS, 2048, INSTID>(	
		tx_readReqAddr_push,
		rx_readReqAddr_pop_req,
		rx_readReqAddr_pop_rsp
	);
    */

#ifdef RETRANS_EN
	merge_retrans_request(tx2retrans_insertMeta, tx2retrans_insertAddrLen, tx2retrans_insertRequest);

	transport_timer<INSTID>(
		rxClearTimer_req,
		txSetTimer_req,
		timer2retrans_req
	);

	retransmitter<INSTID>(	
		rx2retrans_upd,
        retrans2rx_init,
		rx2retrans_req,
		timer2retrans_req,
		tx2retrans_insertRequest,
		retransmitter2exh_eventFifo
	);
#endif

}

#ifdef DBG_IBV
#define ib_transport_protocol_spec_decla(ninst)                 \
template void ib_transport_protocol<DATA_WIDTH, ninst>(		   	\
	stream<ipUdpMeta>& s_axis_rx_meta,		                    \
	stream<net_axis<DATA_WIDTH> >& s_axis_rx_data,		        \
	stream<ipUdpMeta>& m_axis_tx_meta,		                    \
	stream<net_axis<DATA_WIDTH> >& m_axis_tx_data,		        \
	stream<txMeta>& s_axis_sq_meta,		                       	\
	stream<ackMeta>& m_axis_rx_ack_meta,		                \
	stream<memCmd>& m_axis_mem_write_cmd,		                \
	stream<memCmd>& m_axis_mem_read_cmd,		                \
	stream<net_axis<DATA_WIDTH> >& m_axis_mem_write_data,		\
	stream<net_axis<DATA_WIDTH> >& s_axis_mem_read_data,		\
	stream<qpContext>& s_axis_qp_interface,		               	\
	stream<ifConnReq>& s_axis_qp_conn_interface,		        \
	stream<psnPkg>& m_axis_dbg,		                        	\
	ap_uint<32>& regInvalidPsnDropCount,		                \
    ap_uint<32>& regRetransCount,		                        \
	ap_uint<32>& regIbvCountRx,		                       	    \
    ap_uint<32>& regIbvCountTx		                       	    \
);
#else
#define ib_transport_protocol_spec_decla(ninst)                 \
template void ib_transport_protocol<DATA_WIDTH, ninst>(		   	\
	stream<ipUdpMeta>& s_axis_rx_meta,		                    \
	stream<net_axis<DATA_WIDTH> >& s_axis_rx_data,		        \
	stream<ipUdpMeta>& m_axis_tx_meta,		                    \
	stream<net_axis<DATA_WIDTH> >& m_axis_tx_data,		        \
	stream<txMeta>& s_axis_sq_meta,		                       	\
	stream<ackMeta>& m_axis_rx_ack_meta,		                \
	stream<memCmd>& m_axis_mem_write_cmd,		                \
	stream<memCmd>& m_axis_mem_read_cmd,		                \
	stream<net_axis<DATA_WIDTH> >& m_axis_mem_write_data,		\
	stream<net_axis<DATA_WIDTH> >& s_axis_mem_read_data,		\
	stream<qpContext>& s_axis_qp_interface,		               	\
	stream<ifConnReq>& s_axis_qp_conn_interface,		        \
	ap_uint<32>& regInvalidPsnDropCount,		                \
    ap_uint<32>& regRetransCount,		                        \
	ap_uint<32>& regIbvCountRx,		                       	    \
    ap_uint<32>& regIbvCountTx		                       	    \
);
#endif

ib_transport_protocol_spec_decla(0);
ib_transport_protocol_spec_decla(1);
