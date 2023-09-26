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
#include "toe_internals.hpp"

#include "session_lookup_controller/session_lookup_controller.hpp"
#include "state_table/state_table.hpp"
#include "rx_sar_table/rx_sar_table.hpp"
#include "tx_sar_table/tx_sar_table.hpp"
#include "retransmit_timer/retransmit_timer.hpp"
#include "probe_timer/probe_timer.hpp"
#include "close_timer/close_timer.hpp"
#include "event_engine/event_engine.hpp"
#include "ack_delay/ack_delay.hpp"
#include "port_table/port_table.hpp"


#include "rx_engine/rx_engine.hpp"
#include "tx_engine/tx_engine.hpp"

#include "rx_app_if/rx_app_if.hpp"
#include "rx_app_stream_if/rx_app_stream_if.hpp"
#include "tx_app_interface/tx_app_interface.hpp"

/** @ingroup timer
 *
 */
void timerCloseMerger(stream<ap_uint<16> >& in1, stream<ap_uint<16> >& in2, stream<ap_uint<16> >& out)
{
#pragma HLS PIPELINE II=1

	ap_uint<16> sessionID;

	if (!in1.empty())
	{
		in1.read(sessionID);
		out.write(sessionID);
	}
	else if (!in2.empty())
	{
		in2.read(sessionID);
		out.write(sessionID);
	}
}

/** @defgroup timer Timers
 *  @ingroup tcp_module
 *  @param[in]		rxEng2timer_clearRetransmitTimer
 *  @param[in]		txEng2timer_setRetransmitTimer
 *  @param[in]		txEng2timer_setProbeTimer
 *  @param[in]		rxEng2timer_setCloseTimer
 *  @param[out]		timer2stateTable_releaseState
 *  @param[out]		timer2eventEng_setEvent
 *  @param[out]		rtTimer2rxApp_notification
 */
void timerWrapper(	stream<rxRetransmitTimerUpdate>&	rxEng2timer_clearRetransmitTimer,
					stream<txRetransmitTimerSet>&		txEng2timer_setRetransmitTimer,
					stream<ap_uint<16> >&				rxEng2timer_clearProbeTimer,
					stream<ap_uint<16> >&				txEng2timer_setProbeTimer,
					stream<ap_uint<16> >&				rxEng2timer_setCloseTimer,
					stream<ap_uint<16> >&				timer2stateTable_releaseState,
					stream<event>&						timer2eventEng_setEvent,
					stream<appNotification>&			rtTimer2rxApp_notification,
					stream<openStatus>&					rtTimer2txApp_notification)
{
	#pragma HLS INLINE
	// #pragma HLS PIPELINE II=1
	
	static stream<ap_uint<16> > closeTimer2stateTable_releaseState("closeTimer2stateTable_releaseState");
	static stream<ap_uint<16> > rtTimer2stateTable_releaseState("rtTimer2stateTable_releaseState");
	#pragma HLS stream variable=closeTimer2stateTable_releaseState		depth=2
	#pragma HLS stream variable=rtTimer2stateTable_releaseState			depth=2

	static stream<event> rtTimer2eventEng_setEvent("rtTimer2eventEng_setEvent");
	static stream<event> probeTimer2eventEng_setEvent("probeTimer2eventEng_setEvent");
	#pragma HLS stream variable=rtTimer2eventEng_setEvent		depth=2
	#pragma HLS stream variable=probeTimer2eventEng_setEvent	depth=2

	// Merge Events, Order: rtTimer has to be before probeTimer
	stream_merger(rtTimer2eventEng_setEvent, probeTimer2eventEng_setEvent, timer2eventEng_setEvent);

	retransmit_timer(	rxEng2timer_clearRetransmitTimer,
						txEng2timer_setRetransmitTimer,
						rtTimer2eventEng_setEvent,
						rtTimer2stateTable_releaseState,
						rtTimer2rxApp_notification,
						rtTimer2txApp_notification);
	probe_timer(rxEng2timer_clearProbeTimer,
				txEng2timer_setProbeTimer,
				probeTimer2eventEng_setEvent);
	close_timer(rxEng2timer_setCloseTimer,
				closeTimer2stateTable_releaseState);
	stream_merger(		closeTimer2stateTable_releaseState,
						rtTimer2stateTable_releaseState,
						timer2stateTable_releaseState);
}

//TODO use same code as in TX Engine
void rxAppMemAccessBreakdown(stream<mmCmd> &inputMemAccess, stream<mmCmd> &outputMemAccess, stream<ap_uint<1> > &rxAppDoubleAccess) {
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	static bool rxAppBreakdown = false;
	static mmCmd rxAppTempCmd;
	static ap_uint<16> rxAppAccLength = 0;

	if (rxAppBreakdown == false) {
		if (!inputMemAccess.empty())
 		{
			rxAppTempCmd = inputMemAccess.read();
			if ((rxAppTempCmd.saddr.range(15, 0) + rxAppTempCmd.bbt) > 65536) {
				rxAppAccLength = 65536 - rxAppTempCmd.saddr;
				outputMemAccess.write(mmCmd(rxAppTempCmd.saddr, rxAppAccLength));
				rxAppBreakdown = true;
			}
			else
				outputMemAccess.write(rxAppTempCmd);
			//std::cerr << "Mem.Cmd: " << std::hex << rxAppTempCmd.saddr << " - " << rxAppTempCmd.bbt << std::endl;
			rxAppDoubleAccess.write(rxAppBreakdown);
		}
	}
	else if (rxAppBreakdown == true) {
		rxAppTempCmd.saddr.range(15, 0) = 0;
		rxAppAccLength = rxAppTempCmd.bbt - rxAppAccLength;
		outputMemAccess.write(mmCmd(rxAppTempCmd.saddr, rxAppAccLength));
		//std::cerr << "Mem.Cmd: " << std::hex << rxAppTempCmd.saddr << " - " << rxAppTempCmd.bbt - (65536 - rxAppTempCmd.saddr) << std::endl;
		rxAppBreakdown = false;
	}
}

