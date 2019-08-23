/************************************************
Copyright (c) 2019, Systems Group, ETH Zurich.
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
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
************************************************/
#pragma once

#include "../../axi_utils.hpp"

template <class T>
struct mqInsertReq
{
	ap_uint<16> key;
	T			value;
	mqInsertReq() {}
	mqInsertReq(ap_uint<16> key, T value)
		:key(key), value(value) {}
};

enum mqPopOperation {POP, FRONT};

struct mqPopReq
{
	mqPopOperation	op;
	ap_uint<16>		key;
	mqPopReq() {}
	mqPopReq(ap_uint<16> key)
		:op(POP), key(key) {}
	mqPopReq(mqPopOperation op, ap_uint<16> key)
			:op(op), key(key) {}
};

struct mqPointerEntry
{
	ap_uint<16>	head;
	ap_uint<16> tail;
	bool valid;
};

template <class T>
struct mqMetaEntry
{
	T value;
	ap_uint<16> next;
	bool valid;
	bool isTail;
	mqMetaEntry() {}
	mqMetaEntry(mqInsertReq<T>& e)
		:value(e.value), next(0), valid(true), isTail(true) {}
	mqMetaEntry(ap_uint<16> next)
		:next(next) {}
};

template <class T>
struct mqMetaReq
{
	ap_uint<16> idx;
	mqMetaEntry<T> entry;
	bool write;
	bool append;
	mqMetaReq() {}
	mqMetaReq(ap_uint<16> idx)
		:idx(idx), write(false),  append(false) {}
	mqMetaReq(ap_uint<16> idx, ap_uint<16> next)
		:idx(idx), entry(mqMetaEntry<T>(next)), write(false), append(true) {}
	mqMetaReq(ap_uint<16> idx, mqMetaEntry<T> e)
		:idx(idx), entry(e), write(true), append(false) {}
};

struct mqPointerReq
{
	ap_uint<16>	key;
	bool		lock;
	//bool		write;
	mqPointerReq() {}
	mqPointerReq(ap_uint<16> key)
		:key(key), lock(false) {}
	mqPointerReq(ap_uint<16> key, bool l)
		:key(key), lock(l) {}
};

struct mqPointerUpdate
{
	ap_uint<16>	key;
	mqPointerEntry entry;
	mqPointerUpdate() {}
	mqPointerUpdate(ap_uint<16> key, mqPointerEntry e)
		:key(key), entry(e) {}
};

//Key: 16bit
//Value: T


template <int NUM_QUEUES>
void mq_pointer_table(	hls::stream<mqPointerReq>&			pointerReqFifo,
						hls::stream<mqPointerUpdate>&		pointerUpdFifo,
						hls::stream<mqPointerEntry>& 		pointerRspFifo)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	static mqPointerEntry ptr_table[NUM_QUEUES];
	#pragma HLS RESOURCE variable=ptr_table core=RAM_T2P_BRAM
	//#pragma HLS DEPENDENCE variable=ptr_table inter false

	static ap_uint<16> mq_lockedKey;
	static bool mq_isLocked = false;
	static bool mq_wait = false;
	static mqPointerReq mq_request;

	mqPointerUpdate udpate;

	if (!pointerUpdFifo.empty())
	{
		pointerUpdFifo.read(udpate);
		ptr_table[udpate.key] = udpate.entry;
		if (mq_lockedKey == udpate.key)
		{
			mq_isLocked = false;
		}
	}
	else if (!pointerReqFifo.empty() && !mq_wait)
	{
		pointerReqFifo.read(mq_request);
		if (mq_request.lock && mq_isLocked)
		{
			mq_wait = true;
		}
		else
		{
			pointerRspFifo.write(ptr_table[mq_request.key]);
			if (mq_request.lock)
			{
				mq_isLocked = true;
				mq_lockedKey = mq_request.key;
			}
		}
	}
	else if (mq_wait)
	{
		if (!mq_isLocked)
		{
			pointerRspFifo.write(ptr_table[mq_request.key]);
			mq_isLocked = true;
			mq_lockedKey = mq_request.key;
			mq_wait = false;
		}
	}
}

template <int MULTI_QUEUE_SIZE>
void mq_freelist_handler(	hls::stream<ap_uint<16> >& rt_releaseFifo,
						hls::stream<ap_uint<16> >& rt_freeListFifo)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	static ap_uint<16> freeListCounter = 0;
	#pragma HLS reset variable=freeListCounter

	if (!rt_releaseFifo.empty())
	{
		rt_freeListFifo.write(rt_releaseFifo.read());
	}
	else if (freeListCounter < MULTI_QUEUE_SIZE && !rt_freeListFifo.full())
	{
		rt_freeListFifo.write(freeListCounter);
		freeListCounter++;
	}
}

