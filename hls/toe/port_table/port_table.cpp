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

#include "port_table.hpp"

using namespace hls;

/** @ingroup port_table
 *  rxEng and txApp are accessing this table:
 *  rxEng: read
 *  txApp: read -> write
 *  If read and write operation on same address occur at the same time,
 *  read should get the old value, either way it doesn't matter
 *  @param[in]		rxApp2portTable_listen_req
 *  @param[in]		pt_portCheckListening_req_fifo
 *  @param[in]		pt_portCheckListening_req_fifo
 *  @param[out]		portTable2rxApp_listen_rsp
 *  @param[out]		pt_portCheckListening_rsp_fifo
 */
static const int MAX_LISTENING_PORTS = 16;

void listening_port_table(	stream<ap_uint<16> >&	rxApp2portTable_listen_req,
							stream<ap_uint<15> >&	pt_portCheckListening_req_fifo,
							stream<bool>&			portTable2rxApp_listen_rsp,
							stream<bool>&			pt_portCheckListening_rsp_fifo)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	static ap_uint<16>	listenedPorts[MAX_LISTENING_PORTS];
	static bool			portActive[MAX_LISTENING_PORTS];
	#pragma HLS ARRAY_PARTITION variable=listenedPorts complete dim=1
	#pragma HLS ARRAY_PARTITION variable=portActive    complete dim=1

	ap_uint<16> currPort;

	if (!rxApp2portTable_listen_req.empty())
	{
		rxApp2portTable_listen_req.read(currPort);

		if (currPort >= 32768) {
			portTable2rxApp_listen_rsp.write(false);
			return;
		}

		bool already = false;
		for (int i = 0; i < MAX_LISTENING_PORTS; i++) {
			#pragma HLS UNROLL
			if (portActive[i] && listenedPorts[i] == currPort) already = true;
		}

		// Priority-encode lowest free slot (iterate backwards so index 0 wins)
		ap_uint<6> freeSlot = MAX_LISTENING_PORTS;
		for (int i = MAX_LISTENING_PORTS - 1; i >= 0; i--) {
			#pragma HLS UNROLL
			if (!portActive[i]) freeSlot = i;
		}

		if (already) {
			portTable2rxApp_listen_rsp.write(true);
		} else if (freeSlot < MAX_LISTENING_PORTS) {
			listenedPorts[freeSlot] = currPort;
			portActive[freeSlot]    = true;
			portTable2rxApp_listen_rsp.write(true);
		} else {
			portTable2rxApp_listen_rsp.write(false);  // CAM full
		}
	}
	else if (!pt_portCheckListening_req_fifo.empty())
	{
		ap_uint<15> checkPort = pt_portCheckListening_req_fifo.read();
		bool found = false;
		for (int i = 0; i < MAX_LISTENING_PORTS; i++) {
			#pragma HLS UNROLL
			if (portActive[i] && listenedPorts[i] == checkPort) found = true;
		}
		pt_portCheckListening_rsp_fifo.write(found);
	}
}
/** @ingroup port_table
 *  One entry per session: ephemeral ports are 32768..(32768+MAX_SESSIONS-1).
 *  Cursor wraps at MAX_SESSIONS so the table never needs more entries than sessions.
 *  @param[in]		sLookup2portTable_releasePort
 *  @param[in]		pt_portCheckUsed_req_fifo
 *  @param[out]		pt_portCheckUsed_rsp_fifo
 *  @param[out]		portTable2txApp_port_rsp
 */
void free_port_table(	stream<ap_uint<16> >&	sLookup2portTable_releasePort,
						stream<ap_uint<15> >&	pt_portCheckUsed_req_fifo,
						//stream<ap_uint<1> >&	txApp2portTable_port_req,
						stream<bool>&			pt_portCheckUsed_rsp_fifo,
						stream<ap_uint<16> >&	portTable2txApp_port_rsp)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	static const unsigned FREE_PORT_BITS = ConstLog2(MAX_SESSIONS);

	static bool freePortTable[MAX_SESSIONS];
#if defined( __VITIS_HLS__)
	#pragma HLS bind_storage variable=freePortTable type=RAM_S2P impl=LUTRAM
#else
	#pragma HLS RESOURCE variable=freePortTable core=RAM_S2P_LUTRAM
