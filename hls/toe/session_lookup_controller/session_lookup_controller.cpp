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

#include "session_lookup_controller.hpp"

using namespace hls;

/** @ingroup session_lookup_controller
 *  SessionID manager
 *  @param[in]		fin_id, IDs that are released and appended to the SessionID free list
 *  @param[out]		new_id, get a new SessionID from the SessionID free list
 */
void sessionIdManager(	stream<ap_uint<14> >&		new_id,
						stream<ap_uint<14> >&		fin_id)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	static ap_uint<14> counter = 0;
	#pragma HLS reset variable=counter
	ap_uint<14> sessionID;

	if (!fin_id.empty())
	{
		fin_id.read(sessionID);
		new_id.write(sessionID);
	}
	else if (counter < MAX_SESSIONS)
	{
		new_id.write(counter);
		counter++;
	}

}

/** @ingroup session_lookup_controller
 *  Handles the Lookup relies from the RTL Lookup Table, if there was no hit,
 *  it checks if the request is allowed to create a new sessionID and does so.
 *  If it is a hit, the reply is forwarded to the corresponding source.
 *  It also handles the replies of the Session Updates [Inserts/Deletes], in case
 *  of insert the response with the new sessionID is replied to the request source.
 *  @param[in]		txEng2sLookup_rev_req
 *  @param[in]		sessionLookup_rsp
 *  @param[in]		sessionUpdatea_rsp
 *  @param[in]		stateTable2sLookup_releaseSession
 *  @param[in]		lookups
 *  @param[out]		sLookup2rxEng_rsp
 *  @param[out]		sLookup2txApp_rsp
 *  @param[out]		sLookup2txEng_rev_rsp
 *  @param[out]		sessionLookup_req
 *  @param[out]		sLookup2portTable_releasePort
 *  @TODO document more
 */
void lookupReplyHandler(stream<rtlSessionLookupReply>&			sessionLookup_rsp,
						stream<rtlSessionUpdateReply>&			sessionInsert_rsp,
						stream<sessionLookupQuery>&			rxEng2sLooup_req,
						stream<fourTuple>&					txApp2sLookup_req,
						stream<ap_uint<14> >&					sessionIdFreeList,
						stream<rtlSessionLookupRequest>&		sessionLookup_req,
						stream<sessionLookupReply>&				sLookup2rxEng_rsp,
						stream<sessionLookupReply>&				sLookup2txApp_rsp,
						stream<rtlSessionUpdateRequest>&		sessionInsert_req,
						stream<revLupInsert>&					reverseTableInsertFifo)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	static stream<threeTupleInternal>		slc_insertTuples("slc_insertTuples2");
	#pragma HLS STREAM variable=slc_insertTuples depth=4

	static stream<sessionLookupQueryInternal>		slc_queryCache("slc_queryCache");
	#pragma HLS STREAM variable=slc_queryCache depth=8

	fourTuple toeTuple;
	threeTupleInternal tuple;
	sessionLookupQuery query;
	sessionLookupQueryInternal intQuery;
	rtlSessionLookupReply lupReply;
	rtlSessionUpdateReply insertReply;
	ap_uint<16> sessionID;
	ap_uint<14> freeID = 0;

	enum slcFsmStateType {LUP_REQ, LUP_RSP, UPD_RSP};
	static slcFsmStateType slc_fsmState = LUP_REQ;

	switch (slc_fsmState)
	{
	case LUP_REQ:
		if (!txApp2sLookup_req.empty())
		{
			txApp2sLookup_req.read(toeTuple);
			intQuery.tuple.theirIp = toeTuple.dstIp;
			intQuery.tuple.theirPort = toeTuple.dstPort;
			//intQuery.tuple.myIp = toeTuple.srcIp;
			intQuery.tuple.myPort = toeTuple.srcPort;
			intQuery.allowCreation = true;
			intQuery.source = TX_APP;
			sessionLookup_req.write(rtlSessionLookupRequest(intQuery.tuple, intQuery.source));
			slc_queryCache.write(intQuery);
			slc_fsmState = LUP_RSP;
		}
		else if (!rxEng2sLooup_req.empty())
		{
			rxEng2sLooup_req.read(query);
			intQuery.tuple.theirIp = query.tuple.srcIp;
			intQuery.tuple.theirPort = query.tuple.srcPort;
			//intQuery.tuple.myIp = query.tuple.dstIp;
			intQuery.tuple.myPort = query.tuple.dstPort;
			intQuery.allowCreation = query.allowCreation;
			intQuery.source = RX;
			sessionLookup_req.write(rtlSessionLookupRequest(intQuery.tuple, intQuery.source));
			slc_queryCache.write(intQuery);
			slc_fsmState = LUP_RSP;
		}
		break;
	case LUP_RSP:
		if(!sessionLookup_rsp.empty() && !slc_queryCache.empty())
		{
			sessionLookup_rsp.read(lupReply);
			slc_queryCache.read(intQuery);
			if (!lupReply.hit && intQuery.allowCreation && !sessionIdFreeList.empty())
			{
				sessionIdFreeList.read(freeID);
				sessionInsert_req.write(rtlSessionUpdateRequest(intQuery.tuple, freeID, INSERT, lupReply.source));
				slc_insertTuples.write(intQuery.tuple);
				slc_fsmState = UPD_RSP;
			}
			else
			{
				if (lupReply.source == RX)
				{
					sLookup2rxEng_rsp.write(sessionLookupReply(lupReply.sessionID, lupReply.hit));
				}
				else
				{
					sLookup2txApp_rsp.write(sessionLookupReply(lupReply.sessionID, lupReply.hit));
				}
				slc_fsmState = LUP_REQ;
			}
		}
		break;
	//case UPD_REQ:
		//break;
	case UPD_RSP:
		if (!sessionInsert_rsp.empty() && !slc_insertTuples.empty())
		{
			sessionInsert_rsp.read(insertReply);
			slc_insertTuples.read(tuple);
			//updateReplies.write(sessionLookupReply(insertReply.sessionID, true));
			if (insertReply.source == RX)
			{
				sLookup2rxEng_rsp.write(sessionLookupReply(insertReply.sessionID, true));
			}
			else
			{
				sLookup2txApp_rsp.write(sessionLookupReply(insertReply.sessionID, true));
			}
			reverseTableInsertFifo.write(revLupInsert(insertReply.sessionID, tuple));
			slc_fsmState = LUP_REQ;
		}
		break;
	}
}

