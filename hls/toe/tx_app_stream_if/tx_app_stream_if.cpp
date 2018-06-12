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

#include "tx_app_stream_if.hpp"

using namespace hls;

/** @ingroup tx_app_stream_if
 *  Reads the request from the application and loads the necessary metadata,
 *  the FSM decides if the packet is written to the TX buffer or discarded.
 */
void tasi_metaLoader(	stream<appTxMeta>&			appTxDataReqMetaData,
						stream<sessionState>&			stateTable2txApp_rsp,
						stream<txAppTxSarReply>&		txSar2txApp_upd_rsp,
						stream<appTxRsp>&			appTxDataRsp,
						stream<ap_uint<16> >&			txApp2stateTable_req,
						stream<txAppTxSarQuery>&		txApp2txSar_upd_req,
						stream<pkgPushMeta>&			tasi_writeToBufFifo,
						stream<event>&					txAppStream2eventEng_setEvent)
{
#pragma HLS pipeline II=1

	enum tai_states {READ_REQUEST, READ_META, RETRY_SPACE};
	static tai_states tai_state = READ_REQUEST;
	static appTxMeta tasi_writeMeta;
	static ap_uint<8> waitCounter;

	txAppTxSarReply writeSar;
	sessionState state;

	// FSM requests metadata, decides if packet goes to buffer or not
	switch(tai_state)
	{
	case READ_REQUEST:
		if (!appTxDataReqMetaData.empty())
		{
			// Read sessionID
			appTxDataReqMetaData.read(tasi_writeMeta);
			// Get session state
			txApp2stateTable_req.write(tasi_writeMeta.sessionID);
			// Get Ack pointer
			txApp2txSar_upd_req.write(txAppTxSarQuery(tasi_writeMeta.sessionID));
			tai_state = READ_META;
		}
		break;
	case READ_META:
		if (!txSar2txApp_upd_rsp.empty() && !stateTable2txApp_rsp.empty())
		{
			stateTable2txApp_rsp.read(state);
			txSar2txApp_upd_rsp.read(writeSar);
			ap_uint<16> maxWriteLength = (writeSar.ackd - writeSar.mempt) - 1;
#if (TCP_NODELAY)
			//tasi_writeSar.mempt and txSar.not_ackd are supposed to be equal (with a few cycles delay)
			ap_uint<16> usedLength = ((ap_uint<16>) writeSar.mempt - writeSar.ackd);
			ap_uint<16> usableWindow = 0;
			if (writeSar.min_window > usedLength)
			{
				usableWindow = writeSar.min_window - usedLength;
			}
#endif
			if (state != ESTABLISHED)
			{
				tasi_writeToBufFifo.write(pkgPushMeta(true));
				// Notify app about fail
				appTxDataRsp.write(appTxRsp(tasi_writeMeta.length, maxWriteLength, ERROR_NOCONNCECTION));
				tai_state = READ_REQUEST;
			}
#if !(TCP_NODELAY)
			else if(tasi_writeMeta.length > maxWriteLength)
#else
			else if(tasi_writeMeta.length > maxWriteLength || usableWindow < tasi_writeMeta.length)
#endif
			{
				//tasi_writeToBufFifo.write(pkgPushMeta(true));
				// Notify app about fail
				//appTxDataRsp.write(appTxRsp(tasi_writeMeta.length, maxWriteLength, ERROR_NOSPACE));
				waitCounter = 0;
				tai_state = RETRY_SPACE;
			}
			else //if (state == ESTABLISHED && pkgLen <= tasi_maxWriteLength)
			{
				// TODO there seems some redundancy
				tasi_writeToBufFifo.write(pkgPushMeta(tasi_writeMeta.sessionID, writeSar.mempt, tasi_writeMeta.length));
				appTxDataRsp.write(appTxRsp(tasi_writeMeta.length, maxWriteLength, NO_ERROR));
				//tasi_eventCacheFifo.write(eventMeta(tasi_writeSessionID, tasi_writeSar.mempt, pkgLen));
				txAppStream2eventEng_setEvent.write(event(TX, tasi_writeMeta.sessionID, writeSar.mempt, tasi_writeMeta.length));
				txApp2txSar_upd_req.write(txAppTxSarQuery(tasi_writeMeta.sessionID, writeSar.mempt+tasi_writeMeta.length));
				tai_state = READ_REQUEST;
			}
		}
		break;
	case RETRY_SPACE:
		waitCounter++;
		if (waitCounter == 100)
		{
			// Get session state
			txApp2stateTable_req.write(tasi_writeMeta.sessionID);
			// Get Ack pointer
			txApp2txSar_upd_req.write(txAppTxSarQuery(tasi_writeMeta.sessionID));
			tai_state = READ_META;
		}
		break;
	} //switch
}

