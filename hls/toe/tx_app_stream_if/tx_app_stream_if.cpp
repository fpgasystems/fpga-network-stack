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
						stream<mmCmd>&					tasi_meta2pkgPushCmd,
						stream<event>&					txAppStream2eventEng_setEvent)
{
#pragma HLS pipeline II=1

	enum tai_states {READ_REQUEST, READ_META};
	static tai_states tai_state = READ_REQUEST;
	static appTxMeta tasi_writeMeta;

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
			ap_uint<WINDOW_BITS> maxWriteLength = (writeSar.ackd - writeSar.mempt) - 1;
#if (TCP_NODELAY)
			//tasi_writeSar.mempt and txSar.not_ackd are supposed to be equal (with a few cycles delay)
			ap_uint<WINDOW_BITS> usedLength = ((ap_uint<WINDOW_BITS>) writeSar.mempt - writeSar.ackd);
			ap_uint<WINDOW_BITS> usableWindow = 0;
			if (writeSar.min_window > usedLength)
			{
				usableWindow = writeSar.min_window - usedLength;
			}
#endif
			if (state != ESTABLISHED)
			{
				// Notify app about fail
				appTxDataRsp.write(appTxRsp(tasi_writeMeta.sessionID, tasi_writeMeta.length, maxWriteLength, ERROR_NOCONNCECTION));
				tai_state = READ_REQUEST;
			}
#if !(TCP_NODELAY)
			else if(tasi_writeMeta.length > maxWriteLength)
			{
				appTxDataRsp.write(appTxRsp(tasi_writeMeta.sessionID, tasi_writeMeta.length, maxWriteLength, ERROR_NOSPACE));
				tai_state = READ_REQUEST;

			}
#else
			else if(tasi_writeMeta.length > maxWriteLength || usableWindow < tasi_writeMeta.length)
			//else if (usableWindow < tasi_writeMeta.length)
			{
				// Notify app about fail
				appTxDataRsp.write(appTxRsp(tasi_writeMeta.sessionID, tasi_writeMeta.length, usableWindow, ERROR_NOSPACE));
				tai_state = READ_REQUEST;
			}
#endif
			else //if (state == ESTABLISHED && pkgLen <= tasi_maxWriteLength)
			{
				// TODO there seems some redundancy
				ap_uint<32> pkgAddr;
				pkgAddr(31, 30) = 0x00;
				pkgAddr(29, WINDOW_BITS) = tasi_writeMeta.sessionID(13, 0);
				pkgAddr(WINDOW_BITS-1, 0) = writeSar.mempt;

				tasi_meta2pkgPushCmd.write(mmCmd(pkgAddr, tasi_writeMeta.length));
				appTxDataRsp.write(appTxRsp(tasi_writeMeta.sessionID, tasi_writeMeta.length, maxWriteLength, NO_ERROR));
				txAppStream2eventEng_setEvent.write(event(TX, tasi_writeMeta.sessionID, writeSar.mempt, tasi_writeMeta.length));
				txApp2txSar_upd_req.write(txAppTxSarQuery(tasi_writeMeta.sessionID, writeSar.mempt+tasi_writeMeta.length));
				tai_state = READ_REQUEST;
			}
			// tai_state = READ_REQUEST;
		}
		break;
	} //switch
}

/** @ingroup tx_app_stream_if
 *  In case the @tasi_metaLoader decides to write the packet to the memory,
 *  it writes the memory command and pushes the data to the DataMover,
 *  otherwise the packet is dropped.
 */
template <int WIDTH>
void tasi_pkg_pusher(hls::stream<mmCmd>&					tasi_meta2pkgPushCmd,
							hls::stream<net_axis<WIDTH> >& 	appTxDataIn,
							hls::stream<mmCmd>&					txBufferWriteCmd,
							hls::stream<net_axis<WIDTH> >&	txBufferWriteData)

