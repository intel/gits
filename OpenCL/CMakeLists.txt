# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

set(OpenCL_SCRIPTS_DIR "${CMAKE_SOURCE_DIR}/OpenCL/codegen")

# Custom command cannot have actual generated files as OUTPUT, because they
# are included in 3 different targets that build in parallel.
# This makes generator run 3 or more times and it usually fails somewhere,
# as it tries to overwrite currently open files.
# It also cannot be done with only custom target, because that would always
# regenerate and rebuild the files, even if source .py files did not change.

add_custom_command(
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/OpenCL_codegen.stamp"
  COMMAND ${Python3_EXECUTABLE} "${OpenCL_SCRIPTS_DIR}/generate.py" "${CMAKE_CURRENT_BINARY_DIR}"
  COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_CURRENT_BINARY_DIR}/OpenCL_codegen.stamp"
  WORKING_DIRECTORY "${OpenCL_SCRIPTS_DIR}"
  DEPENDS
    "${OpenCL_SCRIPTS_DIR}/generate.py"
    "${OpenCL_SCRIPTS_DIR}/generator_cl.py"
    "${OpenCL_SCRIPTS_DIR}/templates/openclDriversAuto.inl.mako"
    "${OpenCL_SCRIPTS_DIR}/templates/openclDriversInit.cpp.mako"
    "${OpenCL_SCRIPTS_DIR}/templates/openclArgumentsAuto.h.mako"
    "${OpenCL_SCRIPTS_DIR}/templates/openclArgumentsAuto.cpp.mako"
    "${OpenCL_SCRIPTS_DIR}/templates/openclFunctionsAuto.h.mako"
    "${OpenCL_SCRIPTS_DIR}/templates/openclFunctionsAuto.cpp.mako"
    "${OpenCL_SCRIPTS_DIR}/templates/openclHeaderAuto.h.mako"
    "${OpenCL_SCRIPTS_DIR}/templates/openclIDswitch.h.mako"
    "${OpenCL_SCRIPTS_DIR}/templates/openclPrePostAuto.cpp.mako"
    "${OpenCL_SCRIPTS_DIR}/templates/openclRecorderWrapperAuto.h.mako"
    "${OpenCL_SCRIPTS_DIR}/templates/openclRecorderWrapperAuto.cpp.mako"
    "${OpenCL_SCRIPTS_DIR}/templates/openclRecorderWrapperIfaceAuto.h.mako"
    "${OpenCL_SCRIPTS_DIR}/templates/openclTracingAuto.h.mako"
    "${OpenCL_SCRIPTS_DIR}/templates/oclPlugin.def.mako"
    "${OpenCL_SCRIPTS_DIR}/templates/clconstants.lua.mako"
    "${OpenCL_SCRIPTS_DIR}/templates/helperCLAuto.inl.mako"
    "${OpenCL_SCRIPTS_DIR}/templates/helperCLAuto.cpp.mako"
)

