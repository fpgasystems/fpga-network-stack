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

#include "toe.hpp"

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


ap_uint<16> byteSwap16(ap_uint<16> inputVector) {
	return (inputVector.range(7,0), inputVector(15, 8));
}

ap_uint<32> byteSwap32(ap_uint<32> inputVector) {
	return (inputVector.range(7,0), inputVector(15, 8), inputVector.range(23,16), inputVector(31, 24));
}

ap_uint<4> keepToLen(ap_uint<8> keepValue)
{
  switch (keepValue)
  {
  case 0x01:
    return 0x1;
    break;
  case 0x3:
    return 0x2;
    break;
  case 0x07:
    return 0x3;
    break;
  case 0x0F:
    return 0x4;
    break;
  case 0x1F:
    return 0x5;
    break;
  case 0x3F:
    return 0x6;
    break;
  case 0x7F:
    return 0x7;
    break;
  case 0xFF:
    return 0x8;
    break;
  }
}

ap_uint<8> lenToKeep(ap_uint<4> length)
{
  switch(length)
  {
  case 1:
    return 0x01;
    break;
  case 2:
    return 0x03;
    break;
  case 3:
    return 0x07;
    break;
  case 4:
    return 0x0F;
    break;
  case 5:
    return 0x1F;
    break;
  case 6:
    return 0x3F;
    break;
  case 7:
    return 0x7F;
    break;
  case 8:
    return 0xFF;
    break;
  }
}

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

/** @ingroup tcp_module
 * Generic stream merger function
 */