void updateRequestSender(stream<rtlSessionUpdateRequest>&		sessionInsert_req,
					stream<rtlSessionUpdateRequest>&		sessionDelete_req,
					stream<rtlSessionUpdateRequest>&		sessionUpdate_req,
					stream<ap_uint<14> >&					sessionIdFinFifo,
					ap_uint<16>& regSessionCount)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	static ap_uint<16> usedSessionIDs = 0;
	rtlSessionUpdateRequest request;

	if (!sessionInsert_req.empty())
	{
		sessionUpdate_req.write(sessionInsert_req.read());
		usedSessionIDs++;
		regSessionCount = usedSessionIDs;
	}
	else if (!sessionDelete_req.empty())
	{
		sessionDelete_req.read(request);
		sessionUpdate_req.write(request);
		sessionIdFinFifo.write(request.value);
		usedSessionIDs--;
		regSessionCount = usedSessionIDs;
	}
}


void updateReplyHandler(	stream<rtlSessionUpdateReply>&			sessionUpdate_rsp,
							stream<rtlSessionUpdateReply>&			sessionInsert_rsp)
							//stream<ap_uint<14> >&					sessionIdFinFifo)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	rtlSessionUpdateReply upReply;
	threeTupleInternal tuple;

	if (!sessionUpdate_rsp.empty())
	{
		sessionUpdate_rsp.read(upReply);
		if (upReply.op == INSERT)
		{
			sessionInsert_rsp.write(upReply);
		}
		/*else // DELETE
		{
			sessionIdFinFifo.write(upReply.sessionID);
		}*/
	}
}

void reverseLookupTableInterface(	stream<revLupInsert>& revTableInserts,
									stream<ap_uint<16> >& stateTable2sLookup_releaseSession,
									stream<ap_uint<16> >& txEng2sLookup_rev_req,
									stream<ap_uint<16> >& sLookup2portTable_releasePort,
									stream<rtlSessionUpdateRequest> & deleteCache,
									stream<fourTuple>& sLookup2txEng_rev_rsp,
									ap_uint<32>						myIpAddress)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	static threeTupleInternal reverseLookupTable[MAX_SESSIONS];
	#pragma HLS bind_storage variable=reverseLookupTable type=RAM_T2P impl=BRAM
	#pragma HLS DEPENDENCE variable=reverseLookupTable inter false
	static bool tupleValid[MAX_SESSIONS];
	#pragma HLS DEPENDENCE variable=tupleValid inter false

	revLupInsert 		insert;
	fourTuple			toeTuple;
	threeTupleInternal	releaseTuple;
	ap_uint<16>			sessionID;

	if (!revTableInserts.empty())
	{
		revTableInserts.read(insert);
		reverseLookupTable[insert.key] = insert.value;
		tupleValid[insert.key] = true;

	}
	// TODO check if else if necessary
	else if (!stateTable2sLookup_releaseSession.empty())
	{
		stateTable2sLookup_releaseSession.read(sessionID);
		releaseTuple = reverseLookupTable[sessionID];
		if (tupleValid[sessionID]) // if valid
		{
			sLookup2portTable_releasePort.write(releaseTuple.myPort);
			deleteCache.write(rtlSessionUpdateRequest(releaseTuple, sessionID, DELETE, RX));
		}
		tupleValid[sessionID] = false;
	}
	else if (!txEng2sLookup_rev_req.empty())
	{
		txEng2sLookup_rev_req.read(sessionID);
		toeTuple.srcIp = myIpAddress;
		toeTuple.dstIp = reverseLookupTable[sessionID].theirIp;
		toeTuple.srcPort = reverseLookupTable[sessionID].myPort;
		toeTuple.dstPort = reverseLookupTable[sessionID].theirPort;
		sLookup2txEng_rev_rsp.write(toeTuple);
	}
}

