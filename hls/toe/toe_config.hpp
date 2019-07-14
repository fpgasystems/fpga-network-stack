#pragma once

#define WINDOW_SCALE 1

const unsigned DATA_WIDTH = 512;


const unsigned WINDOW_SCALE_BITS = 4;
#if (WINDOW_SCALE)
const unsigned WINDOW_BITS = 16 + WINDOW_SCALE_BITS;
#else
const unsigned WINDOW_BITS = 16;
#endif


const unsigned BUFFER_SIZE = (1 << WINDOW_BITS);
const unsigned CONGESTION_WINDOW_MAX = (BUFFER_SIZE - 2048);
