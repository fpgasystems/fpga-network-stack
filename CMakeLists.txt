# Author:   David Sidler (david.sidler@inf.ethz.ch)

cmake_minimum_required(VERSION 3.0)

project(network-stack-example)

# Include custom Find<Module>.cmake scripts to enable searching for Vivado HLS                                
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)  

# Check if parent directory
get_directory_property(hasParent PARENT_DIRECTORY)

if (DEFINED ENV{IPREPO_DIR})
   set(IPREPO_DIR $ENV{IPREPO_DIR})
elseif (NOT IPREPO_DIR)
   set(IPREPO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/iprepo)
   message("set iprepor dir ${IPREPO_DIR}")
endif()

# set(DEVICE_NAME
if (DEVICE_NAME STREQUAL "vc709")
   set(FPGA_PART xc7vx690tffg1761-2)
   set(FPGA_FAMILY 7series)
endif()
if (DEVICE_NAME STREQUAL "vcu118")
   set(FPGA_PART xcvu9p-flga2104-2L-e)
   set(FPGA_FAMILY ultraplus)
endif()


#HLS IPs
add_subdirectory(hls/arp_server_subnet)
add_subdirectory(hls/dhcp_client)
add_subdirectory(hls/echo_server_application)
add_subdirectory(hls/icmp_server)
add_subdirectory(hls/iperf_client)
add_subdirectory(hls/iperf_udp_client)
add_subdirectory(hls/ip_handler)
add_subdirectory(hls/ipv4)
add_subdirectory(hls/mac_ip_encode)
add_subdirectory(hls/toe)
add_subdirectory(hls/udp)
if (NOT hasParent)
   add_subdirectory(hls/ethernet_frame_padding)
endif()

#Vivado Project (create only if no parent)
if (NOT hasParent)

   # Find Xilinx Vivado
   find_package(Vivado REQUIRED)
   if (NOT VIVADO_FOUND)
      message(FATAL_ERROR "Vivado not found.")
   endif()

   configure_file(${CMAKE_SOURCE_DIR}/projects/create_project.tcl.in create_project.tcl)

   set (PROJECT_DEPENDS
        ${CMAKE_SOURCE_DIR}/rtl/common/network_stack.v
        ${CMAKE_SOURCE_DIR}/projects/create_project.tcl.in
        ${CMAKE_SOURCE_DIR}/projects/network_stack.tcl)

   add_custom_target(project
     COMMAND ${VIVADO_BINARY} -mode batch -source create_project.tcl
     DEPENDS ${PROJECT_DEPENDS})
endif()

# add_dependencies(project ip)
