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
#include "tx_app_interface.hpp"

using namespace hls;

void txEventMerger(	stream<event>&	txApp2eventEng_mergeEvent,
					stream<event>&	txAppStream2event_mergeEvent,
#if (TCP_NODELAY)
					stream<event>&	tasi_txEventCacheFifo,
#endif
					stream<event>&	out)
{
#pragma HLS PIPELINE II=1

	event ev;
	// Merge Events
	if (!txApp2eventEng_mergeEvent.empty())
	{
		out.write(txApp2eventEng_mergeEvent.read());
	}
	else if (!txAppStream2event_mergeEvent.empty())
	{
		txAppStream2event_mergeEvent.read(ev);
		out.write(ev);
#if (TCP_NODELAY)
		if (ev.type == TX)
		{
			tasi_txEventCacheFifo.write(ev);
		}
#endif
	}
}

void txAppStatusHandler(stream<mmStatus>&				txBufferWriteStatus,
						stream<event>&					tasi_eventCacheFifo,
#if !(TCP_NODELAY)
						stream<event>&					txAppStream2eventEng_setEvent,
#endif
						stream<txAppTxSarPush>&			txApp2txSar_app_push)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

   enum fsmStatus {READ_EV, READ_STATUS_1, READ_STATUS_2};
	static fsmStatus tash_state = READ_EV;
	static event ev;
	ap_uint<32> tempLength;

	switch (tash_state) {
	case READ_EV:
		if (!tasi_eventCacheFifo.empty()) {
			tasi_eventCacheFifo.read(ev);
			if (ev.type == TX)
			{
				tash_state = READ_STATUS_1;
 			}
#if !(TCP_NODELAY)
			else
			{
				txAppStream2eventEng_setEvent.write(ev);
			}
#endif
		}
		break;
	case READ_STATUS_1:
		if(!txBufferWriteStatus.empty())
		{
			mmStatus status = txBufferWriteStatus.read();
			if (status.okay)
			{
				tempLength = ev.address + ev.length;
				if (tempLength > BUFFER_SIZE) //(tempLength[WINDOW_BITS] == 1) 
				{
					tash_state = READ_STATUS_2;
				}
				else
				{
					txApp2txSar_app_push.write(txAppTxSarPush(ev.sessionID, ev.address+ev.length)); // App pointer update, pointer is released
#if !(TCP_NODELAY)
					txAppStream2eventEng_setEvent.write(ev);
#endif
					tash_state = READ_EV;
				}
			}

		}
		break;
	case READ_STATUS_2:
		if (!txBufferWriteStatus.empty()) {
			mmStatus status = txBufferWriteStatus.read();
			if (status.okay)
			{
				txApp2txSar_app_push.write(txAppTxSarPush(ev.sessionID, ev.address+ev.length)); // App pointer update, pointer is released
#if !(TCP_NODELAY)
				txAppStream2eventEng_setEvent.write(ev);
#endif
				tash_state = READ_EV;
			}
		}
		break;
	} //switch


}


void tx_app_table(	stream<txSarAckPush>&		txSar2txApp_ack_push,
					stream<txAppTxSarQuery>&	txApp_upd_req,
					stream<txAppTxSarReply>&	txApp_upd_rsp)
{
#pragma HLS PIPELINE II=1

	static txAppTableEntry app_table[MAX_SESSIONS];


	txSarAckPush	ackPush;
	txAppTxSarQuery txAppUpdate;

	if (!txSar2txApp_ack_push.empty())
	{
		txSar2txApp_ack_push.read(ackPush);
		if (ackPush.init)
		{
			// At init this is actually not_ackd
			app_table[ackPush.sessionID].ackd = ackPush.ackd-1;
			app_table[ackPush.sessionID].mempt = ackPush.ackd;
#if (TCP_NODELAY)
			app_table[ackPush.sessionID].min_window = ackPush.min_window;
#endif
		}
		else
		{
			app_table[ackPush.sessionID].ackd = ackPush.ackd;
#if (TCP_NODELAY)
			app_table[ackPush.sessionID].min_window = ackPush.min_window;
#endif
		}
	}
	else if (!txApp_upd_req.empty())
	{
		txApp_upd_req.read(txAppUpdate);
		// Write
		if(txAppUpdate.write)
		{
			app_table[txAppUpdate.sessionID].mempt = txAppUpdate.mempt;
		}
		else // Read
		{
#if !(TCP_NODELAY)
			txApp_upd_rsp.write(txAppTxSarReply(txAppUpdate.sessionID, app_table[txAppUpdate.sessionID].ackd, app_table[txAppUpdate.sessionID].mempt));
#else
			txApp_upd_rsp.write(txAppTxSarReply(txAppUpdate.sessionID, app_table[txAppUpdate.sessionID].ackd, app_table[txAppUpdate.sessionID].mempt, app_table[txAppUpdate.sessionID].min_window));
#endif
		}
	}

}