#if !(RX_DDR_BYPASS)
template <int WIDTH> 
void rxAppMemDataRead(	stream<net_axis<WIDTH> >&	rxBufferReadData,
						stream<net_axis<WIDTH> >&	rxDataRsp,
						stream<ap_uint<1> >&		rxAppDoubleAccess)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	static net_axis<WIDTH> rxAppMemRdRxWord;// = axiWord(0, 0, 0);
	static ap_uint<1> rxAppDoubleAccessFlag = 0;
	static enum rAstate {RXAPP_IDLE = 0, RXAPP_STREAM, RXAPP_JOIN, RXAPP_STREAMMERGED, RXAPP_STREAMUNMERGED, RXAPP_RESIDUE} rxAppState;
	static ap_uint<8> rxAppMemRdOffset = 0;
	static ap_uint<8> rxAppOffsetBuffer = 0;

	switch(rxAppState)
	{
	case RXAPP_IDLE:
		if (!rxAppDoubleAccess.empty() && !rxBufferReadData.empty())
		{
			//rxAppMemRdOffset = 0;
			rxAppDoubleAccessFlag = rxAppDoubleAccess.read();
			rxBufferReadData.read(rxAppMemRdRxWord);
			rxAppMemRdOffset = keepToLen(rxAppMemRdRxWord.keep);						// Count the number of valid bytes in this data word
			if (rxAppMemRdRxWord.last == 1 && rxAppDoubleAccessFlag == 1) {		// If this is the last word and this access was broken down
				rxAppMemRdRxWord.last = ~rxAppDoubleAccessFlag;					// Negate the last flag inn the axiWord and determine if there's an offset
				if (rxAppMemRdOffset == (WIDTH/8)) // No need to offset anything
				{
					// Output the word directly
					rxDataRsp.write(rxAppMemRdRxWord);
					//std::cerr << "Mem.Data: " << std::hex << rxAppMemRdRxWord.data << " - " << rxAppMemRdRxWord.keep << " - " << rxAppMemRdRxWord.last << std::endl;
					// Jump to stream merged since there's no joining to be performed.
					rxAppState = RXAPP_STREAMUNMERGED;							
				}
				else if (rxAppMemRdOffset < (WIDTH/8)) //If this data word is not full
				{
					// Don't output anything and go to RXAPP_JOIN to fetch more data to fill in the data word
					rxAppState = RXAPP_JOIN;
				}
			}
			else if (rxAppMemRdRxWord.last == 1 && rxAppDoubleAccessFlag == 0)	{ // If this is the 1st and last data word of this segment and no mem. access breakdown occured,
				rxDataRsp.write(rxAppMemRdRxWord);								// then output the data word and stay in this state to read the next segment data
				//std::cerr << "Mem.Data: " << std::hex << rxAppMemRdRxWord.data << " - " << rxAppMemRdRxWord.keep << " - " << rxAppMemRdRxWord.last << std::endl;
			}
			else {																// Finally if there are more words in this memory access,
				rxAppState = RXAPP_STREAM;										// then go to RXAPP_STREAM to read them
				rxDataRsp.write(rxAppMemRdRxWord);								// and output the current word
				//std::cerr << "Mem.Data: " << std::hex << rxAppMemRdRxWord.data << " - " << rxAppMemRdRxWord.keep << " - " << rxAppMemRdRxWord.last << std::endl;
			}

		}
		break;
	case RXAPP_STREAM:	// This state outputs the all the data words in the 1st memory access of a segment but the 1st one.
		if (!rxBufferReadData.empty())
		{
			rxBufferReadData.read(rxAppMemRdRxWord);
			rxAppMemRdOffset = keepToLen(rxAppMemRdRxWord.keep);						// Count the number of valid bytes in this data word

			if (rxAppMemRdRxWord.last == 1 && rxAppDoubleAccessFlag == 1) {		// If this is the last word and this access was broken down
				rxAppMemRdRxWord.last = ~rxAppDoubleAccessFlag;					// Negate the last flag inn the axiWord and determine if there's an offset
				if (rxAppMemRdOffset == (WIDTH/8)) {									// No need to offset anything
					rxDataRsp.write(rxAppMemRdRxWord);							// Output the word directly
					//std::cerr << "Mem.Data: " << std::hex << rxAppMemRdRxWord.data << " - " << rxAppMemRdRxWord.keep << " - " << rxAppMemRdRxWord.last << std::endl;
					rxAppState = RXAPP_STREAMUNMERGED;							// Jump to stream merged since there's no joining to be performed.
				}
				else if (rxAppMemRdOffset < (WIDTH/8)) {								// If this data word is not full
					rxAppState = RXAPP_JOIN;									// Don't output anything and go to RXAPP_JOIN to fetch more data to fill in the data word
				}
			}
			else if (rxAppMemRdRxWord.last == 1 && rxAppDoubleAccessFlag == 0) {// If this is the 1st and last data word of this segment and no mem. access breakdown occured,
				rxDataRsp.write(rxAppMemRdRxWord);								// then output the data word and stay in this state to read the next segment data
				//std::cerr << "Mem.Data: " << std::hex << rxAppMemRdRxWord.data << " - " << rxAppMemRdRxWord.keep << " - " << rxAppMemRdRxWord.last << std::endl;
				rxAppState = RXAPP_IDLE;										// and go back to the idle state
			}
			else {																// If the segment data hasn't finished yet
				rxDataRsp.write(rxAppMemRdRxWord);								// output them and stay in this state
				//std::cerr << "Mem.Data: " << std::hex << rxAppMemRdRxWord.data << " - " << rxAppMemRdRxWord.keep << " - " << rxAppMemRdRxWord.last << std::endl;
			}
		}
		break;
	case RXAPP_STREAMUNMERGED:													// This state handles 2nd mem.access data when no realignment is required
		if (!rxBufferReadData.empty())
		{
			net_axis<WIDTH> temp = rxBufferReadData.read();								// If so read the data in a tempVariable
			if (temp.last == 1)													// If this is the last data word...
				rxAppState = RXAPP_IDLE;										// Go back to the output state. Everything else is perfectly fine as is
			rxDataRsp.write(temp);												// Finally, output the data word before changing states
			std::cerr << "Mem.Data: " << std::hex << temp.data << " - " << temp.keep << " - " << temp.last << std::endl;
		}
		break;
	case RXAPP_JOIN:															// This state performs the hand over from the 1st to the 2nd mem. access for this segment if a mem. access has occured
		if (!rxBufferReadData.empty())
		{
			net_axis<WIDTH> temp;
			temp.data = 0;
			temp.keep = ~uint64_t(0);
			temp.last = 0x0;

			temp.data.range((rxAppMemRdOffset * 8) - 1, 0) = rxAppMemRdRxWord.data.range((rxAppMemRdOffset * 8) - 1, 0);	// In any case, insert the data of the new data word in the old one. Here we don't pay attention to the exact number of bytes in the new data word. In case they don't fill the entire remaining gap, there will be garbage in the output but it doesn't matter since the KEEP signal indicates which bytes are valid.
			rxAppMemRdRxWord = rxBufferReadData.read();
			temp.data.range(WIDTH-1, (rxAppMemRdOffset * 8)) = rxAppMemRdRxWord.data.range(((8 - rxAppMemRdOffset) * 8) - 1, 0);				// Buffer & realign temp into rxAppmemRdRxWord (which is a static variable)
			ap_uint<8> tempCounter = keepToLen(rxAppMemRdRxWord.keep);					// Determine how any bytes are valid in the new data word. It might be that this is the only data word of the 2nd segment
			rxAppOffsetBuffer = tempCounter - ((WIDTH/8) - rxAppMemRdOffset);				// Calculate the number of bytes to go into the next & final data word
			if (rxAppMemRdRxWord.last == 1) {
				if ((tempCounter + rxAppMemRdOffset) <= (WIDTH/8)) {						// Check if the residue from the 1st segment and the data in the 1st data word of the 2nd segment fill this data word. If not...
					temp.keep = lenToKeep(tempCounter + rxAppMemRdOffset);	// then set the KEEP value of the output to the sum of the 2 data word's bytes
					temp.last = 1;									// also set the LAST to 1, since this is going to be the final word of this segment
					rxAppState = RXAPP_IDLE;									// And go back to idle when finished with this state
				}
				else
					rxAppState = RXAPP_RESIDUE;									// then go to the RXAPP_RESIDUE to output the remaining data words
			}
			else
				rxAppState = RXAPP_STREAMMERGED;									// then go to the RXAPP_STREAMMERGED to output the remaining data words
			rxDataRsp.write(temp);												// Finally, write the data word to the output
			//std::cerr << "Mem.Data: " << std::hex << temp.data << " - " << temp.keep << " - " << temp.last << std::endl;
		}
		break;
	case RXAPP_STREAMMERGED:													// This state outputs all of the remaining, realigned data words of the 2nd mem.access, which resulted from a data word
		if (!rxBufferReadData.empty())
		{
			net_axis<WIDTH> temp;
			temp.data = 0;
			temp.keep = ~uint64_t(0);
			temp.last = 0;

			temp.data.range((rxAppMemRdOffset * 8) - 1, 0) = rxAppMemRdRxWord.data.range(WIDTH-1, ((8 - rxAppMemRdOffset) * 8));
			rxAppMemRdRxWord = rxBufferReadData.read();							// Read the new data word in
			temp.data.range(WIDTH-1, (rxAppMemRdOffset * 8)) = rxAppMemRdRxWord.data.range(((8 - rxAppMemRdOffset) * 8) - 1, 0);
			ap_uint<8> tempCounter = keepToLen(rxAppMemRdRxWord.keep);			// Determine how any bytes are valid in the new data word. It might be that this is the only data word of the 2nd segment
			rxAppOffsetBuffer = tempCounter - ((WIDTH/8) - rxAppMemRdOffset);				// Calculate the number of bytes to go into the next & final data word
			if (rxAppMemRdRxWord.last == 1) {
				if ((tempCounter + rxAppMemRdOffset) <= (WIDTH/8)) {							// Check if the residue from the 1st segment and the data in the 1st data word of the 2nd segment fill this data word. If not...
					temp.keep = lenToKeep(tempCounter + rxAppMemRdOffset);			// then set the KEEP value of the output to the sum of the 2 data word's bytes
					temp.last = 1;													// also set the LAST to 1, since this is going to be the final word of this segment
					rxAppState = RXAPP_IDLE;										// And go back to idle when finished with this state
				}
				else																// If this not the last word, because it doesn't fit in the available space in this data word
					rxAppState = RXAPP_RESIDUE;										// then go to the RXAPP_RESIDUE to output the remainder of this data word
			}
			rxDataRsp.write(temp);												// Finally, write the data word to the output
			//std::cerr << "Mem.Data: " << std::hex << temp.data << " - " << temp.keep << " - " << temp.last << std::endl;
		}
		break;
	case RXAPP_RESIDUE:
		{
			net_axis<WIDTH> temp;// = axiWord(0, lenToKeep(rxAppOffsetBuffer), 1);
			temp.data = 0;
			temp.keep = lenToKeep(rxAppOffsetBuffer);
			temp.last = 1;

			temp.data.range((rxAppMemRdOffset * 8) - 1, 0) = rxAppMemRdRxWord.data.range(WIDTH-1, ((8 - rxAppMemRdOffset) * 8));
			rxDataRsp.write(temp);												// And finally write the data word to the output
			//std::cerr << "Mem.Data: " << std::hex << temp.data << " - " << temp.keep << " - " << temp.last << std::endl;
			rxAppState = RXAPP_IDLE;											// And go back to the idle stage
		}
		break;
	}
}
#else
template <int WIDTH>
void rxAppMemDataRead(	stream<ap_uint<1> >&	rxBufferReadCmd,
						stream<net_axis<WIDTH> >&		rxBufferReadData,
						stream<net_axis<WIDTH> >&		rxDataRsp)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	static ap_uint<1> ramdr_fsmState = 0;

	switch(ramdr_fsmState)
	{
	case 0:
		if (!rxBufferReadCmd.empty())
		{
			rxBufferReadCmd.read();
			ramdr_fsmState = 1;
		}
		break;
	case 1:
		if (!rxBufferReadData.empty())
		{
			net_axis<WIDTH> currWord = rxBufferReadData.read();
			rxDataRsp.write(currWord);
			if (currWord.last)
			{
				ramdr_fsmState = 0;
			}
		}
		break;
	}

}
#endif



