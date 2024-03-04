# Author:  Johannes de Fine Licht (definelicht@inf.ethz.ch)
# This software is copyrighted under the BSD 3-Clause License. 
#
# Once done this will define:
#   Vitis_FOUND - Indicates whether Vitis was found.
#   Vitis_INCLUDE_DIRS - Include directories for HLS. 
#   Vitis_LIBRARIES - Runtime libraries required for host side code. 
#   Vitis_COMPILER - Path to the compiler executable (v++).
#   Vitis_HLS - Path to HLS executable (vitis_hls or vivado_hls). 
#   Vitis_FLOATING_POINT_LIBRARY - Library required for emulation of fp16.
#   Vitis_VERSION - Version of Vitis installation.
#   Vitis_VERSION_MAJOR - Major version of Vitis installation.
#   Vitis_VERSION_MINOR - Minor version of Vitis installation.
#   Vitis_PLATFORMINFO - Path to the utility for extracting information from installed platforms.
#   Vitis_IP_REPO_DIR - Path to the IP repository directory for Vivado.
#
# To specify the location of Vitis, or to force this script to use a specific version, set the variable VITIS_ROOT to
# the root directory of the desired Vitis installation. Similarly, XRT_ROOT can be used to specify the XRT installation
# that should be used.

if(NOT DEFINED VITIS_ROOT)
  find_path(VITIS_SEARCH_PATH v++
            PATHS ENV XILINX_OPENCL ENV XILINX_VITIS
            PATH_SUFFIXES bin)
  get_filename_component(VITIS_ROOT ${VITIS_SEARCH_PATH} DIRECTORY) 
  mark_as_advanced(VITIS_SEARCH_PATH)
else()
  message(STATUS "Using user defined Vitis directory: ${VITIS_ROOT}")
endif()

# Check if all the necessary components are present. We want to ensure that we
# use the tools bundled together, so we restrict all further finds to only look
# in paths relative to the determined installation. 

find_program(Vitis_COMPILER v++ PATHS ${VITIS_ROOT}/bin NO_DEFAULT_PATH)
find_program(Vitis_PLATFORMINFO platforminfo PATHS ${VITIS_ROOT}/bin NO_DEFAULT_PATH)
set(Vitis_COMPILER ${Vitis_COMPILER} CACHE STRING "Compiler used to build FPGA kernels." FORCE)
set(Vitis_PLATFORMINFO ${Vitis_PLATFORMINFO} CACHE STRING "Utility for extracting information from Xilinx platforms." FORCE)

# Get version number string
get_filename_component(VITIS_VERSION "${VITIS_ROOT}" NAME)
string(REGEX REPLACE "([0-9]+)\\.[0-9]+" "\\1" VITIS_MAJOR_VERSION "${VITIS_VERSION}")
string(REGEX REPLACE "[0-9]+\\.([0-9]+)" "\\1" VITIS_MINOR_VERSION "${VITIS_VERSION}")
set(Vitis_VERSION ${VITIS_VERSION} CACHE STRING "Version of Vitis found" FORCE)
set(Vitis_MAJOR_VERSION ${VITIS_MAJOR_VERSION} CACHE STRING "Major version of Vitis found" FORCE)
set(Vitis_MINOR_VERSION ${VITIS_MINOR_VERSION} CACHE STRING "Minor version of Vitis found" FORCE)
add_definitions(-DVITIS_VERSION=${Vitis_VERSION} -DVITIS_MAJOR_VERSION=${Vitis_MAJOR_VERSION} -DVITIS_MINOR_VERSION=${Vitis_MINOR_VERSION})

find_program(Vitis_VIVADO_HLS NAMES vivado_hls PATHS
             ${VITIS_ROOT}/bin
             ${VITIS_ROOT}/../../Vivado/${Vitis_VERSION}/bin
             ${VITIS_ROOT}/Vivado_HLS/bin NO_DEFAULT_PATH DOC
             "Vivado HLS compiler associated with this version of the tools.")

find_program(VITIS_HLS NAMES vitis_hls vivado_hls PATHS
             ${VITIS_ROOT}/../../Vitis_HLS/${Vitis_VERSION}/bin
             ${VITIS_ROOT}/bin
             ${VITIS_ROOT}/../../Vivado/${Vitis_VERSION}/bin
             ${VITIS_ROOT}/Vivado_HLS/bin NO_DEFAULT_PATH)
mark_as_advanced(VITIS_HLS)
set(Vitis_HLS ${VITIS_HLS} CACHE STRING "Path to HLS executable." FORCE)

find_program(Vitis_VIVADO vivado PATHS
             ${VITIS_ROOT}/../../Vivado/${Vitis_VERSION}/bin
             ${VITIS_ROOT}/Vivado/bin NO_DEFAULT_PATH)

find_path(Vitis_HLS_INCLUDE_DIR hls_stream.h PATHS
          ${VITIS_ROOT}/../../Vitis_HLS/${Vitis_VERSION}/include
          ${VITIS_ROOT}/../../Vivado/${Vitis_VERSION}/include
          ${VITIS_ROOT}/include
          ${VITIS_ROOT}/Vivado_HLS/include
          NO_DEFAULT_PATH)
mark_as_advanced(Vitis_HLS_INCLUDE_DIR)

if(Vitis_VPP OR (Vitis_MAJOR_VERSION GREATER 2018) OR
   (Vitis_MAJOR_VERSION EQUAL 2018 AND Vitis_MINOR_VERSION GREATER 2))
  set(VITIS_USE_XRT TRUE)
else()
  set(VITIS_USE_XRT FALSE)
endif()
set(Vitis_USE_XRT ${VITIS_USE_XRT} CACHE STRING "Use XRT as runtime. Otherwise, use Vitis OpenCL runtime." FORCE)

set(Vitis_IP_REPO_DIR ${CMAKE_BINARY_DIR}/ip_repo CACHE STRING "Path to the IP repository directory for Vivado." FORCE)

