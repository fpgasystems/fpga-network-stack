# Author:  David Sidler (david.sidler@inf.ethz.ch)

cmake_minimum_required(VERSION 3.5)
set (PROJECT_NAME ipv6)
project(${PROJECT_NAME})

# Include custom Find<Module>.cmake scripts to enable searching for Vitis HLS
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake)

# Project properties
set(FPGA_PART xcu55c-fsvh2892-2L-e CACHE STRING "FPGA part")
set(DATA_WIDTH 64 CACHE STRING "Width of data path in bytes")
set(CLOCK_PERIOD 4 CACHE STRING "Target clock period in nanoseconds")

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

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ipv6_config.hpp.in ipv6_config.hpp)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/make.tcl.in make.tcl)

set(HLS_DEPENDS
   ${CMAKE_CURRENT_SOURCE_DIR}/ipv6.cpp 
   ${CMAKE_CURRENT_SOURCE_DIR}/ipv6.hpp
   ${CMAKE_CURRENT_SOURCE_DIR}/ipv6_config.hpp.in
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
