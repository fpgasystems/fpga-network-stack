#pragma once
#include <stdint.h>

const unsigned DATA_WIDTH = FNS_DATA_WIDTH * 8;
const uint16_t MAX_QPS = FNS_ROCE_STACK_MAX_QPS;
const uint16_t PMTU = 4096;
const uint16_t PMTU_WORDS = PMTU / (DATA_WIDTH/8);
static const uint32_t PCIE_BATCH_PKG = 12;
static const uint32_t PCIE_BATCH_SIZE = PMTU * PCIE_BATCH_PKG;