#endif

	static ap_uint<FREE_PORT_BITS> pt_cursor = 0;

	ap_uint<16> currPort;
	ap_uint<16> freePort;

	if (!sLookup2portTable_releasePort.empty())
	{
		sLookup2portTable_releasePort.read(currPort);
		if (currPort >= 32768)
		{
			freePortTable[currPort(FREE_PORT_BITS-1, 0)] = false;
		}
	}
	else if (!pt_portCheckUsed_req_fifo.empty())
	{
		ap_uint<15> checkIdx = pt_portCheckUsed_req_fifo.read();
		pt_portCheckUsed_rsp_fifo.write(freePortTable[checkIdx(FREE_PORT_BITS-1, 0)]);
	}
	else
	{
		bool used = freePortTable[pt_cursor];
		if (used) {
			pt_cursor++;
		} else if (!portTable2txApp_port_rsp.full()) {
			freePort = 0;
			freePort(FREE_PORT_BITS-1, 0) = pt_cursor;
			freePort[15] = 1;
			freePortTable[pt_cursor] = true;
			portTable2txApp_port_rsp.write(freePort);
			pt_cursor++;
		}
	}
	/*if (!txApp2portTable_port_req.empty()) //Fixme this!!!
	{
		txApp2portTable_port_req.read();
	}*/
}


void check_in_multiplexer(	stream<ap_uint<16> >&		rxEng2portTable_check_req,
							stream<ap_uint<15> >&		pt_portCheckListening_req_fifo,
							stream<ap_uint<15> >&		pt_portCheckUsed_req_fifo,
							stream<bool>&				pt_dstFifoOut)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	static const bool LT = true;
	static const bool FT = false;
	static bool	dst = LT;
	ap_uint<16>			checkPort;
	ap_uint<16>			swappedCheckPort;

	// Forward request according to port number, store table to keep order
	if (!rxEng2portTable_check_req.empty())
	{
		rxEng2portTable_check_req.read(checkPort);
		swappedCheckPort(7, 0) = checkPort(15, 8);
		swappedCheckPort(15, 8) = checkPort(7, 0);
		if (swappedCheckPort < 32768)
		{
			pt_portCheckListening_req_fifo.write(swappedCheckPort);
			pt_dstFifoOut.write(LT);
		}
		else
		{
			pt_portCheckUsed_req_fifo.write(swappedCheckPort);
			pt_dstFifoOut.write(FT);
		}
	}
}

/** @ingroup port_table
 *
 */
void check_out_multiplexer(	stream<bool>&				pt_dstFifoIn,
							stream<bool>&				pt_portCheckListening_rsp_fifo,
							stream<bool>&				pt_portCheckUsed_rsp_fifo,
							stream<bool>&				portTable2rxEng_check_rsp)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	//enum portCheckDstType {LT, FT};
	static const bool LT = true;
	static const bool FT = false;
	//static stream<bool> pt_dstFifo("pt_dstFifo");
	//#pragma HLS STREAM variable=pt_dstFifo depth=4

	static bool	dst = LT;


	// Read out responses from tables in order and merge them
	enum cmFsmStateType {READ_DST, READ_LISTENING, READ_USED};
	static cmFsmStateType cm_fsmState = READ_DST;
	switch (cm_fsmState)
	{
	case 0:
		if (!pt_dstFifoIn.empty())
		{
			pt_dstFifoIn.read(dst);
			if (dst == LT)
			{
				cm_fsmState = READ_LISTENING;
			}
			else
			{
				cm_fsmState = READ_USED;
			}
		}
		break;
	case 1:
		if (!pt_portCheckListening_rsp_fifo.empty())
		{
			portTable2rxEng_check_rsp.write(pt_portCheckListening_rsp_fifo.read());
			cm_fsmState = READ_DST;
		}
		break;
	case 2:
		if (!pt_portCheckUsed_rsp_fifo.empty())
		{
			portTable2rxEng_check_rsp.write(pt_portCheckUsed_rsp_fifo.read());
			cm_fsmState = READ_DST;
		}
		break;
	}
}