template <class T, int MULTI_QUEUE_SIZE>
void mq_meta_table(	hls::stream<mqMetaReq<T> >&		meta_upd_req,
					hls::stream<mqMetaEntry<T> >&		meta_rsp)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	static mqMetaEntry<T> meta_table[MULTI_QUEUE_SIZE];
	#pragma HLS RESOURCE variable=meta_table core=RAM_T2P_BRAM
	//#pragma HLS DEPENDENCE variable=meta_table inter false
	mqMetaReq<T> req;

	if (!meta_upd_req.empty())
	{
		meta_upd_req.read(req);
		if (req.write)
		{
			meta_table[req.idx] = req.entry;
		}
		else if (req.append)
		{
			meta_table[req.idx].next = req.entry.next;
			meta_table[req.idx].isTail = false;
		}
		else
		{
			//entry = meta_table[req.idx];
			meta_rsp.write(meta_table[req.idx]);
		}
	}



}

template <class T>
void mq_process_requests(	//stream<retransRelease>&	rx2retrans_release_upd,
					//stream<retransmission>& rx2retrans_req,
					hls::stream<mqPopReq>&			multiQueue_pop_req, //stream<retransmission>& timer2retrans_req, //pop
					hls::stream<mqInsertReq<T> >&	multiQueue_push, //stream<retransEntry>&	tx2retrans_insertRequest, //push
					hls::stream<mqPointerReq>&		pointerReqFifo,
					hls::stream<mqPointerUpdate>&	pointerUpdFifo,
					hls::stream<mqPointerEntry>& 	pointerRspFifo, //TODO reorder
					hls::stream<mqMetaReq<T> >&		metaReqFifo,
					hls::stream<mqMetaEntry<T> >&	metaRspFifo,
					hls::stream<ap_uint<16> >&		freeListFifo,
					hls::stream<ap_uint<16> >&		releaseFifo,
					//stream<bool>&	stopFifo,
					hls::stream<T>&		multiQueue_pop_rsp) //stream<retransEvent>&				retrans2event)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	enum retransStateType {STATE_MAIN, STATE_PUSH, STATE_PUSH_2, STATE_POP, STATE_POP_2}; //FLUSH_META_PIPE};
		static retransStateType rt_state = STATE_MAIN;
		//static ap_uint<16> freeListCounter = 0;
		//static retransRelease release;
		//static ap_uint<16> curr;
		//static ap_uint<16> relCurr;
		static ap_uint<16> newMetaIdx;
		static mqInsertReq<T> insert; //static retransEntry insert;
		//static retransmission retrans;
		static mqPopReq popRequest;
		static mqMetaEntry<T> meta; //TODO register needed??
		static mqPointerEntry ptrMeta;
		//static ap_uint<16> prevIdx;

		switch (rt_state)
		{
		case STATE_MAIN:
			/*if (!rx2retrans_release_upd.empty())
			{
				rx2retrans_release_upd.read(release);
				pointerReqFifo.write(pointerReq(release.qpn, true));
				rt_state = RELEASE_0;
			}
			else*/ if (!multiQueue_push.empty() && !freeListFifo.empty())
			{
				newMetaIdx = freeListFifo.read();
				multiQueue_push.read(insert);
				pointerReqFifo.write(mqPointerReq(insert.key, true));
				rt_state = STATE_PUSH;
			}
			/*else if (!rx2retrans_req.empty())
			{
				rx2retrans_req.read(retrans);
				std::cout << "RX Retransmit triggered!! , psn: " << std::hex << retrans.psn << std::endl;
				pointerReqFifo.write(pointerReq(release.qpn));
				rt_state = RETRANS_0;
			}*/
			else if (!multiQueue_pop_req.empty())
			{
				multiQueue_pop_req.read(popRequest);
				if (popRequest.op == POP) {
					std::cout << "pop(" << popRequest.key << ")\n";
				} else {
					std::cout << "front(" << popRequest.key << ")\n";
				}
				pointerReqFifo.write(mqPointerReq(popRequest.key));
				rt_state = STATE_POP;
			}
			break;
		case STATE_PUSH:
			if (!pointerRspFifo.empty())
			{
				pointerRspFifo.read(ptrMeta);
				if (!ptrMeta.valid)
				{
					//init entry
					ptrMeta.valid = true;
					ptrMeta.head = newMetaIdx;
					ptrMeta.tail = newMetaIdx;
					metaReqFifo.write(mqMetaReq<T>(newMetaIdx, mqMetaEntry<T>(insert)));
					pointerUpdFifo.write(mqPointerUpdate(insert.key, ptrMeta));
					rt_state = STATE_MAIN;
				}
				else
				{
					//meta_table[ptrMeta.tail].next = newPtr;
					//meta_table[ptrMeta.tail].isTail = false;
					//Append new pointer to tail
					metaReqFifo.write(mqMetaReq<T>(ptrMeta.tail, newMetaIdx));
					ptrMeta.tail = newMetaIdx;
					rt_state = STATE_PUSH_2;
				}
				//Write back, TODO move to insert2??
				//pointerUpdFifo.write(pointerUpdate(insert.qpn, ptrMeta));
			}
			break;
		case STATE_PUSH_2:
			metaReqFifo.write(mqMetaReq<T>(newMetaIdx, mqMetaEntry<T>(insert)));
			pointerUpdFifo.write(mqPointerUpdate(insert.key, ptrMeta));
			//meta_table[newPtr] = insert;
			rt_state = STATE_MAIN;
			break;
		/*case RELEASE_0:
			if (!pointerRspFifo.empty())
			{
				std::cout << "releasing: " << release.latest_acked_req << std::endl;
				pointerRspFifo.read(ptrMeta);
				if (ptrMeta.valid)
				{
					//Get continuous stream of meta entries
					metaReqFifo.write(retransMetaReq(ptrMeta.head));
					curr = ptrMeta.head;
					//meta = meta_table[curr];
					rt_state = RELEASE_1;
				}
				else
				{
					std::cout << "RELEASE_0: invalid meta entry\n";
					//Release lock
					pointerUpdFifo.write(pointerUpdate(release.qpn, ptrMeta));
					rt_state = MAIN;
				}
			}
			break;
		case RELEASE_1:
			if (!metaRspFifo.empty())
			{
				//TODO rearrange this thing
				metaRspFifo.read(meta);

				//TODO this should never occur: meta.entry.valid
				std::cout << "meta.psn: " << meta.psn << ", latest acked req: " << release.latest_acked_req << std::endl;
				if (!meta.valid || (meta.psn == release.latest_acked_req))
				{
					if (meta.psn == release.latest_acked_req)
					{
						std::cout << "release success" << std::endl;
					}
					ptrMeta.head = meta.next;
					ptrMeta.valid = !meta.isTail;
					pointerUpdFifo.write(pointerUpdate(release.qpn, ptrMeta));
					//ptr_table[release.qpn] = ptrs;
					//stopFifo.write(true);
					rt_state = MAIN;
				}
				else
				{
					if (meta.isTail)
					{
						ptrMeta.valid = false;
						ptrMeta.head = curr;
						pointerUpdFifo.write(pointerUpdate(release.qpn, ptrMeta));
						//ptr_table[release.qpn] = ptrs;
						//stopFifo.write(true);
						rt_state = MAIN;
					}
					else
					{
	               metaReqFifo.write(retransMetaReq(meta.next));
						//rt_freeListFifo.write(curr); //TODO check correctness
						//rt_state = RELEASE_2;
					}
					curr = meta.next;
				}
				if (meta.valid)
				{
					releaseFifo.write(curr);
				}
			}
			break;
		case RETRANS_0:
			if (!pointerRspFifo.empty())
			{
				pointerRspFifo.read(ptrMeta);
				//curr = ptrMeta.head;
				rt_state = MAIN;
				if (ptrMeta.valid)
				{
					//Get continuous stream of meta entries
					metaReqFifo.write(retransMetaReq(ptrMeta.head));
					curr = ptrMeta.head;
					//meta = meta_table[curr];
					rt_state = RETRANS_1;
				}
			}
			break;
		case RETRANS_1:
			//Find PSN of interest and start retransmitting
			if (!metaRspFifo.empty())
			{
				metaRspFifo.read(meta);
				if (meta.valid)
				{
					if (!meta.isTail)
					{
						metaReqFifo.write(retransMetaReq(meta.next));
					}
					else
					{
						rt_state = MAIN;
					}
					//Check if we should start retransmitting
					if (meta.psn == retrans.psn)
					{
						//Generate event
						std::cout << std::hex << "retransmitting opcode: " << meta.opCode << ", local addr: " << meta.localAddr << ", remote addr: " << meta.remoteAddr << ", length: " << meta.length << std::endl;
						retrans2event.write(retransEvent(meta.opCode, retrans.qpn, meta.localAddr, meta.remoteAddr, meta.length, meta.psn));
						if (!meta.isTail)
						{
							rt_state = RETRANS_2;
						}
					}
				}
				else
				{
					rt_state = MAIN;
				}
			}
			break;
		case RETRANS_2:
			//Retransmit everything until we reach tail
			if (!metaRspFifo.empty())
			{
				metaRspFifo.read(meta);
				if (meta.valid) // && meta.psn == retrans.psn)
				{
					if (!meta.isTail)
					{
						metaReqFifo.write(retransMetaReq(meta.next));
					}
					else
					{
						rt_state = MAIN;
					}
					//Generate event
					std::cout << std::hex << "retransmitting opcode: " << meta.opCode << ", local addr: " << meta.localAddr << ", remote addr: " << meta.remoteAddr << ", length: " << meta.length << std::endl;
					retrans2event.write(retransEvent(meta.opCode, retrans.qpn, meta.localAddr, meta.remoteAddr, meta.length, meta.psn));
				}
				else
				{
					rt_state = MAIN;
				}
			}
			break;*/
		case STATE_POP:
			if (!pointerRspFifo.empty())
			{
				pointerRspFifo.read(ptrMeta);
				if (ptrMeta.valid)
				{
					metaReqFifo.write(mqMetaReq<T>(ptrMeta.head));
					rt_state = STATE_POP_2;
					//meta = meta_table[ptrMeta.head];
					//retrans2event.write(event(meta.opCode, retrans.qpn, meta.vaddr, meta.length, meta.psn));
				}
				else
				{
					rt_state = STATE_MAIN;
				}
			}
			break;
		case STATE_POP_2:
			if (!metaRspFifo.empty())
			{
				metaRspFifo.read(meta);
				if (meta.valid)
				{
					multiQueue_pop_rsp.write(meta.value);
					if (popRequest.op == POP)
					{
						releaseFifo.write(ptrMeta.head);
						ptrMeta.valid = !meta.isTail;
						ptrMeta.head = meta.next;
						pointerUpdFifo.write(mqPointerUpdate(popRequest.key, ptrMeta));
					}
					//retrans2event.write(retransEvent(meta.opCode, retrans.qpn, meta.localAddr, meta.remoteAddr, meta.length, meta.psn));
				}
				rt_state = STATE_MAIN;
			}
			break;
		/*case FLUSH_META_PIPE:
			if (!metaRspFifo.empty())
			{
				metaRspFifo.read(meta);
				if (meta.last)
				{
					rt_state = MAIN;
				}
			}
			break;*/
		}//switch
}