add_custom_target(OpenCL_codegen DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/OpenCL_codegen.stamp")
set_source_files_properties(
  ${CMAKE_CURRENT_SOURCE_DIR}/common/include/openclIDs.h
  "openclDriversAuto.inl"
  "openclDriversInit.cpp"
  "openclArgumentsAuto.h"
  "openclArgumentsAuto.cpp"
  "openclFunctionsAuto.h"
  "openclFunctionsAuto.cpp"
  "openclHeaderAuto.h"
  "openclIDswitch.h"
  "openclPrePostAuto.cpp"
  "openclRecorderWrapperAuto.h"
  "openclRecorderWrapperAuto.cpp"
  "openclRecorderWrapperIfaceAuto.h"
  "openclTracingAuto.h"
  PROPERTIES GENERATED 1
)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(OpenCL_common
  common/include/oclFunction.h
  common/include/oglTypes.h
  common/include/openclArguments.h
  ${CMAKE_CURRENT_BINARY_DIR}/openclArgumentsAuto.h
  common/include/openclArgumentsHelper.h
  ${CMAKE_CURRENT_BINARY_DIR}/openclDriversAuto.inl
  common/include/openclDrivers.h
  ${CMAKE_CURRENT_BINARY_DIR}/openclDriversInit.cpp
  common/include/openclDriversHelper.h
  ${CMAKE_CURRENT_BINARY_DIR}/openclFunctionsAuto.h
  common/include/openclHeader.h
  ${CMAKE_CURRENT_BINARY_DIR}/openclHeaderAuto.h
  common/include/openclIDs.h
  ${CMAKE_CURRENT_BINARY_DIR}/openclIDswitch.h
  common/include/openclLibrary.h
  common/include/openclPlayerRunWrap.h
  common/include/openclStateDynamic.h
  common/include/openclStateTracking.h
  common/include/openclTools.h
  common/include/openclTracing.h
  common/include/opencl_apis_iface.h
  common/include/openclHelperFunctions.h

  common/oclFunction.cpp
  common/openclArguments.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/openclArgumentsAuto.cpp
  common/openclDrivers.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/openclFunctionsAuto.cpp
  common/openclLibrary.cpp
  common/openclStateDynamic.cpp
  common/openclTools.cpp
  common/openclHelperFunctions.cpp
)

add_library(OpenCL_recorder
  recorder/include/openclRecorderSubwrappers.h
  recorder/include/openclRecorderWrapper.h
  ${CMAKE_CURRENT_BINARY_DIR}/openclRecorderWrapperAuto.h
  recorder/include/openclRecorderWrapperIface.h
  ${CMAKE_CURRENT_BINARY_DIR}/openclRecorderWrapperIfaceAuto.h
  recorder/include/openclStateRestore.h

  recorder/openclRecorderWrapper.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/openclRecorderWrapperAuto.cpp
  recorder/openclStateRestore.cpp
)

set(OpenCL_DEF )
if(MSVC)
  set(OpenCL_DEF ${CMAKE_CURRENT_BINARY_DIR}/oclPlugin.def)
  set_source_files_properties(
    ${CMAKE_CURRENT_BINARY_DIR}/oclPlugin.def
    PROPERTIES GENERATED 1
  )
endif()

list(APPEND OpenCL_interceptor_external_cpps
  ../common/legacy/tools_lite.cpp
  ../common/legacy/exception.cpp
  ../common/legacy/dynamic_linker.cpp
  ../common/legacy/gitsLoader.cpp
  ../common/legacy/log.cpp
)
if(WIN32)
  list(APPEND OpenCL_interceptor_external_cpps ../common/legacy/tools_windows.cpp)
endif()

add_library(OpenCL_interceptor SHARED
  interceptor/include/gitsPluginOpenCL.h
  interceptor/include/openclExecWrap.h

  ${OpenCL_interceptor_external_cpps}
  interceptor/gitsPluginOpenCL.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/openclPrePostAuto.cpp
  ${OpenCL_DEF}
)

source_group("External Files" FILES ${OpenCL_interceptor_external_cpps})
source_group("Generated Files" FILES ${OpenCL_DEF} REGULAR_EXPRESSION ".*Auto.*")

set_target_properties(OpenCL_interceptor PROPERTIES OUTPUT_NAME OpenCL)
if ("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
  target_link_libraries(OpenCL_interceptor dl pthread)
endif()

add_dependencies(OpenCL_common common OpenCL_codegen)
add_dependencies(OpenCL_recorder OpenCL_codegen)
add_dependencies(OpenCL_interceptor OpenCL_codegen)

set_target_properties(OpenCL_codegen PROPERTIES FOLDER OpenCL)
set_target_properties(OpenCL_common PROPERTIES FOLDER OpenCL)
set_target_properties(OpenCL_recorder PROPERTIES FOLDER OpenCL)
set_target_properties(OpenCL_interceptor PROPERTIES FOLDER OpenCL SOVERSION 1)

if(MSVC)
  install(FILES $<TARGET_PDB_FILE:OpenCL_interceptor> DESTINATION Recorder/${OpenCL_INSTALL_SUBDIR} OPTIONAL)
endif()

install(TARGETS OpenCL_interceptor
  RUNTIME DESTINATION Recorder/${OpenCL_INSTALL_SUBDIR}
  LIBRARY DESTINATION Recorder/${OpenCL_INSTALL_SUBDIR})

set(OpenCL_CCode_SOURCES
  common/openclDrivers.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/helperCLAuto.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/openclDriversInit.cpp)

set(OpenCL_CCode_HEADERS
  common/include/openclTracing.h
  common/include/openclDrivers.h
  common/include/openclDriversHelper.h
  common/include/openclHeader.h
  ${CMAKE_CURRENT_BINARY_DIR}/openclTracingAuto.h
  ${CMAKE_CURRENT_BINARY_DIR}/openclHeaderAuto.h
  ${CMAKE_CURRENT_BINARY_DIR}/openclDriversAuto.inl
  ${CMAKE_CURRENT_BINARY_DIR}/helperCLAuto.inl)

install(FILES ${OpenCL_CCode_SOURCES} DESTINATION CCode/CCodeSource)
install(FILES ${OpenCL_CCode_HEADERS} DESTINATION CCode/CCodeSource/include)

set(OpenCL_Lua_SOURCES
  ${CMAKE_CURRENT_BINARY_DIR}/clconstants.lua
)
install(FILES ${OpenCL_Lua_SOURCES} DESTINATION LuaScripts/OpenCL)
