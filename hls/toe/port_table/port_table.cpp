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
 *  @param[out]		portTable2rxApp_listen_rsp
 *  @param[out]		pt_portCheckListening_rsp_fifo
 */
void listening_port_table(	stream<ap_uint<16> >&	rxApp2portTable_listen_req,
							stream<ap_uint<15> >&	pt_portCheckListening_req_fifo,
							stream<bool>&			portTable2rxApp_listen_rsp,
							stream<bool>&			pt_portCheckListening_rsp_fifo) {
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	static bool listeningPortTable[32768];
	#pragma HLS RESOURCE variable=listeningPortTable core=RAM_T2P_BRAM
	#pragma HLS DEPENDENCE variable=listeningPortTable inter false

	ap_uint<16> currPort;

	if (!rxApp2portTable_listen_req.empty()) { //check range, TODO make sure currPort is not equal in 2 consecutive cycles
		rxApp2portTable_listen_req.read(currPort);
		if (!listeningPortTable[currPort(14, 0)] && currPort < 32768) {
			listeningPortTable[currPort] = true;
			portTable2rxApp_listen_rsp.write(true);
		}
		else
			portTable2rxApp_listen_rsp.write(false);
	}
	else if (!pt_portCheckListening_req_fifo.empty())
		pt_portCheckListening_rsp_fifo.write(listeningPortTable[pt_portCheckListening_req_fifo.read()]);
}
/** @ingroup port_table
 *  Assumption: We are never going to run out of free ports, since 10K session <<< 32K ports
 *  rxEng: read
 *  txApp: pt_cursor: read -> write
 *  sLookup: write
 *  If a free port is found it is written into @portTable2txApp_port_rsp and cached until @ref tx_app_stream_if reads it out
 *  @param[in]		sLookup2portTable_releasePort
 *  @param[in]		pt_portCheckUsed_req_fifo
 *  @param[out]		pt_portCheckUsed_rsp_fifo
 *  @param[out]		portTable2txApp_port_rsp
 */
void free_port_table(	stream<ap_uint<16> >&	sLookup2portTable_releasePort,
						stream<ap_uint<15> >&	pt_portCheckUsed_req_fifo,
						stream<ap_uint<1> >&	txApp2portTable_port_req,
						stream<bool>&			pt_portCheckUsed_rsp_fifo,
						stream<ap_uint<16> >&	portTable2txApp_port_rsp) {
	#pragma HLS PIPELINE II=1
	#pragma HLS INLINE off

	static bool freePortTable[32768]; //= {false};
	#pragma HLS RESOURCE variable=freePortTable core=RAM_T2P_BRAM
	#pragma HLS DEPENDENCE variable=freePortTable inter false

	static ap_uint<15>	freePort = 0;
	static bool searching 	= false;
	static bool eval		= false;
	static bool temp		= false;
	#pragma HLS DEPENDENCE variable=temp inter false

	if (searching) {
		temp = freePortTable[freePort];
		eval = true;
		searching = false;
	}
	else if (eval) {
		if (!temp) {
			freePortTable[freePort] = true;
			portTable2txApp_port_rsp.write(freePort);
		}
		else
			searching = true;
		eval = false;
		freePort++;
	}
	else if (!pt_portCheckUsed_req_fifo.empty())
		pt_portCheckUsed_rsp_fifo.write(freePortTable[pt_portCheckUsed_req_fifo.read()]);
	else if (!txApp2portTable_port_req.empty()) {
		txApp2portTable_port_req.read();
		searching = true;
	}
	else if (!sLookup2portTable_releasePort.empty()) { //check range, TODO make sure no access to same location in 2 consecutive cycles
		ap_uint<16>		currPort = sLookup2portTable_releasePort.read();
		if (currPort.bit(15) == 1)
			freePortTable[currPort.range(14, 0)] = false; //shift
	}
}

/** @ingroup port_table
 *
 */