/** @ingroup session_lookup_controller
 *  This module acts as a wrapper for the RTL implementation of the SessionID Table.
 *  It also includes the wrapper for the sessionID free list which keeps track of the free SessionIDs
 *  @param[in]		rxLookupIn
 *  @param[in]		stateTable2sLookup_releaseSession
 *  @param[in]		txAppLookupIn
 *  @param[in]		txLookup
 *  @param[in]		lookupIn
 *  @param[in]		updateIn
 *  @param[out]		rxLookupOut
 *  @param[out]		portReleaseOut
 *  @param[out]		txAppLookupOut
 *  @param[out]		txResponse
 *  @param[out]		lookupOut
 *  @param[out]		updateOut
 *  @TODO rename
 */
void session_lookup_controller(	stream<sessionLookupQuery>&			rxEng2sLookup_req,
								stream<sessionLookupReply>&			sLookup2rxEng_rsp,
								stream<ap_uint<16> >&				stateTable2sLookup_releaseSession,
								stream<ap_uint<16> >&				sLookup2portTable_releasePort,
								stream<fourTuple>&					txApp2sLookup_req,
								stream<sessionLookupReply>&			sLookup2txApp_rsp,
								stream<ap_uint<16> >&				txEng2sLookup_rev_req,
								stream<fourTuple>&					sLookup2txEng_rev_rsp,
								stream<rtlSessionLookupRequest>&	sessionLookup_req,
								stream<rtlSessionLookupReply>&		sessionLookup_rsp,
								stream<rtlSessionUpdateRequest>&	sessionUpdate_req,
								//stream<rtlSessionUpdateRequest>&	sessionInsert_req,
								//stream<rtlSessionUpdateRequest>&	sessionDelete_req,
								stream<rtlSessionUpdateReply>&		sessionUpdate_rsp,
								ap_uint<16>& regSessionCount,
								ap_uint<32>						myIpAddress)
{
//#pragma HLS DATAFLOW
#pragma HLS INLINE

	// Fifos
	static stream<sessionLookupQueryInternal> slc_lookups("slc_lookups");
	#pragma HLS stream variable=slc_lookups depth=4
	#pragma HLS aggregate  variable=slc_lookups compact=bit

	static stream<ap_uint<14> > slc_sessionIdFreeList("slc_sessionIdFreeList");
	static stream<ap_uint<14> > slc_sessionIdFinFifo("slc_sessionIdFinFifo");
	#pragma HLS stream variable=slc_sessionIdFreeList depth=16384
	#pragma HLS stream variable=slc_sessionIdFinFifo depth=2

	static stream<rtlSessionUpdateReply>	slc_sessionInsert_rsp("slc_sessionInsert_rsp");
	#pragma HLS STREAM variable=slc_sessionInsert_rsp depth=4

	static stream<rtlSessionUpdateRequest>  sessionInsert_req("sessionInsert_req");
	#pragma HLS STREAM variable=sessionInsert_req depth=4

	static stream<rtlSessionUpdateRequest>  sessionDelete_req("sessionDelete_req");
	#pragma HLS STREAM variable=sessionDelete_req depth=4

	static stream<revLupInsert>				reverseLupInsertFifo("reverseLupInsertFifo");
	#pragma HLS STREAM variable=reverseLupInsertFifo depth=4


	lookupReplyHandler(	sessionLookup_rsp,
						slc_sessionInsert_rsp,
						rxEng2sLookup_req,
						txApp2sLookup_req,
						slc_sessionIdFreeList,
						sessionLookup_req,
						sLookup2rxEng_rsp,
						sLookup2txApp_rsp,
						sessionInsert_req,
						reverseLupInsertFifo
						);//regSessionCount);

	updateRequestSender(sessionInsert_req,
						sessionDelete_req,
						sessionUpdate_req,
						slc_sessionIdFinFifo,
						regSessionCount);

	sessionIdManager(slc_sessionIdFreeList, slc_sessionIdFinFifo);

	updateReplyHandler(	sessionUpdate_rsp,
						slc_sessionInsert_rsp);

	reverseLookupTableInterface(	reverseLupInsertFifo,
									stateTable2sLookup_releaseSession,
									txEng2sLookup_rev_req,
									sLookup2portTable_releasePort,
									sessionDelete_req,
									sLookup2txEng_rev_rsp,
									myIpAddress);
}
