# Author:  David Sidler (david.sidler@inf.ethz.ch)

cmake_minimum_required(VERSION 3.0)

set (PROJECT_NAME toe)
project(${PROJECT_NAME})

# Include custom Find<Module>.cmake scripts to enable searching for Vitis HLS
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake)

# Without this variable set, CMake will build tests when running install
#set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY ON)

# Generate Doxygen if available
#find_package(Doxygen)
#if(Doxygen_FOUND)
#  configure_file(${CMAKE_SOURCE_DIR}/Doxyfile.in Doxyfile)
#  add_custom_target(doxygen ALL
#      COMMAND ${DOXYGEN_EXECUTABLE} Doxyfile 
#      WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
#endif()


set(FPGA_PART xc7vx690tffg1761-2 CACHE STRING "FPGA part")
set(DATA_WIDTH 8 CACHE STRING "Width of data path in bytes")
set(CLOCK_PERIOD 6.4 CACHE STRING "Target clock period in nanoseconds")
# TCP parameters
set(TCP_STACK_MAX_SESSIONS 1000 CACHE STRING "Maximum number of sessions the TCP/IP stack can support")
set(TCP_STACK_MSS 1460 CACHE STRING "Maximum Segment Size (MSS)")
set(TCP_STACK_NODELAY_EN 1 CACHE BOOL "Enabling TCP_NODELAY option")
set(TCP_STACK_RX_DDR_BYPASS_EN 1 CACHE BOOL "Enabling DDR bypass on the RX path")
set(TCP_STACK_FAST_RETRANSMIT_EN 1 CACHE BOOL "Enabling TCP fast retransmit")
set(TCP_STACK_WINDOW_SCALING_EN 1 CACHE BOOL "Enabling TCP window scaling option")

# Find Xilinx Vitis HLS
find_package(VitisHLS REQUIRED)
if (NOT VITIS_HLS_FOUND)
  message(FATAL_ERROR "Vitis HLS not found.")
endif()


include_directories(${CMAKE_CURRENT_BINARY_DIR})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/toe_config.hpp.in toe_config.hpp)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/make.tcl.in make.tcl)


set(EXAMPLE_HLS_DEPENDS
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
    ${CMAKE_CURRENT_SOURCE_DIR}/toe_config.hpp.in)


#Setup HLS custom targets
set(HLS_TARGETS synthesis csim ip installip)

foreach (target ${HLS_TARGETS})
   if (NOT TARGET ${target})
      add_custom_target(${target})
   endif()

   add_custom_target(${target}.${PROJECT_NAME}
      COMMAND ${VITIS_HLS_BINARY} -f make.tcl -tclargs ${target}
      DEPENDS ${EXAMPLE_HLS_DEPENDS})
   add_dependencies(${target} ${target}.${PROJECT_NAME})
endforeach()

#target dependencies
add_dependencies(ip.${PROJECT_NAME} synthesis.${PROJECT_NAME})
add_dependencies(installip.${PROJECT_NAME} ip.${PROJECT_NAME})

# Installation
if (DEFINED ENV{IPREPO_DIR})
   set(IPREPO_DIR $ENV{IPREPO_DIR})
elseif(NOT  IPREPO_DIR)
   set(IPREPO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/iprepo/)
endif()
#install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_prj/solution1/impl/ip/
#   DESTINATION ${IPREPO_DIR}/${PROJECT_NAME}/)
                                   
