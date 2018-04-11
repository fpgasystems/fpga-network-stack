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
#include "iperf_client.hpp"
#include <iostream>

using namespace hls;


int main()
{
	stream<ap_uint<16> > listenPort("listenPort");
	stream<bool> listenPortStatus("listenPortStatus");
	stream<appNotification> notifications("notifications");
	stream<appReadRequest> readRequest("readRequest");
	stream<ap_uint<16> > rxMetaData("rxMetaData");
	stream<axiWord> rxData("rxData");
	stream<ipTuple> openConnection("openConnection");
	stream<openStatus> openConStatus("openConStatus");
	stream<ap_uint<16> > closeConnection("closeConnection");
	stream<ap_uint<16> > txMetaData("txMetaData");
	stream<axiWord> txData("txData");
	stream<ap_int<17> > txStatus("txStatus");
	ap_uint<1> runExperiment;
	ap_uint<1> dualModeEn;
	ap_uint<13> useConn;
	ap_uint<8> pkgWordCount;
	ap_uint<32> ipAddress0 = 0x01010101;
	ap_uint<32> ipAddress1 = 0x01010102;
	ap_uint<32> ipAddress2 = 0x01010103;
	ap_uint<32> ipAddress3 = 0x01010104;

	ap_uint<16> sessionID;
	axiWord currWord;
	int count = 0;
	dualModeEn = 0;
	pkgWordCount = 8;

	while (count < 10000)
	{
		useConn = 2;
		runExperiment = 0;
		if (count == 20)
		{
			runExperiment = 1;
		}
		iperf_client(	listenPort,
						listenPortStatus,
						notifications,
						readRequest,
						rxMetaData,
						rxData,
						openConnection,
						openConStatus,
						closeConnection,
						txMetaData,
						txData,
						txStatus,
						runExperiment,
						dualModeEn,
						useConn,
						pkgWordCount,
						ipAddress0,
						ipAddress1,
						ipAddress2,
						ipAddress3);

		if (!openConnection.empty())
		{
			openConnection.read();
			std::cout << "Opening connection.. at cycle" << count << std::endl;
			openConStatus.write(openStatus(123+count, true));
		}
		if (!txMetaData.empty())
		{
			txMetaData.read(sessionID);
			std::cout << "New Pkg: " << std::dec << sessionID << std::endl;
		}
		while (!txData.empty())
		{
			txData.read(currWord);
			std::cout << std::hex << std::noshowbase;
			std::cout << std::setfill('0');
			std::cout << std::setw(8) << ((uint32_t) currWord.data(63, 32));
			std::cout << std::setw(8) << ((uint32_t) currWord.data(31, 0));
			std::cout << " " << std::setw(2) << ((uint32_t) currWord.keep) << " ";
			std::cout << std::setw(1) << ((uint32_t) currWord.last) << std::endl;
		}
		if (!closeConnection.empty())
		{
			closeConnection.read(sessionID);
			std::cout << "Closing connection: " << std::dec << sessionID << std::endl;
		}
		count++;
	}
	return 0;
}