void checkMuxInput(stream<ap_uint<16> >&		rxEng2portTable_check_req,
				   stream<ap_uint<15> >&		pt_portCheckListening_req_fifo,
				   stream<ap_uint<15> >&		pt_portCheckUsed_req_fifo,
				   stream<bool>&   				pt_dstFifo) {
	#pragma HLS PIPELINE II=1
	#pragma HLS INLINE off

	// Forward request according to port number, store table to keep order
	if (!rxEng2portTable_check_req.empty()) {
		ap_uint<16>			checkPort = rxEng2portTable_check_req.read();
		checkPort = byteSwap16(checkPort);
		if (checkPort < 32768) {
			pt_portCheckListening_req_fifo.write(checkPort.range(14, 0));
			pt_dstFifo.write(true); ////
			}
		else {
			pt_portCheckUsed_req_fifo.write(checkPort.range(14, 0));
			pt_dstFifo.write(false);
		}
	}
}

void checkMuxOutput(stream<bool>&  pt_dstFifo,
					stream<bool>&  pt_portCheckListening_rsp_fifo,
					stream<bool>&  pt_portCheckUsed_rsp_fifo,
					stream<bool>&  portTable2rxEng_check_rsp) {
	#pragma HLS PIPELINE II=1
	#pragma HLS INLINE off

	enum cmFsmStateType {READ_DST = 0, READ_LISTENING, READ_USED}; // Read out responses from tables in order and merge them
	static cmFsmStateType cm_fsmState = READ_DST;
	switch (cm_fsmState) {
	case READ_DST:
		if (!pt_dstFifo.empty()) { ////
			bool	dst = pt_dstFifo.read();
			if (dst == true)
				cm_fsmState = READ_LISTENING;
			else
				cm_fsmState = READ_USED;
		}
		break;
	case READ_LISTENING:
		if (!pt_portCheckListening_rsp_fifo.empty()) {
			portTable2rxEng_check_rsp.write(pt_portCheckListening_rsp_fifo.read());
			cm_fsmState = READ_DST;
		}
		break;
	case READ_USED:
		if (!pt_portCheckUsed_rsp_fifo.empty()) {
			portTable2rxEng_check_rsp.write(pt_portCheckUsed_rsp_fifo.read());
			cm_fsmState = READ_DST;
		}
		break;
	}
}

void check_multiplexer(	stream<ap_uint<16> >&		rxEng2portTable_check_req,
						stream<ap_uint<15> >&		pt_portCheckListening_req_fifo,
						stream<ap_uint<15> >&		pt_portCheckUsed_req_fifo,
						stream<bool>&				pt_portCheckListening_rsp_fifo,
						stream<bool>&				pt_portCheckUsed_rsp_fifo,
						stream<bool>&				portTable2rxEng_check_rsp) {
//#pragma HLS PIPELINE II=1
#pragma HLS INLINE

	static stream<bool> pt_dstFifo("pt_dstFifo");
	#pragma HLS STREAM variable=pt_dstFifo depth=4

	checkMuxInput(rxEng2portTable_check_req, pt_portCheckListening_req_fifo, pt_portCheckUsed_req_fifo, pt_dstFifo);
	checkMuxOutput(pt_dstFifo, pt_portCheckListening_rsp_fifo, pt_portCheckUsed_rsp_fifo, portTable2rxEng_check_rsp);
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
				stream<ap_uint<1> >&		txApp2portTable_port_req,
				stream<ap_uint<16> >&		sLookup2portTable_releasePort,
				stream<bool>&				portTable2rxEng_check_rsp,
				stream<bool>&				portTable2rxApp_listen_rsp,
				stream<ap_uint<16> >&		portTable2txApp_port_rsp)
{
//#pragma HLS dataflow interval=1
//#pragma HLS PIPELINE II=1
#pragma HLS INLINE
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
						txApp2portTable_port_req,
						pt_portCheckUsed_rsp_fifo,
						portTable2txApp_port_rsp);

	/*
	 * Multiplex this query
	 */
	check_multiplexer(rxEng2portTable_check_req,
			pt_portCheckListening_req_fifo,
			pt_portCheckUsed_req_fifo,
			pt_portCheckListening_rsp_fifo,
			pt_portCheckUsed_rsp_fifo,
			portTable2rxEng_check_rsp);
}
