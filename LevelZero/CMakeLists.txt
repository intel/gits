# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

# Custom command cannot have actual generated files as OUTPUT, because they
# are included in 3 different targets that build in parallel.
# This makes generator run 3 or more times and it usually fails somewhere,
# as it tries to overwrite currently open files.
# It also cannot be done with only custom target, because that would always
# regenerate and rebuild the files, even if source .py files did not change.

if(WIN32)
  set(ZE_LOADER_VERSION_MAJOR 1)
  set(ZE_LOADER_VERSION_MINOR 18)
  set(ZE_LOADER_VERSION_PATCH 3)
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/ZeLoaderVersion.rc.in
    ${CMAKE_CURRENT_BINARY_DIR}/ZeLoaderVersion.rc
    @ONLY)
endif()

add_custom_command(
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/L0_codegen.stamp"
  COMMAND ${Python3_EXECUTABLE} "generate.py" "${CMAKE_CURRENT_BINARY_DIR}"
  COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_CURRENT_BINARY_DIR}/L0_codegen.stamp"
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/codegen"
  DEPENDS
    "codegen/generate.py"
    "codegen/generator_l0.py"
    "codegen/generator_callbacks_l0.py"
    "codegen/l0IDs.h"
    "codegen/templates/l0EntryPoints.cpp.mako"
    "codegen/templates/l0WrapperFunctions.cpp.mako"
    "codegen/templates/l0WrapperFunctions.h.mako"
    "codegen/templates/l0WrapperFunctionsIface.h.mako"
    "codegen/templates/l0DriversInit.cpp.mako"
    "codegen/templates/l0ArgumentsAuto.h.mako"
    "codegen/templates/l0ArgumentsAuto.cpp.mako"
    "codegen/templates/l0Function.cpp.mako"
    "codegen/templates/l0Functions.h.mako"
    "codegen/templates/l0Functions.cpp.mako"
    "codegen/templates/l0Header.h.mako"
    "codegen/templates/l0Lua.h.mako"
    "codegen/templates/l0Lua.cpp.mako"
    "codegen/templates/l0Log.h.mako"
    "codegen/templates/l0APITest.lua.mako"
    "codegen/templates/l0Constants.lua.mako"
)

if(WITH_OCLOC)
  include_directories(../ocloc/common/include ${CMAKE_BINARY_DIR}/ocloc)
endif()

add_custom_target(L0_codegen
  DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/L0_codegen.stamp"
  SOURCES
    "codegen/generate.py"
    "codegen/generator_l0.py"
    "codegen/generator_callbacks_l0.py"
    "codegen/l0IDs.h"
    "codegen/templates/l0EntryPoints.cpp.mako"
    "codegen/templates/l0WrapperFunctions.cpp.mako"
    "codegen/templates/l0WrapperFunctions.h.mako"
    "codegen/templates/l0WrapperFunctionsIface.h.mako"
    "codegen/templates/l0DriversInit.cpp.mako"
    "codegen/templates/l0ArgumentsAuto.h.mako"
    "codegen/templates/l0ArgumentsAuto.cpp.mako"
    "codegen/templates/l0Function.cpp.mako"
    "codegen/templates/l0Functions.h.mako"
    "codegen/templates/l0Functions.cpp.mako"
    "codegen/templates/l0Header.h.mako"
    "codegen/templates/l0Lua.h.mako"
    "codegen/templates/l0Lua.cpp.mako"
    "codegen/templates/l0Log.h.mako"
    "codegen/templates/helperL0.h.mako"
    "codegen/templates/helperL0.cpp.mako"
    "codegen/templates/l0APITest.lua.mako"
    "codegen/templates/l0Constants.lua.mako"
    )
set_source_files_properties(
  "l0EntryPoints.cpp"
  "l0WrapperFunctions.cpp"
  "l0WrapperFunctions.h"
  "l0WrapperFunctionsIface.h"
  "l0DriversInit.cpp"
  "l0ArgumentsAuto.h"
  "l0ArgumentsAuto.cpp"
  "l0Function.cpp"
  "l0Functions.h"
  "l0Functions.cpp"
  "l0Header.h"
  "l0Lua.h"
  "l0Lua.cpp"
  "l0IDs.h"
  "l0Log.h"
  "helperL0.h"
  "helperL0.cpp"
  "l0APITest.lua"
  "l0Constants.lua"
  PROPERTIES GENERATED 1
)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(WITH_OPENCL)
  add_definitions(-DZE_ENABLE_OCL_INTEROP)
  include_directories(../OpenCL/common/include ${CMAKE_BINARY_DIR}/OpenCL)
endif()

add_library(L0_common
  common/l0Drivers.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/l0DriversInit.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/l0ArgumentsAuto.h
  ${CMAKE_CURRENT_BINARY_DIR}/l0ArgumentsAuto.cpp
  common/include/l0Drivers.h
  common/include/l0ApisIface.h
  common/l0Library.cpp
  common/include/l0Library.h
  ${CMAKE_CURRENT_BINARY_DIR}/l0Function.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/l0IDs.h
  common/include/l0Function.h
  ${CMAKE_CURRENT_BINARY_DIR}/l0Functions.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/l0Functions.h
  common/l0Arguments.cpp
  common/include/l0Arguments.h
  ${CMAKE_CURRENT_BINARY_DIR}/l0Header.h
  ${CMAKE_CURRENT_BINARY_DIR}/l0Lua.h
  ${CMAKE_CURRENT_BINARY_DIR}/l0Lua.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/l0Log.h
  common/l0Log.cpp
  common/include/l0Structs.h
  common/l0Structs.cpp
  common/include/l0StateDynamic.h
  common/include/l0StateTracking.h
  common/include/l0PlayerRunWrap.h
  common/l0StateDynamic.cpp
  common/include/l0Tools.h
  common/l0Tools.cpp
  common/include/l0HelperFunctions.h
  common/l0HelperFunctions.cpp
  common/include/mockListExecutor.h
  common/mockListExecutor.cpp
)
include_directories(common/include)
if(MSVC)
  target_link_libraries(L0_common SetupAPI.lib)
