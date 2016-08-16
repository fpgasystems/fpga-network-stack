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
#include "session_lookup_controller.hpp"
#include <map>

using namespace hls;

void sessionLookupStub(stream<rtlSessionLookupRequest>& lup_req, stream<rtlSessionLookupReply>& lup_rsp,
						stream<rtlSessionUpdateRequest>& upd_req, stream<rtlSessionUpdateReply>& upd_rsp)
						//stream<ap_uint<14> >& new_id, stream<ap_uint<14> >& fin_id)
{
	static std::map<fourTupleInternal, ap_uint<14> > lookupTable;

	rtlSessionLookupRequest request;
	rtlSessionUpdateRequest update;

	std::map<fourTupleInternal, ap_uint<14> >::const_iterator findPos;

	if (!lup_req.empty())
	{
		lup_req.read(request);
		findPos = lookupTable.find(request.key);
		if (findPos != lookupTable.end()) //hit
		{
			lup_rsp.write(rtlSessionLookupReply(true, findPos->second, request.source));
		}
		else
		{
			lup_rsp.write(rtlSessionLookupReply(false, request.source));
		}
	}

	if (!upd_req.empty()) //TODO what if element does not exist
	{
		upd_req.read(update);
		if (update.op == INSERT) //Is there a check if it already exists?
		{
			// Read free id
			//new_id.read(update.value);
			lookupTable[update.key] = update.value;
			upd_rsp.write(rtlSessionUpdateReply(update.value, INSERT, update.source));

		}
		else // DELETE
		{
			//fin_id.write(update.value);
			lookupTable.erase(update.key);
			upd_rsp.write(rtlSessionUpdateReply(update.value, DELETE, update.source));
		}
	}
}

/*void lookupRequestMerger(	stream<sessionLookupQuery>&				rxLookupIn,
					stream<fourTuple>&						txAppLookupIn,
					stream<rtlSessionLookupRequest>&		rtlLookupIn,
					stream<sessionLookupQueryInternal>&		lookups)
{
	fourTuple hlsTuple;
	fourTupleInternal rtlTuple;
	sessionLookupQuery query;

	if (!txAppLookupIn.empty())
	{
		txAppLookupIn.read(hlsTuple);
		rtlTuple.theirIp = hlsTuple.dstIp;
		rtlTuple.theirPort = hlsTuple.dstPort;
		rtlTuple.myIp = hlsTuple.srcIp;
		rtlTuple.myPort = hlsTuple.srcPort;
		rtlLookupIn.write(rtlSessionLookupRequest(rtlTuple, TX_APP));
		lookups.write(sessionLookupQueryInternal(rtlTuple, true)); //FIXME change this
	}
	else if (!rxLookupIn.empty())
	{
		rxLookupIn.read(query);
		rtlTuple.theirIp = query.tuple.srcIp;
		rtlTuple.theirPort = query.tuple.srcPort;
		rtlTuple.myIp = query.tuple.dstIp;
		rtlTuple.myPort = query.tuple.dstPort;
		rtlLookupIn.write(rtlSessionLookupRequest(rtlTuple, RX));
		lookups.write(sessionLookupQueryInternal(rtlTuple, query.allowCreation));
	}
}*/

void updateRequestMerger(stream<rtlSessionUpdateRequest>& in1, stream<rtlSessionUpdateRequest>& in2, stream<rtlSessionUpdateRequest>& out)
{
	if (!in1.empty())
	{
		out.write(in1.read());
	}
	else if (!in2.empty())
	{
		out.write(in2.read());
	}
}

int main()
{
	stream<sessionLookupQuery>			rxEng2sLookup_req;
	stream<sessionLookupReply>			sLookup2rxEng_rsp("sLookup2rxEng_rsp");
	stream<ap_uint<16> >				stateTable2sLookup_releaseSession;
	stream<ap_uint<16> >				sLookup2portTable_releasePort;
	stream<fourTuple>					txApp2sLookup_req;
	stream<sessionLookupReply>			sLookup2txApp_rsp;
	stream<ap_uint<16> >				txEng2sLookup_rev_req;
	stream<fourTuple>					sLookup2txEng_rev_rsp;
	stream<rtlSessionLookupRequest>		sessionLookup_req;
	stream<rtlSessionLookupReply>		sessionLookup_rsp("test_sessionLookup_rsp");
	stream<rtlSessionUpdateRequest>		sessionUpdate_req;
	stream<rtlSessionUpdateReply>		sessionUpdate_rsp("test_sessionUpdate_rsp");

	stream<rtlSessionUpdateRequest>		sessionInsert_req;
	stream<rtlSessionUpdateRequest>		sessionDelete_req;



	//stream<sessionLookupQueryInternal> lookups("lookups");

	ap_uint<16> regSessionCount;

	int count = 0;
	fourTuple tuple;
	tuple.dstIp = 0x01010101;
	tuple.dstPort = 7;
	tuple.srcIp = 0x0101010a;
	tuple.srcPort = 3489;

	int lastSessionCount = 0;
	while (count < 500)
	{
		if (count == 20)
		{
			rxEng2sLookup_req.write(sessionLookupQuery(tuple, false));
		}

		if (count == 70)
		{
			rxEng2sLookup_req.write(sessionLookupQuery(tuple, true));
		}

		if (count == 80)
		{
			txEng2sLookup_rev_req.write(0);
		}

		if (count == 90)
		{
			txApp2sLookup_req.write(tuple);
		}
		session_lookup_controller(	//lookups,
									rxEng2sLookup_req,
									sLookup2rxEng_rsp,
									stateTable2sLookup_releaseSession,
									sLookup2portTable_releasePort,
									txApp2sLookup_req,
									sLookup2txApp_rsp,
									txEng2sLookup_rev_req,
									sLookup2txEng_rev_rsp,
									sessionLookup_req,
									sessionLookup_rsp,
									sessionUpdate_req,
									//sessionInsert_req,
									//sessionDelete_req,
									sessionUpdate_rsp,
									regSessionCount);
		//lookupRequestMerger(rxEng2sLookup_req, txApp2sLookup_req, sessionLookup_req, lookups);
		//updateRequestMerger(sessionInsert_req, sessionDelete_req, sessionUpdate_req);
		if ( lastSessionCount != regSessionCount)
		{
			std::cout << "SessionCount\t" << regSessionCount << std::endl;
			lastSessionCount = regSessionCount;
		}
		sessionLookupStub(sessionLookup_req, sessionLookup_rsp,	sessionUpdate_req, sessionUpdate_rsp);
		count++;
	}

	sessionLookupReply reply;
	while (!sLookup2rxEng_rsp.empty())
	{
		sLookup2rxEng_rsp.read(reply);
		std::cout << "rxEng "<< reply.sessionID << "\t" << reply.hit << std::endl;
	}
	while (!sLookup2txApp_rsp.empty())
	{
		sLookup2txApp_rsp.read(reply);
		std::cout << "txApp "<< reply.sessionID << "\t" << reply.hit << std::endl;
	}
	while (!sLookup2txEng_rev_rsp.empty())
	{
		sLookup2txEng_rev_rsp.read(tuple);
		std::cout << "txEng " << tuple.dstIp << ":" << tuple.dstPort << "\t " << tuple.srcIp << ":" << tuple.srcPort << std::endl;
	}

	return 0;
}
