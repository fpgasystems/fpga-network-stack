/************************************************
Copyright (c) 2018, Systems Group, ETH Zurich.
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
#include "iperf.hpp"
#include <iostream>

using namespace hls;

int main()
{

	stream<ap_uint<16> > listenPort("listenPort");
	stream<bool> listenPortStatus("listenPortStatus");
	stream<appNotification> notifications;
	stream<appReadRequest> readRequest;
	stream<ap_uint<16> > rxMetaData;
	stream<axiWord> rxData;
	stream<ipTuple> openConnection("openConnection");
	stream<openStatus> openConStatus;
	stream<ap_uint<16> > closeConnection;
	stream<ap_uint<16> > txMetaData;
	stream<axiWord> txData;
	stream<ap_int<17> >	txStatus;


	axiWord dualHeader0;
	axiWord dualHeader1;
	axiWord dualHeader2;
	dualHeader0.data = 0x0100000000000080;
	dualHeader0.keep = 0xff;
	dualHeader0.last = 0;
	dualHeader1.data = 0x0000000089130000;
	dualHeader1.keep = 0xff;
	dualHeader1.last = 0;
	dualHeader2.data = 0x9cffffff00000000;
	dualHeader2.keep = 0xff;
	dualHeader2.last = 0;
	axiWord currWord;
	currWord.data = 0x3736353433323130;
	currWord.keep = 0xff;
	currWord.last = 0;
	int count = 0;
	while (count < 100)
	{
		iperf(	listenPort, listenPortStatus,
				notifications, readRequest,
				rxMetaData, rxData,
				openConnection, openConStatus,
				closeConnection,
				txMetaData, txData,
				txStatus);
		if (!listenPort.empty())
		{
			listenPort.read();
			listenPortStatus.write(true);
		}
		if (!readRequest.empty())
		{
			readRequest.read();
			rxMetaData.write(0);
			rxData.write(dualHeader0);
			rxData.write(dualHeader1);
			rxData.write(dualHeader2);
			currWord.last = 0;
			rxData.write(currWord);
			currWord.last = 1;
			rxData.write(currWord);
		}
		if (count % 7 == 0 && count < 50)
		{
			notifications.write(appNotification(0, 16, 0x0a010101, 5001));
		}
		count++;
	}
	return 0;
}
