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
#include "rx_app_if.hpp"

using namespace hls;

/** @ingroup rx_app_if
 *  This application interface is used to open passive connections
 *  @param[in]		appListeningIn
 *  @param[in]		appStopListeningIn
 *  @param[in]		rxAppPorTableListenIn
 *  @param[in]		rxAppPortTableCloseIn
 *  @param[out]		appListeningOut
 *  @param[out]		rxAppPorTableListenOut
 */
// TODO this does not seem to be very necessary
void rx_app_if(	stream<ap_uint<16> >&				appListenPortReq,
				// This is disabled for the time being, because it adds complexity/potential issues
				//stream<ap_uint<16> >&				appStopListeningIn,
				stream<bool>&						portTable2rxApp_listen_rsp,
				stream<bool>&						appListenPortRsp,
				stream<ap_uint<16> >&				rxApp2portTable_listen_req)
				//stream<ap_uint<16> >&				rxAppPortTableCloseIn,)
{
#pragma HLS PIPELINE II=1

	static bool rai_wait = false;

	static ap_uint<16> rai_counter = 0;

	ap_uint<16> tempPort;
	ap_uint<16> listenPort;
	bool listening;

	// TODO maybe do a state machine
	// Listening Port Open, why not asynchron??
	if (!appListenPortReq.empty() && !rai_wait)
	{
		//appListenPortReq.read(tempPort);
		//listenPort(7, 0) = tempPort(15, 8);
		//listenPort(15, 8) = tempPort(7, 0);
		rxApp2portTable_listen_req.write(appListenPortReq.read());
		rai_wait = true;
	}
	else if (!portTable2rxApp_listen_rsp.empty() && rai_wait)
	{
		portTable2rxApp_listen_rsp.read(listening);
		appListenPortRsp.write(listening);
		rai_wait = false;
	}

	// Listening Port Close
	/*if (!appStopListeningIn.empty())
	{
		rxAppPortTableCloseIn.write(appStopListeningIn.read());
	}*/
}
