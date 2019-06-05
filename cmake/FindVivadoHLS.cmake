# Author:  Johannes de Fine Licht (johannes.definelicht@inf.ethz.ch)
# Created: October 2016
#
# To specify the path to the Vivado HLS installation, provide:
#   -DVIVADO_HLS_ROOT_DIR=<installation directory>
# If successful, this script defines:
#   VIVADO_HLS_FOUND
#   VIVADO_HLS_BINARY
#   VIVADO_HLS_INCLUDE_DIRS

cmake_minimum_required(VERSION 3.0)

find_path(VIVADO_HLS_PATH
  NAMES vivado_hls 
  PATHS ${VIVADO_HLS_ROOT_DIR} ENV XILINX_VIVADO_HLS ENV XILINX_HLS
  PATH_SUFFIXES bin
)

if(NOT EXISTS ${VIVADO_HLS_PATH})

  message(WARNING "Vivado HLS not found.")

else()

  get_filename_component(VIVADO_HLS_ROOT_DIR ${VIVADO_HLS_PATH} DIRECTORY)

  set(VIVADO_HLS_FOUND TRUE)
  set(VIVADO_HLS_INCLUDE_DIRS ${VIVADO_HLS_ROOT_DIR}/include/)
  set(VIVADO_HLS_BINARY ${VIVADO_HLS_ROOT_DIR}/bin/vivado_hls)

  message(STATUS "Found Vivado HLS at ${VIVADO_HLS_ROOT_DIR}.")

endif()
