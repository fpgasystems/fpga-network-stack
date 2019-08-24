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

template <int WIDTH>
void rx_process_ibh(	stream<net_axis<WIDTH> >& input,
						stream<ibhMeta>& metaOut,
						stream<ibOpCode>&	metaOut2,
						stream<net_axis<WIDTH> >& output)
{
#pragma HLS inline off
#pragma HLS pipeline II=1

	static BaseTransportHeader<WIDTH> bth;
	static bool metaWritten = false;

	net_axis<WIDTH> currWord;

	if (!input.empty())
	{
		input.read(currWord);
		bth.parseWord(currWord.data);

		if (bth.isReady())
		{
			output.write(currWord);
			if (!metaWritten)
			{
            metaOut.write(ibhMeta(bth.getOpCode(), bth.getPartitionKey(), bth.getDstQP(), bth.getPsn(), true));
				std::cout << "PROCESS IBH opcode: " << bth.getOpCode() << std::endl;
				metaOut2.write(bth.getOpCode());
				metaWritten = true;
			}
		}
		if (currWord.last)
		{
			bth.clear();
			metaWritten = false;
		}
	}

}

template <int WIDTH>
void rx_process_exh(	stream<net_axis<WIDTH> >& input,
						stream<ibOpCode>&	metaIn,
						stream<exhMeta>&	exhMetaFifo,
						stream<ExHeader<WIDTH> >& metaOut,
						stream<net_axis<WIDTH> >& output)
{
#pragma HLS inline off
#pragma HLS pipeline II=1

	enum fsmStateType {META, ACK_HEADER, RETH_HEADER, NO_HEADER, RPCH_HEADER};
	static fsmStateType state = META;
	static RdmaExHeader<WIDTH> rdmaHeader;
	static AckExHeader<WIDTH> ackHeader;
	//static RdmaPointerChaseHeader<WIDTH> pointerChasingHeader;
	static bool metaWritten = false;

	net_axis<WIDTH> currWord;
	static ibOpCode opCode;

	switch (state)
	{
	case META:
		if (!metaIn.empty())
		{
			metaIn.read(opCode);
			metaWritten = false;
			if (checkIfAethHeader(opCode))
			{
				state = ACK_HEADER;
			}
			else if (checkIfRethHeader(opCode))
			{
				state = RETH_HEADER;
			}
#if POINTER_CHASING_EN
			else if (opCode == RC_RDMA_READ_POINTER_REQUEST)
			{
				state = RPCH_HEADER;
			}
#endif
			else
			{
				state = NO_HEADER;
			}
		}
		break;
	case ACK_HEADER:
		if (!input.empty())
		{
			input.read(currWord);
			ackHeader.parseWord(currWord.data);

			if (ackHeader.isReady())
			{
				if (opCode != RC_ACK)
				{
					output.write(currWord);
				}
				if (!metaWritten)
				{
					exhMetaFifo.write(exhMeta(ackHeader.isNAK()));
					metaOut.write(ExHeader<WIDTH>(ackHeader));
					metaWritten = true;
				}
			}
			if (currWord.last)
			{
				ackHeader.clear();
				state = META;
			}
		}
		break;
	case RETH_HEADER:
		if (!input.empty())
		{
			input.read(currWord);
			rdmaHeader.parseWord(currWord.data);

			if (metaWritten && WIDTH <= RETH_SIZE)
			{
				output.write(currWord);
			}

			if (rdmaHeader.isReady())
			{
				if (!metaWritten)
				{
					if (opCode == RC_RDMA_READ_REQUEST || opCode == RC_RDMA_READ_CONSISTENT_REQUEST)
					{
						exhMetaFifo.write(exhMeta(false, (rdmaHeader.getLength()+(PMTU-1))/PMTU));
					}
					else
					{
						exhMetaFifo.write(exhMeta(false));
					}
					metaOut.write(ExHeader<WIDTH>(rdmaHeader));
					metaWritten = true;
				}
				if (checkIfWriteOrPartReq(opCode) && WIDTH > RETH_SIZE)
				{
					output.write(currWord);
				}
			}
			if (currWord.last)
			{
				rdmaHeader.clear();
				state = META;
			}
		}
		break;
#if POINTER_CHASING_EN
	case RPCH_HEADER:
		if (!input.empty())
		{
			input.read(currWord);
			pointerChasingHeader.parseWord(currWord.data);

			if (pointerChasingHeader.isReady())
			{
				if (!metaWritten)
				{
					//if (opCode == RC_RDMA_READ_REQUEST)
					{
						exhMetaFifo.write(exhMeta(false, (pointerChasingHeader.getLength()+(PMTU-1))/PMTU));
					}
					/*else
					{
						exhMetaFifo.write(exhMeta(false));
					}*/
					metaOut.write(ExHeader<WIDTH>(pointerChasingHeader));
					metaWritten = true;
				}
				//This works together with disabling the RightShift, Assumes WIDTH == 64
				else
				{
					//output.write(currWord);
				}
			}
			if (currWord.last)
			{
				pointerChasingHeader.clear();
				state = META;
			}
		}
		break;
#endif
	case NO_HEADER:
		if (!input.empty())
		{
			input.read(currWord);
			std::cout << "EXH NO HEADER: ";
			print(std::cout, currWord);
			std::cout << std::endl;
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
void rx_ibh_fsm(	stream<ibhMeta>& metaIn,
					stream<exhMeta>&	exhMetaFifo,
					stream<rxStateRsp>& stateTable_rsp,
					stream<rxStateReq>& stateTable_upd_req,
					stream<ibhMeta>& metaOut,
					stream<ackEvent>& ibhEventFifo,
					stream<bool>& ibhDropFifo,
					stream<fwdPolicy>& ibhDropMetaFifo,
#if RETRANS_EN

				stream<rxTimerUpdate>&	rxClearTimer_req,
				stream<retransRelease>&	rx2retrans_release_upd,
#endif
				ap_uint<32>&		regInvalidPsnDropCount)
{
#pragma HLS inline off
#pragma HLS pipeline II=1

	enum fsmStateType{LOAD, PROCESS};
	static fsmStateType fsmState = LOAD;

	static ibhMeta meta;
	static exhMeta emeta;
	static bool isResponse;
	static ap_uint<32> droppedPackets = 0;
	rxStateRsp qpState;


	switch(fsmState)
	{
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
		if (!stateTable_rsp.empty())
		{
			stateTable_rsp.read(qpState);
			//ap_uint<24> oldest_outstanding_psn = qpState.epsn- 8388608;
			//TODO compute or store oldest outstanding??
			//TODO also increment oldest outstanding
			//Check if in order
			//TODO Update oldest_oustanding_psn
			//TODO this is not working with coalescing ACKs
			std::cout << "epsn: " << qpState.epsn << ", packet psn: " << meta.psn << std::endl;
			// For requests we require total order, for responses, there is potential ACK coalescing, see page 299
			// For requests, max_forward == epsn
			//TODO how to deal with other responses if they are not in order??
			if (qpState.epsn == meta.psn || (meta.op_code == RC_ACK && ((qpState.epsn <= meta.psn && meta.psn <= qpState.max_forward)
					|| ((qpState.epsn <= meta.psn || meta.psn <= qpState.max_forward) && qpState.max_forward < qpState.epsn))))
			//if ((qpState.epsn <= meta.psn && meta.psn <= qpState.max_forward)
			//		|| ((qpState.epsn <= meta.psn || meta.psn <= qpState.max_forward) && qpState.max_forward < qpState.epsn))
			{
				std::cout << "NOT DROPPING PSN:" << meta.psn << std::endl;
				//regNotDropping = 1;
				if (meta.op_code != RC_ACK && meta.op_code != RC_RDMA_READ_REQUEST && meta.op_code != RC_RDMA_READ_POINTER_REQUEST && meta.op_code != RC_RDMA_READ_CONSISTENT_REQUEST) //TODO do length check instead
				{
					ibhDropFifo.write(false);
				}
				ibhDropMetaFifo.write(fwdPolicy(false, false));
				//TODO more meta for ACKs
				metaOut.write(meta); //TODO also send for non successful packets
				// Update psn
				//TODO for last param we need vaddr here!
				if (!emeta.isNak)
				{
					stateTable_upd_req.write(rxStateReq(meta.dest_qp, meta.psn+emeta.numPkg, isResponse));
				}
#if RETRANS_EN

				//CASE Requester: Update oldest-unacked-reqeust
				if (isResponse && !emeta.isNak)
				{
					std::cout <<"retrans release, psn: " << meta.psn << std::endl;
					rx2retrans_release_upd.write(retransRelease(meta.dest_qp, meta.psn));
				}
				//CASE Requester: Check if no oustanding requests -> stop timer
				if (isResponse && meta.op_code != RC_RDMA_READ_RESP_MIDDLE)
				{
					rxClearTimer_req.write(rxTimerUpdate(meta.dest_qp, meta.psn == qpState.max_forward));
#ifndef __SYNTHESIS__
					if (meta.psn  == qpState.max_forward)
					{
						std::cout << "clearing transport timer at psn: " << meta.psn << std::endl;
					}
#endif
				}
#endif
			}
			// Check for duplicates
			// For response: epsn = old_unack, old_oustanding = old_valid
			else if ((qpState.oldest_outstanding_psn < qpState.epsn && meta.psn < qpState.epsn && meta.psn >= qpState.oldest_outstanding_psn)
					 || (qpState.oldest_outstanding_psn > qpState.epsn && (meta.psn < qpState.epsn || meta.psn >= qpState.oldest_outstanding_psn)))
			{
				//Read request re-execute
				if (meta.op_code == RC_RDMA_READ_REQUEST || meta.op_code == RC_RDMA_READ_POINTER_REQUEST || meta.op_code == RC_RDMA_READ_CONSISTENT_REQUEST)
				{
					std::cout << "DUPLICATE READ_REQ PSN:" << meta.psn << std::endl;
					ibhDropFifo.write(false);
					ibhDropMetaFifo.write(fwdPolicy(false, false));
					metaOut.write(meta);
					//No release required
					//stateTable_upd_req.write(rxStateReq(meta.dest_qp, meta.psn, meta.partition_key, 0)); //TODO always +1??
				}
				//Write requests acknowledge, see page 313
				else if (checkIfWriteOrPartReq(meta.op_code))
				{
					//Send out ACK
					ibhEventFifo.write(ackEvent(meta.dest_qp)); //TODO do we need PSN???
					std::cout << "DROPPING DUPLICATE PSN:" << meta.psn << std::endl;
					droppedPackets++;
					regInvalidPsnDropCount = droppedPackets;
					ibhDropFifo.write(true);
					//Meta is required for ACK, TODO no longer
					ibhDropMetaFifo.write(fwdPolicy(false, true));
				}
				//TODO what about duplicate responses
				//drop them
				else
				{
					//Case Requester: Valid ACKs -> reset timer TODO
					//for now we just drop everything
					if (meta.op_code != RC_ACK) //TODO do length check instead
					{
						ibhDropFifo.write(true);
					}
					ibhDropMetaFifo.write(fwdPolicy(true, false));
				}
			}
			else // completely invalid
			{
				//behavior, see page 313
				std::cout << "DROPPING INVALID PSN:" << meta.psn << std::endl;
				droppedPackets++;
				regInvalidPsnDropCount = droppedPackets;
				ibhDropMetaFifo.write(fwdPolicy(true, false));
				//Issue NAK TODO NAK has to be in sequence
				if (meta.op_code != RC_ACK)
				{
					ibhDropFifo.write(true);
					//Do not generate further ACK/NAKs until we received a valid pkg
					if (qpState.retryCounter == 0x7)
					{
						if (isResponse)
						{
							ibhEventFifo.write(ackEvent(meta.dest_qp, meta.psn, true));
						}
						else
						{
							//Setting NAK to epsn, since otherwise epsn-1 is used
							ibhEventFifo.write(ackEvent(meta.dest_qp, qpState.epsn, true));
						}
						qpState.retryCounter--;
					}
					//We do not increment PSN
					stateTable_upd_req.write(rxStateReq(meta.dest_qp, qpState.epsn, qpState.retryCounter, isResponse));

				}
			}

			fsmState = LOAD;
		}
		break;
	}
}

//Currently not used!!
template <int WIDTH>
void drop_ooo_ibh(	stream<net_axis<WIDTH> >& input,
					stream<bool>& metaIn,
					stream<net_axis<WIDTH> >& output)
{
#pragma HLS inline off
#pragma HLS pipeline II=1

	enum fsmType {META, FWD, DROP};
	static fsmType state = META;

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
				if (currWord.last)
				{
					state = META;
				}
			}
			break;
		case DROP:
			if (!input.empty())
			{
				input.read(currWord);
				if (currWord.last)
				{
					state = META;
				}
			}
			break;
	} //switch
}

// Followed by ICRC TODO remove ICRC
/* For reliable connections, page 246, 266, 269
 * RDM WRITE ONLY: RETH, PayLd
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
template <int WIDTH>
void rx_exh_fsm(	stream<ibhMeta>&				metaIn,
					stream<ap_uint<16> >& 			udpLengthFifo,
					stream<dmaState>&				msnTable2rxExh_rsp,
#if RETRANS_EN
					stream<rxReadReqRsp>&			readReqTable_rsp,
#endif
					stream<ap_uint<64> >&			rx_readReqAddr_pop_rsp,
					stream<ExHeader<WIDTH> >&	headerInput,
					stream<routedMemCmd>&			memoryWriteCmd,
					stream<readRequest>&			readRequestFifo,
#if POINTER_CHASING_EN
					stream<ptrChaseMeta>&			m_axis_rx_pcmeta,
#endif
					stream<rxMsnReq>&				rxExh2msnTable_upd_req,
//#if RETRANS_EN
					stream<rxReadReqUpdate>&		readReqTable_upd_req,
//#endif
					stream<mqPopReq>&				rx_readReqAddr_pop_req,
					stream<ackEvent>&				rx_exhEventMetaFifo,
#if RETRANS_EN
					stream<retransmission>&			rx2retrans_req,
#endif
					stream<pkgSplitType>&	rx_pkgSplitTypeFifo,
					stream<pkgShiftType>&	rx_pkgShiftTypeFifo)
{
#pragma HLS inline off
#pragma HLS pipeline II=1

	enum pe_fsmStateType {META, DMA_META, DATA};
	static pe_fsmStateType pe_fsmState = META;
	static ibhMeta meta;
	net_axis<WIDTH> currWord;
	static ExHeader<WIDTH> exHeader;
	static dmaState dmaMeta;
	static ap_uint<16> udpLength;
	ap_uint<32> payLoadLength;
	static bool consumeReadAddr;
	static rxReadReqRsp readReqMeta;
	static ap_uint<64> readReqAddr;


	switch (pe_fsmState)
	{
	case META:
		if (!metaIn.empty() && !headerInput.empty())
		{
			metaIn.read(meta);
			headerInput.read(exHeader);

			rxExh2msnTable_upd_req.write(rxMsnReq(meta.dest_qp));
			consumeReadAddr = false;
#if RETRANS_EN1
			if (meta.op_code == RC_ACK)
			{
				readReqTable_upd_req.write(rxReadReqUpdate(meta.dest_qp));
			}
#endif
			if (meta.op_code == RC_RDMA_READ_RESP_ONLY || meta.op_code == RC_RDMA_READ_RESP_FIRST)
			{
				consumeReadAddr = true;
				rx_readReqAddr_pop_req.write(mqPopReq(meta.dest_qp));
			}
			pe_fsmState = DMA_META;
		}
		break;
	case DMA_META:
#if !(RETRANS_EN)
		if (!msnTable2rxExh_rsp.empty() && !udpLengthFifo.empty() && (!consumeReadAddr || !rx_readReqAddr_pop_rsp.empty()))
#else
		if (!msnTable2rxExh_rsp.empty() && !udpLengthFifo.empty() && (!consumeReadAddr || !rx_readReqAddr_pop_rsp.empty()) && (meta.op_code != RC_ACK || !readReqTable_rsp.empty()))
#endif
		{
			msnTable2rxExh_rsp.read(dmaMeta);
			udpLengthFifo.read(udpLength);
#if RETRANS_EN
			if (meta.op_code == RC_ACK)
			{
				readReqTable_rsp.read(readReqMeta);
			}
#endif
			if (consumeReadAddr)
			{
				rx_readReqAddr_pop_rsp.read(readReqAddr);
			}
			pe_fsmState = DATA;
		}
		break;
	case DATA: //TODO merge with DMA_META
		switch(meta.op_code)
		{
		case RC_RDMA_WRITE_ONLY:
		//case RC_RDMA_WRITE_ONLY_WIT_IMD:
		case RC_RDMA_WRITE_FIRST:
		case RC_RDMA_PART_ONLY:
		case RC_RDMA_PART_FIRST:
		{
			// [BTH][RETH][PayLd]
			RdmaExHeader<WIDTH> rdmaHeader = exHeader.getRdmaHeader();
			axiRoute route = ((meta.op_code == RC_RDMA_WRITE_ONLY) || (meta.op_code == RC_RDMA_WRITE_FIRST)) ? ROUTE_DMA : ROUTE_CUSTOM;

			if (rdmaHeader.getLength() != 0)
			{
				//Compute payload length
				payLoadLength = udpLength - (8 + 12 + 16 + 4); //UDP, BTH, RETH, CRC
				//compute remaining length
				ap_uint<32> headerLen = rdmaHeader.getLength();
				ap_uint<32> remainingLength =  headerLen - payLoadLength;

				//Send write request
				if ((meta.op_code == RC_RDMA_WRITE_ONLY) || (meta.op_code == RC_RDMA_WRITE_FIRST))
				{
					memoryWriteCmd.write(routedMemCmd(rdmaHeader.getVirtualAddress(), payLoadLength, route));
				}
				else if ((meta.op_code == RC_RDMA_PART_FIRST || (meta.op_code == RC_RDMA_PART_ONLY)))
				{
					memoryWriteCmd.write(routedMemCmd(rdmaHeader.getVirtualAddress(), headerLen, route));
				}
				// Update state
				//TODO msn, only for ONLY??
				rxExh2msnTable_upd_req.write(rxMsnReq(meta.dest_qp, dmaMeta.msn+1, rdmaHeader.getVirtualAddress()+payLoadLength, remainingLength));
				// Trigger ACK
				rx_exhEventMetaFifo.write(ackEvent(meta.dest_qp)); //TODO does this require PSN??
				//std::cout << std::hex << "LEGNTH" << header.getLength() << std::endl;
				rx_pkgSplitTypeFifo.write(pkgSplitType(meta.op_code, route));
				rx_pkgShiftTypeFifo.write(SHIFT_RETH);
				pe_fsmState = META;
			}
			break;
		}
		case RC_RDMA_WRITE_MIDDLE:
		case RC_RDMA_WRITE_LAST:
		case RC_RDMA_PART_MIDDLE:
		case RC_RDMA_PART_LAST:
		{
			// [BTH][PayLd]
			/*std::cout << "PROCESS_EXH: ";
			print(std::cout, currWord);
			std::cout << std::endl;*/

			//Fwd data words
			axiRoute route = ((meta.op_code == RC_RDMA_WRITE_MIDDLE) || (meta.op_code == RC_RDMA_WRITE_LAST)) ? ROUTE_DMA : ROUTE_CUSTOM;
			payLoadLength = udpLength - (8 + 12 + 4); //UDP, BTH, CRC
			//compute remaining length
			ap_uint<32> remainingLength = dmaMeta.dma_length - payLoadLength;
			//Send write request
			if ((meta.op_code == RC_RDMA_WRITE_MIDDLE) || (meta.op_code == RC_RDMA_WRITE_LAST))
			{
				memoryWriteCmd.write(routedMemCmd(dmaMeta.vaddr, payLoadLength, route));
			}
			/*else if ((meta.op_code == RC_RDMA_PART_MIDDLE) || (meta.op_code == RC_RDMA_PART_LAST))
			{
				memoryWriteCmd.write(routedMemCmd(dmaMeta.vaddr, payLoadLength, route));
			}*/
			//TODO msn only on LAST??
			rxExh2msnTable_upd_req.write(rxMsnReq(meta.dest_qp, dmaMeta.msn+1, dmaMeta.vaddr+payLoadLength, remainingLength));
			// Trigger ACK
			rx_exhEventMetaFifo.write(ackEvent(meta.dest_qp)); //TODO does this require PSN??
			rx_pkgSplitTypeFifo.write(pkgSplitType(meta.op_code, route));
			rx_pkgShiftTypeFifo.write(SHIFT_NONE);
			pe_fsmState = META;

#ifndef __SYNTHESIS__
			if ((meta.op_code == RC_RDMA_WRITE_LAST)  || (meta.op_code == RC_RDMA_PART_LAST))
			{
				assert(remainingLength == 0);
			}
#endif
			break;
		}
		/*case RC_RDMA_WRITE_LAST_WITH_IMD:
			//TODO sth ;) fire interrupt
			break;*/
		case RC_RDMA_READ_REQUEST:
		case RC_RDMA_READ_CONSISTENT_REQUEST:
		{
			// [BTH][RETH]
			RdmaExHeader<WIDTH> rdmaHeader = exHeader.getRdmaHeader();
			if (rdmaHeader.getLength() != 0)
			{
				axiRoute route = (meta.op_code == RC_RDMA_READ_CONSISTENT_REQUEST) ? ROUTE_CUSTOM : ROUTE_DMA;
				readRequestFifo.write(readRequest(meta.dest_qp, rdmaHeader.getVirtualAddress(), rdmaHeader.getLength(), meta.psn, route));
				rxExh2msnTable_upd_req.write(rxMsnReq(meta.dest_qp, dmaMeta.msn+1));
			}
			pe_fsmState = META;
			break;
		}
