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
#include "rx_sar_table.hpp"

using namespace hls;

/** @ingroup rx_sar_table
 * 	This data structure stores the RX(receiving) sliding window
 *  and handles concurrent access from the @ref rx_engine, @ref rx_app_if
 *  and @ref tx_engine. It also holds the owner of each session and the RX
 *  credit of each owner (64 B beats): admitted segments take credit, rxCreditIn returns it.
 *  @param[in]		rxEng2rxSar_upd_req
 *  @param[in]		rxApp2rxSar_upd_req
 *  @param[in]		txEng2rxSar_upd_req
 *  @param[in]		txApp2rxSar_owner
 *  @param[in]		timer2rxSar_ownerReq
 *  @param[in]		rxCreditIn
 *  @param[out]		rxSar2rxEng_upd_rsp
 *  @param[out]		rxSar2rxApp_upd_rsp
 *  @param[out]		rxSar2txEng_upd_rsp
 *  @param[out]		rxSar2timer_ownerRsp
 */
void rx_sar_table(	stream<rxSarRecvd>&			rxEng2rxSar_upd_req,
					stream<rxSarAppd>&			rxApp2rxSar_upd_req,
					stream<ap_uint<16> >&		txEng2rxSar_req, //read only
					stream<rxSarOwner>&			txApp2rxSar_owner,
					stream<ap_uint<16> >&		timer2rxSar_ownerReq,
					stream<rxCredit>&			rxCreditIn,
					stream<rxSarEntry>&			rxSar2rxEng_upd_rsp,
					stream<rxSarAppd>&			rxSar2rxApp_upd_rsp,
					stream<rxSarReply>&			rxSar2txEng_rsp,
					stream<ap_uint<OWNER_BITS> >&	rxSar2timer_ownerRsp)
{

	static rxSarEntry rx_table[MAX_SESSIONS];
	// Credit taken per owner; the owner's credit is RX_CREDIT - taken
	static ap_uint<16> taken[1 << OWNER_BITS];

#pragma HLS PIPELINE II=1

#if defined( __VITIS_HLS__)
#pragma HLS bind_storage variable=rx_table type=RAM_1P impl=BRAM
#else
#pragma HLS RESOURCE variable=rx_table core=RAM_1P_BRAM
#endif
#pragma HLS DEPENDENCE variable=rx_table inter false
#pragma HLS ARRAY_PARTITION variable=taken complete

	// Credit taken and returned in this cycle, both applied at the end
	ap_uint<OWNER_BITS> takeOwner = 0;
	ap_uint<16> takeBeats = 0;
	rxCredit ret(0, 0);
	if (!rxCreditIn.empty())
	{
		rxCreditIn.read(ret);
	}

	// Read only access from the Tx Engine
	if(!txEng2rxSar_req.empty())
	{
		ap_uint<16> addr = txEng2rxSar_req.read();
		rxSarEntry entry = rx_table[addr];
		rxSarReply reply (entry);

		//Pre-calculated usedLength, windowSize to improve timing in metaLoader
		//The window is at most the owner's credit
#if (WINDOW_SCALE)
				ap_uint<WINDOW_BITS> actualWindowSize = (entry.appd - ((ap_uint<WINDOW_BITS>)entry.recvd)) - 1; // This works even for wrap around
#else
				ap_uint<16> actualWindowSize = (entry.appd - ((ap_uint<16>)entry.recvd)) - 1; // This works even for wrap around
#endif
#if (RX_DDR_BYPASS)
				ap_uint<22> creditBytes = ((ap_uint<22>)(RX_CREDIT - taken[entry.owner])) << 6;
				if (creditBytes < actualWindowSize)
				{
					actualWindowSize = creditBytes;
				}
#endif
#if (WINDOW_SCALE)
				// An unscaled window must fit the 16-bit field
				if (entry.win_shift == 0 && actualWindowSize > 0xFFFF)
				{
					actualWindowSize = 0xFFFF;
				}
				reply.windowSize = actualWindowSize >> entry.win_shift;
#else
				reply.windowSize = actualWindowSize;
#endif

		rxSar2txEng_rsp.write(reply);
	}
	// Read or Write access from the Rx App I/F to update the application pointer
	else if(!rxApp2rxSar_upd_req.empty())
	{
		rxSarAppd in_appd = rxApp2rxSar_upd_req.read();
		if(in_appd.write)
		{
			rx_table[in_appd.sessionID].appd = in_appd.appd;
		}
		else
		{
			rxSar2rxApp_upd_rsp.write(rxSarAppd(in_appd.sessionID, rx_table[in_appd.sessionID].appd));
		}
	}
	// Read or Write access from the Rx Engine
	else if(!rxEng2rxSar_upd_req.empty())
	{
		rxSarRecvd in_recvd = rxEng2rxSar_upd_req.read();
		if (in_recvd.write)
		{
			rx_table[in_recvd.sessionID].recvd = in_recvd.recvd;
			rx_table[in_recvd.sessionID].head = in_recvd.head;
			rx_table[in_recvd.sessionID].offset = in_recvd.offset;
			rx_table[in_recvd.sessionID].gap = in_recvd.gap;
			if (in_recvd.init)
			{
				rx_table[in_recvd.sessionID].appd = in_recvd.recvd;
#if (WINDOW_SCALE)
				rx_table[in_recvd.sessionID].win_shift = in_recvd.win_shift;
#endif
				if (in_recvd.setOwner)
				{
					rx_table[in_recvd.sessionID].owner = in_recvd.owner;
				}
			}
			takeOwner = in_recvd.owner;
			takeBeats = in_recvd.beats;
		}
		else
		{
			rxSarEntry entry = rx_table[in_recvd.sessionID];
			entry.credit = RX_CREDIT - taken[entry.owner];
			rxSar2rxEng_upd_rsp.write(entry);
		}
	}
	// Owner of an actively opened session
	else if (!txApp2rxSar_owner.empty())
	{
		rxSarOwner in_owner = txApp2rxSar_owner.read();
		rx_table[in_owner.sessionID].owner = in_owner.owner;
	}
	// Owner for a timer notification, one request in flight
	else if (!timer2rxSar_ownerReq.empty())
	{
		rxSar2timer_ownerRsp.write(rx_table[timer2rxSar_ownerReq.read()].owner);
	}

#if (RX_DDR_BYPASS)
	for (int i = 0; i < (1 << OWNER_BITS); i++)
	{
		#pragma HLS UNROLL
		ap_uint<16> upd = taken[i];
		if (i == takeOwner) upd += takeBeats;
		if (i == ret.owner) upd -= ret.beats;
		taken[i] = upd;
	}
#endif
}