{
	#pragma HLS PIPELINE II=1
	#pragma HLS INLINE off

	enum fsmStateType {IDLE, IMPROVE_TIMING, CUT_FIRST, ALIGN_SECOND, FWD_ALIGNED, RESIDUE};
	static fsmStateType tasiPkgPushState = IDLE;
	static mmCmd cmd;
	static ap_uint<WINDOW_BITS> remainingLength = 0;
	static ap_uint<WINDOW_BITS> lengthFirstPkg;
	static ap_uint<8> offset = 0;
	static net_axis<WIDTH> prevWord;

	switch (tasiPkgPushState)
	{
	case IDLE:
		if (!tasi_meta2pkgPushCmd.empty())
		{
			tasi_meta2pkgPushCmd.read(cmd);
			tasiPkgPushState = IMPROVE_TIMING;
		}
		break;
	case IMPROVE_TIMING:
		if ((cmd.saddr(WINDOW_BITS-1, 0) + cmd.bbt) > BUFFER_SIZE)
		{
			lengthFirstPkg = BUFFER_SIZE - cmd.saddr;
			remainingLength = lengthFirstPkg;
			offset = lengthFirstPkg(DATA_KEEP_BITS - 1, 0);

			txBufferWriteCmd.write(mmCmd(cmd.saddr, lengthFirstPkg));
			tasiPkgPushState = CUT_FIRST;

		}
		else
		{
			txBufferWriteCmd.write(cmd);
			tasiPkgPushState = FWD_ALIGNED;
		}
	break;
	case CUT_FIRST:
	if (!appTxDataIn.empty())
	{
		appTxDataIn.read(prevWord);
		net_axis<WIDTH> sendWord = prevWord;

		if (remainingLength > (WIDTH/8))
		{
			remainingLength -= (WIDTH/8);
		}
		//This means that the second packet is aligned
		else if (remainingLength == (WIDTH/8))
		{
			sendWord.last = 1;

			cmd.saddr(WINDOW_BITS-1, 0) = 0;
			cmd.bbt -= lengthFirstPkg;
			txBufferWriteCmd.write(cmd);
			tasiPkgPushState = FWD_ALIGNED;
		}
		else
		{
			sendWord.keep = lenToKeep(remainingLength);
			sendWord.last = 1;

			cmd.saddr(WINDOW_BITS-1, 0) = 0;
			cmd.bbt -= lengthFirstPkg;
			txBufferWriteCmd.write(cmd);
			tasiPkgPushState = ALIGN_SECOND;
			//If only part of a word is left
			if (prevWord.last)
			{
				tasiPkgPushState = RESIDUE;
			}
		}

		txBufferWriteData.write(sendWord);
	}
	break;
	case FWD_ALIGNED:	// This is the non-realignment state
		if (!appTxDataIn.empty())
		{
			net_axis<WIDTH> currWord = appTxDataIn.read();
         std::cout << "HELP: ";
        //  printLE(std::cout, currWord);
         std::cout << std::endl;
			txBufferWriteData.write(currWord);
			if (currWord.last)
			{
				tasiPkgPushState = IDLE;
			}
		}
		break;
	case ALIGN_SECOND: // We go into this state when we need to realign things
		if (!appTxDataIn.empty())
		{
			net_axis<WIDTH> currWord = appTxDataIn.read();
			net_axis<WIDTH> sendWord;
			sendWord = alignWords<WIDTH>(offset, prevWord, currWord);
			sendWord.last = (currWord.keep[offset] == 0);

			txBufferWriteData.write(sendWord);
			prevWord = currWord;
			if (currWord.last)
			{
				tasiPkgPushState = IDLE;
				if (!sendWord.last)
				{
					tasiPkgPushState = RESIDUE;
				}
			}

		}
		break;
	case RESIDUE: //last word
		net_axis<WIDTH> sendWord;
#ifndef __SYNTHESIS__
		sendWord.data(WIDTH-1, WIDTH - (offset*8)) = 0;
#endif
		net_axis<WIDTH> emptyWord;
		sendWord = alignWords<WIDTH>(offset, prevWord, emptyWord);
		sendWord.last = 1;
		txBufferWriteData.write(sendWord);
		tasiPkgPushState = IDLE;
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
template <int WIDTH>
void tx_app_stream_if(	stream<appTxMeta>&				appTxDataReqMetaData,
						stream<net_axis<WIDTH> >&				appTxDataReq,
						stream<sessionState>&			stateTable2txApp_rsp,
						stream<txAppTxSarReply>&		txSar2txApp_upd_rsp, //TODO rename
						stream<appTxRsp>&			appTxDataRsp,
						stream<ap_uint<16> >&			txApp2stateTable_req,
						stream<txAppTxSarQuery>&		txApp2txSar_upd_req, //TODO rename
						stream<mmCmd>&					txBufferWriteCmd,
						stream<net_axis<WIDTH> >&				txBufferWriteData,
#if (TCP_NODELAY)
						stream<net_axis<WIDTH> >&				txApp2txEng_data_stream,
#endif
						stream<event>&					txAppStream2eventEng_setEvent)
{
#pragma HLS INLINE

	static stream<mmCmd> tasi_meta2pkgPushCmd("tasi_meta2pkgPushCmd");
	#pragma HLS stream variable=tasi_meta2pkgPushCmd depth=128
	#pragma HLS aggregate  variable=tasi_meta2pkgPushCmd compact=bit

	tasi_metaLoader(	appTxDataReqMetaData,
						stateTable2txApp_rsp,
						txSar2txApp_upd_rsp,
						appTxDataRsp,
						txApp2stateTable_req,
						txApp2txSar_upd_req,
						tasi_meta2pkgPushCmd,
						txAppStream2eventEng_setEvent);

#if (TCP_NODELAY)
	static hls::stream<net_axis<WIDTH> > tasi_dataFifo("tasi_dataFifo");
	#pragma HLS stream variable=tasi_dataFifo depth=1024
	#pragma HLS aggregate  variable=tasi_dataFifo compact=bit

	toe_duplicate_stream(appTxDataReq, tasi_dataFifo, txApp2txEng_data_stream);
#endif

	tasi_pkg_pusher<WIDTH>(tasi_meta2pkgPushCmd,
#if (TCP_NODELAY)
									tasi_dataFifo,
#else
									appTxDataReq,
#endif
						txBufferWriteCmd,
						txBufferWriteData);

}

template void tx_app_stream_if<DATA_WIDTH>(	stream<appTxMeta>&				appTxDataReqMetaData,
						stream<net_axis<DATA_WIDTH> >&				appTxDataReq,
						stream<sessionState>&			stateTable2txApp_rsp,
						stream<txAppTxSarReply>&		txSar2txApp_upd_rsp, //TODO rename
						stream<appTxRsp>&			appTxDataRsp,
						stream<ap_uint<16> >&			txApp2stateTable_req,
						stream<txAppTxSarQuery>&		txApp2txSar_upd_req, //TODO rename
						stream<mmCmd>&					txBufferWriteCmd,
						stream<net_axis<DATA_WIDTH> >&				txBufferWriteData,
#if (TCP_NODELAY)
						stream<net_axis<DATA_WIDTH> >&				txApp2txEng_data_stream,
#endif
						stream<event>&					txAppStream2eventEng_setEvent);