#if POINTER_CHASING_EN
		case RC_RDMA_READ_POINTER_REQUEST:
		{
			// [BTH][RPCH]
			RdmaPointerChaseHeader<WIDTH> pcHeader = exHeader.getPointerChasingHeader();
			if (pcHeader.getLength() != 0)
			{
				readRequestFifo.write(readRequest(meta.dest_qp, pcHeader.getVirtualAddress(), pcHeader.getLength(), meta.psn, ROUTE_CUSTOM));
				m_axis_rx_pcmeta.write(ptrChaseMeta(pcHeader.getPredicateKey(), pcHeader.getPredicateMask(), pcHeader.getPredicateOp(), pcHeader.getPtrOffset(), pcHeader.getIsRelPtr(), pcHeader.getNextPtrOffset(), pcHeader.getNextPtrValid()));
				rxExh2msnTable_upd_req.write(rxMsnReq(meta.dest_qp, dmaMeta.msn+1));
			}
			pe_fsmState = META;
			break;
		}
#endif
		case RC_RDMA_READ_RESP_ONLY:
		case RC_RDMA_READ_RESP_FIRST:
		case RC_RDMA_READ_RESP_LAST:
		{
			// [BTH][AETH][PayLd]
			//AETH for first and last
			AckExHeader<WIDTH> ackHeader = exHeader.getAckHeader();
			if (ackHeader.isNAK())
			{
				//Trigger retransmit
#if RETRANS_EN
				rx2retrans_req.write(retransmission(meta.dest_qp, meta.psn));
#endif
			}
			else
			{
				readReqTable_upd_req.write((rxReadReqUpdate(meta.dest_qp, meta.psn)));
			}
			//Write out meta
			payLoadLength = udpLength - (8 + 12 + 4 + 4); //UDP, BTH, AETH, CRC
			rx_pkgShiftTypeFifo.write(SHIFT_AETH);
			if (meta.op_code != RC_RDMA_READ_RESP_LAST)
			{
				memoryWriteCmd.write(routedMemCmd(readReqAddr, payLoadLength));
				//TODO maybe not the best way to store the vaddr in the msnTable
				rxExh2msnTable_upd_req.write(rxMsnReq(meta.dest_qp, dmaMeta.msn, readReqAddr+payLoadLength, 0));
			}
			else
			{
				memoryWriteCmd.write(routedMemCmd(dmaMeta.vaddr, payLoadLength));
			}
			rx_pkgSplitTypeFifo.write(pkgSplitType(meta.op_code));
			pe_fsmState = META;
			break;
		}
		case RC_RDMA_READ_RESP_MIDDLE:
			// [BTH][PayLd]
			payLoadLength = udpLength - (8 + 12 + 4); //UDP, BTH, CRC
			rx_pkgShiftTypeFifo.write(SHIFT_NONE);
			memoryWriteCmd.write(routedMemCmd(dmaMeta.vaddr, payLoadLength));
			//TODO how does msn have to be handled??
			rxExh2msnTable_upd_req.write(rxMsnReq(meta.dest_qp, dmaMeta.msn+1, dmaMeta.vaddr+payLoadLength, 0));
			rx_pkgSplitTypeFifo.write(pkgSplitType(meta.op_code));
			pe_fsmState = META;
			break;
		case RC_ACK:
		{
			// [BTH][AETH]
			AckExHeader<WIDTH> ackHeader = exHeader.getAckHeader();
			std::cout << "syndrome: " << ackHeader.getSyndrome() << std::endl;
#if RETRANS_EN
			if (ackHeader.isNAK())
			{
				//Trigger retransmit
				rx2retrans_req.write(retransmission(meta.dest_qp, meta.psn));
			}
			else if (readReqMeta.oldest_outstanding_readreq < meta.psn && readReqMeta.valid)
			{
				//Trigger retransmit
				rx2retrans_req.write(retransmission(meta.dest_qp, readReqMeta.oldest_outstanding_readreq));
			}
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

template <int WIDTH>
void rx_exh_payload(stream<pkgSplitType>&	metaIn,
					stream<net_axis<WIDTH> >&		input,
					stream<routed_net_axis<WIDTH> >&	rx_exh2rethShiftFifo,
					stream<net_axis<WIDTH> >&		rx_exh2aethShiftFifo,
					stream<routed_net_axis<WIDTH> >&	rx_exhNoShiftFifo)
{
#pragma HLS inline off
#pragma HLS pipeline II=1

	enum fsmStateType {META, PKG};
	static fsmStateType rep_state = META;
	static pkgSplitType meta;

	net_axis<WIDTH> currWord;

	switch (rep_state)
	{
	case META:
		if (!metaIn.empty())
		{
			metaIn.read(meta);
			rep_state = PKG;
		}
		break;
	case PKG:
		if (!input.empty())
		{
			input.read(currWord);

			if (checkIfRethHeader(meta.op_code))
			{
				std::cout << "EXH PAYLOAD:";
				print(std::cout, currWord);
				std::cout << std::endl;
				rx_exh2rethShiftFifo.write(routed_net_axis<WIDTH>(currWord, meta.route));
			}
			else if ((meta.op_code == RC_RDMA_READ_RESP_ONLY) || (meta.op_code == RC_RDMA_READ_RESP_FIRST) ||
					(meta.op_code == RC_RDMA_READ_RESP_LAST))
			{
				rx_exh2aethShiftFifo.write(currWord);
			}
			else
			{
				rx_exhNoShiftFifo.write(routed_net_axis<WIDTH>(currWord, meta.route));
			}

			if (currWord.last)
			{
				rep_state = META;
			}
		}
		break;
	} //switch
}

void handle_read_requests(	stream<readRequest>&	requestIn,
							stream<memCmdInternal>&	memoryReadCmd,
							stream<event>&			readEventFifo)
{
#pragma HLS inline off
#pragma HLS pipeline II=1

	enum hrr_fsmStateType {META, GENERATE};
	static hrr_fsmStateType hrr_fsmState = META;
	static readRequest request; //Need QP, dma_length, vaddr
	static txMeta writeMeta;
	ibOpCode readOpcode;
	ap_uint<48> readAddr;
	ap_uint<32> readLength;
	ap_uint<32> dmaLength;

	switch (hrr_fsmState)
	{
	case META:
		if (!requestIn.empty())
		{
			requestIn.read(request);
			readAddr = request.vaddr;
			readLength = request.dma_length;
			dmaLength = request.dma_length;
			readOpcode = RC_RDMA_READ_RESP_ONLY;
			if (request.dma_length > PMTU)
			{
				readLength = PMTU;
				request.vaddr += PMTU;
				request.dma_length -= PMTU;
				readOpcode = RC_RDMA_READ_RESP_FIRST;
				hrr_fsmState = GENERATE;
			}
#if !POINTER_CHASING_EN
			memoryReadCmd.write(memCmdInternal(request.qpn, readAddr, dmaLength));
#else
			memoryReadCmd.write(memCmdInternal(request.qpn, readAddr, dmaLength, request.route));
#endif
			//event needs to contain QP, opCode, length, psn
			readEventFifo.write(event(readOpcode, request.qpn, readLength, request.psn));
		}
		break;
	case GENERATE:
		readAddr = request.vaddr;
		readLength = request.dma_length;
		if (request.dma_length > PMTU)
		{
			readLength = PMTU;
			request.vaddr += PMTU;
			request.dma_length -= PMTU;
			readOpcode = RC_RDMA_READ_RESP_MIDDLE;
		}
		else
		{
			readOpcode = RC_RDMA_READ_RESP_LAST;
			hrr_fsmState = META;
		}
		//memoryReadCmd.write(memCmdInternal(request.qpn, readAddr, readLength, (readOpcode == RC_RDMA_READ_RESP_LAST)));
		request.psn++;
		readEventFifo.write(event(readOpcode, request.qpn, readLength, request.psn));
		break;
	}
}
/*
 * For everything, except READ_RSP, we get PSN from state_table
 */
template <int WIDTH>
void generate_ibh(	stream<ibhMeta>&			metaIn,
					stream<ap_uint<24> >&		dstQpIn,
					stream<stateTableEntry>&	stateTable2txIbh_rsp,
					//stream<net_axis<WIDTH> >&			input,
					stream<txStateReq>&			txIbh2stateTable_upd_req,
#if RETRANS_EN
					stream<retransMeta>&		tx2retrans_insertMeta,
#endif
					stream<BaseTransportHeader<WIDTH> >& tx_ibhHeaderFifo)
{
#pragma HLS inline off
#pragma HLS pipeline II=1

	enum fsmStateType {META, GET_PSN};
	static fsmStateType gi_state = META;
	static BaseTransportHeader<WIDTH> header;

	static ibhMeta meta;
	net_axis<WIDTH> currWord;
	stateTableEntry qpState; //TODO what is really required
	ap_uint<24> dstQp;

	switch(gi_state)
	{
	case META:
		if (!metaIn.empty() && !dstQpIn.empty())
		{
			metaIn.read(meta);
			dstQpIn.read(dstQp);
			meta.partition_key = 0xFFFF; //TODO this is hard-coded, where does it come from??
			header.clear();

			header.setOpCode(meta.op_code);
			header.setPartitionKey(meta.partition_key);
			//PSN only valid for READ_RSP, otherwise we get it in state GET_PSN
			header.setPsn(meta.psn);
			header.setDstQP(dstQp); //TODO ist meta.dest_qp required??
			if (meta.validPSN)
			{
				tx_ibhHeaderFifo.write(header);
				//gi_state = HEADER;
			}
			else
			{
				txIbh2stateTable_upd_req.write(txStateReq(meta.dest_qp)); //TODO this is actually our qp
				gi_state = GET_PSN;
			}
		}
		break;
	case GET_PSN:
		if (!stateTable2txIbh_rsp.empty())
		{
			stateTable2txIbh_rsp.read(qpState);
			if (meta.op_code == RC_ACK)
			{
				header.setPsn(qpState.resp_epsn-1); //TODO -1 necessary??
			}
			else
			{
				header.setPsn(qpState.req_next_psn);
				header.setAckReq(true);
				//Update PSN
				ap_uint<24> nextPsn = qpState.req_next_psn + meta.numPkg;
				txIbh2stateTable_upd_req.write(txStateReq(meta.dest_qp, nextPsn));

				//Store Packet descirptor in retransmitter table
#if RETRANS_EN
				tx2retrans_insertMeta.write(retransMeta(meta.dest_qp, qpState.req_next_psn, meta.op_code));
#endif
			}
			tx_ibhHeaderFifo.write(header);
			gi_state = META;
		}
		break;
	}
}


/*
 * Types currently supported: DETH, RETH, AETH, ImmDt, IETH
 *
 * For reliable connections, page 246, 266, 269
 * RDM WRITE ONLY: RETH, PayLd
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
template <int WIDTH>
void generate_exh(	stream<event>&			metaIn,
#if POINTER_CHASING_EN
					stream<ptrChaseMeta>&	s_axis_tx_pcmeta,
#endif
					stream<txMsnRsp>&		msnTable2txExh_rsp,
					stream<ap_uint<16> >&	txExh2msnTable_req,
					stream<txReadReqUpdate>&	tx_readReqTable_upd,
					stream<ap_uint<16> >&	lengthFifo,
					stream<txPacketInfo>&	packetInfoFifo,
#if RETRANS_EN
					stream<ap_uint<24> >&	txSetTimer_req,
					//stream<retransAddrLen>&		tx2retrans_insertAddrLen,
#endif
					stream<net_axis<WIDTH> >&		output)
{
#pragma HLS inline off
#pragma HLS pipeline II=1

	enum ge_fsmStateType {META, GET_MSN, PROCESS};
	static ge_fsmStateType ge_state = META;
	static event meta;
	net_axis<WIDTH> sendWord;
	static RdmaExHeader<WIDTH> rdmaHeader;
	static AckExHeader<WIDTH>  ackHeader;
#if POINTER_CHASING_EN
	static ptrChaseMeta pcMeta;
	static RdmaPointerChaseHeader<WIDTH> pointerChaseHeader;
#endif
	static bool metaWritten;
	static txMsnRsp msnMeta;
	ap_uint<16> udpLen;
	txPacketInfo info;



	switch(ge_state)
	{
	case META:
		if (!metaIn.empty())
		{
			rdmaHeader.clear();
			ackHeader.clear();
#if POINTER_CHASING_EN
			pointerChaseHeader.clear();
#endif
			metaIn.read(meta);
			metaWritten = false;
			//if (meta.op_code == RC_RDMA_READ_RESP_ONLY || meta.op_code == RC_RDMA_READ_RESP_FIRST || meta.op_code == RC_RDMA_READ_RESP_MIDDLE || meta.op_code == RC_RDMA_READ_RESP_LAST || meta.op_code == RC_ACK)
			{
				txExh2msnTable_req.write(meta.qpn);
				ge_state = GET_MSN;
			}
			//else
			{
				// Start Timer for RDMW_WRITE_* & RDMA_READ_REQUEST

				//txSetTimer_req.write(meta.qpn);
				//ge_state = PROCESS;
			}
#if RETRANS_EN
			//TODO PART HIST
			if (meta.op_code == RC_RDMA_WRITE_ONLY || meta.op_code == RC_RDMA_WRITE_FIRST || meta.op_code == RC_RDMA_WRITE_MIDDLE || meta.op_code == RC_RDMA_WRITE_LAST || meta.op_code == RC_RDMA_READ_REQUEST)
			{
				txSetTimer_req.write(meta.qpn);
			}
#endif
		}
		break;
	case GET_MSN:
#if POINTER_CHASING_EN
		if (!msnTable2txExh_rsp.empty() && (meta.op_code != RC_RDMA_READ_POINTER_REQUEST || !s_axis_tx_pcmeta.empty()))
#else
		if (!msnTable2txExh_rsp.empty())// && (meta.op_code != RC_RDMA_READ_POINTER_REQUEST || !s_axis_tx_pcmeta.empty()))
#endif
		{
			msnTable2txExh_rsp.read(msnMeta);
#if POINTER_CHASING_EN
			if (meta.op_code == RC_RDMA_READ_POINTER_REQUEST)
			{
				s_axis_tx_pcmeta.read(pcMeta);
			}
#endif
			ge_state = PROCESS;
		}
		break;
	case PROCESS:
		{
			sendWord.last = 0;
			switch(meta.op_code)
			{
			case RC_RDMA_WRITE_ONLY:
			case RC_RDMA_WRITE_FIRST:
			case RC_RDMA_PART_ONLY:
			case RC_RDMA_PART_FIRST:
			{
				// [BTH][RETH][PayLd]
				rdmaHeader.setVirtualAddress(meta.addr);
				rdmaHeader.setLength(meta.length); //TODO Move up??
				rdmaHeader.setRemoteKey(msnMeta.r_key);
				ap_uint<8> remainingLength = rdmaHeader.consumeWord(sendWord.data);
				sendWord.keep = ~0;
				sendWord.last = (remainingLength == 0);
				std::cout << "RDMA_WRITE_ONLY/FIRST ";
				print(std::cout, sendWord);
				std::cout << std::endl;
				output.write(sendWord);
				if (remainingLength == 0)
				{
					//TODO
				}
				if (!metaWritten) //TODO we are losing 1 cycle here
				{
					info.isAETH = false;
					info.hasHeader = true;
					info.hasPayload = (meta.length != 0); //TODO should be true
					packetInfoFifo.write(info);


					/*std::cout << "RDMA_WRITE_ONLY/FIRST ";
					print(std::cout, sendWord);
					std::cout << std::endl;
					output.write(sendWord);*/

					//BTH: 12, RETH: 16, PayLd: x, ICRC: 4
					ap_uint<32> payloadLen = meta.length;
					if ((meta.op_code == RC_RDMA_WRITE_FIRST) || (meta.op_code == RC_RDMA_PART_FIRST))
					{
						payloadLen = PMTU;
					}
					udpLen = 12+16+payloadLen+4; //TODO dma_len can be much larger, for multiple packets we need to split this into multiple packets
					lengthFifo.write(udpLen);
					//Store meta for retransmit
/*#if RETRANS_EN
					if (!meta.validPsn) //indicates retransmission
					{
						tx2retrans_insertAddrLen.write(retransAddrLen(meta.addr, meta.length));
					}
#endif*/
					metaWritten = true;
				}
				break;
			}
			case RC_RDMA_WRITE_MIDDLE:
			case RC_RDMA_WRITE_LAST:
			case RC_RDMA_PART_MIDDLE:
			case RC_RDMA_PART_LAST:
				// [BTH][PayLd]
				info.isAETH = false;
				info.hasHeader = false;
				info.hasPayload = (meta.length != 0); //TODO should be true
				packetInfoFifo.write(info);
				//BTH: 12, PayLd: x, ICRC: 4
				udpLen = 12+meta.length+4;
				lengthFifo.write(udpLen);
				//Store meta for retransmit