template <int WIDTH>
void tx_app_interface(	stream<appTxMeta>&			appTxDataReqMetadata,
					stream<net_axis<WIDTH> >&				appTxDataReq,
					stream<sessionState>&			stateTable2txApp_rsp,
					stream<txSarAckPush>&			txSar2txApp_ack_push,
					stream<mmStatus>&				txBufferWriteStatus,

					stream<ipTuple>&				appOpenConnReq,
					stream<ap_uint<16> >&			appCloseConnReq,
					stream<sessionLookupReply>&		sLookup2txApp_rsp,
					stream<ap_uint<16> >&			portTable2txApp_port_rsp,
					stream<sessionState>&			stateTable2txApp_upd_rsp,
					stream<openStatus>&				conEstablishedFifo,

					stream<appTxRsp>&			appTxDataRsp,
					stream<ap_uint<16> >&			txApp2stateTable_req,
					stream<mmCmd>&					txBufferWriteCmd,
					stream<net_axis<WIDTH> >&				txBufferWriteData,
#if (TCP_NODELAY)
					stream<net_axis<WIDTH> >&				txApp2txEng_data_stream,
#endif
					stream<txAppTxSarPush>&			txApp2txSar_push,

					stream<openStatus>&				appOpenConnRsp,
					stream<fourTuple>&				txApp2sLookup_req,
					//stream<ap_uint<1> >&			txApp2portTable_port_req,
					stream<stateQuery>&				txApp2stateTable_upd_req,
					stream<event>&					txApp2eventEng_setEvent,
					stream<openStatus>&				rtTimer2txApp_notification,
					ap_uint<32>						myIpAddress)
{
//#pragma HLS DATAFLOW
	#pragma HLS INLINE

	// Fifos
	static stream<event> txApp2eventEng_mergeEvent("txApp2eventEng_mergeEvent");
	static stream<event> txAppStream2event_mergeEvent("txAppStream2event_mergeEvent");
	#pragma HLS stream variable=txApp2eventEng_mergeEvent		depth=64
	#pragma HLS stream variable=txAppStream2event_mergeEvent	depth=64
	#pragma HLS aggregate  variable=txApp2eventEng_mergeEvent compact=bit
	#pragma HLS aggregate  variable=txAppStream2event_mergeEvent compact=bit

	static stream<event> txApp_eventCacheFifo("txApp_eventCacheFifo");
	static stream<event> txApp_txEventCache("txApp_txEventCache");
	#pragma HLS stream variable=txApp_eventCacheFifo	depth=64
	#pragma HLS aggregate  variable=txApp_eventCacheFifo compact=bit
	#pragma HLS stream variable=txApp_txEventCache	depth=64
	#pragma HLS aggregate  variable=txApp_txEventCache compact=bit

	static stream<txAppTxSarQuery>		txApp2txSar_upd_req("txApp2txSar_upd_req");
	static stream<txAppTxSarReply>		txSar2txApp_upd_rsp("txSar2txApp_upd_rsp");
	#pragma HLS stream variable=txApp2txSar_upd_req		depth=64
	#pragma HLS stream variable=txSar2txApp_upd_rsp		depth=64
	#pragma HLS aggregate  variable=txApp2txSar_upd_req compact=bit
	#pragma HLS aggregate  variable=txSar2txApp_upd_rsp compact=bit

	// Before merging, check status for TX
	//txAppEvSplitter(txAppStream2event_mergeEvent, tasi_txSplit2mergeFifo, txApp_txEventCache);
	//txAppStatusHandler(txBufferWriteStatus, txApp_txEventCache, txApp2txSar_push);
	// Merge Events
	txEventMerger(	txApp2eventEng_mergeEvent,
					txAppStream2event_mergeEvent,
#if (TCP_NODELAY)
					txApp_txEventCache,
					txApp2eventEng_setEvent);
#else
					txApp_eventCacheFifo);
