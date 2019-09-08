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
#include "../toe.hpp"
#include "../toe_internals.hpp"
#include "../tx_app_if/tx_app_if.hpp"
#include "../tx_app_stream_if/tx_app_stream_if.hpp"

using namespace hls;

struct txAppTableEntry
{
	ap_uint<WINDOW_BITS>		ackd;
	ap_uint<WINDOW_BITS>		mempt;
#if (TCP_NODELAY)
	ap_uint<WINDOW_BITS> 	min_window;
#endif
	txAppTableEntry() {}
};

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
					stream<ap_uint<16> >&				txApp2stateTable_req,
					stream<mmCmd>&					txBufferWriteCmd,
					stream<net_axis<WIDTH> >&				txBufferWriteData,
#if (TCP_NODELAY)
					stream<net_axis<WIDTH> >&				txApp2txEng_data_stream,
#endif
					stream<txAppTxSarPush>&			txApp2txSar_push,

					stream<openStatus>&				appOpenConnRsp,
					stream<fourTuple>&				txApp2sLookup_req,
					stream<stateQuery>&				txApp2stateTable_upd_req,
					stream<event>&					txApp2eventEng_setEvent,
					stream<openStatus>&				rtTimer2txApp_notification,
					ap_uint<32>						myIpAddress);
