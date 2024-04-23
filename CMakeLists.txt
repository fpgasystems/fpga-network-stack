# Author: David Sidler (david.sidler@inf.ethz.ch)

cmake_minimum_required(VERSION 3.0)

project(fpga-network-stack)

# Include custom Find<Module>.cmake scripts to enable searching for Vitis HLS
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/xilinx-cmake)

# Target options
set(FNS_PLATFORM "xilinx_u55c_gen3x16_xdma_3_202210_1" CACHE STRING "FPGA platform")
#set(FNS_CLOCK_PERIOD "3.2" CACHE STRING "Target clock period in nanoseconds")

# Domain options
set(FNS_DATA_WIDTH 64 CACHE STRING "Width of data path in bytes")
set(FNS_ROCE_STACK_MAX_QPS 500 CACHE STRING "Maximum number of queue pairs the RoCE stack can support")
set(FNS_TCP_STACK_FAST_RETRANSMIT_EN 1 CACHE STRING "Enabling TCP fast retransmit")
set(FNS_TCP_STACK_NODELAY_EN 1 CACHE STRING "Toggles Nagle's Algorithm on/off")
set(FNS_TCP_STACK_MAX_SESSIONS 1000 CACHE STRING "Maximum number of sessions the TCP/IP stack can support")
set(FNS_TCP_STACK_MSS 4096 CACHE STRING "Maximum Segment Size (MSS)")
set(FNS_TCP_STACK_RX_DDR_BYPASS_EN 1 CACHE STRING "Enabling DDR bypass on the RX path")
set(FNS_TCP_STACK_WINDOW_SCALING_EN 1 CACHE STRING "Enabling TCP window scaling option")

# HLS IPs
add_subdirectory(hls/arp_server_subnet)
add_subdirectory(hls/dhcp_client)
add_subdirectory(hls/icmp_server)
add_subdirectory(hls/ip_handler)
add_subdirectory(hls/ipv4)
add_subdirectory(hls/mac_ip_encode)
add_subdirectory(hls/toe)
add_subdirectory(hls/hash_table)
add_subdirectory(hls/ethernet_frame_padding)
add_subdirectory(hls/udp)

# add_subdirectory(hls/echo_server_application)
# add_subdirectory(hls/iperf_client)
# add_subdirectory(hls/iperf_udp)
# add_subdirectory(hls/rocev2)