/** @ingroup tx_app_stream_if
 *  In case the @tasi_metaLoader decides to write the packet to the memory,
 *  it writes the memory command and pushes the data to the DataMover,
 *  otherwise the packet is dropped.
 */
void tasi_pkg_pusher(	stream<axiWord>& 				tasi_pkgBuffer,
						stream<pkgPushMeta>&			tasi_writeToBufFifo,
						stream<mmCmd>&					txBufferWriteCmd,
#if !(TCP_NODELAY)
						stream<axiWord>&				txBufferWriteData)

#else
						stream<axiWord>&				txBufferWriteData,
						stream<axiWord>&				txApp2txEng_data_stream)
#endif
{
#pragma HLS pipeline II=1 enable_flush
#pragma HLS INLINE off

	static ap_uint<3> tasiPkgPushState = 0;
	static pkgPushMeta tasi_pushMeta;
	static mmCmd txAppTempCmd = mmCmd(0, 0);
	static ap_uint<16> txAppBreakTemp = 0;
	static uint8_t lengthBuffer = 0;
	static ap_uint<3> accessResidue = 0;
	static bool txAppBreakdown = false;
	static axiWord pushWord = axiWord(0, 0xFF, 0);

	static uint16_t txAppPktCounter = 0;
	static uint32_t txAppWordCounter = 0;


	switch (tasiPkgPushState) {
	case 0:
		if (!tasi_writeToBufFifo.empty() && !txBufferWriteCmd.full()) {
			tasi_writeToBufFifo.read(tasi_pushMeta);
			if (!tasi_pushMeta.drop) {
				ap_uint<32> pkgAddr;
				pkgAddr(31, 30) = 0x01;
				pkgAddr(29, 16) = tasi_pushMeta.sessionID(13, 0);
				pkgAddr(15, 0) = tasi_pushMeta.address;
				txAppTempCmd = mmCmd(pkgAddr, tasi_pushMeta.length);
				mmCmd tempCmd = txAppTempCmd;
				if ((txAppTempCmd.saddr.range(15, 0) + txAppTempCmd.bbt) > 65536) {
					txAppBreakTemp = 65536 - txAppTempCmd.saddr;
					txAppTempCmd.bbt -= txAppBreakTemp;
					tempCmd = mmCmd(txAppTempCmd.saddr, txAppBreakTemp);
					txAppBreakdown = true;
				}
				else
				{
					txAppBreakTemp = txAppTempCmd.bbt;
				}
				txBufferWriteCmd.write(tempCmd);
			}
			tasiPkgPushState = 1;
		}
		break;
	case 1:
		if (!tasi_pkgBuffer.empty()) {
			tasi_pkgBuffer.read(pushWord);
#if (TCP_NODELAY)
			txApp2txEng_data_stream.write(pushWord);
#endif
			axiWord outputWord = pushWord;
			ap_uint<4> byteCount = keepToLen(pushWord.keep);
			if (!tasi_pushMeta.drop)
			{
				if (txAppBreakTemp > 8)
				{
					txAppBreakTemp -= 8;
				}
				else
				{
					if (txAppBreakdown == true) {				/// Changes are to go in here
						if (txAppTempCmd.saddr.range(15, 0) % 8 != 0) // If the word is not perfectly aligned then there is some magic to be worked.
						{
							outputWord.keep = lenToKeep(txAppBreakTemp);
						}
						outputWord.last = 1;
						tasiPkgPushState = 2;
						accessResidue = byteCount - txAppBreakTemp;
						lengthBuffer = txAppBreakTemp;	// Buffer the number of bits consumed.
					}
					else
						tasiPkgPushState = 0;
				}
				txAppWordCounter++;
				txBufferWriteData.write(outputWord);
			} // if drop
			else {
				if (pushWord.last == 1)
				{
					tasiPkgPushState = 0;
				}
			}
		}
		break;
	case 2:
		if (!txBufferWriteCmd.full()) {
			if (txAppTempCmd.saddr.range(15, 0) % 8 == 0)
				tasiPkgPushState = 3;
			//else if (txAppTempCmd.bbt +  accessResidue > 8 || accessResidue > 0)
			else if (txAppTempCmd.bbt - accessResidue > 0)
				tasiPkgPushState = 4;
			else
				tasiPkgPushState = 5;
			txAppTempCmd.saddr.range(15, 0) = 0;
			txAppBreakTemp = txAppTempCmd.bbt;
			txBufferWriteCmd.write(mmCmd(txAppTempCmd.saddr, txAppBreakTemp));
			txAppBreakdown = false;

		}
		break;
	case 3:	// This is the non-realignment state
		if (!tasi_pkgBuffer.empty() & !txBufferWriteData.full()) {
			tasi_pkgBuffer.read(pushWord);
#if (TCP_NODELAY)
			txApp2txEng_data_stream.write(pushWord);
#endif
			if (!tasi_pushMeta.drop) {
				txAppWordCounter++;
				txBufferWriteData.write(pushWord);
			}
			if (pushWord.last == 1)
				tasiPkgPushState = 0;
		}
		break;
	case 4: // We go into this state when we need to realign things
		if (!tasi_pkgBuffer.empty() && !txBufferWriteData.full()) {
			axiWord outputWord = axiWord(0, 0xFF, 0);
			outputWord.data.range(((8-lengthBuffer)*8) - 1, 0) = pushWord.data.range(63, lengthBuffer*8);
			pushWord = tasi_pkgBuffer.read();
#if (TCP_NODELAY)
			txApp2txEng_data_stream.write(pushWord);
#endif
			outputWord.data.range(63, (8-lengthBuffer)*8) = pushWord.data.range((lengthBuffer * 8), 0 );

			if (!tasi_pushMeta.drop) {
				if (pushWord.last == 1) {
					if (txAppBreakTemp - accessResidue > lengthBuffer)	{ // In this case there's residue to be handled
						txAppBreakTemp -=8;
						tasiPkgPushState = 5;
					}
					else {
						tasiPkgPushState = 0;
						outputWord.keep = lenToKeep(txAppBreakTemp);
						outputWord.last = 1;
					}
				}
				else
					txAppBreakTemp -= 8;
				txBufferWriteData.write(outputWord);
			}
			else {
				if (pushWord.last == 1)
					tasiPkgPushState = 0;
			}
		}
		break;
	case 5:
		if (!txBufferWriteData.full()) {
			if (!tasi_pushMeta.drop) {
				axiWord outputWord = axiWord(0, lenToKeep(txAppBreakTemp), 1);
				outputWord.data.range(((8-lengthBuffer)*8) - 1, 0) = pushWord.data.range(63, lengthBuffer*8);
				txBufferWriteData.write(outputWord);
				tasiPkgPushState = 0;
			}
		}
		break;
	} //switch
}
/** @ingroup tx_app_stream_if
 *  This application interface is used to transmit data streams of established connections.
 *  The application sends the Session-ID on through @p writeMetaDataIn and the data stream
 *  on @p writeDataIn. The interface checks then the state of the connection and loads the
 *  application pointer into the memory. It then writes the data into the memory. The application
 *  is notified through @p writeReturnStatusOut if the write to the buffer succeeded. In case
 *  of success the length of the write is returned, otherwise -1;
 *  @param[in]		appTxDataReqMetaData
 *  @param[in]		appTxDataReq
 *  @param[in]		stateTable2txApp_rsp
 *  @param[in]		txSar2txApp_upd_rsp
 *  @param[out]		appTxDataRsp
 *  @param[out]		txApp2stateTable_req
 *  @param[out]		txApp2txSar_upd_req
 *  @param[out]		txBufferWriteCmd
 *  @param[out]		txBufferWriteData
 *  @param[out]		txAppStream2eventEng_setEvent
 */