# Currently only x86 support
if(CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(X86)|(amd64)|(AMD64)")

  #----------------------------------------------------------------------------
  # Floating point library
  #----------------------------------------------------------------------------

  find_library(Vitis_FLOATING_POINT_LIBRARY Ip_floating_point_v7_0_bitacc_cmodel
               PATHS
               ${VITIS_ROOT}/lnx64/tools/fpo_v7_0
               ${VITIS_ROOT}/../../Vitis_HLS/${Vitis_VERSION}/lnx64/tools/fpo_v7_0
               ${VITIS_ROOT}/../../Vivado/${Vitis_VERSION}/lnx64/tools/fpo_v7_0
               ${VITIS_ROOT}/Vivado_HLS/lnx64/tools/fpo_v7_0)
  mark_as_advanced(Vitis_FLOATING_POINT_LIBRARY)

  get_filename_component(VITIS_FP_DIR ${Vitis_FLOATING_POINT_LIBRARY}
                         DIRECTORY) 
  mark_as_advanced(VITIS_FP_DIR)

  set(Vitis_FLOATING_POINT_LIBRARY ${Vitis_FLOATING_POINT_LIBRARY} 
      ${Vitis_FLOATING_POINT_LIBMPFR} ${Vitis_FLOATING_POINT_LIBGMP})

  find_library(Vitis_FLOATING_POINT_LIBGMP gmp
               PATHS ${VITIS_FP_DIR} NO_DEFAULT_PATH)
  mark_as_advanced(Vitis_FLOATING_POINT_LIBGMP)

  find_library(Vitis_FLOATING_POINT_LIBMPFR mpfr
               PATHS ${VITIS_FP_DIR} NO_DEFAULT_PATH)
  mark_as_advanced(Vitis_FLOATING_POINT_LIBMPFR)

  #----------------------------------------------------------------------------
  # OpenCL runtime
  #----------------------------------------------------------------------------

  mark_as_advanced(VITIS_RUNTIME_DIR)

  if(NOT Vitis_USE_XRT)

    set(VITIS_RUNTIME_DIR ${VITIS_ROOT}/runtime)

    # Older versions of Vitis ship with their own OpenCL headers. Make sure
    # to use them.
    find_path(Vitis_OPENCL_INCLUDE_DIR opencl.h
              PATHS ${VITIS_RUNTIME_DIR}
              ${VITIS_RUNTIME_DIR}/include
              ${VITIS_RUNTIME_DIR}/x86_64/include
              PATH_SUFFIXES 1_1/CL 1_2/CL 2_0/CL
              NO_DEFAULT_PATH)
    get_filename_component(Vitis_OPENCL_INCLUDE_DIR
                           ${Vitis_OPENCL_INCLUDE_DIR} DIRECTORY) 

  else()

    if(NOT DEFINED XRT_ROOT)

      find_path(XRT_SEARCH_PATH libxilinxopencl.so 
                PATHS ENV XILINX_XRT
                      /opt/xilinx/xrt /opt/Xilinx/xrt
                      /tools/Xilinx/xrt /tools/xilinx/xrt
                PATH_SUFFIXES lib)
      get_filename_component(XRT_ROOT ${XRT_SEARCH_PATH} DIRECTORY CACHE) 
      mark_as_advanced(XRT_SEARCH_PATH)

      if(NOT XRT_SEARCH_PATH)
        message(FATAL_ERROR "The Xilinx Runtime (XRT) was not found. You can specify the XRT directory with the XRT_ROOT variable or set the XILINX_XRT environment variable.")
      endif()

      message(STATUS "Found Xilinx Runtime (XRT): ${XRT_ROOT}")

    endif()

    set(VITIS_RUNTIME_DIR ${XRT_ROOT})

    # XRT doesn't ship with its own OpenCL headers and standard OpenCL library:
    # use system OpenCL libraries and headers.
    find_package(OpenCL REQUIRED)
    set(Vitis_OPENCL_INCLUDE_DIR ${OpenCL_INCLUDE_DIRS})

  endif()

  find_library(Vitis_LIBXILINXOPENCL xilinxopencl
               PATHS ${VITIS_RUNTIME_DIR}
                     ${VITIS_RUNTIME_DIR}/lib
                     ${VITIS_RUNTIME_DIR}/lib/x86_64
               NO_DEFAULT_PATH)
  mark_as_advanced(Vitis_LIBXILINXOPENCL)

  get_filename_component(VITIS_RUNTIME_LIB_FOLDER ${Vitis_LIBXILINXOPENCL}  
                         DIRECTORY) 
  mark_as_advanced(VITIS_RUNTIME_LIB_FOLDER)

  # Only succeed if libraries were found
  if(Vitis_LIBXILINXOPENCL)
    set(Vitis_LIBRARIES ${OpenCL_LIBRARIES} ${Vitis_LIBXILINXOPENCL}
        CACHE STRING "OpenCL runtime libraries." FORCE)
  endif()

  # For some reason, the executable finds the floating point library on the
  # RPATH, but not libgmp. TODO: debug further.
  set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
  set(CMAKE_INSTALL_RPATH
      "${CMAKE_INSTALL_RPATH}:${VITIS_RUNTIME_LIB_FOLDER}:${VITIS_FP_DIR}")

  find_path(Vitis_OPENCL_EXTENSIONS_INCLUDE_DIR cl_ext.h
            PATHS ${VITIS_RUNTIME_DIR}/include
            PATH_SUFFIXES 1_1/CL 1_2/CL 2_0/CL CL
            NO_DEFAULT_PATH)
  get_filename_component(Vitis_OPENCL_EXTENSIONS_INCLUDE_DIR 
                         ${Vitis_OPENCL_EXTENSIONS_INCLUDE_DIR} DIRECTORY) 

  # Only succeed if both include paths were found
  if(Vitis_HLS_INCLUDE_DIR AND Vitis_OPENCL_INCLUDE_DIR AND
     Vitis_OPENCL_EXTENSIONS_INCLUDE_DIR)
    set(Vitis_INCLUDE_DIRS ${Vitis_HLS_INCLUDE_DIR}
        ${Vitis_OPENCL_INCLUDE_DIR} ${Vitis_OPENCL_EXTENSIONS_INCLUDE_DIR} 
        CACHE STRING "Vitis include directories." FORCE)
  endif()

else()
  message(WARNING "Unsupported architecture: ${CMAKE_SYSTEM_PROCESSOR}")
endif()

# Function to convert each path in a list to an absolute path, if it isn't already
function(hlslib_make_paths_absolute OUTPUT_FILES)
  string(REPLACE " " ";" OUTPUT_FILES ${OUTPUT_FILES})
  set(_OUTPUT_FILES)
  foreach(KERNEL_FILE_PATH ${ARGN})
    if(NOT IS_ABSOLUTE ${KERNEL_FILE_PATH})
      set(_KERNEL_FILE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/${KERNEL_FILE_PATH})
      if(NOT EXISTS ${_KERNEL_FILE_PATH})
        message(FATAL_ERROR "File ${KERNEL_FILE_PATH} not found.")
      endif()
      set(KERNEL_FILE_PATH ${_KERNEL_FILE_PATH})
    endif()
    set(_OUTPUT_FILES ${_OUTPUT_FILES} ${KERNEL_FILE_PATH})
  endforeach()
  set(${OUTPUT_FILES} ${_OUTPUT_FILES} PARENT_SCOPE)
endfunction()

# Function to recover the part name used by the given platform
function(hlslib_get_part_by_platform PLATFORM_PART PLATFORM)
  message(STATUS "Querying Vitis platform ${PLATFORM}")
  execute_process(COMMAND ${Vitis_PLATFORMINFO} --platform ${PLATFORM} -jhardwarePlatform.board.part
                  OUTPUT_VARIABLE _PLATFORM_PART)
  string(STRIP "${_PLATFORM_PART}" _PLATFORM_PART)
  if(_PLATFORM_PART)
    set(${PLATFORM_PART} ${_PLATFORM_PART} PARENT_SCOPE)
  else()
    message(WARNING "Xilinx platform ${PLATFORM} was not found. Please consult \"${Vitis_PLATFORMINFO} -l\" for a list of installed platforms.")
  endif()
endfunction()

# Function to write ip TCL scripts
function (write_ip_tcl_script)
  set(oneValueArgs DESTINATION IP PART CMD)
  set(multiValueArgs HLS_FLAGS HW_FILES TB_FILES APPEND)
  cmake_parse_arguments(TCL "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  set(TCL_HLS_FLAGS "-Wno-unknown-pragmas ${TCL_HLS_FLAGS}")
  string(REPLACE ";" " " TCL_HW_FILES "${TCL_HW_FILES}")
  string(REPLACE ";" " " TCL_TB_FILES "${TCL_TB_FILES}")

  if (TCL_TB_FILES)
    set(TCL_TB_FILES "add_files -tb -cflags \"${TCL_HLS_FLAGS}\" -csimflags \"${TCL_HLS_FLAGS}\" \"${TCL_TB_FILES}\"")
  endif()

  file(WRITE ${TCL_DESTINATION}
    "\
open_project ${TCL_IP}\ 
open_solution ${TCL_PART}\ 
set_part ${TCL_PART}\ 
add_files -cflags \"${TCL_HLS_FLAGS}\" -csimflags \"${TCL_HLS_FLAGS}\" \"${TCL_HW_FILES}\"\ 
${TCL_TB_FILES}\ 
set_top ${TCL_IP}\ 
${TCL_APPEND}\ 
${TCL_CMD}\ 
exit")
endfunction()

# Function to write ip TCL scripts
function (write_rtl_kernel_export_script)
  set(oneValueArgs DESTINATION PACKAGE_TCL_PATH XO_PATH KERNEL_NAME PACKAGED_PATH XML_PATH)
  cmake_parse_arguments(TCL "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(TCL_XML_PATH)
    set(TCL_XML_PATH "-kernel_xml ${TCL_XML_PATH}")
  endif()

  file(WRITE ${TCL_DESTINATION}
    "\
source -notrace ${TCL_PACKAGE_TCL_PATH}\ 
package_xo -xo_path ${TCL_XO_PATH} -kernel_name ${TCL_KERNEL_NAME} -force -ip_directory ${TCL_PACKAGED_PATH} ${TCL_XML_PATH}")
endfunction()

# Used for building IPs that can be later used by kernels
function(add_vitis_ip
         IP_TARGET)

  # Keyword arguments
  cmake_parse_arguments(
      IP
      ""
      "IP;VERSION;VENDOR;PLATFORM_PART"
      "FILES;TB_FILES;DEPENDS;INCLUDE_DIRS;HLS_FLAGS;HLS_CONFIG;COMPILE_FLAGS;DISPLAY_NAME;DESCRIPTION"
      ${ARGN})

  # Verify that input is sane
  if(NOT IP_FILES)
    message(FATAL_ERROR "Must pass IP file(s) to add_vitis_ip using the FILES keyword.")
  endif()
  hlslib_make_paths_absolute(IP_FILES ${IP_FILES})
  hlslib_make_paths_absolute(IP_TB_FILES ${IP_TB_FILES})

  # Convert non-target dependencies to absolute paths
  string(REPLACE " " ";" IP_DEPENDS "${IP_DEPENDS}")
  unset(_IP_DEPENDS)
  foreach(DEP ${IP_DEPENDS})
    if(NOT TARGET ${DEP}) 
      hlslib_make_paths_absolute(DEP ${DEP})
    endif()
    set(_IP_DEPENDS ${_IP_DEPENDS} ${DEP})
  endforeach()
  set(IP_DEPENDS ${IP_FILES} ${_IP_DEPENDS} ${IP_TB_FILES})

  # Create the target that will carry properties. Adding the depends here does not actually work, so we have to store
  # them as a property, retrieve them later, and add them manually to each target
  add_custom_target(${IP_TARGET} DEPENDS ${IP_DEPENDS})

  # Use the target name as the IP name if the IP name hasn't been explicitly passed
  if(DEFINED IP_IP)
    set(IP_NAME ${IP_IP})
  else()
    set(IP_NAME ${IP_TARGET})
  endif()

  # Add additional include directories specified
  string(REPLACE " " ";" IP_INCLUDE_DIRS "${IP_INCLUDE_DIRS}")
  hlslib_make_paths_absolute(IP_INCLUDE_DIRS ${IP_INCLUDE_DIRS})
  foreach(INCLUDE_DIR ${IP_INCLUDE_DIRS})
    set(IP_HLS_FLAGS "${IP_HLS_FLAGS} -I${INCLUDE_DIR}")
  endforeach()

  # Clean up HLS flags to make sure they use string syntax (not list syntax),
  # and that there are no superfluous spaces.
  string(REGEX REPLACE ";|[ \t\r\n][ \t\r\n]+" " " IP_HLS_FLAGS "${IP_HLS_FLAGS}")
  string(STRIP "${IP_HLS_FLAGS}" IP_HLS_FLAGS)

  set(IP_PROJECT_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/${IP_NAME}/${IP_PLATFORM_PART})

  # C-Simulation target
  write_ip_tcl_script(
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/${IP_NAME}_csim.tcl
    IP ${IP_NAME}
    PART ${IP_PLATFORM_PART}
    CMD "csim_design -clean -O -setup"
    HLS_FLAGS ${IP_HLS_FLAGS}
    HW_FILES ${IP_FILES}
    TB_FILES ${IP_TB_FILES}
  )
  add_custom_command(OUTPUT ${IP_PROJECT_BUILD_DIR}/csim/build/csim.exe
                    COMMENT "Running c-simulation for ${IP_NAME}."
                    COMMAND ${Vitis_HLS} -f ${CMAKE_CURRENT_BINARY_DIR}/${IP_NAME}_csim.tcl
                    DEPENDS ${IP_NAME} ${IP_DEPENDS})
  add_custom_target(csim.${IP_NAME} DEPENDS
                    ${IP_NAME}/${IP_PLATFORM_PART}/csim/build/csim.exe)
  set_property(TARGET csim.${IP_NAME} APPEND PROPERTY ADDITIONAL_CLEAN_FILES
               ${IP_PROJECT_BUILD_DIR}/csim vitis_hls.log)

  # Synthesis target
  write_ip_tcl_script(
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/${IP_NAME}_synthesis.tcl
    IP ${IP_NAME}
    PART ${IP_PLATFORM_PART}
    CMD "csynth_design"
    HLS_FLAGS ${IP_HLS_FLAGS}
    HW_FILES ${IP_FILES}
    TB_FILES ${IP_TB_FILES}
  )
  add_custom_command(OUTPUT ${IP_PROJECT_BUILD_DIR}/impl/vhdl/${IP_NAME}.vhd
                    COMMENT "Running c-synthesis for ${IP_NAME}."
                    COMMAND ${Vitis_HLS} -f ${CMAKE_CURRENT_BINARY_DIR}/${IP_NAME}_synthesis.tcl
                    DEPENDS ${IP_NAME} ${IP_DEPENDS})
  add_custom_target(synth.${IP_NAME} DEPENDS
                    ${IP_PROJECT_BUILD_DIR}/impl/vhdl/${IP_NAME}.vhd)
  set_property(TARGET synth.${IP_NAME} APPEND PROPERTY ADDITIONAL_CLEAN_FILES
              ${IP_PROJECT_BUILD_DIR}/syn
              ${IP_PROJECT_BUILD_DIR}/impl vitis_hls.log)

  # Co-simulation target
  write_ip_tcl_script(
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/${IP_NAME}_cosim.tcl
    IP ${IP_NAME}
    PART ${IP_PLATFORM_PART}
    CMD "cosim_design -trace_level all -O"
    HLS_FLAGS ${IP_HLS_FLAGS}
    HW_FILES ${IP_FILES}
    TB_FILES ${IP_TB_FILES}
  )
  add_custom_command(OUTPUT ${IP_PROJECT_BUILD_DIR}/sim/verilog/${IP_NAME}.v
                    COMMENT "Running co-simulation for ${IP_NAME}."
                    COMMAND ${Vitis_HLS} -f ${CMAKE_CURRENT_BINARY_DIR}/${IP_NAME}_cosim.tcl
                    DEPENDS ${IP_NAME} ${IP_DEPENDS})
  add_custom_target(cosim.${IP_NAME} DEPENDS 
                    synth.${IP_NAME}
                    ${IP_PROJECT_BUILD_DIR}/sim/verilog/${IP_NAME}.v)
  set_property(TARGET cosim.${IP_NAME} APPEND PROPERTY ADDITIONAL_CLEAN_FILES
              ${IP_PROJECT_BUILD_DIR}/sim vitis_hls.log)

  # Export IP target
  write_ip_tcl_script(
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/${IP_NAME}_ip.tcl
    IP ${IP_NAME}
    PART ${IP_PLATFORM_PART}
    CMD "export_design \
  -format ip_catalog \
  -display_name \"${IP_DISPLAY_NAME}\" \
  -description \"${IP_DESCRIPTION}\" \
  -vendor \"${IP_VENDOR}\" \
  -ipname \"${IP_NAME}\" \
  -version \"${IP_VERSION}\" \
  -output \"${Vitis_IP_REPO_DIR}/${IP_NAME}.zip\""
    HLS_FLAGS ${IP_HLS_FLAGS}
    HW_FILES ${IP_FILES}
    TB_FILES ${IP_TB_FILES}
  )
  add_custom_command(OUTPUT ${Vitis_IP_REPO_DIR}/${IP_NAME}.zip
                    COMMENT "Exporting design for ${IP_NAME}."
                    COMMAND ${Vitis_HLS} -f ${CMAKE_CURRENT_BINARY_DIR}/${IP_NAME}_ip.tcl
                    DEPENDS ${IP_NAME} ${IP_DEPENDS})
  add_custom_command(OUTPUT ${Vitis_IP_REPO_DIR}/${IP_NAME}/component.xml 
                    COMMENT "Extracting IP for ${IP_NAME}."
                    COMMAND rm -rf ${Vitis_IP_REPO_DIR}/${IP_NAME} && unzip -qo ${Vitis_IP_REPO_DIR}/${IP_NAME}.zip -d ${Vitis_IP_REPO_DIR}/${IP_NAME}
                    DEPENDS ${Vitis_IP_REPO_DIR}/${IP_NAME}.zip)
  add_custom_target(ip.${IP_NAME} DEPENDS
                    synth.${IP_NAME}
                    ${Vitis_IP_REPO_DIR}/${IP_NAME}/component.xml)
  set_property(TARGET ip.${IP_NAME} APPEND PROPERTY ADDITIONAL_CLEAN_FILES
              ${IP_PROJECT_BUILD_DIR}/syn vitis_hls.log)

  # Handy targets (csim, cosim, )
  # csim
  # if (NOT TARGET csim)
  #   add_custom_target(csim
  #                     COMMENT "Running c-simulation for Vitis IPs.")
  # endif()
  # add_dependencies(csim csim.${IP_NAME})
  # synth
  if (NOT TARGET synth)
    add_custom_target(synth
                      COMMENT "Running synthesis for Vitis IPs.")
  endif()
  add_dependencies(synth synth.${IP_NAME})
  # cosim
  if (NOT TARGET cosim)
    add_custom_target(cosim
                      COMMENT "Running co-simulation for Vitis IPs.")
  endif()
  add_dependencies(cosim cosim.${IP_NAME})
  # ip
  if (NOT TARGET ip)
    add_custom_target(ip
                      COMMENT "Exporting design for Vitis IPs.")
  endif()
  add_dependencies(ip ip.${IP_NAME})

  # Pass variables the program target through properties
  set_target_properties(${IP_TARGET} PROPERTIES IP_FILES "${IP_FILES}")
  set_target_properties(${IP_TARGET} PROPERTIES IP_COMPUTE_UNITS "${IP_COMPUTE_UNITS}")
  set_target_properties(${IP_TARGET} PROPERTIES IP_NAME "${IP_NAME}")
  set_target_properties(${IP_TARGET} PROPERTIES IP_TB_FILES "${IP_TB_FILES}")
  set_target_properties(${IP_TARGET} PROPERTIES HLS_FLAGS "${IP_HLS_FLAGS}")
  set_target_properties(${IP_TARGET} PROPERTIES COMPILE_FLAGS "${IP_COMPILE_FLAGS}")
  set_target_properties(${IP_TARGET} PROPERTIES LINK_FLAGS "${IP_LINK_FLAGS}")
  set_target_properties(${IP_TARGET} PROPERTIES HLS_CONFIG "${IP_HLS_CONFIG}")
  set_target_properties(${IP_TARGET} PROPERTIES DEPENDS "${IP_DEPENDS}")

endfunction()

# Used for building RTL kernels
function(add_vivado_kernel
        KERNEL_TARGET)

  # Keyword arguments
  cmake_parse_arguments(
      KERNEL
      ""
      "KERNEL;PACKAGE_TCL_PATH;XML_PATH"
      "FILES;COMPUTE_UNITS;IPS;DEPENDS;INCLUDE_DIRS;HLS_FLAGS;HLS_CONFIG;COMPILE_FLAGS;PORT_MAPPING;SLR_MAPPING"
      ${ARGN})

  # Verify that input is sane
  if(NOT KERNEL_FILES)
    message(FATAL_ERROR "Must pass kernel file(s) to add_vivado_kernel using the FILES keyword.")
  endif()
  hlslib_make_paths_absolute(KERNEL_FILES ${KERNEL_FILES})

  if(NOT KERNEL_PACKAGE_TCL_PATH)
    message(FATAL_ERROR "Must pass kernel package.tcl to add_vivado_kernel using the PACKAGE_TCL_PATH keyword.")
  endif()
  hlslib_make_paths_absolute(KERNEL_PACKAGE_TCL_PATH ${KERNEL_PACKAGE_TCL_PATH})

  if(KERNEL_XML_PATH)
    hlslib_make_paths_absolute(KERNEL_XML_PATH ${KERNEL_XML_PATH})
  endif()

  # Convert non-target dependencies to absolute paths
  string(REPLACE " " ";" KERNEL_DEPENDS "${KERNEL_DEPENDS}")
  unset(_KERNEL_DEPENDS)
  foreach(DEP ${KERNEL_DEPENDS})
    if(NOT TARGET ${DEP}) 
      hlslib_make_paths_absolute(DEP ${DEP})
    endif()
    set(_KERNEL_DEPENDS ${_KERNEL_DEPENDS} ${DEP})
  endforeach()
  set(KERNEL_DEPENDS ${KERNEL_FILES} ${KERNEL_PACKAGE_TCL_PATH} ${KERNEL_XML_PATH} ${_KERNEL_DEPENDS})

  # Create the target that will carry properties. Adding the depends here does not actually work, so we have to store
  # them as a property, retrieve them later, and add them manually to each target
  add_custom_target(${KERNEL_TARGET} DEPENDS ${KERNEL_DEPENDS})

  # Use the target name as the kernel name if the kernel name hasn't been
  # explicitly passed
  if(DEFINED KERNEL_KERNEL)
    set(KERNEL_NAME ${KERNEL_KERNEL})
  else()
    set(KERNEL_NAME ${KERNEL_TARGET})
  endif()

  # Default the number of compute units
  if(NOT KERNEL_COMPUTE_UNITS)
    set(KERNEL_COMPUTE_UNITS 1)
  endif()
  if(${KERNEL_COMPUTE_UNITS} GREATER 1)
    set(KERNEL_LINK_FLAGS "${KERNEL_LINK_FLAGS} --connectivity.nk ${KERNEL_NAME}:${KERNEL_COMPUTE_UNITS}")
  endif()

  # Specify port mapping
  string(REPLACE " " ";" KERNEL_PORT_MAPPING "${KERNEL_PORT_MAPPING}")
  foreach(MAPPING ${KERNEL_PORT_MAPPING})
    string(REGEX MATCH "[A-Za-z0-9_]+\\.[^: \t\n]+:[^: \t\n]+" HAS_KERNEL_NAME ${MAPPING})
    if(HAS_KERNEL_NAME)
      set(KERNEL_LINK_FLAGS "${KERNEL_LINK_FLAGS} --connectivity.sp ${MAPPING}") 
    else()
      string(REGEX MATCH "[^: \t\n]+:[^: \t\n]+" IS_MEMORY_BANK ${MAPPING})
      if(IS_MEMORY_BANK)
        set(KERNEL_LINK_FLAGS "${KERNEL_LINK_FLAGS} --connectivity.sp ${KERNEL_NAME}_1.${MAPPING}") 
      else()
        message(FATAL_ERROR "Unrecognized port mapping \"${MAPPING}\".")
      endif()
    endif()
  endforeach()

  # Specify SLR mapping
  string(REPLACE " " ";" KERNEL_SLR_MAPPING "${KERNEL_SLR_MAPPING}")
  foreach(MAPPING ${KERNEL_SLR_MAPPING})
    string(REGEX MATCH "[A-Za-z0-9_]+:[^: \t\n]+" HAS_KERNEL_NAME ${MAPPING})
    if(HAS_KERNEL_NAME)
      set(KERNEL_LINK_FLAGS "${KERNEL_LINK_FLAGS} --connectivity.slr ${MAPPING}") 
    else()
      set(KERNEL_LINK_FLAGS "${KERNEL_LINK_FLAGS} --connectivity.slr ${KERNEL_NAME}_1:${MAPPING}") 
    endif()
  endforeach()

  # HLS flags
  string(FIND "${KERNEL_HLS_FLAGS}" "-std=" FOUND)
  if(FOUND EQUAL -1)
    set(KERNEL_HLS_FLAGS "${KERNEL_HLS_FLAGS} -std=c++11")
  endif()

  # Add additional include directories specified
  string(REPLACE " " ";" KERNEL_INCLUDE_DIRS "${KERNEL_INCLUDE_DIRS}")
  hlslib_make_paths_absolute(KERNEL_INCLUDE_DIRS ${KERNEL_INCLUDE_DIRS})
  foreach(INCLUDE_DIR ${KERNEL_INCLUDE_DIRS})
    set(KERNEL_HLS_FLAGS "${KERNEL_HLS_FLAGS} -I${INCLUDE_DIR}")
  endforeach()

  # Clean up HLS flags to make sure they use string syntax (not list syntax),
  # and that there are no superfluous spaces.
  string(REGEX REPLACE ";|[ \t\r\n][ \t\r\n]+" " " KERNEL_HLS_FLAGS "${KERNEL_HLS_FLAGS}")
  string(STRIP "${KERNEL_HLS_FLAGS}" KERNEL_HLS_FLAGS)

  # Create IP dependencies
  string(REPLACE " " ";" KERNEL_IPS "${KERNEL_IPS}")
  unset(_KERNEL_IPS)
  foreach(DEP ${KERNEL_IPS})
    if(NOT TARGET ip.${DEP})
      message(FATAL_ERROR "Must pass kernel IPs created with add_vitis_ip to add_vivado_kernel using the IPS keyword.")
    endif()
    set(_KERNEL_IPS ${_KERNEL_IPS} ip.${DEP})
  endforeach()
  set(KERNEL_DEPENDS ${KERNEL_DEPENDS} ${_KERNEL_IPS})
  
  # export xo target
  set(KERNEL_XO_PATH ${CMAKE_BINARY_DIR}/${KERNEL_NAME}.xo)
  write_rtl_kernel_export_script(
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/${KERNEL_NAME}_gen_xo.tcl
    PACKAGE_TCL_PATH ${KERNEL_PACKAGE_TCL_PATH}
    PACKAGED_PATH ${CMAKE_CURRENT_BINARY_DIR}/packaged_kernel_${KERNEL_NAME}
    XO_PATH ${KERNEL_XO_PATH}
    KERNEL_NAME ${KERNEL_NAME}
    XML_PATH ${KERNEL_XML_PATH}
  )
  add_custom_command(OUTPUT ${KERNEL_XO_PATH}
                     COMMENT "Exporting kernel .xo ${KERNEL_NAME}"
                     COMMAND ${Vitis_VIVADO} -mode batch -source ${CMAKE_CURRENT_BINARY_DIR}/${KERNEL_NAME}_gen_xo.tcl
                     DEPENDS ${KERNEL_DEPENDS})
  add_custom_target(kernel.${KERNEL_NAME} DEPENDS
                    ${KERNEL_XO_PATH})
  set(KERNEL_DEPENDS ${KERNEL_DEPENDS} ${KERNEL_XO_PATH})

  # Pass variables the program target through properties
  set_target_properties(${KERNEL_TARGET} PROPERTIES KERNEL_FILES "${KERNEL_FILES}")
  set_target_properties(${KERNEL_TARGET} PROPERTIES KERNEL_COMPUTE_UNITS "${KERNEL_COMPUTE_UNITS}")
  set_target_properties(${KERNEL_TARGET} PROPERTIES KERNEL_NAME "${KERNEL_NAME}")
  set_target_properties(${KERNEL_TARGET} PROPERTIES HLS_FLAGS "${KERNEL_HLS_FLAGS}")
  set_target_properties(${KERNEL_TARGET} PROPERTIES COMPILE_FLAGS "${KERNEL_COMPILE_FLAGS}")
  set_target_properties(${KERNEL_TARGET} PROPERTIES LINK_FLAGS "${KERNEL_LINK_FLAGS}")
  set_target_properties(${KERNEL_TARGET} PROPERTIES HLS_CONFIG "${KERNEL_HLS_CONFIG}")
  set_target_properties(${KERNEL_TARGET} PROPERTIES DEPENDS "${KERNEL_DEPENDS}")

endfunction()

# The name of the kernel is expected to match the target name. If it does not,
# the kernel name can be passed separately with the KERNEL keyword.
function(add_vitis_kernel
         KERNEL_TARGET)

  # Keyword arguments
  cmake_parse_arguments(
      KERNEL
      ""
      "KERNEL"
      "FILES;COMPUTE_UNITS;DEPENDS;INCLUDE_DIRS;HLS_FLAGS;HLS_CONFIG;COMPILE_FLAGS;PORT_MAPPING;SLR_MAPPING"
      ${ARGN})

  # Verify that input is sane
  if(NOT KERNEL_FILES)
    message(FATAL_ERROR "Must pass kernel file(s) to add_vitis_kernel using the FILES keyword.")
  endif()
  hlslib_make_paths_absolute(KERNEL_FILES ${KERNEL_FILES})

  # Convert non-target dependencies to absolute paths
  string(REPLACE " " ";" KERNEL_DEPENDS "${KERNEL_DEPENDS}")
  unset(_KERNEL_DEPENDS)
  foreach(DEP ${KERNEL_DEPENDS})
    if(NOT TARGET ${DEP}) 
      hlslib_make_paths_absolute(DEP ${DEP})
    endif()
    set(_KERNEL_DEPENDS ${_KERNEL_DEPENDS} ${DEP})
  endforeach()
  set(KERNEL_DEPENDS ${KERNEL_FILES} ${_KERNEL_DEPENDS})

  # Create the target that will carry properties. Adding the depends here does not actually work, so we have to store
  # them as a property, retrieve them later, and add them manually to each target
  add_custom_target(${KERNEL_TARGET} DEPENDS ${KERNEL_DEPENDS})

  # Use the target name as the kernel name if the kernel name hasn't been
  # explicitly passed
  if(DEFINED KERNEL_KERNEL)
    set(KERNEL_NAME ${KERNEL_KERNEL})
  else()
    set(KERNEL_NAME ${KERNEL_TARGET})
  endif()

  # Default the number of compute units
  if(NOT KERNEL_COMPUTE_UNITS)
    set(KERNEL_COMPUTE_UNITS 1)
  endif()
  if(${KERNEL_COMPUTE_UNITS} GREATER 1)
    set(KERNEL_LINK_FLAGS "${KERNEL_LINK_FLAGS} --connectivity.nk ${KERNEL_NAME}:${KERNEL_COMPUTE_UNITS}")
  endif()

  # Specify port mapping
  string(REPLACE " " ";" KERNEL_PORT_MAPPING "${KERNEL_PORT_MAPPING}")
  foreach(MAPPING ${KERNEL_PORT_MAPPING})
    string(REGEX MATCH "[A-Za-z0-9_]+\\.[^: \t\n]+:[^: \t\n]+" HAS_KERNEL_NAME ${MAPPING})
    if(HAS_KERNEL_NAME)
      set(KERNEL_LINK_FLAGS "${KERNEL_LINK_FLAGS} --connectivity.sp ${MAPPING}") 
    else()
      string(REGEX MATCH "[^: \t\n]+:[^: \t\n]+" IS_MEMORY_BANK ${MAPPING})
      if(IS_MEMORY_BANK)
        set(KERNEL_LINK_FLAGS "${KERNEL_LINK_FLAGS} --connectivity.sp ${KERNEL_NAME}_1.${MAPPING}") 
      else()
        message(FATAL_ERROR "Unrecognized port mapping \"${MAPPING}\".")
      endif()
    endif()
  endforeach()

  # Specify SLR mapping
  string(REPLACE " " ";" KERNEL_SLR_MAPPING "${KERNEL_SLR_MAPPING}")
  foreach(MAPPING ${KERNEL_SLR_MAPPING})
    string(REGEX MATCH "[A-Za-z0-9_]+:[^: \t\n]+" HAS_KERNEL_NAME ${MAPPING})
    if(HAS_KERNEL_NAME)
      set(KERNEL_LINK_FLAGS "${KERNEL_LINK_FLAGS} --connectivity.slr ${MAPPING}") 
    else()
      set(KERNEL_LINK_FLAGS "${KERNEL_LINK_FLAGS} --connectivity.slr ${KERNEL_NAME}_1:${MAPPING}") 
    endif()
  endforeach()

  # Mandatory flags for HLS when building kernels that use hlslib
  string(FIND "${KERNEL_HLS_FLAGS}" "-DHLSLIB_SYNTHESIS" FOUND)
  if(FOUND EQUAL -1)
    set(KERNEL_HLS_FLAGS "${KERNEL_HLS_FLAGS} -DHLSLIB_SYNTHESIS")
  endif()
  string(FIND "${KERNEL_HLS_FLAGS}" "-DHLSLIB_XILINX" FOUND)
  if(FOUND EQUAL -1)
    set(KERNEL_HLS_FLAGS "${KERNEL_HLS_FLAGS} -DHLSLIB_XILINX")
  endif()
  string(FIND "${KERNEL_HLS_FLAGS}" "-std=" FOUND)
  if(FOUND EQUAL -1)
    set(KERNEL_HLS_FLAGS "${KERNEL_HLS_FLAGS} -std=c++11")
  endif()

  # Pass the Vitis version to HLS
  string(FIND "${KERNEL_HLS_FLAGS}" "-DVITIS_MAJOR_VERSION=" FOUND)
  if(FOUND EQUAL -1)
    set(KERNEL_HLS_FLAGS "${KERNEL_HLS_FLAGS} -DVITIS_MAJOR_VERSION=${Vitis_MAJOR_VERSION}")
  endif()
  string(FIND "${KERNEL_HLS_FLAGS}" "-DVITIS_MINOR_VERSION=" FOUND)
  if(FOUND EQUAL -1)
    set(KERNEL_HLS_FLAGS "${KERNEL_HLS_FLAGS} -DVITIS_MINOR_VERSION=${Vitis_MINOR_VERSION}")
  endif()
  string(FIND "${KERNEL_HLS_FLAGS}" "-DVITIS_VERSION=" FOUND)
  if(FOUND EQUAL -1)
    set(KERNEL_HLS_FLAGS "${KERNEL_HLS_FLAGS} -DVITIS_VERSION=${Vitis_VERSION}")
  endif()

  # Tell hlslib whether we're using Vitis HLS or Vivado HLS
  if(NOT Vitis_USE_VITIS_HLS)
    set(KERNEL_HLS_FLAGS "${KERNEL_HLS_FLAGS} -D__VIVADO_HLS__")
  else()
    set(KERNEL_HLS_FLAGS "${KERNEL_HLS_FLAGS} -D__VITIS_HLS__")
  endif()

  # Add additional include directories specified
  string(REPLACE " " ";" KERNEL_INCLUDE_DIRS "${KERNEL_INCLUDE_DIRS}")
  hlslib_make_paths_absolute(KERNEL_INCLUDE_DIRS ${KERNEL_INCLUDE_DIRS})
  foreach(INCLUDE_DIR ${KERNEL_INCLUDE_DIRS})
    set(KERNEL_HLS_FLAGS "${KERNEL_HLS_FLAGS} -I${INCLUDE_DIR}")
  endforeach()

  # Clean up HLS flags to make sure they use string syntax (not list syntax),
  # and that there are no superfluous spaces.
  string(REGEX REPLACE ";|[ \t\r\n][ \t\r\n]+" " " KERNEL_HLS_FLAGS "${KERNEL_HLS_FLAGS}")
  string(STRIP "${KERNEL_HLS_FLAGS}" KERNEL_HLS_FLAGS)

  # Pass variables the program target through properties
  set_target_properties(${KERNEL_TARGET} PROPERTIES KERNEL_FILES "${KERNEL_FILES}")
  set_target_properties(${KERNEL_TARGET} PROPERTIES KERNEL_COMPUTE_UNITS "${KERNEL_COMPUTE_UNITS}")
  set_target_properties(${KERNEL_TARGET} PROPERTIES KERNEL_NAME "${KERNEL_NAME}")
  set_target_properties(${KERNEL_TARGET} PROPERTIES HLS_FLAGS "${KERNEL_HLS_FLAGS}")
  set_target_properties(${KERNEL_TARGET} PROPERTIES COMPILE_FLAGS "${KERNEL_COMPILE_FLAGS}")
  set_target_properties(${KERNEL_TARGET} PROPERTIES LINK_FLAGS "${KERNEL_LINK_FLAGS}")
  set_target_properties(${KERNEL_TARGET} PROPERTIES HLS_CONFIG "${KERNEL_HLS_CONFIG}")
  set_target_properties(${KERNEL_TARGET} PROPERTIES DEPENDS "${KERNEL_DEPENDS}")

endfunction()

function(add_vitis_program
         PROGRAM_TARGET
         PROGRAM_PLATFORM)

  # Keyword arguments
  cmake_parse_arguments(
      PROGRAM
      ""
      "CLOCK;CONFIG;SAVE_TEMPS;DEBUGGING;PROFILING"
      "KERNELS;BUILD_FLAGS;LINK_FLAGS;CONNECTIVITY;DEPENDS"
      ${ARGN})

  # Verify that input is sane
  if(NOT PROGRAM_KERNELS)
    if(TARGET ${PROGRAM_TARGET})
      # If no kernels were specified, try using the same target name
      set(PROGRAM_KERNELS ${PROGRAM_TARGET})
    else()
      # Otherwise we have to give up
      message(FATAL_ERROR "Must pass kernel targets created with add_vitis_kernel to add_vitis_program using the KERNELS keyword.")
    endif()
  endif()
  foreach(KERNEL ${PROGRAM_KERNELS})
    if(NOT TARGET ${KERNEL})
      message(FATAL_ERROR "${KERNEL} is not a valid Vitis kernel. Kernels must have been created with the add_vitis_kernel or add_vivado_kernel functions.")
    endif()
  endforeach()

  # Convert non-target dependencies to absolute paths
  string(REPLACE " " ";" PROGRAM_DEPENDS "${PROGRAM_DEPENDS}")
  unset(_PROGRAM_DEPENDS)
  foreach(DEP ${PROGRAM_DEPENDS})
    if(NOT TARGET ${DEP}) 
      hlslib_make_paths_absolute(DEP ${DEP})
    endif()
    set(_PROGRAM_DEPENDS ${_PROGRAM_DEPENDS} ${DEP})
  endforeach()
  set(PROGRAM_DEPENDS ${PROGRAM_DEPENDS} ${_PROGRAM_DEPENDS})

  # Recover the part name used by the given platform
  if(NOT PROGRAM_PLATFORM_PART OR NOT "${${PROGRAM_TARGET}_PLATFORM}" STREQUAL "${PROGRAM_PLATFORM}")
    hlslib_get_part_by_platform(PROGRAM_PLATFORM ${PROGRAM_PLATFORM_PART})
  endif()
  if(PROGRAM_PLATFORM_PART AND NOT "${${PROGRAM_TARGET}_PLATFORM}")
    # Cache this so we don't have to rerun platforminfo if the platform didn't change
    set(${PROGRAM_TARGET}_PLATFORM ${PROGRAM_PLATFORM} CACHE INTERNAL "")
  endif()

  # Specify the platform
  set(PROGRAM_BUILD_FLAGS "${PROGRAM_BUILD_FLAGS} --platform ${PROGRAM_PLATFORM}")

  # Augment with frequency flag if specified
  if(PROGRAM_CLOCK)
    set(PROGRAM_BUILD_FLAGS "${PROGRAM_BUILD_FLAGS} --kernel_frequency ${PROGRAM_CLOCK}")
  endif()

  # Pass config file if specified
  if(PROGRAM_CONFIG)
    set(PROGRAM_BUILD_FLAGS "${PROGRAM_BUILD_FLAGS} --config ${PROGRAM_CONFIG}")
  endif()

  # Save temporaries if instructed to do so
  if(PROGRAM_SAVE_TEMPS)
    set(PROGRAM_BUILD_FLAGS "${PROGRAM_BUILD_FLAGS} --save-temps")
  endif()

  # Default to -O3 if no other optimization flag is passed
  string(FIND "${PROGRAM_BUILD_FLAGS} ${PROGRAM_LINK_FLAGS}" "-O" FOUND_SHORT)
  string(FIND "${PROGRAM_BUILD_FLAGS} ${PROGRAM_LINK_FLAGS}" "--optimize" FOUND_LONG)
  if(FOUND_SHORT EQUAL -1 AND FOUND_LONG EQUAL -1)
    set(PROGRAM_BUILD_FLAGS "${PROGRAM_BUILD_FLAGS} -O3")
  endif()

  # Optional profiling flags
  if(PROGRAM_PROFILING)
    set(PROGRAM_LINK_FLAGS "${PROGRAM_LINK_FLAGS} --profile.data all:all:all --profile.exec all:all --profile.stall all:all")
  endif()

  # Optional debugging flags
  if(PROGRAM_DEBUGGING)
    foreach(KERNEL ${PROGRAM_KERNELS})
      # Append _1 to match the Vitis convention (only supports single compute unit kernels)
      get_target_property(KERNEL_NAME ${KERNEL} KERNEL_NAME)
      set(PROGRAM_LINK_FLAGS "${PROGRAM_LINK_FLAGS} --debug --debug.chipscope ${KERNEL_NAME}_1")
    endforeach()
  endif()

  # Mapping between streaming interfaces on kernels
  string(REPLACE " " ";" PROGRAM_CONNECTIVITY "${PROGRAM_CONNECTIVITY}")
  foreach(MAPPING ${PROGRAM_CONNECTIVITY})
    string(REGEX MATCH "[^\\.: \t\n]+\\.[^\\.: \t\n]+:[^\\.: \t\n]+\\.[^\\.: \t\n]+" IS_MAPPING ${MAPPING})
    if(IS_MAPPING)
      set(PROGRAM_LINK_FLAGS "${PROGRAM_LINK_FLAGS} --connectivity.sc ${MAPPING}") 
    else()
      message(FATAL_ERROR "Unrecognized kernel mapping \"${MAPPING}\".")
    endif()
  endforeach()

  # Clean up build flags by removing superfluous whitespace and convert from
  # string syntax to list syntax,
  string(REGEX REPLACE "[ \t\r\n][ \t\r\n]+" " " PROGRAM_BUILD_FLAGS "${PROGRAM_BUILD_FLAGS}")
  string(STRIP "${PROGRAM_BUILD_FLAGS}" PROGRAM_BUILD_FLAGS)
  string(REGEX REPLACE " " ";" PROGRAM_BUILD_FLAGS "${PROGRAM_BUILD_FLAGS}")

  unset(PROGRAM_XO_FILES_SW_EMU)
  unset(PROGRAM_XO_FILES_HW_EMU)
  unset(PROGRAM_XO_FILES_HW)

  foreach(KERNEL ${PROGRAM_KERNELS})
  
    get_target_property(KERNEL_NAME ${KERNEL} KERNEL_NAME)
    get_target_property(KERNEL_FILES ${KERNEL} KERNEL_FILES)
    get_target_property(KERNEL_HLS_FLAGS ${KERNEL} HLS_FLAGS)
    get_target_property(KERNEL_HLS_CONFIG ${KERNEL} HLS_CONFIG)
    get_target_property(KERNEL_COMPILE_FLAGS ${KERNEL} COMPILE_FLAGS)
    get_target_property(KERNEL_LINK_FLAGS ${KERNEL} LINK_FLAGS)
    get_target_property(KERNEL_DEPENDS ${KERNEL} DEPENDS)

    set(KERNEL_COMPILE_FLAGS "${PROGRAM_COMPILE_FLAGS} ${KERNEL_COMPILE_FLAGS} --advanced.prop kernel.${KERNEL_NAME}.kernel_flags=\"${KERNEL_HLS_FLAGS}\"") 
    set(PROGRAM_LINK_FLAGS "${PROGRAM_LINK_FLAGS} ${KERNEL_LINK_FLAGS}")

    # If HLS TCL config commands are provided, generate a file that can be passed
    if(KERNEL_HLS_CONFIG)
      set(HLS_TCL_FILE ${CMAKE_CURRENT_BINARY_DIR}/${KERNEL}_config.tcl)
      file(WRITE ${HLS_TCL_FILE} "${KERNEL_HLS_CONFIG}")
      set(KERNEL_COMPILE_FLAGS "${KERNEL_COMPILE_FLAGS} --hls.pre_tcl ${HLS_TCL_FILE}")
    endif()
    
    # Canonicalize flags
    string(REGEX REPLACE "[ \t\r\n][ \t\r\n]+" " " KERNEL_COMPILE_FLAGS "${KERNEL_COMPILE_FLAGS}")
    string(STRIP "${KERNEL_COMPILE_FLAGS}" KERNEL_COMPILE_FLAGS)
    string(REGEX REPLACE " " ";" KERNEL_COMPILE_FLAGS "${KERNEL_COMPILE_FLAGS}")

    # Software emulation target
    set(KERNEL_XO_FILE_SW_EMU ${CMAKE_CURRENT_BINARY_DIR}/${KERNEL_NAME}_sw_emu.xo)
    set(PROGRAM_XO_FILES_SW_EMU ${PROGRAM_XO_FILES_SW_EMU} ${KERNEL_XO_FILE_SW_EMU})
    add_custom_command(
      OUTPUT ${KERNEL_XO_FILE_SW_EMU}
      COMMENT "Compiling ${KERNEL} for software emulation."
      COMMAND ${CMAKE_COMMAND} -E env
              XILINX_PATH=${CMAKE_CURRENT_BINARY_DIR}
              ${Vitis_COMPILER} --compile --target sw_emu
              --kernel ${KERNEL_NAME}
              ${KERNEL_COMPILE_FLAGS} 
              ${PROGRAM_BUILD_FLAGS}
              ${KERNEL_FILES}
              --output ${KERNEL_XO_FILE_SW_EMU}
      DEPENDS ${KERNEL} ${KERNEL_DEPENDS})
    add_custom_target(compile_${KERNEL}_sw_emu DEPENDS
                      ${KERNEL_XO_FILE_SW_EMU})
    if(NOT TARGET compile_sw_emu)
      add_custom_target(compile_sw_emu COMMENT "Compiling software emulation targets."
                        DEPENDS compile_${KERNEL}_sw_emu)
    else()
      add_dependencies(compile_sw_emu compile_${KERNEL}_sw_emu)
    endif()

    # Hardware emulation target
    set(KERNEL_XO_FILE_HW_EMU ${CMAKE_CURRENT_BINARY_DIR}/${KERNEL_NAME}_hw_emu.xo)
    set(PROGRAM_XO_FILES_HW_EMU ${PROGRAM_XO_FILES_HW_EMU} ${KERNEL_XO_FILE_HW_EMU})
    add_custom_command(
      OUTPUT ${KERNEL_XO_FILE_HW_EMU}
      COMMENT "Compiling ${KERNEL} for hardware emulation."
      COMMAND ${CMAKE_COMMAND} -E env
              XILINX_PATH=${CMAKE_CURRENT_BINARY_DIR}
              ${Vitis_COMPILER} --compile --target hw_emu
              --kernel ${KERNEL_NAME}
              ${KERNEL_COMPILE_FLAGS} 
              ${PROGRAM_BUILD_FLAGS}
              ${KERNEL_FILES}
              --output ${KERNEL_XO_FILE_HW_EMU}
      DEPENDS ${KERNEL} ${KERNEL_DEPENDS})
    add_custom_target(compile_${KERNEL}_hw_emu DEPENDS
                      ${KERNEL_XO_FILE_HW_EMU})
    if(NOT TARGET compile_hw_emu)
      add_custom_target(compile_hw_emu COMMENT "Compiling hardware emulation targets."
                        DEPENDS compile_${KERNEL}_hw_emu)
    else()
      add_dependencies(compile_hw_emu compile_${KERNEL}_hw_emu)
    endif()

    # Hardware target
    set(KERNEL_XO_FILE_HW ${CMAKE_CURRENT_BINARY_DIR}/${KERNEL_NAME}_hw.xo)
    set(PROGRAM_XO_FILES_HW ${PROGRAM_XO_FILES_HW} ${KERNEL_XO_FILE_HW})
    add_custom_command(
      OUTPUT ${KERNEL_XO_FILE_HW}
      COMMENT "Compiling ${KERNEL_NAME} for hardware."
      COMMAND ${CMAKE_COMMAND} -E env
              XILINX_PATH=${CMAKE_CURRENT_BINARY_DIR}
              ${Vitis_COMPILER} --compile --target hw
              --kernel ${KERNEL_NAME}
              ${KERNEL_COMPILE_FLAGS}
              ${PROGRAM_BUILD_FLAGS}
              ${KERNEL_FILES}
              --output ${KERNEL_XO_FILE_HW}
      DEPENDS ${KERNEL} ${KERNEL_DEPENDS})
    add_custom_target(compile_${KERNEL}_hw DEPENDS
                      ${KERNEL_XO_FILE_HW})
    if(NOT TARGET compile_hw)
      add_custom_target(compile_hw COMMENT "Compiling hardware targets."
                        DEPENDS compile_${KERNEL}_hw)
    else()
      add_dependencies(compile_hw compile_${KERNEL}_hw)
    endif()

    if(PROGRAM_PLATFORM_PART)
      # Make separate synthesis target, which is faster to run than Vitis compile
      if(PROGRAM_CLOCK)
        set(KERNEL_HLS_TCL_CLOCK "create_clock -period ${PROGRAM_CLOCK}MHz -name default\n")
      endif()
      string(REPLACE ";" " " KERNEL_FILES "${KERNEL_FILES}") 
      file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${KERNEL}_synthesis.tcl
           "\
open_project ${KERNEL} \ 
open_solution -flow_target vitis ${PROGRAM_PLATFORM_PART} \ 
set_part ${PROGRAM_PLATFORM_PART} \ 
add_files -cflags \"${KERNEL_HLS_FLAGS}\" \"${KERNEL_FILES}\" \ 
set_top ${KERNEL_NAME} \ 
${KERNEL_HLS_TCL_CLOCK}\
${KERNEL_HLS_CONFIG} \ 
csynth_design \ 
exit")
      add_custom_command(OUTPUT ${KERNEL}/${PROGRAM_PLATFORM_PART}/${PROGRAM_PLATFORM_PART}.log
                         COMMENT "Running high-level synthesis for ${KERNEL}."
                         COMMAND ${Vitis_HLS} -f ${CMAKE_CURRENT_BINARY_DIR}/${KERNEL}_synthesis.tcl
                         # CMake does not seem to rerun the custom target if the target that it depends
                         # on is rerun, so we have to re-add the files as dependencies here.
                         DEPENDS ${KERNEL} ${KERNEL_DEPENDS})
      add_custom_target(synthesize_${KERNEL} DEPENDS  
                        ${KERNEL}/${PROGRAM_PLATFORM_PART}/${PROGRAM_PLATFORM_PART}.log)
      set_property(TARGET synthesize_${KERNEL} APPEND PROPERTY ADDITIONAL_CLEAN_FILES
                   ${CMAKE_CURRENT_BINARY_DIR}/${KERNEL} vitis_hls.log)
      if(NOT TARGET synthesis)
        add_custom_target(synthesis COMMENT "Running high-level synthesis for Vitis kernels."
                          DEPENDS synthesize_${KERNEL})
      else()
        add_dependencies(synthesis synthesize_${KERNEL})
      endif()
    endif()

  endforeach()

  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/emconfig.json
    COMMENT "Generating emconfig.json file for hardware emulation."
    COMMAND ${VITIS_ROOT}/bin/emconfigutil --platform ${PROGRAM_PLATFORM})
  if(NOT TARGET ${PROGRAM_PLATFORM}_emconfig)
    add_custom_target(${PROGRAM_PLATFORM}_emconfig DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/emconfig.json)
  endif()

  # Canonicalize link flags
  string(REGEX REPLACE "[ \t\r\n][ \t\r\n]+" " " PROGRAM_LINK_FLAGS "${PROGRAM_LINK_FLAGS}")
  string(STRIP "${PROGRAM_LINK_FLAGS}" PROGRAM_LINK_FLAGS)
  string(REGEX REPLACE " " ";" PROGRAM_LINK_FLAGS "${PROGRAM_LINK_FLAGS}")

  # Software emulation target
  set(PROGRAM_XCLBIN_SW_EMU ${CMAKE_CURRENT_BINARY_DIR}/${PROGRAM_TARGET}_sw_emu.xclbin)
  add_custom_command(
    OUTPUT ${PROGRAM_XCLBIN_SW_EMU}
    COMMENT "Linking ${PROGRAM_TARGET} for software emulation."
    COMMAND ${CMAKE_COMMAND} -E env
            XILINX_PATH=${CMAKE_CURRENT_BINARY_DIR}
            ${Vitis_COMPILER} --link --target sw_emu
            ${PROGRAM_BUILD_FLAGS}
            ${PROGRAM_LINK_FLAGS}
            ${PROGRAM_XO_FILES_SW_EMU}
            --output ${PROGRAM_XCLBIN_SW_EMU}
    # Back up the binary in case it accidentally gets cleaned when outdated
    POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy
                       ${PROGRAM_XCLBIN_SW_EMU} ${PROGRAM_XCLBIN_SW_EMU}.bak
    DEPENDS ${PROGRAM_XO_FILES_SW_EMU}
            ${PROGRAM_PLATFORM}_emconfig
            ${PROGRAM_DEPENDS})
  add_custom_target(link_${PROGRAM_TARGET}_sw_emu DEPENDS ${PROGRAM_XCLBIN_SW_EMU})
  add_custom_target(${PROGRAM_TARGET}_sw_emu DEPENDS ${PROGRAM_XCLBIN_SW_EMU})

  # Hardware emulation target
  set(PROGRAM_XCLBIN_HW_EMU ${CMAKE_CURRENT_BINARY_DIR}/${PROGRAM_TARGET}_hw_emu.xclbin)
  add_custom_command(
    OUTPUT ${PROGRAM_XCLBIN_HW_EMU}
    COMMENT "Linking ${PROGRAM_TARGET} for hardware emulation."
    COMMAND ${CMAKE_COMMAND} -E env
            XILINX_PATH=${CMAKE_CURRENT_BINARY_DIR}
            ${Vitis_COMPILER} --link --target hw_emu
            ${PROGRAM_BUILD_FLAGS}
            ${PROGRAM_LINK_FLAGS}
            ${PROGRAM_XO_FILES_HW_EMU}
            --output ${PROGRAM_XCLBIN_HW_EMU}
    # Back up the binary in case it accidentally gets cleaned when outdated
    POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy
                       ${PROGRAM_XCLBIN_HW_EMU} ${PROGRAM_XCLBIN_HW_EMU}.bak
    DEPENDS ${PROGRAM_XO_FILES_HW_EMU}
            ${PROGRAM_PLATFORM}_emconfig
            ${PROGRAM_DEPENDS})
  add_custom_target(link_${PROGRAM_TARGET}_hw_emu DEPENDS ${PROGRAM_XCLBIN_HW_EMU})
  add_custom_target(${PROGRAM_TARGET}_hw_emu DEPENDS ${PROGRAM_XCLBIN_HW_EMU})

  # Hardware target
  set(PROGRAM_XCLBIN_HW ${CMAKE_CURRENT_BINARY_DIR}/${PROGRAM_TARGET}_hw.xclbin)
  add_custom_command(
    OUTPUT ${PROGRAM_XCLBIN_HW}
    COMMENT "Linking ${PROGRAM_TARGET} for hardware."
    COMMAND ${CMAKE_COMMAND} -E env
            XILINX_PATH=${CMAKE_CURRENT_BINARY_DIR}
            ${Vitis_COMPILER} --link --target hw
            ${PROGRAM_BUILD_FLAGS}
            ${PROGRAM_LINK_FLAGS}
            ${PROGRAM_XO_FILES_HW}
            --output ${PROGRAM_XCLBIN_HW}
    # Back up the binary in case it accidentally gets cleaned when outdated
    POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy
                       ${PROGRAM_XCLBIN_HW} ${PROGRAM_XCLBIN_HW}.bak
    DEPENDS ${PROGRAM_XO_FILES_HW}
            ${PROGRAM_DEPENDS})
  add_custom_target(link_${PROGRAM_TARGET}_hw DEPENDS ${PROGRAM_XCLBIN_HW})
  add_custom_target(${PROGRAM_TARGET}_hw DEPENDS link_${PROGRAM_TARGET}_hw)

  # Shorthand to compile kernels, so user can just run "make hw" or "make hw_emu"
  if(NOT TARGET link_sw_emu)
    add_custom_target(link_sw_emu COMMENT "Linking software emulation targets."
                      DEPENDS link_${PROGRAM_TARGET}_sw_emu)
  else()
    add_dependencies(link_sw_emu link_${PROGRAM_TARGET}_sw_emu)
  endif()
  if(NOT TARGET link_hw_emu)
    add_custom_target(link_hw_emu COMMENT "Linking hardware emulation targets."
                      DEPENDS link_${PROGRAM_TARGET}_hw_emu)
  else()
    add_dependencies(link_hw_emu link_${PROGRAM_TARGET}_hw_emu)
  endif()
  if(NOT TARGET link_hw)
    add_custom_target(link_hw COMMENT "Linking hardware targets."
                      DEPENDS link_${PROGRAM_TARGET}_hw)
  else()
    add_dependencies(link_hw link_${PROGRAM_TARGET}_hw)
  endif()
  if(NOT TARGET sw_emu)
    add_custom_target(sw_emu COMMENT "Building software emulation targets."
                      DEPENDS link_sw_emu)
  else()
    add_dependencies(sw_emu ${PROGRAM_TARGET}_sw_emu)
  endif()
  if(NOT TARGET hw_emu)
    add_custom_target(hw_emu COMMENT "Building hardware emulation targets."
                      DEPENDS link_hw_emu)
  else()
    add_dependencies(hw_emu ${PROGRAM_TARGET}_hw_emu)
  endif()
  if(NOT TARGET hw)
    add_custom_target(hw COMMENT "Building hardware targets."
                      DEPENDS link_hw)
  else()
    add_dependencies(hw ${PROGRAM_TARGET}_hw)
  endif()

  # Add Xilinx build directory to clean target
  set_directory_properties(PROPERTIES ADDITIONAL_CLEAN_FILES ${CMAKE_CURRENT_BINARY_DIR}/_x)

endfunction()

set(Vitis_EXPORTS
    Vitis_COMPILER
    Vitis_HLS
    Vitis_INCLUDE_DIRS
    Vitis_LIBRARIES
    Vitis_FLOATING_POINT_LIBRARY 
    Vitis_VERSION
    Vitis_MAJOR_VERSION
    Vitis_MINOR_VERSION
    Vitis_PLATFORMINFO
    Vitis_IP_REPO_DIR)
mark_as_advanced(Vitis_EXPORTS)

# include(FindPackageHandleStandardArgs)
# # Handle the QUIETLY and REQUIRED arguments and set Vitis_FOUND to TRUE if all
# # listed variables were found.
# find_package_handle_standard_args(Vitis DEFAULT_MSG ${Vitis_EXPORTS})