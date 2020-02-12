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
							stream<bool>&			pt_portCheckListening_rsp_fifo)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	static bool listeningPortTable[32768];
	#pragma HLS RESOURCE variable=listeningPortTable core=RAM_T2P_BRAM
	#pragma HLS DEPENDENCE variable=listeningPortTable inter false

	ap_uint<16> currPort;

	if (!rxApp2portTable_listen_req.empty()) //check range, TODO make sure currPort is not equal in 2 consecutive cycles
	{
		rxApp2portTable_listen_req.read(currPort);
		//return true when the port is already open
		if (listeningPortTable[currPort(14, 0)] && currPort < 32768)
		{
			portTable2rxApp_listen_rsp.write(true);
		}
		else if (!listeningPortTable[currPort(14, 0)] && currPort < 32768)
		{
			listeningPortTable[currPort] = true;
			portTable2rxApp_listen_rsp.write(true);
		}
		else
		{
			portTable2rxApp_listen_rsp.write(false);
		}
	}
	else if (!pt_portCheckListening_req_fifo.empty())
	{
		//pt_portCheckListening_req_fifo.read(checkPort15);
		//pt_portCheckListening_rsp_fifo.write(listeningPortTable[checkPort15]);
		pt_portCheckListening_rsp_fifo.write(listeningPortTable[pt_portCheckListening_req_fifo.read()]);
	}
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
						//stream<ap_uint<1> >&	txApp2portTable_port_req,
						stream<bool>&			pt_portCheckUsed_rsp_fifo,
						stream<ap_uint<16> >&	portTable2txApp_port_rsp)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	static bool freePortTable[32768];
	#pragma HLS RESOURCE variable=freePortTable core=RAM_T2P_BRAM
	#pragma HLS DEPENDENCE variable=freePortTable inter false


	// Free Ports Cache
	//static stream<ap_uint<16> > pt_freePortsFifo("pt_freePortsFifo");
	//#pragma HLS STREAM variable=pt_freePortsFifo depth=8

	static ap_uint<15>	pt_cursor = 0;

	ap_uint<16>			currPort;
	ap_uint<16>			freePort;

	if (!sLookup2portTable_releasePort.empty()) //check range, TODO make sure no acces to same location in 2 consecutive cycles
	{
		sLookup2portTable_releasePort.read(currPort);
		if (currPort >= 32768)
		{
			freePortTable[currPort(14, 0)] = false; //shift
		}
	}
	else if (!pt_portCheckUsed_req_fifo.empty())
	{
		pt_portCheckUsed_rsp_fifo.write(freePortTable[pt_portCheckUsed_req_fifo.read()]);
	}
	else
	{
		if (!freePortTable[pt_cursor] && !portTable2txApp_port_rsp.full()) //This is not perfect, but yeah
		{
			freePort(14, 0) = pt_cursor;
			freePort[15] = 1;
			freePortTable[pt_cursor] = true;
			portTable2txApp_port_rsp.write(freePort);
		}
	}
	pt_cursor++;

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

#pragma HLS DATA_PACK variable=rxEng2portTable_check_req
#pragma HLS DATA_PACK variable=rxApp2portTable_listen_req
#pragma HLS DATA_PACK variable=sLookup2portTable_releasePort
#pragma HLS DATA_PACK variable=portTable2rxEng_check_rsp
#pragma HLS DATA_PACK variable=portTable2rxApp_listen_rsp
#pragma HLS DATA_PACK variable=portTable2txApp_port_rsp

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
