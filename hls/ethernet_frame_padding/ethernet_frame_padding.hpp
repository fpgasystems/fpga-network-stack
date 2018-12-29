#ifndef ETH_FRAME_PADDING
#define ETH_FRAME_PADDING

#include "../tcp_ip.hpp"


void ethernet_fram_padding(	hls::stream<axiWord>&			dataIn,
				hls::stream<axiWord>&			dataOut);

#endif