endif()

add_library(L0_recorder
  recorder/l0RecorderWrapper.cpp
  recorder/include/l0RecorderWrapper.h
  recorder/include/l0RecorderWrapperIface.h
  recorder/l0StateRestore.cpp
  recorder/include/l0StateRestore.h
  recorder/include/l0RecorderSubWrappers.h
  ${CMAKE_CURRENT_BINARY_DIR}/l0WrapperFunctionsIface.h
  ${CMAKE_CURRENT_BINARY_DIR}/l0WrapperFunctions.h
  ${CMAKE_CURRENT_BINARY_DIR}/l0WrapperFunctions.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/l0Header.h
)
include_directories(recorder/include)

set(L0_interceptor_sources
  ../common/legacy/tools_lite.cpp
  ../common/legacy/exception.cpp
  ../common/legacy/dynamic_linker.cpp
  ../common/legacy/gitsLoader.cpp
  interceptor/gitsPluginL0.cpp
  interceptor/include/gitsPluginL0.h
  interceptor/include/l0ExecWrap.h
  ${CMAKE_CURRENT_BINARY_DIR}/l0EntryPoints.cpp
  ../common/legacy/log.cpp
)
if(WIN32)
  list(APPEND L0_interceptor_sources ../common/legacy/tools_windows.cpp)
endif()

if (NOT MSVC)
  set(L0_interceptor_sources
    ${L0_interceptor_sources}
    interceptor/l0LinuxInjector.cpp
  )
endif()

if(WIN32)
  add_library(L0_interceptor SHARED
    ${L0_interceptor_sources}
    ${CMAKE_CURRENT_BINARY_DIR}/ZeLoaderVersion.rc
  )
else()
  add_library(L0_interceptor SHARED
    ${L0_interceptor_sources}
  )
endif()

include_directories(interceptor/include)

set_target_properties(L0_interceptor PROPERTIES OUTPUT_NAME ze_loader)
if ("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
  target_link_libraries(L0_interceptor dl pthread)
endif()

add_dependencies(L0_common common L0_codegen config_codegen)
add_dependencies(L0_recorder L0_codegen config_codegen)
add_dependencies(L0_interceptor L0_codegen config_codegen)

set_target_properties(
  L0_codegen L0_common L0_recorder L0_interceptor
  PROPERTIES
  FOLDER LevelZero
  CXX_STANDARD 17)
set_target_properties(L0_interceptor PROPERTIES FOLDER LevelZero SOVERSION 1)

set(warnings
  $<$<CXX_COMPILER_ID:GNU>:
    -Wall -Wextra -Werror>
  $<$<CXX_COMPILER_ID:MSVC>:
    /WX /W4>)

target_compile_options(L0_common PRIVATE ${warnings})
target_compile_options(L0_interceptor PRIVATE ${warnings})
target_compile_options(L0_recorder PRIVATE ${warnings})

if(MSVC)
  install(FILES $<TARGET_PDB_FILE:L0_interceptor> DESTINATION Recorder/${L0_INSTALL_SUBDIR} OPTIONAL)
endif()

install(TARGETS L0_interceptor
  RUNTIME DESTINATION Recorder/${L0_INSTALL_SUBDIR}
  LIBRARY DESTINATION Recorder/${L0_INSTALL_SUBDIR})

set(L0_Lua_SOURCES
  ${CMAKE_CURRENT_BINARY_DIR}/l0APITest.lua
  ${CMAKE_CURRENT_BINARY_DIR}/l0Constants.lua
)
install(FILES ${L0_Lua_SOURCES} DESTINATION LuaScripts/LevelZero)

set(L0_CCode_SOURCES
  ${CMAKE_CURRENT_BINARY_DIR}/helperL0.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/l0DriversInit.cpp
  common/l0Drivers.cpp
  common/l0Log.cpp)

set(L0_CCode_HEADERS
  ${CMAKE_CURRENT_BINARY_DIR}/helperL0.h
  common/include/l0Drivers.h
  ${CMAKE_CURRENT_BINARY_DIR}/l0Header.h
  ${CMAKE_CURRENT_BINARY_DIR}/l0Log.h)

install(FILES ${L0_CCode_SOURCES} DESTINATION CCode/CCodeSource)
install(FILES ${L0_CCode_HEADERS} DESTINATION CCode/CCodeSource/include)

if(WIN32)
  if(MSVC)
    install(FILES $<TARGET_PDB_FILE:L0_interceptor> RENAME ze_intel_gpu64.pdb DESTINATION Recorder/${L0_INSTALL_SUBDIR} OPTIONAL)
  endif()
  install(FILES $<TARGET_FILE:L0_interceptor> RENAME ze_intel_gpu64.dll DESTINATION Recorder/${L0_INSTALL_SUBDIR} OPTIONAL)
endif()