/*#if RETRANS_EN
				if (!meta.validPsn) //indicates retransmission
				{
					tx2retrans_insertAddrLen.write(retransAddrLen(meta.addr, meta.length));
				}
#endif*/
				ge_state = META;
				break;
			case RC_RDMA_READ_REQUEST:
			case RC_RDMA_READ_CONSISTENT_REQUEST:
			{
				// [BTH][RETH]
				rdmaHeader.setVirtualAddress(meta.addr);
				rdmaHeader.setLength(meta.length); //TODO Move up??
				rdmaHeader.setRemoteKey(msnMeta.r_key);
				ap_uint<8> remainingLength = rdmaHeader.consumeWord(sendWord.data);
				sendWord.keep = ~0;
				sendWord.last = (remainingLength == 0);
				std::cout << "RDMA_READ_RWQ ";
				print(std::cout, sendWord);
				std::cout << std::endl;
				output.write(sendWord);
				if (!metaWritten) //TODO we are losing 1 cycle here
				{
					info.isAETH = false;
					info.hasHeader = true;
					info.hasPayload = false; //(meta.length != 0); //TODO should be true
					packetInfoFifo.write(info);

					/*std::cout << "RDMA_READ_RWQ ";
					print(std::cout, sendWord);
					std::cout << std::endl;
					output.write(sendWord);*/

					//BTH: 12, RETH: 16, PayLd: x, ICRC: 4
					udpLen = 12+16+0+4; //TODO dma_len can be much larger, for multiple packets we need to split this into multiple packets
					lengthFifo.write(udpLen);
					//Update Read Req max FWD header, TODO it is not exacly clear if meta.psn or meta.psn+numPkgs should be used
					//TODO i think psn is only used here!!
					tx_readReqTable_upd.write(txReadReqUpdate(meta.qpn, meta.psn));
					//Store meta for retransmit
/*#if RETRANS_EN
					if (!meta.validPsn) //indicates retransmission
					{
						tx2retrans_insertAddrLen.write(retransAddrLen(meta.addr, meta.length));
					}
#endif*/
					metaWritten = true;
				}
				break;
			}