void tx_app_stream_if(	stream<appTxMeta>&				appTxDataReqMetaData,
						stream<axiWord>&				appTxDataReq,
						stream<sessionState>&			stateTable2txApp_rsp,
						stream<txAppTxSarReply>&		txSar2txApp_upd_rsp, //TODO rename
						stream<appTxRsp>&			appTxDataRsp,
						stream<ap_uint<16> >&			txApp2stateTable_req,
						stream<txAppTxSarQuery>&		txApp2txSar_upd_req, //TODO rename
						stream<mmCmd>&					txBufferWriteCmd,
						stream<axiWord>&				txBufferWriteData,
#if (TCP_NODELAY)
						stream<axiWord>&				txApp2txEng_data_stream,
#endif
						stream<event>&					txAppStream2eventEng_setEvent)
{
#pragma HLS INLINE

	static stream<pkgPushMeta> tasi_writeToBufFifo("tasi_writeToBufFifo");
	#pragma HLS stream variable=tasi_writeToBufFifo depth=32
	#pragma HLS DATA_PACK variable=tasi_writeToBufFifo

	tasi_metaLoader(	appTxDataReqMetaData,
						stateTable2txApp_rsp,
						txSar2txApp_upd_rsp,
						appTxDataRsp,
						txApp2stateTable_req,
						txApp2txSar_upd_req,
						tasi_writeToBufFifo,
						txAppStream2eventEng_setEvent);

	tasi_pkg_pusher(	appTxDataReq,
						tasi_writeToBufFifo,
						txBufferWriteCmd,
#if !(TCP_NODELAY)
						txBufferWriteData);
#else
						txBufferWriteData,
						txApp2txEng_data_stream);
#endif

}
