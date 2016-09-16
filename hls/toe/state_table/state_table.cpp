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

#include "state_table.hpp"

using namespace hls;

/** @ingroup state_table
 *  Stores the TCP connection state of each session. It is accessed
 *  from the @ref rx_engine, @ref tx_app_if and from @ref tx_engine.
 *  It also receives Session-IDs from the @ref close_timer, those sessions
 *  are closed and the IDs forwarded to the @ref session_lookup_controller which
 *  releases this ID.
 *  @param[in]		rxEng2stateTable_upd_req
 *  @param[in]		txApp2stateTable_upd_req
 *  @param[in]		txApp2stateTable_req
 *  @param[in]		timer2stateTable_releaseState
 *  @param[out]		stateTable2rxEng_upd_rsp
 *  @param[out]		stateTable2TxApp_upd_rsp
 *  @param[out]		stateTable2txApp_rsp
 *  @param[out]		stateTable2sLookup_releaseSession
 */
void state_table(	stream<stateQuery>&			rxEng2stateTable_upd_req,
					stream<stateQuery>&			txApp2stateTable_upd_req,
					stream<ap_uint<16> >&		txApp2stateTable_req,
					stream<ap_uint<16> >&		timer2stateTable_releaseState,
					stream<sessionState>&		stateTable2rxEng_upd_rsp,
					stream<sessionState>&		stateTable2TxApp_upd_rsp,
					stream<sessionState>&		stateTable2txApp_rsp,
					stream<ap_uint<16> >&		stateTable2sLookup_releaseSession)
{
#pragma HLS PIPELINE II=1

	static sessionState state_table[MAX_SESSIONS];
	#pragma HLS RESOURCE variable=state_table core=RAM_2P_BRAM
	#pragma HLS DEPENDENCE variable=state_table inter false

	static ap_uint<16> stt_txSessionID;
	static ap_uint<16> stt_rxSessionID;
	static bool stt_rxSessionLocked = false;
	static bool stt_txSessionLocked = false;

	static stateQuery stt_txAccess;
	static stateQuery stt_rxAccess;
	static bool stt_txWait = false;
	static bool stt_rxWait = false;

	static ap_uint<16> stt_closeSessionID;
	static bool stt_closeWait = false;

	ap_uint<16> sessionID;


	// TX App If
	if(!txApp2stateTable_upd_req.empty() && !stt_txWait)
	{
		txApp2stateTable_upd_req.read(stt_txAccess);
		if ((stt_txAccess.sessionID == stt_rxSessionID) && stt_rxSessionLocked) //delay
		{
			stt_txWait = true;
		}
		else
		{
			if (stt_txAccess.write)
			{
				state_table[stt_txAccess.sessionID] = stt_txAccess.state;
				stt_txSessionLocked = false;
			}
			else
			{
				stateTable2TxApp_upd_rsp.write(state_table[stt_txAccess.sessionID]);
				//lock on every read
				stt_txSessionID = stt_txAccess.sessionID;
				stt_txSessionLocked = true;
			}
		}
	}
	// TX App Stream If
	else if (!txApp2stateTable_req.empty())
	{
		txApp2stateTable_req.read(sessionID);
		stateTable2txApp_rsp.write(state_table[sessionID]);
	}
	// RX Engine
	else if(!rxEng2stateTable_upd_req.empty() && !stt_rxWait)
	{
		rxEng2stateTable_upd_req.read(stt_rxAccess);
		if ((stt_rxAccess.sessionID == stt_txSessionID) && stt_txSessionLocked)
		{
			stt_rxWait = true;
		}
		else
		{
			if (stt_rxAccess.write)
			{
				if (stt_rxAccess.state == CLOSED)// && state_table[stt_rxAccess.sessionID] != CLOSED) // We check if it was not closed before, not sure if necessary
				{
					stateTable2sLookup_releaseSession.write(stt_rxAccess.sessionID);
				}
				state_table[stt_rxAccess.sessionID] = stt_rxAccess.state;
				stt_rxSessionLocked = false;
			}
			else
			{
				stateTable2rxEng_upd_rsp.write(state_table[stt_rxAccess.sessionID]);
				stt_rxSessionID = stt_rxAccess.sessionID;
				stt_rxSessionLocked = true;
			}
		}
	}
	// Timer release
	else if (!timer2stateTable_releaseState.empty() && !stt_closeWait) //can only be a close
	{
		timer2stateTable_releaseState.read(stt_closeSessionID);
		// Check if locked
		if (((stt_closeSessionID == stt_rxSessionID) && stt_rxSessionLocked) ||  ((stt_closeSessionID == stt_txSessionID) && stt_txSessionLocked))
		{
			stt_closeWait = true;
		}
		else
		{
			state_table[stt_closeSessionID] = CLOSED;
			stateTable2sLookup_releaseSession.write(stt_closeSessionID);
		}
	}
	else if (stt_txWait)
	{
		if ((stt_txAccess.sessionID != stt_rxSessionID) || !stt_rxSessionLocked)
		{
			if (stt_txAccess.write)
			{
				state_table[stt_txAccess.sessionID] = stt_txAccess.state;
				stt_txSessionLocked = false;
			}
			else
			{
				stateTable2TxApp_upd_rsp.write(state_table[stt_txAccess.sessionID]);
				stt_txSessionID = stt_txAccess.sessionID;
				stt_txSessionLocked = true;
			}
			stt_txWait = false;
		}
	}
	else if (stt_rxWait)
	{
		if ((stt_rxAccess.sessionID != stt_txSessionID) || !stt_txSessionLocked)
		{
			if (stt_rxAccess.write)
			{
				if (stt_rxAccess.state == CLOSED)
				{
					stateTable2sLookup_releaseSession.write(stt_rxAccess.sessionID);
				}
				state_table[stt_rxAccess.sessionID] = stt_rxAccess.state;
				stt_rxSessionLocked = false;
			}
			else
			{
				stateTable2rxEng_upd_rsp.write(state_table[stt_rxAccess.sessionID]);
				stt_rxSessionID = stt_rxAccess.sessionID;
				stt_rxSessionLocked = true;
			}
			stt_rxWait = false;
		}
	}
	else if (stt_closeWait)
	{
		if (((stt_closeSessionID != stt_rxSessionID) || !stt_rxSessionLocked) && ((stt_closeSessionID != stt_txSessionID) || !stt_txSessionLocked))
		{
			state_table[stt_closeSessionID] = CLOSED;
			stateTable2sLookup_releaseSession.write(stt_closeSessionID);
			stt_closeWait = false;
		}
	}
}