#if POINTER_CHASING_EN
			case RC_RDMA_READ_POINTER_REQUEST:
			{
				// [BTH][RCTH]
				pointerChaseHeader.setVirtualAddress(meta.addr);
				pointerChaseHeader.setLength(meta.length); //TODO Move up??
				pointerChaseHeader.setRemoteKey(msnMeta.r_key);
				pointerChaseHeader.setPredicateKey(pcMeta.key);
				pointerChaseHeader.setPredicateMask(pcMeta.mask);
				pointerChaseHeader.setPredicateOp(pcMeta.op);
				pointerChaseHeader.setPtrOffset(pcMeta.ptrOffset);
				pointerChaseHeader.setIsRelPtr(pcMeta.relPtrOffset);
				pointerChaseHeader.setNextPtrOffset(pcMeta.nextPtrOffset);
				pointerChaseHeader.setNexPtrValid(pcMeta.nextPtrValid);
				
				ap_uint<8> remainingLength = pointerChaseHeader.consumeWord(sendWord.data);
				sendWord.keep = ~0; //0xFFFFFFFF; //TODO, set as much as required
				sendWord.last = (remainingLength == 0);
				std::cout << "RC_RDMA_READ_POINTER_REQUEST ";
				print(std::cout, sendWord);
				std::cout << std::endl;
				output.write(sendWord);
				if (!metaWritten)//TODO we are losing 1 cycle here
				{
					info.isAETH = false; //TODO fix this
					info.hasHeader = true;
					info.hasPayload = false; //(meta.length != 0); //TODO should be true
					packetInfoFifo.write(info);


					/*std::cout << "RC_RDMA_READ_POINTER_REQUEST ";
					print(std::cout, sendWord);
					std::cout << std::endl;
					output.write(sendWord);*/

					//BTH: 12, RCTH: 28, PayLd: x, ICRC: 4
					udpLen = 12+28+0+4;
					lengthFifo.write(udpLen);
					//Update Read Req max FWD header, TODO it is not exacly clear if meta.psn or meta.psn+numPkgs should be used
					//TODO i think psn is only used here!!
					tx_readReqTable_upd.write(txReadReqUpdate(meta.qpn, meta.psn));
					//Store meta for retransmit
/*#if RETRANS_EN
					if (!meta.validPsn) //indicates retransmission
					{
						tx2retrans_insertAddrLen.write(retransAddrLen(meta.addr, meta.length));
					}
#endif*/
					metaWritten = true;
				}
				break;
			}
