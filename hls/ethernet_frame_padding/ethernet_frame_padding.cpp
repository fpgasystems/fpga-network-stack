#include "ethernet_frame_padding.hpp"

void ethernet_frame_padding(	hls::stream<axiWord>&			dataIn,
				hls::stream<axiWord>&			dataOut)
{
#pragma HLS PIPELINE II=1
#pragma HLS INTERFACE ap_ctrl_none port=return

#pragma HLS resource core=AXI4Stream variable=dataIn metadata="-bus_bundle s_axis"
#pragma HLS resource core=AXI4Stream variable=dataOut metadata="-bus_bundle m_axis"


	static ap_uint<1> state = 0;
	static ap_uint<8> wordCounter = 0;
	axiWord currWord;
	axiWord sendWord;

	switch(state)
	{
	case 0:
		if (!dataIn.empty())
		{
			dataIn.read(currWord);
			sendWord.data = currWord.data;
			sendWord.keep = currWord.keep;
			sendWord.last = currWord.last;

			wordCounter++;
			if (currWord.last)
			{
				if (wordCounter < 8)
				{
					sendWord.keep = 0xFF;
					sendWord.last = 0;
					state = 1;
				}
				else
				{
					if (wordCounter == 8 && sendWord.keep < 0xF)
					{
						sendWord.keep = 0x0F;
					}
					wordCounter = 0;
				}
			}
			dataOut.write(sendWord);
		}
		break;
	case 1:
		sendWord.data = 0;
		sendWord.keep = 0xFF;
		sendWord.last = 0x0;
		wordCounter++;
		if (wordCounter == 8)
		{
			sendWord.keep = 0x0F;
			sendWord.last = 0x1;
			wordCounter = 0;
			state = 0;
		}
		dataOut.write(sendWord);
		break;
	} //switch
}
