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
#include "retransmitter.hpp"
#include <iostream>

int main()
{

	static stream<retransRelease>	rx2retrans_release_upd("rx2retrans_release_upd");
	static stream<retransmission> rx2retrans_req("rx2retrans_req");
	static stream<retransmission> timer2retrans_req("timer2retrans_req");
	static stream<retransEntry>	tx2retrans_insertRequest("tx2retrans_insertRequest");
	static stream<event>			retrans2event("retrans2event");

	int count = 0;
	while (count < 1000)
	{
		if (count == 5)
		{
			tx2retrans_insertRequest.write(retransEntry(0x11, 0xabcdea, RC_RDMA_WRITE_ONLY, 0xfeea, 16));
			tx2retrans_insertRequest.write(retransEntry(0x11, 0xabcdeb, RC_RDMA_READ_REQUEST, 0xfeeb, 16));
			tx2retrans_insertRequest.write(retransEntry(0x11, 0xabcdec, RC_RDMA_WRITE_ONLY, 0xfeec, 16));
			tx2retrans_insertRequest.write(retransEntry(0x11, 0xabcded, RC_RDMA_READ_REQUEST, 0xfeed, 16));
			tx2retrans_insertRequest.write(retransEntry(0x11, 0xabcdee, RC_RDMA_WRITE_ONLY, 0xfeec, 16));
			tx2retrans_insertRequest.write(retransEntry(0x11, 0xabcdef, RC_RDMA_READ_REQUEST, 0xfeed, 16));
		}
		if (count == 20)
		{
			timer2retrans_req.write(retransmission(0x11));
		}

		if (count == 25)
		{
			rx2retrans_req.write(retransmission(0x11, 0xabcdec));
		}

		//release
		if (count == 30)
		{
			rx2retrans_release_upd(retransRelease(0x11, 0xabcdec));
		}

		//trigger again
		if (count == 20)
		{
			timer2retrans_req.write(retransmission(0x11));
		}

		if (count == 25)
		{
			rx2retrans_req.write(retransmission(0x11, 0xabcdec));
		}

		if (count == 0)
		void retransmitter(	rx2retrans_release_upd,
							rx2retrans_req,
							timer2retrans_req,
							tx2retrans_insertRequest,
							retrans2event);

		count++;
	}

	event ev;
	while(!retrans2event.empty())
	{
		retrans2event.read(ev);
		std::cout << "Retransmission of qp " << ev.qpn << ", addr: " << ev.addr << ", len: " << ev.length << std::endl;
	}

	return 0;
}