template <class T, int NUM_QUEUES, int MULTI_QUEUE_SIZE>
void multi_queue(	hls::stream<mqInsertReq<T> >&	multiQueue_push,
					hls::stream<mqPopReq>&			multiQueue_pop_req,
					hls::stream<T>&					multiQueue_pop_rsp)
{
#pragma HLS DATAFLOW
#pragma HLS INLINE

	static hls::stream<mqPointerReq>		mq_pointerReqFifo("mq_pointerReqFifo");
	static hls::stream<mqPointerUpdate>		mq_pointerUpdFifo("mq_pointerUpdFifo");
	static hls::stream<mqPointerEntry> 		mq_pointerRspFifo("mq_pointerRspFifo"); //TODO reorder
	#pragma HLS STREAM depth=2 variable=mq_pointerReqFifo
	#pragma HLS STREAM depth=2 variable=mq_pointerUpdFifo
	#pragma HLS STREAM depth=2 variable=mq_pointerRspFifo

	static hls::stream<mqMetaReq<T> >			mq_metaReqFifo("mq_metaReqFifo");
	static hls::stream<mqMetaEntry<T> >			mq_metaRspFifo("mq_metaRspFifo");
	//static stream<ap_uint<16> > rt_freeListFifo("rt_freeListFifo");
	#pragma HLS STREAM depth=2 variable=mq_metaReqFifo
	#pragma HLS STREAM depth=2 variable=mq_metaRspFifo
	//static stream<bool> rt_stopFifo("rt_stopFifo");
	//#pragma HLS STREAM depth=2 variable=rt_stopFifo

	static hls::stream<ap_uint<16> > mq_freeListFifo("mq_freeListFifo");
	static hls::stream<ap_uint<16> > mq_releaseFifo("mq_releaseFifo");
	#pragma HLS STREAM depth=MULTI_QUEUE_SIZE variable=mq_freeListFifo
	#pragma HLS STREAM depth=2 variable=mq_releaseFifo

	mq_freelist_handler<MULTI_QUEUE_SIZE>(mq_releaseFifo, mq_freeListFifo);

	mq_pointer_table<NUM_QUEUES>(mq_pointerReqFifo, mq_pointerUpdFifo, mq_pointerRspFifo);

	mq_meta_table<T,MULTI_QUEUE_SIZE>(mq_metaReqFifo,
									mq_metaRspFifo);

	mq_process_requests<T>(	multiQueue_pop_req,
							multiQueue_push,
							mq_pointerReqFifo,
							mq_pointerUpdFifo,
							mq_pointerRspFifo,
							mq_metaReqFifo,
							mq_metaRspFifo,
							mq_freeListFifo,
							mq_releaseFifo,
							multiQueue_pop_rsp);

}