template <int WIDTH>
void rxAppWrapper(	stream<appReadRequest>&			appRxDataReq,
					stream<rxSarAppd>&				rxSar2rxApp_upd_rsp,
					stream<ap_uint<16> >&			appListenPortReq,
					stream<bool>&					portTable2rxApp_listen_rsp,
					stream<appNotification>&		rxEng2rxApp_notification,
					stream<appNotification>&		timer2rxApp_notification,
					stream<ap_uint<16> >&			appRxDataRspMetadata,
					stream<rxSarAppd>&				rxApp2rxSar_upd_req,
#if !(RX_DDR_BYPASS)
					stream<mmCmd>&					rxBufferReadCmd,
#endif
					stream<bool>&					appListenPortRsp,
					stream<ap_uint<16> >& 			rxApp2portTable_listen_req,
					stream<appNotification>&		appNotification,
					stream<net_axis<WIDTH> > 				&rxBufferReadData,
					stream<net_axis<WIDTH> > 				&rxDataRsp)
{
	#pragma HLS INLINE
	// #pragma HLS PIPELINE II=1

	static stream<mmCmd>			rxAppStreamIf2memAccessBreakdown("rxAppStreamIf2memAccessBreakdown");
	static stream<ap_uint<1> >		rxAppDoubleAccess("rxAppDoubleAccess");
	#pragma HLS stream variable=rxAppStreamIf2memAccessBreakdown	depth=16
	#pragma HLS stream variable=rxAppDoubleAccess					depth=16

#if (RX_DDR_BYPASS)
	static stream<ap_uint<1> >		rxBufferReadCmd("rxBufferReadCmd");
	#pragma HLS stream variable=rxBufferReadCmd					depth=4
#endif

	 // RX Application Stream Interface
#if !(RX_DDR_BYPASS)
	rx_app_stream_if(appRxDataReq, rxSar2rxApp_upd_rsp, appRxDataRspMetadata,
						rxApp2rxSar_upd_req, rxAppStreamIf2memAccessBreakdown);
	rxAppMemAccessBreakdown(rxAppStreamIf2memAccessBreakdown, rxBufferReadCmd, rxAppDoubleAccess);
	rxAppMemDataRead<WIDTH>(rxBufferReadData, rxDataRsp, rxAppDoubleAccess);
#else
	rx_app_stream_if(appRxDataReq, rxSar2rxApp_upd_rsp, appRxDataRspMetadata,
						rxApp2rxSar_upd_req, rxBufferReadCmd);
	rxAppMemDataRead(rxBufferReadCmd, rxBufferReadData, rxDataRsp);
#endif

	// RX Application Interface
	rx_app_if(	appListenPortReq, portTable2rxApp_listen_rsp,


		 	 	appListenPortRsp, rxApp2portTable_listen_req);

	stream_merger(rxEng2rxApp_notification, timer2rxApp_notification, appNotification);
}