/** @ingroup port_table
 *  The @ref port_table contains an array of 65536 entries, one for each port number.
 *  It receives passive opening (listening) request from @ref rx_app_if, Request to check
 *  if the port is open from the @ref rx_engine and requests for a free port from the
 *  @ref tx_app_if.
 *  @param[in]		rxEng2portTable_check_req
 *  @param[in]		rxApp2portTable_listen_req
 *  @param[in]		txApp2portTable_req
 *  @param[in]		sLookup2portTable_releasePort
 *  @param[out]		portTable2rxEng_check_rsp
 *  @param[out]		portTable2rxApp_listen_rsp
 *  @param[out]		portTable2txApp_rsp
 */
void port_table(stream<ap_uint<16> >&		rxEng2portTable_check_req,
				stream<ap_uint<16> >&		rxApp2portTable_listen_req,
				//stream<ap_uint<1> >&		txApp2portTable_port_req,
				stream<ap_uint<16> >&		sLookup2portTable_releasePort,
				stream<bool>&				portTable2rxEng_check_rsp,
				stream<bool>&				portTable2rxApp_listen_rsp,
				stream<ap_uint<16> >&		portTable2txApp_port_rsp)
{
//#pragma HLS DATAFLOW
#pragma HLS INLINE
#if defined( __VITIS_HLS__)
#pragma HLS aggregate  variable=rxEng2portTable_check_req compact=bit
#pragma HLS aggregate  variable=rxApp2portTable_listen_req compact=bit
#pragma HLS aggregate  variable=sLookup2portTable_releasePort compact=bit
#pragma HLS aggregate  variable=portTable2rxEng_check_rsp compact=bit
#pragma HLS aggregate  variable=portTable2rxApp_listen_rsp compact=bit
#pragma HLS aggregate  variable=portTable2txApp_port_rsp compact=bit
#else
#pragma HLS DATA_PACK variable=rxEng2portTable_check_req
#pragma HLS DATA_PACK variable=rxApp2portTable_listen_req
#pragma HLS DATA_PACK variable=sLookup2portTable_releasePort
#pragma HLS DATA_PACK variable=portTable2rxEng_check_rsp
#pragma HLS DATA_PACK variable=portTable2rxApp_listen_rsp
#pragma HLS DATA_PACK variable=portTable2txApp_port_rsp
#endif

	/*
	 * Fifos necessary for multiplexing Check requests
	 */
	static stream<ap_uint<15> >	pt_portCheckListening_req_fifo("pt_portCheckListening_req_fifo");
	static stream<ap_uint<15> >	pt_portCheckUsed_req_fifo("pt_portCheckUsed_req_fifo");
	#pragma HLS STREAM variable=pt_portCheckListening_req_fifo depth=2
	#pragma HLS STREAM variable=pt_portCheckUsed_req_fifo depth=2

	static stream<bool> pt_portCheckListening_rsp_fifo("pt_portCheckListening_rsp_fifo");
	static stream<bool> pt_portCheckUsed_rsp_fifo("pt_portCheckUsed_rsp_fifo");
	#pragma HLS STREAM variable=pt_portCheckListening_rsp_fifo depth=2
	#pragma HLS STREAM variable=pt_portCheckUsed_rsp_fifo depth=2

	static stream<bool> pt_dstFifo("pt_dstFifo");
	#pragma HLS STREAM variable=pt_dstFifo depth=4

	/*
	 * Listening PortTable
	 */
	listening_port_table(	rxApp2portTable_listen_req,
							pt_portCheckListening_req_fifo,
							portTable2rxApp_listen_rsp,
							pt_portCheckListening_rsp_fifo);

	/*
	 * Free PortTable
	 */
	free_port_table(sLookup2portTable_releasePort,
						pt_portCheckUsed_req_fifo,
						//txApp2portTable_port_req,
						pt_portCheckUsed_rsp_fifo,
						portTable2txApp_port_rsp);

	/*
	 * Multiplex this query
	 */
	check_in_multiplexer(	rxEng2portTable_check_req,
							pt_portCheckListening_req_fifo,
							pt_portCheckUsed_req_fifo,
							pt_dstFifo);
	check_out_multiplexer(	pt_dstFifo,
							pt_portCheckListening_rsp_fifo,
							pt_portCheckUsed_rsp_fifo,
							portTable2rxEng_check_rsp);
}
