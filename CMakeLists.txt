# CMake Configuration
cmake_minimum_required(VERSION 3.5)
project(network-stack-example)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)  
get_directory_property(hasParent PARENT_DIRECTORY)

# Synthesis parameters
# NOTE: HLS synthesis expects the FPGA part, clock period and uncertainty which are inherited from Coyote
set(NETWORK_BANDWIDTH 100 CACHE STRING "Network bandwidth.")
set(NETWORK_INTERFACE 100 CACHE STRING "Network interface.")
set(DATA_WIDTH 64 CACHE STRING "Data width")
if (NOT DEFINED ${TCP_STACK_MSS})
   set(TCP_STACK_MSS 4096)
endif()
if (NOT DEFINED ${TCP_STACK_WINDOW_SCALING_EN})
   set(TCP_STACK_WINDOW_SCALING_EN 1)
endif()

# Build process
if(EN_TCP OR EN_RDMA)
   add_subdirectory(hls/arp_server_subnet)
   add_subdirectory(hls/icmp_server)
   add_subdirectory(hls/ip_handler)
   add_subdirectory(hls/ipv4)
   add_subdirectory(hls/mac_ip_encode)
   add_subdirectory(hls/ethernet_frame_padding_512)
endif()

if(EN_TCP)
   add_subdirectory(hls/toe)
   add_subdirectory(hls/hash_table)
endif()

if(EN_RDMA)
   add_subdirectory(hls/rocev2)
endif()
