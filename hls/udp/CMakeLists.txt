# Author:  David Sidler (david.sidler@inf.ethz.ch)

cmake_minimum_required(VERSION 3.0)

set (PROJECT_NAME udp)
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


# Find Xilinx Vitis HLS
find_package(VitisHLS REQUIRED)
if (NOT VITIS_HLS_FOUND)
  message(FATAL_ERROR "Vitis HLS not found.")
endif()


include_directories(${CMAKE_CURRENT_BINARY_DIR})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/udp_config.hpp.in udp_config.hpp)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/make.tcl.in make.tcl)


set(EXAMPLE_HLS_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/udp.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/udp.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/udp_config.hpp.in)


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
                                   
