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

#include "rx_app_stream_if.hpp"

/** @ingroup rx_app_stream_if
 *  This application interface is used to receive data streams of established connections.
 *  The Application polls data from the buffer by sending a readRequest. The module checks
 *  if the readRequest is valid then it sends a read request to the memory. After processing
 *  the request the MetaData containig the Session-ID is also written back.
 *  @param[in]		appRxDataReq
 *  @param[in]		rxSar2rxApp_upd_rsp
 *  @param[out]		appRxDataRspMetadata
 *  @param[out]		rxApp2rxSar_upd_req
 *  @param[out]		rxBufferReadCmd
 */
void rx_app_stream_if(hls::stream<appReadRequest>&		appRxDataReq,
					  hls::stream<rxSarAppd>&			rxSar2rxApp_upd_rsp,
					  hls::stream<ap_uint<16> >&		appRxDataRspMetadata,
					  hls::stream<rxSarAppd>&			rxApp2rxSar_upd_req,
#if !(RX_DDR_BYPASS)
					  hls::stream<mmCmd>&				rxBufferReadCmd)
#else
					  stream<ap_uint<1> >&			rxBufferReadCmd)
#endif
{
	#pragma HLS PIPELINE II=1
	#pragma HLS INLINE off


	static ap_uint<16>				rasi_readLength;
	static ap_uint<1>				rasi_fsmState 	= 0;

	switch (rasi_fsmState)
	{
		case 0:
			if (!appRxDataReq.empty())
			{
				appReadRequest	app_read_request = appRxDataReq.read();
 				// Make sure length is not 0, otherwise Data Mover will hang up
				if (app_read_request.length != 0)
				{
					// Get app pointer
					rxApp2rxSar_upd_req.write(rxSarAppd(app_read_request.sessionID));
					rasi_readLength = app_read_request.length;
					rasi_fsmState = 1;
				}
			}
			break;
		case 1:
			if (!rxSar2rxApp_upd_rsp.empty())
			{
				rxSarAppd	rxSar = rxSar2rxApp_upd_rsp.read();
				appRxDataRspMetadata.write(rxSar.sessionID);
#if !(RX_DDR_BYPASS)
				ap_uint<32> pkgAddr = 0;
				pkgAddr(29, WINDOW_BITS) = rxSar.sessionID(13, 0);
				pkgAddr(WINDOW_BITS-1, 0) = rxSar.appd;
				rxBufferReadCmd.write(mmCmd(pkgAddr, rasi_readLength));
#else
				rxBufferReadCmd.write(1);
#endif
				// Update app read pointer
				rxApp2rxSar_upd_req.write(rxSarAppd(rxSar.sessionID, rxSar.appd+rasi_readLength)); // Update me
				rasi_fsmState = 0;
			}
			break;
	}
}
