# Author:  David Sidler (david.sidler@inf.ethz.ch)

cmake_minimum_required(VERSION 3.5)
set (PROJECT_NAME rocev2)
project(${PROJECT_NAME})

# Include custom Find<Module>.cmake scripts to enable searching for Vitis HLS
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../cmake)

# Project properties
set(FPGA_PART xcu55c-fsvh2892-2L-e CACHE STRING "FPGA part")
set(DATA_WIDTH 64 CACHE STRING "Width of data path in bytes")
set(CLOCK_PERIOD 4 CACHE STRING "Target clock period in nanoseconds")
set(PMTU_BYTES 4096 CACHE STRING "PMTU size.")
set(RETRANS_EN 1 CACHE STRING "Re-transmission logic enable")
set(ROCE_STACK_MAX_QPS 256 CACHE STRING "Maximum number of queue pairs the RoCE stack can support")

# Make sure Vitis HLS is installed
find_package(VitisHLS REQUIRED)
if (NOT VITIS_HLS_FOUND)
   message(FATAL_ERROR "Vitis HLS not found.")
endif()

# IP path
if(DEFINED ENV{IPREPO_DIR})
   set(IPREPO_DIR $ENV{IPREPO_DIR})
elseif(NOT IPREPO_DIR)
   set(IPREPO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/iprepo/)
endif()

# Source files, dependencies
include_directories(${CMAKE_CURRENT_BINARY_DIR})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../ipv4/ipv4_config.hpp.in ipv4_config.hpp)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../ipv6/ipv6_config.hpp.in ipv6_config.hpp)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../udp/udp_config.hpp.in udp_config.hpp)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/rocev2_config.hpp.in rocev2_config.hpp)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/make.tcl.in make.tcl)

set(HLS_DEPENDS
   ${CMAKE_CURRENT_SOURCE_DIR}/rocev2.cpp 
   ${CMAKE_CURRENT_SOURCE_DIR}/rocev2.hpp
   ${CMAKE_CURRENT_SOURCE_DIR}/rocev2_config.hpp.in
   ${CMAKE_CURRENT_SOURCE_DIR}/test_ib_transport.cpp
)

# 'services' target for Coyote, which runs HLS synthesis and IP export
if (NOT TARGET services)
   add_custom_target(services)
endif()

if(VITIS_HLS_MODE STREQUAL "vitis_hls")
   add_custom_target(
      services.${PROJECT_NAME}
      COMMAND ${VITIS_HLS_BINARY} -f make.tcl
      DEPENDS ${HLS_DEPENDS}
   )
else()
   add_custom_target(
      services.${PROJECT_NAME}
      COMMAND ${VITIS_HLS_BINARY} --tcl make.tcl --mode hls
      DEPENDS ${HLS_DEPENDS}
   )
endif()

add_dependencies(services services.${PROJECT_NAME})