/** @defgroup tcp_module TCP Module
 *  @defgroup app_if Application Interface
 *  @ingroup tcp_module
 *  @image top_module.png
 *  @param[in]		ipRxData
 *  @param[in]		rxBufferWriteStatus
 *  @param[in]		txBufferWriteStatus
 *  @param[in]		rxBufferReadData
 *  @param[in]		txBufferReadData
 *  @param[out]		ipTxData
 *  @param[out]		rxBufferWriteCmd
 *  @param[out]		rxBufferReadCmd
 *  @param[out]		txWriteReadCmd
 *  @param[out]		txBufferReadCmd
 *  @param[out]		rxBufferWriteData
 *  @param[out]		txBufferWriteData
 *  @param[in]		sessionLookup_rsp
 *  @param[in]		sessionUpdate_rsp
 *  @param[in]		finSessionIdIn
 *  @param[out]		sessionLookup_req
 *  @param[out]		sessionUpdate_req
 *  @param[out]		writeNewSessionId
 *  @param[in]		listenPortReq
 *  @param[in]		rxDataReq
 *  @param[in]		openConnReq
 *  @param[in]		closeConnReq
 *  @param[in]		txDataReqMeta
 *  @param[in]		txDataReq
 *  @param[out]		listenPortRsp
 *  @param[out]		notification
 *  @param[out]		rxDataRspMeta
 *  @param[out]		rxDataRsp
 *  @param[out]		openConnRsp
 *  @param[out]		txDataRsp
 */