#endif
	//txAppEvChecker(txApp_eventCache, txApp_txEventCache, txApp2eventEng_setEvent);
	txAppStatusHandler(	txBufferWriteStatus,
#if (TCP_NODELAY)
						txApp_txEventCache,
#else
						txApp_eventCacheFifo,
						txApp2eventEng_setEvent,
#endif
						txApp2txSar_push);

	// TX application Stream Interface
	tx_app_stream_if<WIDTH>(	appTxDataReqMetadata,
						appTxDataReq,
						stateTable2txApp_rsp,
						txSar2txApp_upd_rsp,
						appTxDataRsp,
						txApp2stateTable_req,
						txApp2txSar_upd_req,
						txBufferWriteCmd,
						txBufferWriteData,
#if (TCP_NODELAY)
						txApp2txEng_data_stream,
#endif
						txAppStream2event_mergeEvent);

	// TX Application Interface
	tx_app_if(	appOpenConnReq,
				appCloseConnReq,
				sLookup2txApp_rsp,
				portTable2txApp_port_rsp,
				stateTable2txApp_upd_rsp,
				conEstablishedFifo,
				appOpenConnRsp,
				txApp2sLookup_req,
				//txApp2portTable_port_req,
				txApp2stateTable_upd_req,
				txApp2eventEng_mergeEvent,
				rtTimer2txApp_notification,
				myIpAddress);

	// TX App Meta Table
	tx_app_table(	txSar2txApp_ack_push,
					txApp2txSar_upd_req,
					txSar2txApp_upd_rsp);
}

template void tx_app_interface<DATA_WIDTH>(	stream<appTxMeta>&			appTxDataReqMetadata,
					stream<net_axis<DATA_WIDTH> >&				appTxDataReq,
					stream<sessionState>&			stateTable2txApp_rsp,
					stream<txSarAckPush>&			txSar2txApp_ack_push,
					stream<mmStatus>&				txBufferWriteStatus,

					stream<ipTuple>&				appOpenConnReq,
					stream<ap_uint<16> >&			appCloseConnReq,
					stream<sessionLookupReply>&		sLookup2txApp_rsp,
					stream<ap_uint<16> >&			portTable2txApp_port_rsp,
					stream<sessionState>&			stateTable2txApp_upd_rsp,
					stream<openStatus>&				conEstablishedFifo,

					stream<appTxRsp>&			appTxDataRsp,
					stream<ap_uint<16> >&			txApp2stateTable_req,
					stream<mmCmd>&					txBufferWriteCmd,
					stream<net_axis<DATA_WIDTH> >&				txBufferWriteData,
#if (TCP_NODELAY)
					stream<net_axis<DATA_WIDTH> >&				txApp2txEng_data_stream,
#endif
					stream<txAppTxSarPush>&			txApp2txSar_push,

					stream<openStatus>&				appOpenConnRsp,
					stream<fourTuple>&				txApp2sLookup_req,
					//stream<ap_uint<1> >&			txApp2portTable_port_req,
					stream<stateQuery>&				txApp2stateTable_upd_req,
					stream<event>&					txApp2eventEng_setEvent,
					stream<openStatus>&				rtTimer2txApp_notification,
					ap_uint<32>						myIpAddress);