template <typename T>
void stream_merger(stream<T>& in1, stream<T>& in2, stream<T>& out)
{
#pragma HLS PIPELINE II=1


	if (!in1.empty())
	{
		out.write(in1.read());
	}
	else if (!in2.empty())
	{
		out.write(in2.read());
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
	#pragma HLS PIPELINE II=1
	
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
void rxAppMemAccessBreakdown(stream<mmCmd> &inputMemAccess, stream<mmCmd> &outputMemAccess, stream<ap_uint<1> > &rxAppDoubleAccess) {
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	static bool rxAppBreakdown = false;
	static mmCmd rxAppTempCmd;
	static ap_uint<16> rxAppAccLength = 0;

	if (rxAppBreakdown == false) {
		if (!inputMemAccess.empty() && !outputMemAccess.full()) {
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
		if (!outputMemAccess.full()) {
			rxAppTempCmd.saddr.range(15, 0) = 0;
			rxAppAccLength = rxAppTempCmd.bbt - rxAppAccLength;
			outputMemAccess.write(mmCmd(rxAppTempCmd.saddr, rxAppAccLength));
			//std::cerr << "Mem.Cmd: " << std::hex << rxAppTempCmd.saddr << " - " << rxAppTempCmd.bbt - (65536 - rxAppTempCmd.saddr) << std::endl;
			rxAppBreakdown = false;
		}
	}
}

#if !(RX_DDR_BYPASS)
void rxAppMemDataRead(stream<axiWord> &rxBufferReadData, stream<axiWord> &rxDataRsp, stream<ap_uint<1> > &rxAppDoubleAccess) {
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	static axiWord rxAppMemRdRxWord = axiWord(0, 0, 0);
	static ap_uint<1> rxAppDoubleAccessFlag = 0;
	static enum rAstate {RXAPP_IDLE = 0, RXAPP_STREAM, RXAPP_JOIN, RXAPP_STREAMMERGED, RXAPP_STREAMUNMERGED, RXAPP_RESIDUE} rxAppState;
	static ap_uint<4> rxAppMemRdOffset = 0;
	static ap_uint<8> rxAppOffsetBuffer = 0;
	switch(rxAppState) {
	case RXAPP_IDLE:
		if (!rxAppDoubleAccess.empty() && !rxBufferReadData.empty() && !rxDataRsp.full()) {
			//rxAppMemRdOffset = 0;
			rxAppDoubleAccessFlag = rxAppDoubleAccess.read();
			rxBufferReadData.read(rxAppMemRdRxWord);
			rxAppMemRdOffset = keepToLen(rxAppMemRdRxWord.keep);						// Count the number of valid bytes in this data word
			if (rxAppMemRdRxWord.last == 1 && rxAppDoubleAccessFlag == 1) {		// If this is the last word and this access was broken down
				rxAppMemRdRxWord.last = ~rxAppDoubleAccessFlag;					// Negate the last flag inn the axiWord and determine if there's an offset
				if (rxAppMemRdOffset == 8) {									// No need to offset anything
					rxDataRsp.write(rxAppMemRdRxWord);							// Output the word directly
					//std::cerr << "Mem.Data: " << std::hex << rxAppMemRdRxWord.data << " - " << rxAppMemRdRxWord.keep << " - " << rxAppMemRdRxWord.last << std::endl;
					rxAppState = RXAPP_STREAMUNMERGED;							// Jump to stream merged since there's no joining to be performed.
				}
				else if (rxAppMemRdOffset < 8) {								// If this data word is not full
					rxAppState = RXAPP_JOIN;									// Don't output anything and go to RXAPP_JOIN to fetch more data to fill in the data word
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
	case RXAPP_STREAM:															// This state outputs the all the data words in the 1st memory access of a segment but the 1st one.
		if (!rxBufferReadData.empty() && !rxDataRsp.full()) {					// Verify that there's data in the input and space in the output
			rxBufferReadData.read(rxAppMemRdRxWord);							// Read the data word in
			rxAppMemRdOffset = keepToLen(rxAppMemRdRxWord.keep);						// Count the number of valid bytes in this data word
			if (rxAppMemRdRxWord.last == 1 && rxAppDoubleAccessFlag == 1) {		// If this is the last word and this access was broken down
				rxAppMemRdRxWord.last = ~rxAppDoubleAccessFlag;					// Negate the last flag inn the axiWord and determine if there's an offset
				if (rxAppMemRdOffset == 8) {									// No need to offset anything
					rxDataRsp.write(rxAppMemRdRxWord);							// Output the word directly
					//std::cerr << "Mem.Data: " << std::hex << rxAppMemRdRxWord.data << " - " << rxAppMemRdRxWord.keep << " - " << rxAppMemRdRxWord.last << std::endl;
					rxAppState = RXAPP_STREAMUNMERGED;							// Jump to stream merged since there's no joining to be performed.
				}
				else if (rxAppMemRdOffset < 8) {								// If this data word is not full
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
		if (!rxBufferReadData.empty() && !rxDataRsp.full()) {					// First determine that there's data to input and space in the output
			axiWord temp = rxBufferReadData.read();								// If so read the data in a tempVariable
			if (temp.last == 1)													// If this is the last data word...
				rxAppState = RXAPP_IDLE;										// Go back to the output state. Everything else is perfectly fine as is
			rxDataRsp.write(temp);												// Finally, output the data word before changing states
			std::cerr << "Mem.Data: " << std::hex << temp.data << " - " << temp.keep << " - " << temp.last << std::endl;
		}
		break;
	case RXAPP_JOIN:															// This state performs the hand over from the 1st to the 2nd mem. access for this segment if a mem. access has occured
		if (!rxBufferReadData.empty() && !rxDataRsp.full()) {					// First determine that there's data to input and space in the output
			axiWord temp = axiWord(0, 0xFF, 0);
			temp.data.range((rxAppMemRdOffset * 8) - 1, 0) = rxAppMemRdRxWord.data.range((rxAppMemRdOffset * 8) - 1, 0);	// In any case, insert the data of the new data word in the old one. Here we don't pay attention to the exact number of bytes in the new data word. In case they don't fill the entire remaining gap, there will be garbage in the output but it doesn't matter since the KEEP signal indicates which bytes are valid.
			rxAppMemRdRxWord = rxBufferReadData.read();
			temp.data.range(63, (rxAppMemRdOffset * 8)) = rxAppMemRdRxWord.data.range(((8 - rxAppMemRdOffset) * 8) - 1, 0);				// Buffer & realign temp into rxAppmemRdRxWord (which is a static variable)
			ap_uint<4> tempCounter = keepToLen(rxAppMemRdRxWord.keep);					// Determine how any bytes are valid in the new data word. It might be that this is the only data word of the 2nd segment
			rxAppOffsetBuffer = tempCounter - (8 - rxAppMemRdOffset);				// Calculate the number of bytes to go into the next & final data word
			if (rxAppMemRdRxWord.last == 1) {
				if ((tempCounter + rxAppMemRdOffset) <= 8) {						// Check if the residue from the 1st segment and the data in the 1st data word of the 2nd segment fill this data word. If not...
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
		if (!rxBufferReadData.empty() && !rxDataRsp.full()) {					// Verify that there's data at the input and that the output is ready to receive data
			axiWord temp = axiWord(0, 0xFF, 0);
			temp.data.range((rxAppMemRdOffset * 8) - 1, 0) = rxAppMemRdRxWord.data.range(63, ((8 - rxAppMemRdOffset) * 8));
			rxAppMemRdRxWord = rxBufferReadData.read();							// Read the new data word in
			temp.data.range(63, (rxAppMemRdOffset * 8)) = rxAppMemRdRxWord.data.range(((8 - rxAppMemRdOffset) * 8) - 1, 0);
			ap_uint<4> tempCounter = keepToLen(rxAppMemRdRxWord.keep);			// Determine how any bytes are valid in the new data word. It might be that this is the only data word of the 2nd segment
			rxAppOffsetBuffer = tempCounter - (8 - rxAppMemRdOffset);				// Calculate the number of bytes to go into the next & final data word
			if (rxAppMemRdRxWord.last == 1) {
				if ((tempCounter + rxAppMemRdOffset) <= 8) {							// Check if the residue from the 1st segment and the data in the 1st data word of the 2nd segment fill this data word. If not...
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
		if (!rxDataRsp.full()) {
			axiWord temp = axiWord(0, lenToKeep(rxAppOffsetBuffer), 1);
			temp.data.range((rxAppMemRdOffset * 8) - 1, 0) = rxAppMemRdRxWord.data.range(63, ((8 - rxAppMemRdOffset) * 8));
			rxDataRsp.write(temp);												// And finally write the data word to the output
			//std::cerr << "Mem.Data: " << std::hex << temp.data << " - " << temp.keep << " - " << temp.last << std::endl;
			rxAppState = RXAPP_IDLE;											// And go back to the idle stage
		}
		break;
	}
}
#else
void rxAppMemDataRead(	stream<ap_uint<1> >&	rxBufferReadCmd,
						stream<axiWord>&		rxBufferReadData,
						stream<axiWord>&		rxDataRsp)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	static ap_uint<1> ramdr_fsmState = 0;
	axiWord currWord;

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
		if (!rxBufferReadData.empty() && !rxDataRsp.full())
		{
			rxBufferReadData.read(currWord);
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
					stream<axiWord> 				&rxBufferReadData,
					stream<axiWord> 				&rxDataRsp)
{
	#pragma HLS INLINE
	#pragma HLS PIPELINE II=1

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
	rxAppMemDataRead(rxBufferReadData, rxDataRsp, rxAppDoubleAccess);
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
void toe(	// Data & Memory Interface
			stream<axiWord>&						ipRxData,
#if !(RX_DDR_BYPASS)
			stream<mmStatus>&						rxBufferWriteStatus,
#endif
			stream<mmStatus>&						txBufferWriteStatus,
			stream<axiWord>&						rxBufferReadData,
			stream<axiWord>&						txBufferReadData,
			stream<axiWord>&						ipTxData,
#if !(RX_DDR_BYPASS)
			stream<mmCmd>&							rxBufferWriteCmd,
			stream<mmCmd>&							rxBufferReadCmd,
#endif
			stream<mmCmd>&							txBufferWriteCmd,
			stream<mmCmd>&							txBufferReadCmd,
			stream<axiWord>&						rxBufferWriteData,
			stream<axiWord>&						txBufferWriteData,
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
			stream<axiWord>&						txDataReq,

			stream<bool>&							listenPortRsp,
			stream<appNotification>&				notification,
			stream<ap_uint<16> >&					rxDataRspMeta,
			stream<axiWord>&						rxDataRsp,
			stream<openStatus>&						openConnRsp,
			stream<appTxRsp>&					txDataRsp,
#if RX_DDR_BYPASS
			// Data counts for external FIFO
			ap_uint<32>						axis_data_count,
			ap_uint<32>						axis_max_data_count,
#endif
			//IP Address Input
			ap_uint<32>								myIpAddress,
			//statistic
			ap_uint<16>&							regSessionCount)
{
#pragma HLS DATAFLOW
#pragma HLS INTERFACE ap_ctrl_none port=return
//#pragma HLS PIPELINE II=1
//#pragma HLS INLINE off

	/*
	 * PRAGMAs
	 */
	// Data & Memory interface
	#pragma HLS resource core=AXI4Stream variable=ipRxData metadata="-bus_bundle s_axis_tcp_data"
	#pragma HLS resource core=AXI4Stream variable=ipTxData metadata="-bus_bundle m_axis_tcp_data"

	#pragma HLS resource core=AXI4Stream variable=rxBufferWriteData metadata="-bus_bundle m_axis_rxwrite_data"
	#pragma HLS resource core=AXI4Stream variable=rxBufferReadData metadata="-bus_bundle s_axis_rxread_data"

	#pragma HLS resource core=AXI4Stream variable=txBufferWriteData metadata="-bus_bundle m_axis_txwrite_data"
	#pragma HLS resource core=AXI4Stream variable=txBufferReadData metadata="-bus_bundle s_axis_txread_data"

#if !(RX_DDR_BYPASS)
	#pragma HLS resource core=AXI4Stream variable=rxBufferWriteCmd metadata="-bus_bundle m_axis_rxwrite_cmd"
	#pragma HLS resource core=AXI4Stream variable=rxBufferReadCmd metadata="-bus_bundle m_axis_rxread_cmd"
	#pragma HLS DATA_PACK variable=rxBufferWriteCmd
	#pragma HLS DATA_PACK variable=rxBufferReadCmd
#endif
	#pragma HLS resource core=AXI4Stream variable=txBufferWriteCmd metadata="-bus_bundle m_axis_txwrite_cmd"
	#pragma HLS resource core=AXI4Stream variable=txBufferReadCmd metadata="-bus_bundle m_axis_txread_cmd"
	#pragma HLS DATA_PACK variable=txBufferWriteCmd
	#pragma HLS DATA_PACK variable=txBufferReadCmd



#if !(RX_DDR_BYPASS)
	#pragma HLS resource core=AXI4Stream variable=rxBufferWriteStatus metadata="-bus_bundle s_axis_rxwrite_sts"
	#pragma HLS DATA_PACK variable=rxBufferWriteStatus
#endif
	#pragma HLS resource core=AXI4Stream variable=txBufferWriteStatus metadata="-bus_bundle s_axis_txwrite_sts"
	#pragma HLS DATA_PACK variable=txBufferWriteStatus

	// SmartCam Interface
	#pragma HLS resource core=AXI4Stream variable=sessionLookup_req metadata="-bus_bundle m_axis_session_lup_req"
	#pragma HLS resource core=AXI4Stream variable=sessionLookup_rsp metadata="-bus_bundle s_axis_session_lup_rsp"
	#pragma HLS resource core=AXI4Stream variable=sessionUpdate_req metadata="-bus_bundle m_axis_session_upd_req"
	//#pragma HLS resource core=AXI4Stream variable=sessionInsert_req metadata="-bus_bundle m_axis_session_ins_req"
	//#pragma HLS resource core=AXI4Stream variable=sessionDelete_req metadata="-bus_bundle m_axis_session_del_req"
	#pragma HLS resource core=AXI4Stream variable=sessionUpdate_rsp metadata="-bus_bundle s_axis_session_upd_rsp"
	#pragma HLS DATA_PACK variable=sessionLookup_req
	#pragma HLS DATA_PACK variable=sessionLookup_rsp
	#pragma HLS DATA_PACK variable=sessionUpdate_req
	//#pragma HLS DATA_PACK variable=sessionInsert_req
	//#pragma HLS DATA_PACK variable=sessionDelete_req
	#pragma HLS DATA_PACK variable=sessionUpdate_rsp

	// Application Interface
	#pragma HLS resource core=AXI4Stream variable=listenPortRsp metadata="-bus_bundle m_axis_listen_port_rsp"
	#pragma HLS resource core=AXI4Stream variable=listenPortReq metadata="-bus_bundle s_axis_listen_port_req"
	//#pragma HLS resource core=AXI4Stream variable=appClosePortIn metadata="-bus_bundle s_axis_close_port"

	#pragma HLS resource core=AXI4Stream variable=notification metadata="-bus_bundle m_axis_notification"
	#pragma HLS resource core=AXI4Stream variable=rxDataReq metadata="-bus_bundle s_axis_rx_data_req"

	#pragma HLS resource core=AXI4Stream variable=rxDataRspMeta metadata="-bus_bundle m_axis_rx_data_rsp_metadata"
	#pragma HLS resource core=AXI4Stream variable=rxDataRsp metadata="-bus_bundle m_axis_rx_data_rsp"

	#pragma HLS resource core=AXI4Stream variable=openConnReq metadata="-bus_bundle s_axis_open_conn_req"
	#pragma HLS resource core=AXI4Stream variable=openConnRsp metadata="-bus_bundle m_axis_open_conn_rsp"
	#pragma HLS resource core=AXI4Stream variable=closeConnReq metadata="-bus_bundle s_axis_close_conn_req"

	#pragma HLS resource core=AXI4Stream variable=txDataReqMeta metadata="-bus_bundle s_axis_tx_data_req_metadata"
	#pragma HLS resource core=AXI4Stream variable=txDataReq metadata="-bus_bundle s_axis_tx_data_req"
	#pragma HLS resource core=AXI4Stream variable=txDataRsp metadata="-bus_bundle m_axis_tx_data_rsp"
	#pragma HLS DATA_PACK variable=notification
	#pragma HLS DATA_PACK variable=rxDataReq
	#pragma HLS DATA_PACK variable=openConnReq
	#pragma HLS DATA_PACK variable=openConnRsp
	#pragma HLS DATA_PACK variable=txDataReqMeta
	#pragma HLS DATA_PACK variable=txDataRsp

#if RX_DDR_BYPASS
	#pragma HLS INTERFACE ap_stable register port=axis_data_count
	#pragma HLS INTERFACE ap_stable register port=axis_max_data_count
#endif

	#pragma HLS INTERFACE ap_stable register port=myIpAddress
	#pragma HLS INTERFACE ap_vld port=regSessionCount

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
	#pragma HLS DATA_PACK variable=rxEng2sLookup_req
	#pragma HLS DATA_PACK variable=sLookup2rxEng_rsp
	#pragma HLS DATA_PACK variable=txApp2sLookup_req
	#pragma HLS DATA_PACK variable=sLookup2txApp_rsp
	#pragma HLS DATA_PACK variable=sLookup2txEng_rev_rsp

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
	#pragma HLS DATA_PACK variable=rxEng2stateTable_upd_req
	#pragma HLS DATA_PACK variable=txApp2stateTable_upd_req
	//#pragma HLS DATA_PACK variable=txApp2stateTable_req

	// RX Sar Table
	static stream<rxSarRecvd>			rxEng2rxSar_upd_req("rxEng2rxSar_upd_req");
	static stream<rxSarEntry>			rxSar2rxEng_upd_rsp("rxSar2rxEng_upd_rsp");
	static stream<rxSarAppd>			rxApp2rxSar_upd_req("rxApp2rxSar_upd_req");
	static stream<rxSarAppd>			rxSar2rxApp_upd_rsp("rxSar2rxApp_upd_rsp");
	static stream<ap_uint<16> >			txEng2rxSar_req("txEng2rxSar_req");
	static stream<rxSarEntry>			rxSar2txEng_rsp("rxSar2txEng_rsp");
	#pragma HLS stream variable=rxEng2rxSar_upd_req		depth=2
	#pragma HLS stream variable=rxSar2rxEng_upd_rsp		depth=2
	#pragma HLS stream variable=rxApp2rxSar_upd_req		depth=2
	#pragma HLS stream variable=rxSar2rxApp_upd_rsp		depth=2
	#pragma HLS stream variable=txEng2rxSar_req			depth=2
	#pragma HLS stream variable=rxSar2txEng_rsp			depth=2
	#pragma HLS DATA_PACK variable=rxEng2rxSar_upd_req
	#pragma HLS DATA_PACK variable=rxSar2rxEng_upd_rsp
	#pragma HLS DATA_PACK variable=rxApp2rxSar_upd_req
	#pragma HLS DATA_PACK variable=rxSar2rxApp_upd_rsp
	#pragma HLS DATA_PACK variable=rxSar2txEng_rsp

	// TX Sar Table
	static stream<txTxSarQuery>			txEng2txSar_upd_req("txEng2txSar_upd_req");
	static stream<txTxSarReply>			txSar2txEng_upd_rsp("txSar2txEng_upd_rsp");
	//static stream<txAppTxSarQuery>		txApp2txSar_upd_req("txApp2txSar_upd_req");
	//static stream<txAppTxSarReply>		txSar2txApp_upd_rsp("txSar2txApp_upd_rsp");
	static stream<rxTxSarQuery>			rxEng2txSar_upd_req("rxEng2txSar_upd_req");
	static stream<rxTxSarReply>			txSar2rxEng_upd_rsp("txSar2rxEng_upd_rsp");
	static stream<txSarAckPush>			txSar2txApp_ack_push("txSar2txApp_ack_push");
	static stream<txAppTxSarPush>		txApp2txSar_push("txApp2txSar_push");
	#pragma HLS stream variable=txEng2txSar_upd_req		depth=2
	#pragma HLS stream variable=txSar2txEng_upd_rsp		depth=2
	//#pragma HLS stream variable=txApp2txSar_upd_req		depth=2
	//#pragma HLS stream variable=txSar2txApp_upd_rsp		depth=2
	#pragma HLS stream variable=rxEng2txSar_upd_req		depth=2
	#pragma HLS stream variable=txSar2rxEng_upd_rsp		depth=2
	#pragma HLS stream variable=txSar2txApp_ack_push	depth=2
	#pragma HLS stream variable=txApp2txSar_push		depth=2
	#pragma HLS DATA_PACK variable=txEng2txSar_upd_req
	#pragma HLS DATA_PACK variable=txSar2txEng_upd_rsp
	//#pragma HLS DATA_PACK variable=txApp2txSar_upd_req
	//#pragma HLS DATA_PACK variable=txSar2txApp_upd_rsp
	#pragma HLS DATA_PACK variable=rxEng2txSar_upd_req
	#pragma HLS DATA_PACK variable=txSar2rxEng_upd_rsp
	#pragma HLS DATA_PACK variable=txSar2txApp_ack_push
	#pragma HLS DATA_PACK variable=txApp2txSar_push

	// Retransmit Timer
	static stream<rxRetransmitTimerUpdate>		rxEng2timer_clearRetransmitTimer("rxEng2timer_clearRetransmitTimer");
	static stream<txRetransmitTimerSet>			txEng2timer_setRetransmitTimer("txEng2timer_setRetransmitTimer");
	#pragma HLS stream variable=rxEng2timer_clearRetransmitTimer depth=2
	#pragma HLS stream variable=txEng2timer_setRetransmitTimer depth=2
	#pragma HLS DATA_PACK variable=rxEng2timer_clearRetransmitTimer
	#pragma HLS DATA_PACK variable=txEng2timer_setRetransmitTimer
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
	#pragma HLS stream variable=txApp2eventEng_setEvent			depth=4
	#pragma HLS stream variable=timer2eventEng_setEvent			depth=4 //TODO maybe reduce to 2, there should be no evil cycle
	#pragma HLS stream variable=eventEng2ackDelay_event				depth=4
	#pragma HLS stream variable=eventEng2txEng_event				depth=16
	#pragma HLS DATA_PACK variable=rxEng2eventEng_setEvent
	#pragma HLS DATA_PACK variable=txApp2eventEng_setEvent
	#pragma HLS DATA_PACK variable=timer2eventEng_setEvent
	#pragma HLS DATA_PACK variable=eventEng2ackDelay_event
	#pragma HLS DATA_PACK variable=eventEng2txEng_event

	// Application Interface
	static stream<openStatus>				conEstablishedFifo("conEstablishedFifo");
	#pragma HLS stream variable=conEstablishedFifo depth=4
	#pragma HLS DATA_PACK variable=conEstablishedFifo

	static stream<appNotification> 			rxEng2rxApp_notification("rxEng2rxApp_notification");
	static stream<appNotification>			timer2rxApp_notification("timer2rxApp_notification");
	static stream<openStatus>				timer2txApp_notification("timer2txApp_notification");
	#pragma HLS stream variable=rxEng2rxApp_notification		depth=4
	#pragma HLS stream variable=timer2rxApp_notification		depth=4
	#pragma HLS stream variable=timer2txApp_notification		depth=4
	#pragma HLS DATA_PACK variable=rxEng2rxApp_notification
	#pragma HLS DATA_PACK variable=timer2rxApp_notification
	#pragma HLS DATA_PACK variable=timer2txApp_notification

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

   static stream<axiWord>                 txApp2txEng_data_stream("txApp2txEng_data_stream");
   #pragma HLS stream variable=txApp2txEng_data_stream   depth=2048
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
								regSessionCount);
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
	#pragma HLS stream variable=ackDelayFifoReadCount		depth=2
	static stream<ap_uint<1> > ackDelayFifoWriteCount("ackDelayFifoWriteCount");
	#pragma HLS stream variable=ackDelayFifoWriteCount		depth=2
	static stream<ap_uint<1> > txEngFifoReadCount("txEngFifoReadCount");
	#pragma HLS stream variable=txEngFifoReadCount		depth=2
	event_engine(txApp2eventEng_setEvent, rxEng2eventEng_setEvent, timer2eventEng_setEvent, eventEng2ackDelay_event,
					ackDelayFifoReadCount, ackDelayFifoWriteCount, txEngFifoReadCount);
	ack_delay(eventEng2ackDelay_event, eventEng2txEng_event, ackDelayFifoReadCount, ackDelayFifoWriteCount);

	/*
	 * Engines
	 */
	// RX Engine
	rx_engine(	ipRxData,
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
	tx_engine(	eventEng2txEng_event,
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
	 rxAppWrapper(	rxDataReq,
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

	tx_app_interface(	txDataReqMeta,
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