template <int WIDTH>
void toe(	// Data & Memory Interface
			stream<net_axis<WIDTH> >&						ipRxData,
#if !(RX_DDR_BYPASS)
			stream<mmStatus>&						rxBufferWriteStatus,
#endif
			stream<mmStatus>&						txBufferWriteStatus,
			stream<net_axis<WIDTH> >&						rxBufferReadData,
			stream<net_axis<WIDTH> >&						txBufferReadData,
			stream<net_axis<WIDTH> >&						ipTxData,
#if !(RX_DDR_BYPASS)
			stream<mmCmd>&							rxBufferWriteCmd,
			stream<mmCmd>&							rxBufferReadCmd,
#endif
			stream<mmCmd>&							txBufferWriteCmd,
			stream<mmCmd>&							txBufferReadCmd,
			stream<net_axis<WIDTH> >&						rxBufferWriteData,
			stream<net_axis<WIDTH> >&						txBufferWriteData,
			// SmartCam Interface
			stream<rtlSessionLookupReply>&			sessionLookup_rsp,
			stream<rtlSessionUpdateReply>&			sessionUpdate_rsp,
			//stream<ap_uint<14> >&					readFinSessionId,
			stream<rtlSessionLookupRequest>&		sessionLookup_req,
			stream<rtlSessionUpdateRequest>&		sessionUpdate_req,
			//stream<rtlSessionUpdateRequest>&		sessionInsert_req,
			//stream<rtlSessionUpdateRequest>&		sessionDelete_req,
			//stream<ap_uint<14> >&					writeNewSessionId,
			// Application Interface
			stream<ap_uint<16> >&					listenPortReq,
			// This is disabled for the time being, due to complexity concerns
			//stream<ap_uint<16> >&					appClosePortIn,
			stream<appReadRequest>&					rxDataReq,
			stream<ipTuple>&						openConnReq,
			stream<ap_uint<16> >&					closeConnReq,
			stream<appTxMeta>&					   txDataReqMeta,
			stream<net_axis<WIDTH> >&						txDataReq,

			stream<bool>&							listenPortRsp,
			stream<appNotification>&				notification,
			stream<ap_uint<16> >&					rxDataRspMeta,
			stream<net_axis<WIDTH> >&						rxDataRsp,
			stream<openStatus>&						openConnRsp,
			stream<appTxRsp>&					txDataRsp,
#if RX_DDR_BYPASS
			// Data counts for external FIFO
			ap_uint<16>						axis_data_count,
			ap_uint<16>						axis_max_data_count,
#endif
			//IP Address Input
			ap_uint<32>								myIpAddress,
			//statistic
			ap_uint<16>&							regSessionCount)
{
	#pragma HLS INLINE

	/*
	 * FIFOs
	 */
	// Session Lookup
	static stream<sessionLookupQuery>		rxEng2sLookup_req("rxEng2sLookup_req");
	static stream<sessionLookupReply>		sLookup2rxEng_rsp("sLookup2rxEng_rsp");
	static stream<fourTuple>				txApp2sLookup_req("txApp2sLookup_req");
	static stream<sessionLookupReply>		sLookup2txApp_rsp("sLookup2txApp_rsp");
	static stream<ap_uint<16> >				txEng2sLookup_rev_req("txEng2sLookup_rev_req");
	static stream<fourTuple>				sLookup2txEng_rev_rsp("sLookup2txEng_rev_rsp");
	#pragma HLS stream variable=rxEng2sLookup_req			depth=4
	#pragma HLS stream variable=sLookup2rxEng_rsp			depth=4
	#pragma HLS stream variable=txApp2sLookup_req			depth=4
	#pragma HLS stream variable=sLookup2txApp_rsp			depth=4
	#pragma HLS stream variable=txEng2sLookup_rev_req		depth=4
	#pragma HLS stream variable=sLookup2txEng_rev_rsp		depth=4
	#pragma HLS aggregate variable=rxEng2sLookup_req compact=bit
	#pragma HLS aggregate variable=sLookup2rxEng_rsp compact=bit
	#pragma HLS aggregate variable=txApp2sLookup_req compact=bit
	#pragma HLS aggregate variable=sLookup2txApp_rsp compact=bit
	#pragma HLS aggregate variable=sLookup2txEng_rev_rsp compact=bit

	// State Table
	static stream<stateQuery>			rxEng2stateTable_upd_req("rxEng2stateTable_upd_req");
	static stream<sessionState>			stateTable2rxEng_upd_rsp("stateTable2rxEng_upd_rsp");
	static stream<stateQuery>			txApp2stateTable_upd_req("txApp2stateTable_upd_req");
	static stream<sessionState>			stateTable2txApp_upd_rsp("stateTable2txApp_upd_rsp");
	static stream<ap_uint<16> >			txApp2stateTable_req("txApp2stateTable_req");
	static stream<sessionState>			stateTable2txApp_rsp("stateTable2txApp_rsp");
	static stream<ap_uint<16> >			stateTable2sLookup_releaseSession("stateTable2sLookup_releaseSession");
	#pragma HLS stream variable=rxEng2stateTable_upd_req			depth=2
	#pragma HLS stream variable=stateTable2rxEng_upd_rsp			depth=2
	#pragma HLS stream variable=txApp2stateTable_upd_req			depth=2
	#pragma HLS stream variable=stateTable2txApp_upd_rsp			depth=2
	#pragma HLS stream variable=txApp2stateTable_req				depth=2
	#pragma HLS stream variable=stateTable2txApp_rsp				depth=2
	#pragma HLS stream variable=stateTable2sLookup_releaseSession	depth=2
	#pragma HLS aggregate variable=rxEng2stateTable_upd_req compact=bit
	#pragma HLS aggregate variable=txApp2stateTable_upd_req compact=bit
	//#pragma HLS aggregate variable=txApp2stateTable_req compact=bit

	// RX Sar Table
	static stream<rxSarRecvd>			rxEng2rxSar_upd_req("rxEng2rxSar_upd_req");
	static stream<rxSarEntry>			rxSar2rxEng_upd_rsp("rxSar2rxEng_upd_rsp");
	static stream<rxSarAppd>			rxApp2rxSar_upd_req("rxApp2rxSar_upd_req");
	static stream<rxSarAppd>			rxSar2rxApp_upd_rsp("rxSar2rxApp_upd_rsp");
	static stream<ap_uint<16> >			txEng2rxSar_req("txEng2rxSar_req");
	static stream<rxSarReply>			rxSar2txEng_rsp("rxSar2txEng_rsp");
	#pragma HLS stream variable=rxEng2rxSar_upd_req		depth=2
	#pragma HLS stream variable=rxSar2rxEng_upd_rsp		depth=2
	#pragma HLS stream variable=rxApp2rxSar_upd_req		depth=2
	#pragma HLS stream variable=rxSar2rxApp_upd_rsp		depth=2
	#pragma HLS stream variable=txEng2rxSar_req			depth=2
	#pragma HLS stream variable=rxSar2txEng_rsp			depth=2
	#pragma HLS aggregate variable=rxEng2rxSar_upd_req compact=bit
	#pragma HLS aggregate variable=rxSar2rxEng_upd_rsp compact=bit
	#pragma HLS aggregate variable=rxApp2rxSar_upd_req compact=bit
	#pragma HLS aggregate variable=rxSar2rxApp_upd_rsp compact=bit
	#pragma HLS aggregate variable=rxSar2txEng_rsp compact=bit

	// TX Sar Table
	static stream<txTxSarQuery>			txEng2txSar_upd_req("txEng2txSar_upd_req");
	static stream<txTxSarReply>			txSar2txEng_upd_rsp("txSar2txEng_upd_rsp");
	//static stream<txAppTxSarQuery>		txApp2txSar_upd_req("txApp2txSar_upd_req");
	//static stream<txAppTxSarReply>		txSar2txApp_upd_rsp("txSar2txApp_upd_rsp");
	static stream<rxTxSarQuery>			rxEng2txSar_upd_req("rxEng2txSar_upd_req");
	static stream<rxTxSarReply>			txSar2rxEng_upd_rsp("txSar2rxEng_upd_rsp");
	static stream<txSarAckPush>			txSar2txApp_ack_push("txSar2txApp_ack_push");
	static stream<txAppTxSarPush>		txApp2txSar_push("txApp2txSar_push");
	#pragma HLS stream variable=txEng2txSar_upd_req		depth=16
	#pragma HLS stream variable=txSar2txEng_upd_rsp		depth=16
	//#pragma HLS stream variable=txApp2txSar_upd_req		depth=2
	//#pragma HLS stream variable=txSar2txApp_upd_rsp		depth=2
	#pragma HLS stream variable=rxEng2txSar_upd_req		depth=16
	#pragma HLS stream variable=txSar2rxEng_upd_rsp		depth=16
	#pragma HLS stream variable=txSar2txApp_ack_push	depth=16
	#pragma HLS stream variable=txApp2txSar_push		depth=16
	#pragma HLS aggregate variable=txEng2txSar_upd_req compact=bit
	#pragma HLS aggregate variable=txSar2txEng_upd_rsp compact=bit
	//#pragma HLS aggregate variable=txApp2txSar_upd_req compact=bit
	//#pragma HLS aggregate variable=txSar2txApp_upd_rsp compact=bit
	#pragma HLS aggregate variable=rxEng2txSar_upd_req compact=bit
	#pragma HLS aggregate variable=txSar2rxEng_upd_rsp compact=bit
	#pragma HLS aggregate variable=txSar2txApp_ack_push compact=bit
	#pragma HLS aggregate variable=txApp2txSar_push compact=bit

	// Retransmit Timer
	static stream<rxRetransmitTimerUpdate>		rxEng2timer_clearRetransmitTimer("rxEng2timer_clearRetransmitTimer");
	static stream<txRetransmitTimerSet>			txEng2timer_setRetransmitTimer("txEng2timer_setRetransmitTimer");
	#pragma HLS stream variable=rxEng2timer_clearRetransmitTimer depth=2
	#pragma HLS stream variable=txEng2timer_setRetransmitTimer depth=2
	#pragma HLS aggregate variable=rxEng2timer_clearRetransmitTimer compact=bit
	#pragma HLS aggregate variable=txEng2timer_setRetransmitTimer compact=bit
	// Probe Timer
	static stream<ap_uint<16> >					rxEng2timer_clearProbeTimer("rxEng2timer_clearProbeTimer");
	static stream<ap_uint<16> >					txEng2timer_setProbeTimer("txEng2timer_setProbeTimer");
	#pragma HLS stream variable=txEng2timer_setProbeTimer depth=2
	// Close Timer
	static stream<ap_uint<16> >					rxEng2timer_setCloseTimer("rxEng2timer_setCloseTimer");
	#pragma HLS stream variable=rxEng2timer_setCloseTimer depth=2
	// Timer Session release Fifos
	static stream<ap_uint<16> >					timer2stateTable_releaseState("timer2stateTable_releaseState");
	#pragma HLS stream variable=timer2stateTable_releaseState			depth=2

	// Event Engine
	static stream<extendedEvent>			rxEng2eventEng_setEvent("rxEng2eventEng_setEvent");
	static stream<event>					txApp2eventEng_setEvent("txApp2eventEng_setEvent");
	//static stream<event>					appStreamEventFifo("appStreamEventFifo");
	//static stream<event>					retransmitEventFifo("retransmitEventFifo");
	static stream<event>					timer2eventEng_setEvent("timer2eventEng_setEvent");
	static stream<extendedEvent>			eventEng2ackDelay_event("eventEng2ackDelay_event");
	static stream<extendedEvent>			eventEng2txEng_event("eventEng2txEng_event");
	#pragma HLS stream variable=rxEng2eventEng_setEvent				depth=512
	#pragma HLS stream variable=txApp2eventEng_setEvent			depth=64
	#pragma HLS stream variable=timer2eventEng_setEvent			depth=64 //TODO maybe reduce to 2, there should be no evil cycle
	#pragma HLS stream variable=eventEng2ackDelay_event				depth=64
	#pragma HLS stream variable=eventEng2txEng_event				depth=64
	#pragma HLS aggregate variable=rxEng2eventEng_setEvent compact=bit
	#pragma HLS aggregate variable=txApp2eventEng_setEvent compact=bit
	#pragma HLS aggregate variable=timer2eventEng_setEvent compact=bit
	#pragma HLS aggregate variable=eventEng2ackDelay_event compact=bit
	#pragma HLS aggregate variable=eventEng2txEng_event compact=bit

	// Application Interface
	static stream<openStatus>				conEstablishedFifo("conEstablishedFifo");
	#pragma HLS stream variable=conEstablishedFifo depth=4
	#pragma HLS aggregate variable=conEstablishedFifo compact=bit

	static stream<appNotification> 			rxEng2rxApp_notification("rxEng2rxApp_notification");
	static stream<appNotification>			timer2rxApp_notification("timer2rxApp_notification");
	static stream<openStatus>				timer2txApp_notification("timer2txApp_notification");
	#pragma HLS stream variable=rxEng2rxApp_notification		depth=4
	#pragma HLS stream variable=timer2rxApp_notification		depth=4
	#pragma HLS stream variable=timer2txApp_notification		depth=4
	#pragma HLS aggregate variable=rxEng2rxApp_notification compact=bit
	#pragma HLS aggregate variable=timer2rxApp_notification compact=bit
	#pragma HLS aggregate variable=timer2txApp_notification compact=bit

	// Port Table
	static stream<ap_uint<16> >				rxEng2portTable_check_req("rxEng2portTable_check_req");
	static stream<bool>						portTable2rxEng_check_rsp("portTable2rxEng_check_rsp");
	static stream<ap_uint<16> >				rxApp2portTable_listen_req("rxApp2portTable_listen_req");
	static stream<bool>						portTable2rxApp_listen_rsp("portTable2rxApp_listen_rsp");
	//static stream<ap_uint<1> >				txApp2portTable_port_req("txApp2portTable_port_req");
	static stream<ap_uint<16> >				portTable2txApp_port_rsp("portTable2txApp_port_rsp");
	static stream<ap_uint<16> >				sLookup2portTable_releasePort("sLookup2portTable_releasePort");
	#pragma HLS stream variable=rxEng2portTable_check_req			depth=4
	#pragma HLS stream variable=portTable2rxEng_check_rsp			depth=4
	#pragma HLS stream variable=rxApp2portTable_listen_req			depth=4
	#pragma HLS stream variable=portTable2rxApp_listen_rsp			depth=4
	//#pragma HLS stream variable=txApp2portTable_port_req			depth=4
	#pragma HLS stream variable=portTable2txApp_port_rsp			depth=4
	#pragma HLS stream variable=sLookup2portTable_releasePort		depth=4

   static stream<net_axis<WIDTH> >                 txApp2txEng_data_stream("txApp2txEng_data_stream");
   #pragma HLS stream variable=txApp2txEng_data_stream   depth=1024
	/*
	 * Data Structures
	 */
	// Session Lookup Controller
	session_lookup_controller(	rxEng2sLookup_req,
								sLookup2rxEng_rsp,
								stateTable2sLookup_releaseSession,
								sLookup2portTable_releasePort,
								txApp2sLookup_req,
								sLookup2txApp_rsp,
								txEng2sLookup_rev_req,
								sLookup2txEng_rev_rsp,
								sessionLookup_req,
								sessionLookup_rsp,
								sessionUpdate_req,
								//sessionInsert_req,
								//sessionDelete_req,
								sessionUpdate_rsp,
								regSessionCount,
								myIpAddress);
	// State Table
	state_table(	rxEng2stateTable_upd_req,
					txApp2stateTable_upd_req,
					txApp2stateTable_req,
					timer2stateTable_releaseState,
					stateTable2rxEng_upd_rsp,
					stateTable2txApp_upd_rsp,
					stateTable2txApp_rsp,
					stateTable2sLookup_releaseSession);
	// RX Sar Table
	rx_sar_table(	rxEng2rxSar_upd_req,
					rxApp2rxSar_upd_req,
					txEng2rxSar_req,
					rxSar2rxEng_upd_rsp,
					rxSar2rxApp_upd_rsp,
					rxSar2txEng_rsp);
	// TX Sar Table
	tx_sar_table(	rxEng2txSar_upd_req,
					//txApp2txSar_upd_req,
					txEng2txSar_upd_req,
					txApp2txSar_push,
					txSar2rxEng_upd_rsp,
					//txSar2txApp_upd_rsp,
					txSar2txEng_upd_rsp,
					txSar2txApp_ack_push);
	// Port Table
	port_table(		rxEng2portTable_check_req,
					rxApp2portTable_listen_req,
					//txApp2portTable_port_req,
					sLookup2portTable_releasePort,
					portTable2rxEng_check_rsp,
					portTable2rxApp_listen_rsp,
					portTable2txApp_port_rsp);
	// Timers
	timerWrapper(	rxEng2timer_clearRetransmitTimer,
					txEng2timer_setRetransmitTimer,
					rxEng2timer_clearProbeTimer,
					txEng2timer_setProbeTimer,
					rxEng2timer_setCloseTimer,
					timer2stateTable_releaseState,
					timer2eventEng_setEvent,
					timer2rxApp_notification,
					timer2txApp_notification);

	static stream<ap_uint<1> > ackDelayFifoReadCount("ackDelayFifoReadCount");
	#pragma HLS stream variable=ackDelayFifoReadCount		depth=64
	static stream<ap_uint<1> > ackDelayFifoWriteCount("ackDelayFifoWriteCount");
	#pragma HLS stream variable=ackDelayFifoWriteCount		depth=64
	static stream<ap_uint<1> > txEngFifoReadCount("txEngFifoReadCount");
	#pragma HLS stream variable=txEngFifoReadCount		depth=64
	event_engine(txApp2eventEng_setEvent, rxEng2eventEng_setEvent, timer2eventEng_setEvent, eventEng2ackDelay_event,
					ackDelayFifoReadCount, ackDelayFifoWriteCount, txEngFifoReadCount);
	ack_delay(eventEng2ackDelay_event, eventEng2txEng_event, ackDelayFifoReadCount, ackDelayFifoWriteCount);

	/*
	 * Engines
	 */
	// RX Engine
	rx_engine<WIDTH>(	ipRxData,
				sLookup2rxEng_rsp,
				stateTable2rxEng_upd_rsp,
				portTable2rxEng_check_rsp,
				rxSar2rxEng_upd_rsp,
				txSar2rxEng_upd_rsp,
#if !(RX_DDR_BYPASS)
				rxBufferWriteStatus,
#endif
				rxBufferWriteData,
				rxEng2sLookup_req,
				rxEng2stateTable_upd_req,
				rxEng2portTable_check_req,
				rxEng2rxSar_upd_req,
				rxEng2txSar_upd_req,
				rxEng2timer_clearRetransmitTimer,
				rxEng2timer_clearProbeTimer,
				rxEng2timer_setCloseTimer,
				conEstablishedFifo, //remove this
				rxEng2eventEng_setEvent,
#if !(RX_DDR_BYPASS)
				rxBufferWriteCmd,
				rxEng2rxApp_notification
#else
				rxEng2rxApp_notification,
				axis_data_count,
				axis_max_data_count
#endif
				);
	// TX Engine
	tx_engine<WIDTH>(	eventEng2txEng_event,
				rxSar2txEng_rsp,
				txSar2txEng_upd_rsp,
				txBufferReadData,
#if (TCP_NODELAY)
            txApp2txEng_data_stream,
#endif
				sLookup2txEng_rev_rsp,
				txEng2rxSar_req,
				txEng2txSar_upd_req,
				txEng2timer_setRetransmitTimer,
				txEng2timer_setProbeTimer,
				txBufferReadCmd,
				txEng2sLookup_rev_req,
				ipTxData,
				txEngFifoReadCount);

	/*
	 * Application Interfaces
	 */
	 rxAppWrapper<WIDTH>(	rxDataReq,
			 	 	rxSar2rxApp_upd_rsp,
			 	 	listenPortReq,
			 	 	portTable2rxApp_listen_rsp,
			 	 	rxEng2rxApp_notification,
			 	 	timer2rxApp_notification,
			 	 	rxDataRspMeta,
			 	 	rxApp2rxSar_upd_req,
#if !(RX_DDR_BYPASS)
			 	 	rxBufferReadCmd,
#endif
			 	 	listenPortRsp,
			 	 	rxApp2portTable_listen_req,
			 	 	notification,
			 	 	rxBufferReadData,
					rxDataRsp);

	tx_app_interface<WIDTH>(	txDataReqMeta,
						txDataReq,
						stateTable2txApp_rsp,
						//txSar2txApp_upd_rsp,
						txSar2txApp_ack_push,
						txBufferWriteStatus,
						openConnReq,
						closeConnReq,
						sLookup2txApp_rsp,
						portTable2txApp_port_rsp,
						stateTable2txApp_upd_rsp,
						conEstablishedFifo,
						txDataRsp,
						txApp2stateTable_req,
						//txApp2txSar_upd_req,
						txBufferWriteCmd,
						txBufferWriteData,
#if (TCP_NODELAY)
                  txApp2txEng_data_stream,
#endif
						txApp2txSar_push,
						openConnRsp,
						txApp2sLookup_req,
						//txApp2portTable_port_req,
						txApp2stateTable_upd_req,
						txApp2eventEng_setEvent,
						timer2txApp_notification,
						myIpAddress);

}

