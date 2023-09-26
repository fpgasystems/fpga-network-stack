# Author:  Johannes de Fine Licht (johannes.definelicht@inf.ethz.ch)
# Created: October 2016
#
# To specify the path to the Vitis HLS installation, provide:
#   -DVITIS_HLS_ROOT_DIR=<installation directory>
# If successful, this script defines:
#   VITIS_HLS_FOUND
#   VITIS_HLS_BINARY
#   VITIS_HLS_INCLUDE_DIRS

cmake_minimum_required(VERSION 3.0)

find_path(VITIS_HLS_PATH
  NAMES vitis_hls 
  PATHS ${VITIS_HLS_ROOT_DIR} ENV XILINX_VITIS_HLS ENV XILINX_HLS
  PATH_SUFFIXES bin
)

if(NOT EXISTS ${VITIS_HLS_PATH})

  message(WARNING "Vitis HLS not found.")

else()

  get_filename_component(VITIS_HLS_ROOT_DIR ${VITIS_HLS_PATH} DIRECTORY)

  set(VITIS_HLS_FOUND TRUE)
  set(VITIS_HLS_INCLUDE_DIRS ${VITIS_HLS_ROOT_DIR}/include/)
  set(VITIS_HLS_BINARY ${VITIS_HLS_ROOT_DIR}/bin/vitis_hls)

  message(STATUS "Found Vitis HLS at ${VITIS_HLS_ROOT_DIR}.")

endif()