#endif
			case RC_RDMA_READ_RESP_ONLY:
			case RC_RDMA_READ_RESP_FIRST:
			case RC_RDMA_READ_RESP_LAST:
			{
				// [BTH][AETH][PayLd]
				//AETH for first and last
				ackHeader.setSyndrome(0x1f);
				ackHeader.setMsn(msnMeta.msn);
				std::cout << "RDMA_READ_RESP MSN:" << ackHeader.getMsn() << std::endl;
				ackHeader.consumeWord(sendWord.data); //TODO
				{
					info.isAETH = true;
					info.hasHeader = true;
					info.hasPayload = (meta.length != 0); //TODO should be true
					packetInfoFifo.write(info);

					sendWord.keep((AETH_SIZE/8)-1, 0) = 0xFF;
					sendWord.keep(WIDTH-1, (AETH_SIZE/8)) = 0;
					sendWord.last = 1;

					std::cout << "RDMA_READ_RESP ";
					print(std::cout, sendWord);
					std::cout << std::endl;
					output.write(sendWord);

					//BTH: 12, AETH: 4, PayLd: x, ICRC: 4
					udpLen = 12+4+meta.length+4;
					//std::cout << "length: " << tempLen << ", dma len: " << meta.length << std::endl;
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
					ackHeader.setSyndrome(0x1f);
				}
				else
				{
					//PSN SEQ error
					ackHeader.setSyndrome(0x60);
				}
				ackHeader.setMsn(msnMeta.msn);
				std::cout << "RC_ACK MSN:" << ackHeader.getMsn() << std::endl;
				ackHeader.consumeWord(sendWord.data); //TODO
				{
					info.isAETH = true;
					info.hasHeader = true;
					info.hasPayload = false;
					packetInfoFifo.write(info);

					sendWord.keep(AETH_SIZE/8-1, 0) = 0xFF;
					sendWord.keep(WIDTH-1, (AETH_SIZE/8)) = 0;
					sendWord.last = 1;

					std::cout << "RC_ACK ";
					print(std::cout, sendWord);
					std::cout << std::endl;
					output.write(sendWord);

					//BTH: 12, AETH: 4, ICRC: 4
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

template <int WIDTH>
void append_payload(stream<txPacketInfo>&	packetInfoFifo,
					stream<net_axis<WIDTH> >&	tx_headerFifo,
					stream<net_axis<WIDTH> >&	tx_aethPayloadFifo,
					stream<net_axis<WIDTH> >&	tx_rethPayloadFifo,
					stream<net_axis<WIDTH> >&	tx_rawPayloadFifo,
					stream<net_axis<WIDTH> >&	tx_packetFifo)
{
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
		if (!packetInfoFifo.empty())
		{
			firstPayload = true;
			packetInfoFifo.read(info);

			if (info.hasHeader)
			{
				state = HEADER;
			}
			else
			{
				state = RAW_PAYLOAD;
			}
		}
		break;
	case HEADER:
		if (!tx_headerFifo.empty())
		{
			tx_headerFifo.read(prevWord);
			/*std::cout << "HEADER:";
			print(std::cout, prevWord);
			std::cout << std::endl;*/
			//TODO last is not necessary
			if (!prevWord.last) // || prevWord.keep[(WIDTH/8)-1] == 1) //One of them should be sufficient..
			{
				tx_packetFifo.write(prevWord);
			}
			else //last
			{
				if (!info.hasPayload)
				{
					state = INFO;
					tx_packetFifo.write(prevWord);
				}
				else // hasPayload
				{
					prevWord.last = 0;
					if (info.isAETH)
					{
						state = AETH_PAYLOAD;
					}
					else //RETH
					{
						if (WIDTH <= RETH_SIZE)
						{
							tx_packetFifo.write(prevWord);
						}
						state = RETH_PAYLOAD;
					}
				}
			}
		}
		break;
	case AETH_PAYLOAD:
		if (!tx_aethPayloadFifo.empty())
		{
			tx_aethPayloadFifo.read(currWord);
			std::cout << "PAYLOAD WORD: ";
			print(std::cout, currWord);
			std::cout << std::endl;

			sendWord = currWord;
			if (firstPayload)
			{
				sendWord.data(31, 0) = prevWord.data(31, 0);
				firstPayload = false;
			}
			std::cout << "AETH PAY: ";
			print(std::cout, sendWord);
			std::cout << std::endl;

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
			tx_rethPayloadFifo.read(currWord);
			std::cout << "PAYLOAD WORD: ";
			print(std::cout, currWord);
			std::cout << std::endl;

			sendWord = currWord;
			if (firstPayload && WIDTH > RETH_SIZE)
			{
				sendWord.data(127, 0) = prevWord.data(127, 0);
				firstPayload = false;
			}

			std::cout << "RETH PAYLOAD: ";
			print(std::cout, sendWord);
			std::cout << std::endl;


			tx_packetFifo.write(sendWord);
			if (currWord.last)
			{
				state = INFO;
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

//TODO this introduces 1 cycle for WIDTH > 64
template <int WIDTH>
void prepend_ibh_header(stream<BaseTransportHeader<WIDTH> >& tx_ibhHeaderFifo,
						stream<net_axis<WIDTH> >& tx_ibhPayloadFifo,
						stream<net_axis<WIDTH> >& m_axis_tx_data)
{
#pragma HLS inline off
#pragma HLS pipeline II=1

	enum pihStatea {GET_HEADER, HEADER, PARTIAL_HEADER, BODY};
	static pihStatea state = GET_HEADER;
	static BaseTransportHeader<WIDTH> header;
	static ap_uint<WIDTH> headerData;
	net_axis<WIDTH> currWord;

	switch (state)
	{
	case GET_HEADER:
		if (!tx_ibhHeaderFifo.empty())
		{
			tx_ibhHeaderFifo.read(header);
			if (BTH_SIZE >= WIDTH)
			{
				state = HEADER;
			}
			else
			{
				state = PARTIAL_HEADER;
			}
		}
		break;
	case HEADER:
	{
		ap_uint<8> remainingLength = header.consumeWord(currWord.data);
		if (remainingLength < (WIDTH/8))
		{
			state = PARTIAL_HEADER;
		}
		currWord.keep = ~0;
		currWord.last = 0;
		m_axis_tx_data.write(currWord);
		break;
	}
	case PARTIAL_HEADER:
		if (!tx_ibhPayloadFifo.empty())
		{
			tx_ibhPayloadFifo.read(currWord);
			std::cout << "IBH PARTIAL PAYLOAD: ";
			print(std::cout, currWord);
			std::cout << std::endl;

			header.consumeWord(currWord.data);
			m_axis_tx_data.write(currWord);

			std::cout << "IBH PARTIAL HEADER: ";
			print(std::cout, currWord);
			std::cout << std::endl;

			state = BODY;
			if (currWord.last)
			{
				state = GET_HEADER;
			}
		}
		break;
	case BODY:
		if (!tx_ibhPayloadFifo.empty())
		{
			tx_ibhPayloadFifo.read(currWord);
			m_axis_tx_data.write(currWord);

			std::cout << "IBH PAYLOAD WORD: ";
			print(std::cout, currWord);
			std::cout << std::endl;

			if (currWord.last)
			{
				state = GET_HEADER;
			}
		}
		break;
	}
}


void local_req_handler(	stream<txMeta>&				s_axis_tx_meta,
#if RETRANS_EN
						stream<retransEvent>&		retransEventFifo,
#endif
						stream<memCmdInternal>&		tx_local_memCmdFifo, //TODO rename
						stream<mqInsertReq<ap_uint<64> > >&		tx_localReadAddrFifo,
#if !RETRANS_EN
						stream<event>&				tx_localTxMeta)
#else
						stream<event>&				tx_localTxMeta,
						stream<retransAddrLen>&		tx2retrans_insertAddrLen)
#endif
{
#pragma HLS inline off
#pragma HLS pipeline II=1

	enum fsmStateType {META, GENERATE};
	static fsmStateType lrh_state;
	static txMeta meta;

	event ev;
	retransEvent rev;
	ibOpCode writeOpcode;
	ap_uint<48> raddr;
	ap_uint<48> laddr;
	ap_uint<32> length;
	ap_uint<32> dmaLength;

	switch (lrh_state)
	{
	case META:
#if RETRANS_EN
		if (!retransEventFifo.empty())
		{
			retransEventFifo.read(rev);
			tx_localTxMeta.write(event(rev.op_code, rev.qpn, rev.remoteAddr, rev.length, rev.psn));
			if (rev.op_code != RC_RDMA_READ_REQUEST)
			{
				length = rev.length;
				std::cout << std::dec << "length to retranmist: " << rev.length << ", local addr: " << std::hex << rev.localAddr << ", remote addres: " << rev.remoteAddr << ", psn: " << rev.psn << std::endl;
				if (ev.op_code == RC_RDMA_WRITE_FIRST || ev.op_code == RC_RDMA_PART_FIRST)
				{
					length = PMTU;
				}
				tx_local_memCmdFifo.write(memCmdInternal(rev.qpn, rev.localAddr, length));
			}
		}
		else if (!s_axis_tx_meta.empty())
#else
		if (!s_axis_tx_meta.empty())
#endif
		{
			s_axis_tx_meta.read(meta);
			if (meta.op_code == APP_READ || meta.op_code == APP_POINTER || meta.op_code == APP_READ_CONSISTENT)
			{
				if (meta.op_code == APP_READ)
				{
					tx_localTxMeta.write(event(RC_RDMA_READ_REQUEST, meta.qpn, meta.remote_vaddr, meta.length));
				}
				else if (meta.op_code == APP_READ_CONSISTENT)
				{
					tx_localTxMeta.write(event(RC_RDMA_READ_CONSISTENT_REQUEST, meta.qpn, meta.remote_vaddr, meta.length));
				}
#if POINTER_CHASING_EN
				else
				{
					tx_localTxMeta.write(event(RC_RDMA_READ_POINTER_REQUEST, meta.qpn, meta.remote_vaddr, meta.length));
				}
#endif
				tx_localReadAddrFifo.write(mqInsertReq<ap_uint<64> >(meta.qpn, meta.local_vaddr));
#if RETRANS_EN
				tx2retrans_insertAddrLen.write(retransAddrLen(meta.local_vaddr, meta.remote_vaddr, meta.length));
#endif
			}
			else //APP_WRITE, APP_PART
			{
				laddr = meta.local_vaddr;
				raddr = meta.remote_vaddr;
				dmaLength = meta.length;
				writeOpcode = (meta.op_code == APP_PART) ? RC_RDMA_PART_ONLY : RC_RDMA_WRITE_ONLY;

				if (meta.length > PMTU)
				{
					meta.local_vaddr += PMTU;
					meta.remote_vaddr += PMTU;
					meta.length -= PMTU;
					writeOpcode = (meta.op_code == APP_PART) ? RC_RDMA_PART_FIRST : RC_RDMA_WRITE_FIRST;
					lrh_state = GENERATE;
				}
				//TODO retintroduce this functionality
				/*if (dmaLength > PCIE_BATCH_SIZE)
				{
					dmaLength -= PCIE_BATCH_SIZE;
					tx_local_memCmdFifo.write(memCmdInternal(meta.qpn, laddr, PCIE_BATCH_SIZE));
				}
				else*/
				{
					tx_local_memCmdFifo.write(memCmdInternal(meta.qpn, laddr, dmaLength));
				}
				//event needs to contain QP, opCode, length, psn
				tx_localTxMeta.write(event(writeOpcode, meta.qpn, raddr, dmaLength));
#if RETRANS_EN
				tx2retrans_insertAddrLen.write(retransAddrLen(laddr, raddr, dmaLength));
#endif
			}
		}
		break;
	case GENERATE:
		laddr = meta.local_vaddr;
		raddr = meta.remote_vaddr;
		length = meta.length;
		if (meta.length > PMTU)
		{
			length = PMTU;
			meta.local_vaddr += PMTU;
			meta.remote_vaddr += PMTU;
			meta.length -= PMTU;
			writeOpcode = (meta.op_code == APP_PART) ? RC_RDMA_PART_MIDDLE : RC_RDMA_WRITE_MIDDLE;
		}
		else
		{
			writeOpcode = (meta.op_code == APP_PART) ? RC_RDMA_PART_LAST : RC_RDMA_WRITE_LAST;
			lrh_state = META;
		}
		//tx_local_memCmdFifo.write(memCmdInternal(meta.qpn, laddr, length, (writeOpcode == RC_RDMA_WRITE_LAST || writeOpcode == RC_RDMA_PART_LAST)));
		tx_localTxMeta.write(event(writeOpcode, meta.qpn, raddr, length));
#if RETRANS_EN
		tx2retrans_insertAddrLen.write(retransAddrLen(laddr, raddr, length));
#endif
		break;
	} //switch

}



//TODO this only works with axi width 64
template <int WIDTH>
void fpga_data_handler(	stream<net_axis<WIDTH> >&	s_axis_tx_data,
						stream<net_axis<WIDTH> >&	appTxData) //switch to internal format
{
#pragma HLS inline off
#pragma HLS pipeline II=1

	static ap_uint<16> remainingLength;

	net_axis<WIDTH> currWord;

	if (!s_axis_tx_data.empty())
	{
		s_axis_tx_data.read(currWord);
		remainingLength -= (WIDTH/8); //TODO only works with WIDTH == 64
		if (remainingLength == 0)
		{
			currWord.last = 1;
			remainingLength = PMTU;
		}
		appTxData.write(currWord);
	}

}

/*
 * rx_ackEventFifo RC_ACK from ibh and exh
 * rx_readEvenFifo READ events from RX side
 * tx_appMetaFifo, retransmission events, WRITEs and READ_REQ only
 */
void meta_merger(	stream<ackEvent>&	rx_ackEventFifo,
					stream<event>&		rx_readEvenFifo,
					stream<event>&		tx_appMetaFifo,
					//stream<event>&		timer2exhFifo,
					stream<ap_uint<16> >&	tx_connTable_req,
					stream<ibhMeta>&	tx_ibhMetaFifo,
					stream<event>&		tx_exhMetaFifo)
{
#pragma HLS inline off
#pragma HLS pipeline II=1

	ackEvent aev;
	event ev;
	ap_uint<16> key = 0; //TODO hack

	if (!rx_ackEventFifo.empty())
	{
		rx_ackEventFifo.read(aev);

		tx_connTable_req.write(aev.qpn(15, 0));
		// PSN used for read response
		tx_ibhMetaFifo.write(ibhMeta(RC_ACK, key, aev.qpn, aev.psn, aev.validPsn));
		tx_exhMetaFifo.write(event(aev));
	}
	else if (!rx_readEvenFifo.empty())
	{
		rx_readEvenFifo.read(ev);
		tx_connTable_req.write(ev.qpn(15, 0));
		// PSN used for read response
		tx_ibhMetaFifo.write(ibhMeta(ev.op_code, key, ev.qpn, ev.psn, ev.validPsn));
		tx_exhMetaFifo.write(ev);
	}
	else if (!tx_appMetaFifo.empty()) //TODO rename
	{
		tx_appMetaFifo.read(ev);

		ap_uint<22> numPkg = 1;
		if (ev.op_code == RC_RDMA_READ_REQUEST || ev.op_code == RC_RDMA_READ_POINTER_REQUEST || ev.op_code == RC_RDMA_READ_CONSISTENT_REQUEST)
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

//TODO maybe all ACKS should be triggered by ibhFSM?? what is the guarantee we should/have to give
//TODO this should become a BRAM, storage type of thing
template <int WIDTH>
void ipUdpMetaHandler(	stream<ipUdpMeta>&		input,
						stream<ExHeader<WIDTH> >& exHeaderInput,
						stream<fwdPolicy>&			dropMetaIn,
						//stream<dstTuple>&		output,
						//stream<ap_uint<16> >&	remcrc_lengthFifo,
						stream<ap_uint<16> >&	exh_lengthFifo,
						stream<ExHeader<WIDTH> >& exHeaderOutput)
{
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

void tx_ipUdpMetaMerger(	stream<connTableEntry>& tx_connTable2ibh_rsp,
							stream<ap_uint<16> >&	tx_lengthFifo,
							stream<ipUdpMeta>&		m_axis_tx_meta,
							stream<ap_uint<24> >&	tx_dstQpFifo)
{
#pragma HLS inline off
#pragma HLS pipeline II=1

	connTableEntry connMeta;
	ap_uint<16> len;

	if (!tx_connTable2ibh_rsp.empty() && !tx_lengthFifo.empty())
	{
		tx_connTable2ibh_rsp.read(connMeta);
		tx_lengthFifo.read(len);
		std::cout << "PORT: " << connMeta.remote_udp_port << std::endl;
		m_axis_tx_meta.write(ipUdpMeta(connMeta.remote_ip_address, RDMA_DEFAULT_PORT, connMeta.remote_udp_port, len));
		tx_dstQpFifo.write(connMeta.remote_qpn);
	}
}

void qp_interface(	stream<qpContext>& 			contextIn,
					stream<stateTableEntry>&	stateTable2qpi_rsp,
					stream<ifStateReq>&			qpi2stateTable_upd_req,
					stream<ifMsnReq>&			if2msnTable_init)
{
#pragma HLS inline off
#pragma HLS pipeline II=1

	enum fstStateType{GET_STATE, UPD_STATE};
	static fstStateType qp_fsmState = GET_STATE;
	static qpContext context;
	stateTableEntry state;

	switch(qp_fsmState)
	{
	case GET_STATE:
		if (!contextIn.empty())
		{
			contextIn.read(context);
			qpi2stateTable_upd_req.write(context.qp_num);
			qp_fsmState = UPD_STATE;
		}
		break;
	case UPD_STATE:
		if (!stateTable2qpi_rsp.empty())
		{
			stateTable2qpi_rsp.read(state);
			//TODO check if valid transition
			qpi2stateTable_upd_req.write(ifStateReq(context.qp_num, context.newState, context.remote_psn, context.local_psn));
			if2msnTable_init.write(ifMsnReq(context.qp_num, context.r_key)); //TODO store virtual address somewhere??
			qp_fsmState = GET_STATE;
		}
		break;
	}
}

void three_merger(stream<event>& in0, stream<event>& in1, stream<event>& in2, stream<event>& out)
{
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

template <int WIDTH>
void mem_cmd_merger(stream<memCmdInternal>& remoteReadRequests,
					stream<memCmdInternal>& localReadRequests,
					stream<routedMemCmd>&			out,
					stream<pkgInfo>&		pkgInfoFifo)
{
#pragma HLS inline off
#pragma HLS pipeline II=1

	memCmdInternal cmd;

	if (!remoteReadRequests.empty())
	{
		remoteReadRequests.read(cmd);
		out.write(routedMemCmd(cmd.addr, cmd.len, cmd.route));
#if POINTER_CHASING_EN
		if (cmd.route == ROUTE_CUSTOM)
		{
			pkgInfoFifo.write(pkgInfo(AETH, FIFO, ((cmd.len+(WIDTH/8)-1)/(WIDTH/8))));
		}
		else
#endif
		{
			pkgInfoFifo.write(pkgInfo(AETH, MEM, ((cmd.len+(WIDTH/8)-1)/(WIDTH/8))));
		}
	}
	else if (!localReadRequests.empty())
	{
		localReadRequests.read(cmd);
		//CHECK if data in memory
		if (cmd.addr != 0)
		{
			out.write(routedMemCmd(cmd.addr, cmd.len, cmd.route));
			pkgInfoFifo.write(pkgInfo(RETH, MEM, ((cmd.len+(WIDTH/8)-1)/(WIDTH/8))));
		}
		else
		{
			pkgInfoFifo.write(pkgInfo(RETH, FIFO, ((cmd.len+(WIDTH/8)-1)/(WIDTH/8))));
		}
	}

}

void merge_retrans_request(	stream<retransMeta>&		tx2retrans_insertMeta,
							stream<retransAddrLen>&		tx2retrans_insertAddrLen,
							stream<retransEntry>&		tx2retrans_insertRequest)
{
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

template <int WIDTH>
void merge_rx_pkgs(	stream<pkgShiftType>&	rx_pkgShiftTypeFifo,
					stream<net_axis<WIDTH> >&		rx_aethSift2mergerFifo,
					stream<routed_net_axis<WIDTH> >&	rx_rethSift2mergerFifo,
					stream<routed_net_axis<WIDTH> >&	rx_NoSift2mergerFifo,
					stream<routed_net_axis<WIDTH> >&	m_axis_mem_write_data)
{
#pragma HLS inline off
#pragma HLS pipeline II=1

	enum mrpStateType{IDLE, FWD_AETH, FWD_RETH, FWD_NONE};
	static mrpStateType state = IDLE;

	pkgShiftType type;

	switch (state)
	{
	case IDLE:
		if (!rx_pkgShiftTypeFifo.empty())
		{
			rx_pkgShiftTypeFifo.read(type);
			if (type == SHIFT_AETH)
			{
				state = FWD_AETH;
			}
			else if (type == SHIFT_RETH)
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
		if (!rx_aethSift2mergerFifo.empty())
		{
			net_axis<WIDTH> currWord;
			rx_aethSift2mergerFifo.read(currWord);
			m_axis_mem_write_data.write(routed_net_axis<WIDTH>(currWord, ROUTE_DMA));
			if (currWord.last)
			{
				state = IDLE;
			}
		}
		break;
	case FWD_RETH:
		if (!rx_rethSift2mergerFifo.empty())
		{
			routed_net_axis<WIDTH> currWord;
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
			routed_net_axis<WIDTH> currWord;
			rx_NoSift2mergerFifo.read(currWord);
			m_axis_mem_write_data.write(currWord);
			if (currWord.last)
			{
				state = IDLE;
			}
		}
	}//switch
}

template <int WIDTH>
void tx_pkg_arbiter(stream<pkgInfo>&	tx_pkgInfoFifo,
					stream<net_axis<WIDTH> >&	s_axis_tx_data,
					stream<net_axis<WIDTH> >&	s_axis_mem_read_data,
					stream<net_axis<WIDTH> >&	remoteReadData,
					stream<net_axis<WIDTH> >&	localReadData,
					stream<net_axis<WIDTH> >&	rawPayFifo)
{
#pragma HLS inline off
#pragma HLS pipeline II=1

	enum mrpStateType{IDLE, FWD_MEM_AETH, FWD_MEM_RETH, FWD_MEM_RAW, FWD_STREAM_AETH, FWD_STREAM_RETH, FWD_STREAM_RAW};
	static mrpStateType state = IDLE;
	static ap_uint<8> wordCounter = 0;

	static pkgInfo info;
	net_axis<WIDTH> currWord;

	switch (state)
	{
	case IDLE:
		if (!tx_pkgInfoFifo.empty())
		{
			tx_pkgInfoFifo.read(info);
			wordCounter = 0;
			if (info.source == MEM)
			{
				if (info.type == AETH)
				{
					state = FWD_MEM_AETH;
				}
				else
				{
					state = FWD_MEM_RETH;
				}
			}
			else
			{
				if (info.type == AETH)
				{
					state = FWD_STREAM_AETH;
				}
				else
				{
					state = FWD_STREAM_RETH;
				}
			}
		}
		break;
	case FWD_STREAM_AETH:
		if (!s_axis_tx_data.empty())
		{
			s_axis_tx_data.read(currWord);
			wordCounter++;
			if (currWord.last)
			{
				state = IDLE;
			}
			if (wordCounter == PMTU_WORDS)
			{
				currWord.last = 1;
				wordCounter = 0;
				info.words -= PMTU_WORDS;
				//Check if next one is READ_RSP_MIDDLE
				if (info.words > PMTU_WORDS)
				{
					state = FWD_STREAM_RAW;
				}
			}
			remoteReadData.write(currWord);
		}
		break;
	case FWD_STREAM_RETH:
		if (!s_axis_tx_data.empty())
		{
			s_axis_tx_data.read(currWord);
			wordCounter++;
			if (currWord.last)
			{
				state = IDLE;
			}
			if (wordCounter == PMTU_WORDS)
			{
				currWord.last = 1;
				wordCounter = 0;
			}
			localReadData.write(currWord);

		}
		break;
	case FWD_MEM_AETH:
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
				wordCounter = 0;
				info.words -= PMTU_WORDS;
				//Check if next one is READ_RSP_MIDDLE
				if (info.words > PMTU_WORDS)
				{
					state = FWD_MEM_RAW;
				}
			}
			remoteReadData.write(currWord);
		}
		break;
	case FWD_MEM_RETH:
		if (!s_axis_mem_read_data.empty())
		{
			s_axis_mem_read_data.read(currWord);
			std::cout << "RETH DATA FROM MEMORY: ";
			print(std::cout, currWord);
			std::cout << std::endl;


			wordCounter++;
			if (currWord.last)
			{
				state = IDLE;
			}
			if (wordCounter == PMTU_WORDS)
			{
				currWord.last = 1;
				wordCounter = 0;
				info.words -= PMTU_WORDS;
				state = FWD_MEM_RAW;
			}
			localReadData.write(currWord);
		}
		break;
	case FWD_MEM_RAW:
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
				wordCounter = 0;
				info.words -= PMTU_WORDS;
				if (info.type == AETH && info.words <= PMTU_WORDS)
				{
					state = FWD_MEM_AETH;
				}
			}
			rawPayFifo.write(currWord);
		}
		break;
	case FWD_STREAM_RAW:
		if (!s_axis_tx_data.empty())
		{
			s_axis_tx_data.read(currWord);
			wordCounter++;
			if (currWord.last)
			{
				state = IDLE;
			}
			if (wordCounter == PMTU_WORDS)
			{
				currWord.last = 1;
				wordCounter = 0;
				info.words -= PMTU_WORDS;
				if (info.type == AETH && info.words <= PMTU_WORDS)
				{
					state = FWD_STREAM_AETH;
				}
			}
			rawPayFifo.write(currWord);
		}
		break;
	}//switch

}

template <int WIDTH>
void ib_transport_protocol(	//RX
							stream<ipUdpMeta>&	s_axis_rx_meta,
							stream<net_axis<WIDTH> >&	s_axis_rx_data,
							//stream<net_axis<WIDTH> >&	m_axis_rx_data,
							//TX
							stream<txMeta>&		s_axis_tx_meta,
							stream<net_axis<WIDTH> >&	s_axis_tx_data,
							stream<ipUdpMeta>&	m_axis_tx_meta,
							stream<net_axis<WIDTH> >&	m_axis_tx_data,
							//Memory
							stream<routedMemCmd>&		m_axis_mem_write_cmd,
							stream<routedMemCmd>&		m_axis_mem_read_cmd,
							//stream<mmStatus>&	s_axis_mem_write_status,

							stream<routed_net_axis<WIDTH> >&	m_axis_mem_write_data,
							stream<net_axis<WIDTH> >&	s_axis_mem_read_data,

							//Interface
							stream<qpContext>& s_axis_qp_interface,
							stream<ifConnReq>&	s_axis_qp_conn_interface,

							//Pointer chasing
#if POINTER_CHASING_EN
							stream<ptrChaseMeta>&	m_axis_rx_pcmeta,
							stream<ptrChaseMeta>&	s_axis_tx_pcmeta,
#endif

							ap_uint<32>&		regInvalidPsnDropCount
							)
{
#pragma HLS INLINE


	static stream<net_axis<WIDTH> > 			rx_ibh2shiftFifo("rx_ibh2shiftFifo");
	static stream<net_axis<WIDTH> > 			rx_shift2exhFifo("rx_shift2exhFifo");
	static stream<net_axis<WIDTH> > 			rx_exh2dropFifo("rx_exh2dropFifo");
	static stream<net_axis<WIDTH> >			rx_ibhDrop2exhFifo("rx_ibhDrop2exhFifo");
	static stream<ibhMeta> 			rx_ibh2fsm_MetaFifo("rx_ibh2fsm_MetaFifo");
	static stream<ibhMeta>			rx_fsm2exh_MetaFifo("rx_fsm2exh_MetaFifo");
	static stream<routed_net_axis<WIDTH> >	rx_exh2rethShiftFifo("rx_exh2rethShiftFifo");
	static stream<net_axis<WIDTH> >			rx_exh2aethShiftFifo("rx_exh2aethShiftFifo");
	static stream<routed_net_axis<WIDTH> >	rx_exhNoShiftFifo("rx_exhNoShiftFifo");
	static stream<routed_net_axis<WIDTH> >	rx_rethSift2mergerFifo("rx_rethSift2mergerFifo");
	static stream<net_axis<WIDTH> >			rx_aethSift2mergerFifo("rx_aethSift2mergerFifo");
	static stream<pkgSplitType>		rx_pkgSplitTypeFifo("rx_pkgSplitTypeFifo");
	static stream<pkgShiftType> 	rx_pkgShiftTypeFifo("rx_pkgShiftTypeFifo");
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
	#pragma HLS DATA_PACK variable=rx_ibh2fsm_MetaFifo
	#pragma HLS DATA_PACK variable=rx_fsm2exh_MetaFifo
	#pragma HLS DATA_PACK variable=rx_pkgSplitTypeFifo
	#pragma HLS DATA_PACK variable=rx_pkgShiftTypeFifo

	static stream<ackEvent>  rx_ibhEventFifo("rx_ibhEventFifo"); //TODO rename
	static stream<ackEvent>  rx_exhEventMetaFifo("rx_exhEventMetaFifo");
	static stream<memCmdInternal> rx_remoteMemCmd("rx_remoteMemCmd");
	#pragma HLS STREAM depth=2 variable=rx_ibhEventFifo
	#pragma HLS STREAM depth=2 variable=rx_exhEventMetaFifo
	#pragma HLS STREAM depth=512 variable=rx_remoteMemCmd
	#pragma HLS DATA_PACK variable=rx_ibhEventFifo
	#pragma HLS DATA_PACK variable=rx_exhEventMetaFifo
	#pragma HLS DATA_PACK variable=rx_remoteMemCmd

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
	#pragma HLS DATA_PACK variable=tx_exhMetaFifo
	#pragma HLS DATA_PACK variable=tx_ibhHeaderFifo
	#pragma HLS DATA_PACK variable=tx_localMemCmdFifo

	static stream<txPacketInfo>	tx_packetInfoFifo("tx_packetInfoFifo");
	static stream<ap_uint<16> > tx_lengthFifo("tx_lengthFifo");
	#pragma HLS STREAM depth=2 variable=tx_packetInfoFifo
	#pragma HLS STREAM depth=4 variable=tx_lengthFifo
	#pragma HLS DATA_PACK variable=tx_packetInfoFifo

	static stream<bool> rx_ibhDropFifo("rx_ibhDropFifo");
	static stream<fwdPolicy> rx_ibhDropMetaFifo("rx_ibhDropMetaFifo");
	#pragma HLS STREAM depth=2 variable=rx_ibhDropFifo
	#pragma HLS STREAM depth=2 variable=rx_ibhDropMetaFifo
	#pragma HLS DATA_PACK variable=rx_ibhDropMetaFifo

	//Connection Table
	static stream<ap_uint<16> >	tx_ibhconnTable_req("tx_ibhconnTable_req");
	//static stream<ifConnReq>		qpi2connTable_req("qpi2connTable_req");
	static stream<connTableEntry>	tx_connTable2ibh_rsp("tx_connTable2ibh_rsp");
	//static stream<connTableEntry> connTable2qpi_rsp("connTable2qpi_rsp");
	#pragma HLS STREAM depth=2 variable=tx_ibhconnTable_req
	#pragma HLS STREAM depth=8 variable=tx_connTable2ibh_rsp
	#pragma HLS DATA_PACK variable=tx_connTable2qpi_rsp

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
	#pragma HLS DATA_PACK variable=rxIbh2stateTable_upd_req
	#pragma HLS DATA_PACK variable=txIbh2stateTable_upd_req
	#pragma HLS DATA_PACK variable=qpi2stateTable_upd_req
	#pragma HLS DATA_PACK variable=stateTable2rxIbh_rsp
	#pragma HLS DATA_PACK variable=stateTable2txIbh_rsp
	#pragma HLS DATA_PACK variable=stateTable2qpi_rsp

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
	#pragma HLS DATA_PACK variable=rxExh2msnTable_upd_req
	#pragma HLS DATA_PACK variable=if2msnTable_init
	#pragma HLS DATA_PACK variable=msnTable2rxExh_rsp
	#pragma HLS DATA_PACK variable=msnTable2txExh_rsp

	static stream<ap_uint<16> > exh_lengthFifo("exh_lengthFifo");
	static stream<readRequest>	rx_readRequestFifo("rx_readRequestFifo");
	static stream<event>		rx_readEvenFifo("rx_readEvenFifo");
	static stream<ackEvent>		rx_ackEventFifo("rx_ackEventFifo");
	#pragma HLS STREAM depth=4 variable=exh_lengthFifo
	#pragma HLS STREAM depth=8 variable=rx_readRequestFifo
	#pragma HLS STREAM depth=512 variable=rx_readEvenFifo
	#pragma HLS STREAM depth=4 variable=rx_ackEventFifo
	#pragma HLS DATA_PACK variable=rx_readRequestFifo
	#pragma HLS DATA_PACK variable=rx_readEvenFifo
	#pragma HLS DATA_PACK variable=rx_ackEventFifo

	// Read Req Table
	static stream<txReadReqUpdate>	tx_readReqTable_upd("tx_readReqTable_upd");
	static stream<rxReadReqUpdate>	rx_readReqTable_upd_req("rx_readReqTable_upd_req");
	static stream<rxReadReqRsp>		rx_readReqTable_upd_rsp("rx_readReqTable_upd_rsp");
	#pragma HLS STREAM depth=2 variable=tx_readReqTable_upd
	#pragma HLS STREAM depth=2 variable=rx_readReqTable_upd_req
	#pragma HLS STREAM depth=2 variable=rx_readReqTable_upd_rsp
	#pragma HLS DATA_PACK variable=tx_readReqTable_upd
	#pragma HLS DATA_PACK variable=rx_readReqTable_upd_req
	#pragma HLS DATA_PACK variable=rx_readReqTable_upd_rsp

	// Outstanding Read Req Table
	//TODO merge these two
	static stream<mqInsertReq<ap_uint<64> > >	tx_readReqAddr_push("tx_readReqAddr_push");
	static stream<mqPopReq> rx_readReqAddr_pop_req("rx_readReqAddr_pop_req");
	static stream<ap_uint<64> > rx_readReqAddr_pop_rsp("rx_readReqAddr_pop_rsp");
	#pragma HLS STREAM depth=2 variable=tx_readReqAddr_push
	#pragma HLS STREAM depth=2 variable=rx_readReqAddr_pop_req
	#pragma HLS STREAM depth=2 variable=rx_readReqAddr_pop_rsp
	#pragma HLS DATA_PACK variable=rx_readReqAddr_pop_req
	#pragma HLS DATA_PACK variable=rx_readReqAddr_pop_rsp


	/*
	 * TIMER & RETRANSMITTER
	 */
#if RETRANS_EN
	static stream<rxTimerUpdate> rxClearTimer_req("rxClearTimer_req");
	static stream<ap_uint<24> > txSetTimer_req("txSetTimer_req");
	static stream<retransRelease>	rx2retrans_release_upd("rx2retrans_release_upd");
	static stream<retransmission> rx2retrans_req("rx2retrans_req");
	static stream<retransmission> timer2retrans_req("timer2retrans_req");
	static stream<retransMeta>		tx2retrans_insertMeta("tx2retrans_insertMeta");
	static stream<retransAddrLen>	tx2retrans_insertAddrLen("tx2retrans_insertAddrLen");
	static stream<retransEntry>		tx2retrans_insertRequest("tx2retrans_insertRequest");
	static stream<retransEvent> 	retransmitter2exh_eventFifo("retransmitter2exh_eventFifo");
	#pragma HLS STREAM depth=2 variable=rxClearTimer_req
	#pragma HLS STREAM depth=2 variable=txSetTimer_req
	#pragma HLS STREAM depth=2 variable=rx2retrans_release_upd
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
	qp_interface(s_axis_qp_interface, stateTable2qpi_rsp, qpi2stateTable_upd_req, if2msnTable_init);


	/*
	 * RX PATH
	 */
	static stream<ibOpCode> rx_ibh2exh_MetaFifo("rx_ibh2exh_MetaFifo");
	static stream<ExHeader<WIDTH> > rx_exh2drop_MetaFifo("rx_exh2drop_MetaFifo");
	static stream<ExHeader<WIDTH> > rx_drop2exhFsm_MetaFifo("rx_drop2exhFsm_MetaFifo");
	static stream<exhMeta>	rx_exhMetaFifo("rx_exhMetaFifo");
	#pragma HLS STREAM depth=2 variable=rx_ibh2exh_MetaFifo
	#pragma HLS STREAM depth=8 variable=rx_exh2drop_MetaFifo
	#pragma HLS STREAM depth=2 variable=rx_drop2exhFsm_MetaFifo
	#pragma HLS STREAM depth=2 variable=rx_exhMetaFifo
	#pragma HLS DATA_PACK variable=rx_ibh2exh_MetaFifo
	#pragma HLS DATA_PACK variable=rx_exh2drop_MetaFifo
	#pragma HLS DATA_PACK variable=rx_drop2exhFsm_MetaFifo
	#pragma HLS DATA_PACK variable=rx_exhMetaFifo

	rx_process_ibh(s_axis_rx_data, rx_ibh2fsm_MetaFifo,rx_ibh2exh_MetaFifo, rx_ibh2shiftFifo);

	rshiftWordByOctet<net_axis<WIDTH>, WIDTH,11>(((BTH_SIZE%WIDTH)/8), rx_ibh2shiftFifo, rx_shift2exhFifo);

	rx_process_exh(	rx_shift2exhFifo,
					rx_ibh2exh_MetaFifo,
					rx_exhMetaFifo,
					rx_exh2drop_MetaFifo, //TODO check if this has to be dropped
					rx_exh2dropFifo);


	rx_ibh_fsm(	rx_ibh2fsm_MetaFifo,
				rx_exhMetaFifo,
				stateTable2rxIbh_rsp,
				rxIbh2stateTable_upd_req,
				rx_fsm2exh_MetaFifo,
				rx_ibhEventFifo,
				rx_ibhDropFifo,
				rx_ibhDropMetaFifo,
#if RETRANS_EN
				rxClearTimer_req,
				rx2retrans_release_upd,
#endif
				regInvalidPsnDropCount);

	drop_ooo_ibh(rx_exh2dropFifo, rx_ibhDropFifo, rx_ibhDrop2exhFifo);

	//some hack TODO, make this nicer.. not sure what this is still for
	ipUdpMetaHandler(s_axis_rx_meta, rx_exh2drop_MetaFifo, rx_ibhDropMetaFifo, exh_lengthFifo, rx_drop2exhFsm_MetaFifo);

	rx_exh_fsm(	rx_fsm2exh_MetaFifo,
				exh_lengthFifo,
				msnTable2rxExh_rsp,
#if RETRANS_EN
				rx_readReqTable_upd_rsp,
#endif
				rx_readReqAddr_pop_rsp,
				rx_drop2exhFsm_MetaFifo,
				//rx_ibhDrop2exhFifo,
				m_axis_mem_write_cmd,
				rx_readRequestFifo,
#if POINTER_CHASING_EN
				m_axis_rx_pcmeta,
#endif
				rxExh2msnTable_upd_req,
				rx_readReqTable_upd_req,
				rx_readReqAddr_pop_req,
				rx_exhEventMetaFifo,
#if RETRANS_EN

				rx2retrans_req,
#endif
				//rx_rethSift2mergerFifo,
				//rx_exh2aethShiftFifo,
				//rx_exhNoShiftFifo,
				rx_pkgSplitTypeFifo,
				rx_pkgShiftTypeFifo);

	rx_exh_payload(	rx_pkgSplitTypeFifo,
					rx_ibhDrop2exhFifo,
//#if AXI_WIDTH == 64
//					rx_rethSift2mergerFifo,
//#else
					rx_exh2rethShiftFifo,
//#endif
					rx_exh2aethShiftFifo,
					rx_exhNoShiftFifo);

	handle_read_requests(	rx_readRequestFifo,
							rx_remoteMemCmd,
							rx_readEvenFifo);

	//TODO is order important??
	stream_merger<ackEvent>(rx_exhEventMetaFifo, rx_ibhEventFifo, rx_ackEventFifo);

	// RETH: 16 bytes
	//TODO not required for AXI_WIDTH == 64, also this seems to have a bug, this goes together with the hack in process_exh where we don't write the first word out
//#if AXI_WIDTH != 64
	rshiftWordByOctet<routed_net_axis<WIDTH>, WIDTH,12>(((RETH_SIZE%WIDTH)/8), rx_exh2rethShiftFifo, rx_rethSift2mergerFifo);
//#endif
	// AETH: 4 bytes
	rshiftWordByOctet<net_axis<WIDTH>, WIDTH,13>(((AETH_SIZE%WIDTH)/8), rx_exh2aethShiftFifo, rx_aethSift2mergerFifo);

	merge_rx_pkgs(rx_pkgShiftTypeFifo, rx_aethSift2mergerFifo, rx_rethSift2mergerFifo, rx_exhNoShiftFifo, m_axis_mem_write_data);



	/*
	 * TX PATH
	 */

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

	local_req_handler(	s_axis_tx_meta,
#if RETRANS_EN
						retransmitter2exh_eventFifo,
#endif
						tx_localMemCmdFifo,
						tx_readReqAddr_push,
#if !RETRANS_EN
						tx_appMetaFifo);
#else
						tx_appMetaFifo,
						tx2retrans_insertAddrLen);
#endif


	//Only used when FPGA does standalon, currently disabled
#ifdef FPGA_STANDALONE
	fpga_data_handler(s_axis_tx_data, tx_appDataFifo);
#endif

	tx_pkg_arbiter(	tx_pkgInfoFifo,
					s_axis_tx_data,
					s_axis_mem_read_data,
					tx_split2aethShift,
#ifdef FPGA_STANDALONE
					tx_split2rethMerge);
#else
					tx_rethMerge2rethShift,
#endif
					tx_rawPayFifo);

#ifdef FPGA_STANDALONE
	stream_merger(tx_split2rethMerge, tx_appDataFifo, tx_rethMerge2rethShift);
#endif
	//merges and orders event going to TX path
	meta_merger(rx_ackEventFifo, rx_readEvenFifo, tx_appMetaFifo, tx_ibhconnTable_req, tx_ibhMetaFifo, tx_exhMetaFifo);

	//Shift playload by 4 bytes for AETH (data from memory)
	lshiftWordByOctet<WIDTH,12>(((AETH_SIZE%WIDTH)/8), tx_split2aethShift, tx_aethShift2payFifo);
	//Shift payload another 12 bytes for RETH (data from application)
	lshiftWordByOctet<WIDTH,13>(((RETH_SIZE%WIDTH)/8), tx_rethMerge2rethShift, tx_rethShift2payFifo);

	//Generate EXH
	generate_exh(	tx_exhMetaFifo,
#if POINTER_CHASING_EN
					s_axis_tx_pcmeta,
#endif
					msnTable2txExh_rsp,
					txExh2msnTable_req,
					tx_readReqTable_upd,
					tx_lengthFifo,
					tx_packetInfoFifo,
#if RETRANS_EN
					txSetTimer_req,
#endif
					tx_exh2payFifo);
	// Append payload to AETH or RETH
	append_payload(tx_packetInfoFifo, tx_exh2payFifo, tx_aethShift2payFifo, tx_rethShift2payFifo, tx_rawPayFifo, tx_exh2shiftFifo);

	// BTH: 12 bytes
	lshiftWordByOctet<WIDTH,11>(((BTH_SIZE%WIDTH)/8), tx_exh2shiftFifo, tx_shift2ibhFifo);
	generate_ibh(	tx_ibhMetaFifo,
					tx_dstQpFifo,
					stateTable2txIbh_rsp,
					txIbh2stateTable_upd_req,
#if RETRANS_EN
					tx2retrans_insertMeta,
#endif
					tx_ibhHeaderFifo);

	//prependt ib header
	prepend_ibh_header(tx_ibhHeaderFifo, tx_shift2ibhFifo, m_axis_tx_data);

	//Get Meta data for UDP & IP layer
	tx_ipUdpMetaMerger(tx_connTable2ibh_rsp, tx_lengthFifo, m_axis_tx_meta, tx_dstQpFifo);


	//merge read requests
	mem_cmd_merger<WIDTH>(rx_remoteMemCmd, tx_localMemCmdFifo, m_axis_mem_read_cmd, tx_pkgInfoFifo);


	// Data structures

	conn_table(	tx_ibhconnTable_req,
				s_axis_qp_conn_interface,
				tx_connTable2ibh_rsp);

	state_table(rxIbh2stateTable_upd_req,
				txIbh2stateTable_upd_req,
				qpi2stateTable_upd_req,
				stateTable2rxIbh_rsp,
				stateTable2txIbh_rsp,
				stateTable2qpi_rsp);

	msn_table(	rxExh2msnTable_upd_req,
				txExh2msnTable_req,
				if2msnTable_init,
				msnTable2rxExh_rsp,
				msnTable2txExh_rsp);

	read_req_table(	tx_readReqTable_upd,
#if !RETRANS_EN
					rx_readReqTable_upd_req);
#else
					rx_readReqTable_upd_req,
					rx_readReqTable_upd_rsp);
#endif

	multi_queue<ap_uint<64>,MAX_QPS, 2048>(	tx_readReqAddr_push,
											rx_readReqAddr_pop_req,
											rx_readReqAddr_pop_rsp);

#if RETRANS_EN
	merge_retrans_request(tx2retrans_insertMeta, tx2retrans_insertAddrLen, tx2retrans_insertRequest);

	transport_timer(rxClearTimer_req,
					txSetTimer_req,
					timer2retrans_req);

	retransmitter(	rx2retrans_release_upd,
					rx2retrans_req,
					timer2retrans_req,
					tx2retrans_insertRequest,
					retransmitter2exh_eventFifo);
#endif
}

template void ib_transport_protocol<DATA_WIDTH>(	//RX
							stream<ipUdpMeta>&	s_axis_rx_meta,
							stream<net_axis<DATA_WIDTH> >&	s_axis_rx_data,
							//stream<net_axis<DATA_WIDTH> >&	m_axis_rx_data,
							//TX
							stream<txMeta>&		s_axis_tx_meta,
							stream<net_axis<DATA_WIDTH> >&	s_axis_tx_data,
							stream<ipUdpMeta>&	m_axis_tx_meta,
							stream<net_axis<DATA_WIDTH> >&	m_axis_tx_data,
							//Memory
							stream<routedMemCmd>&		m_axis_mem_write_cmd,
							stream<routedMemCmd>&		m_axis_mem_read_cmd,
							//stream<mmStatus>&	s_axis_mem_write_status,

							stream<routed_net_axis<DATA_WIDTH> >&	m_axis_mem_write_data,
							stream<net_axis<DATA_WIDTH> >&	s_axis_mem_read_data,

							//Interface
							stream<qpContext>& s_axis_qp_interface,
							stream<ifConnReq>&	s_axis_qp_conn_interface,

							//Pointer chasing
#if POINTER_CHASING_EN
							stream<ptrChaseMeta>&	m_axis_rx_pcmeta,
							stream<ptrChaseMeta>&	s_axis_tx_pcmeta,
#endif

							ap_uint<32>&		regInvalidPsnDropCount
							);