void toe_top(	// Data & Memory Interface
			stream<ap_axiu<DATA_WIDTH, 0, 0, 0> >&						ipRxData,
#if !(RX_DDR_BYPASS)
			stream<mmStatus>&						rxBufferWriteStatus,
#endif
			stream<mmStatus>&						txBufferWriteStatus,
			stream<ap_axiu<DATA_WIDTH, 0, 0, 0> >&						rxBufferReadData,
			stream<ap_axiu<DATA_WIDTH, 0, 0, 0> >&						txBufferReadData,
			stream<ap_axiu<DATA_WIDTH, 0, 0, 0> >&						ipTxData,
#if !(RX_DDR_BYPASS)
			stream<mmCmd>&							rxBufferWriteCmd,
			stream<mmCmd>&							rxBufferReadCmd,
#endif
			stream<mmCmd>&							txBufferWriteCmd,
			stream<mmCmd>&							txBufferReadCmd,
			stream<ap_axiu<DATA_WIDTH, 0, 0, 0> >&						rxBufferWriteData,
			stream<ap_axiu<DATA_WIDTH, 0, 0, 0> >&						txBufferWriteData,

			// SmartCam Interface
			stream<rtlSessionLookupReply>&			sessionLookup_rsp,
			stream<rtlSessionUpdateReply>&			sessionUpdate_rsp,
			stream<rtlSessionLookupRequest>&		sessionLookup_req,
			stream<rtlSessionUpdateRequest>&		sessionUpdate_req,
			// Application Interface
			stream<ap_uint<16> >&					listenPortReq,
			// This is disabled for the time being, due to complexity concerns
			//stream<ap_uint<16> >&					appClosePortIn,
			stream<appReadRequest>&					rxDataReq,
			stream<ipTuple>&						openConnReq,
			stream<ap_uint<16> >&					closeConnReq,
			stream<appTxMeta>&					   txDataReqMeta,
			stream<ap_axiu<DATA_WIDTH, 0, 0, 0> >&						txDataReq,

			stream<bool>&							listenPortRsp,
			stream<appNotification>&				notification,
			stream<ap_uint<16> >&					rxDataRspMeta,
			stream<ap_axiu<DATA_WIDTH, 0, 0, 0> >&						rxDataRsp,
			stream<openStatus>&						openConnRsp,
			stream<appTxRsp>&					txDataRsp,
#if RX_DDR_BYPASS
			// Data counts for external FIFO
			ap_uint<16>						axis_data_count,
			ap_uint<16>						axis_max_data_count,
#endif
			//IP Address Input
			ap_uint<32>								myIpAddress,
			//statistic
			ap_uint<16>&							regSessionCount)
{
#pragma HLS DATAFLOW disable_start_propagation
#pragma HLS INTERFACE ap_ctrl_none port=return

	/*
	 * PRAGMAs
	 */
	// Data & Memory interface
   #pragma HLS INTERFACE axis register port=ipRxData name=s_axis_tcp_data
	#pragma HLS INTERFACE axis register port=ipTxData name=m_axis_tcp_data

	#pragma HLS INTERFACE axis register port=rxBufferWriteData name=m_axis_rxwrite_data
	#pragma HLS INTERFACE axis register port=rxBufferReadData name=s_axis_rxread_data

	#pragma HLS INTERFACE axis register port=txBufferWriteData name=m_axis_txwrite_data
	#pragma HLS INTERFACE axis register port=txBufferReadData name=s_axis_txread_data

#if !(RX_DDR_BYPASS)
	#pragma HLS INTERFACE axis register port=rxBufferWriteCmd name=m_axis_rxwrite_cmd
	#pragma HLS INTERFACE axis register port=rxBufferReadCmd name=m_axis_rxread_cmd
	#pragma HLS aggregate variable=rxBufferWriteCmd compact=bit
	#pragma HLS aggregate variable=rxBufferReadCmd compact=bit
#endif
	#pragma HLS INTERFACE axis register port=txBufferWriteCmd name=m_axis_txwrite_cmd
	#pragma HLS INTERFACE axis register port=txBufferReadCmd name=m_axis_txread_cmd
	#pragma HLS aggregate variable=txBufferWriteCmd compact=bit
	#pragma HLS aggregate variable=txBufferReadCmd compact=bit



#if !(RX_DDR_BYPASS)
	#pragma HLS INTERFACE axis register port=rxBufferWriteStatus name=s_axis_rxwrite_sts
	#pragma HLS aggregate variable=rxBufferWriteStatus compact=bit
#endif
	#pragma HLS INTERFACE axis register port=txBufferWriteStatus name=s_axis_txwrite_sts
	#pragma HLS aggregate variable=txBufferWriteStatus compact=bit

	// SmartCam Interface
	#pragma HLS INTERFACE axis register port=sessionLookup_req name=m_axis_session_lup_req
	#pragma HLS INTERFACE axis register port=sessionLookup_rsp name=s_axis_session_lup_rsp
	#pragma HLS INTERFACE axis register port=sessionUpdate_req name=m_axis_session_upd_req
	#pragma HLS INTERFACE axis register port=sessionUpdate_rsp name=s_axis_session_upd_rsp
	#pragma HLS aggregate variable=sessionLookup_req compact=bit
	#pragma HLS aggregate variable=sessionLookup_rsp compact=bit
	#pragma HLS aggregate variable=sessionUpdate_req compact=bit
	#pragma HLS aggregate variable=sessionUpdate_rsp compact=bit

	// Application Interface
	#pragma HLS INTERFACE axis register port=listenPortRsp name=m_axis_listen_port_rsp
	#pragma HLS INTERFACE axis register port=listenPortReq name=s_axis_listen_port_req

	#pragma HLS INTERFACE axis register port=notification name=m_axis_notification
	#pragma HLS INTERFACE axis register port=rxDataReq name=s_axis_rx_data_req

	#pragma HLS INTERFACE axis register port=rxDataRspMeta name=m_axis_rx_data_rsp_metadata
	#pragma HLS INTERFACE axis register port=rxDataRsp name=m_axis_rx_data_rsp

	#pragma HLS INTERFACE axis register port=openConnReq name=s_axis_open_conn_req
	#pragma HLS INTERFACE axis register port=openConnRsp name=m_axis_open_conn_rsp
	#pragma HLS INTERFACE axis register port=closeConnReq name=s_axis_close_conn_req

	#pragma HLS INTERFACE axis register port=txDataReqMeta name=s_axis_tx_data_req_metadata
	#pragma HLS INTERFACE axis register port=txDataReq name=s_axis_tx_data_req
	#pragma HLS INTERFACE axis register port=txDataRsp name=m_axis_tx_data_rsp
	#pragma HLS aggregate variable=notification compact=bit
	#pragma HLS aggregate variable=rxDataReq compact=bit
	#pragma HLS aggregate variable=openConnReq compact=bit
	#pragma HLS aggregate variable=openConnRsp compact=bit
	#pragma HLS aggregate variable=txDataReqMeta compact=bit
	#pragma HLS aggregate variable=txDataRsp compact=bit

#if RX_DDR_BYPASS
	#pragma HLS INTERFACE ap_none register port=axis_data_count
	#pragma HLS INTERFACE ap_none register port=axis_max_data_count
#endif

	#pragma HLS INTERFACE ap_none register port=myIpAddress
	#pragma HLS INTERFACE ap_vld port=regSessionCount


	static hls::stream<net_axis<DATA_WIDTH> > ipRxData_internal;
	#pragma HLS STREAM depth=2 variable=ipRxData_internal
	static hls::stream<net_axis<DATA_WIDTH> > rxBufferReadData_internal;
	#pragma HLS STREAM depth=2 variable=rxBufferReadData_internal
	static hls::stream<net_axis<DATA_WIDTH> > txBufferReadData_internal;
	#pragma HLS STREAM depth=2 variable=txBufferReadData_internal
	static hls::stream<net_axis<DATA_WIDTH> > ipTxData_internal;
	#pragma HLS STREAM depth=2 variable=ipTxData_internal
	static hls::stream<net_axis<DATA_WIDTH> > rxBufferWriteData_internal;
	#pragma HLS STREAM depth=2 variable=rxBufferWriteData_internal
	static hls::stream<net_axis<DATA_WIDTH> > txBufferWriteData_internal;
	#pragma HLS STREAM depth=2 variable=txBufferWriteData_internal
	static hls::stream<net_axis<DATA_WIDTH> > txDataReq_internal;
	#pragma HLS STREAM depth=2 variable=txDataReq_internal
	static hls::stream<net_axis<DATA_WIDTH> > rxDataRsp_internal;
	#pragma HLS STREAM depth=2 variable=rxDataRsp_internal

	convert_axis_to_net_axis<DATA_WIDTH>(ipRxData, 
							ipRxData_internal);

	convert_axis_to_net_axis<DATA_WIDTH>(rxBufferReadData, 
							rxBufferReadData_internal);

	convert_axis_to_net_axis<DATA_WIDTH>(txBufferReadData, 
							txBufferReadData_internal);

	convert_net_axis_to_axis<DATA_WIDTH>(ipTxData_internal, 
							ipTxData);

	convert_net_axis_to_axis<DATA_WIDTH>(rxBufferWriteData_internal, 
							rxBufferWriteData);

	convert_net_axis_to_axis<DATA_WIDTH>(txBufferWriteData_internal, 
							txBufferWriteData);

	convert_axis_to_net_axis<DATA_WIDTH>(txDataReq, 
							txDataReq_internal);

	convert_net_axis_to_axis<DATA_WIDTH>(rxDataRsp_internal, 
							rxDataRsp);
				
	

	toe<DATA_WIDTH>(ipRxData_internal,
#if !(RX_DDR_BYPASS)
					rxBufferWriteStatus,
#endif
					txBufferWriteStatus,
					rxBufferReadData_internal,
					txBufferReadData_internal,
					ipTxData_internal,
#if !(RX_DDR_BYPASS)
					rxBufferWriteCmd,
					rxBufferReadCmd,
#endif
					txBufferWriteCmd,
					txBufferReadCmd,
					rxBufferWriteData_internal,
					txBufferWriteData_internal,
					sessionLookup_rsp,
					sessionUpdate_rsp,
					sessionLookup_req,
					sessionUpdate_req,
					
					listenPortReq,
					rxDataReq,
					openConnReq,
					closeConnReq,
					txDataReqMeta,
					txDataReq_internal,
					listenPortRsp,
					notification,
					rxDataRspMeta,
					rxDataRsp_internal,
					openConnRsp,
					txDataRsp,
#if (RX_DDR_BYPASS)
					axis_data_count,
					axis_max_data_count,
#endif
					myIpAddress,
					regSessionCount);
}
