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
#include "multi_queue.hpp"
#include <vector>
#include <queue>
//#include <random>

struct TestValue
{
	ap_uint<32> abc;
	ap_uint<32> cba;
};

int main()
{
	hls::stream<mqInsertReq<TestValue> >	multiQueue_push("multiQueue_push");
	hls::stream<mqPopReq>					multiQueue_pop_req("multiQueue_pop_req");
	hls::stream<TestValue>					multiQueue_rsp("multiQueue_rsp");

	const int NUM_QUEUES = 128;
	const int QUEUE_SIZE = 2048;

	ap_uint<16> key = 13;

	int count = 0;
	while (count < 100000)
	{
		if (count < 10)
		{
			TestValue t;
			t.abc = count+16;
			t.cba = count+16;
			mqInsertReq<TestValue> insert = mqInsertReq<TestValue>(key, t);
			multiQueue_push.write(insert);
		}
		if (count >= 10 && count < 30)
		{
			if (count % 2 == 0) {
				multiQueue_pop_req.write(mqPopReq(POP,key));
			} else {
				multiQueue_pop_req.write(mqPopReq(FRONT, key));
			}
		}

		multi_queue<TestValue, NUM_QUEUES, QUEUE_SIZE>(	multiQueue_push,
														multiQueue_pop_req,
														multiQueue_rsp);
		count++;
	}

	TestValue ret;
	while (!multiQueue_rsp.empty())
	{
		multiQueue_rsp.read(ret);
		std::cout << "abc: " << ret.abc << ", cba: " << ret.cba << std::endl;
	}

	return 0;
}
