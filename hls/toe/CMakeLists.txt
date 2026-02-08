# Author:  David Sidler (david.sidler@inf.ethz.ch)

cmake_minimum_required(VERSION 3.5)
set (PROJECT_NAME toe)
project(${PROJECT_NAME})

# Include custom Find<Module>.cmake scripts to enable searching for Vitis HLS
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake)

# Project properties
# set(FPGA_PART xcu55c-fsvh2892-2L-e CACHE STRING "FPGA part")
set(DATA_WIDTH 64 CACHE STRING "Width of data path in bytes")
set(CLOCK_PERIOD 4 CACHE STRING "Target clock period in nanoseconds")

# TCP parameters
set(TCP_STACK_MAX_SESSIONS 1000 CACHE STRING "Maximum number of sessions the TCP/IP stack can support")
set(TCP_STACK_MSS 4096 CACHE STRING "Maximum Segment Size (MSS)")
set(TCP_STACK_NODELAY_EN 1 CACHE BOOL "Enabling TCP_NODELAY option")
set(TCP_STACK_RX_DDR_BYPASS_EN 1 CACHE BOOL "Enabling DDR bypass on the RX path")
set(TCP_STACK_FAST_RETRANSMIT_EN 1 CACHE BOOL "Enabling TCP fast retransmit")
set(TCP_STACK_WINDOW_SCALING_EN 1 CACHE BOOL "Enabling TCP window scaling option")

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
set(HLS_DEPENDS
   ${CMAKE_CURRENT_SOURCE_DIR}/ack_delay/ack_delay.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/close_timer/close_timer.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/event_engine/event_engine.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/port_table/port_table.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/probe_timer/probe_timer.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/retransmit_timer/retransmit_timer.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/rx_app_if/rx_app_if.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/rx_app_stream_if/rx_app_stream_if.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/rx_engine/rx_engine.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/rx_sar_table/rx_sar_table.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/session_lookup_controller/session_lookup_controller.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/state_table/state_table.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/tx_app_if/tx_app_if.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/tx_app_stream_if/tx_app_stream_if.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/tx_engine/tx_engine.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/tx_sar_table/tx_sar_table.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/tx_app_interface/tx_app_interface.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/toe.cpp 
   ${CMAKE_CURRENT_SOURCE_DIR}/toe.hpp
   ${CMAKE_CURRENT_SOURCE_DIR}/toe_config.hpp.in
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
                                   
