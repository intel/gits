# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(WITH_LEVELZERO)
  include_directories(../LevelZero/common/include ${CMAKE_BINARY_DIR}/LevelZero)
endif()

add_library(ocloc_common
  common/oclocDrivers.cpp
  common/include/oclocDrivers.h
  common/oclocLibrary.cpp
  common/include/oclocLibrary.h
  common/oclocArguments.cpp
  common/include/oclocArguments.h
  common/oclocFunction.cpp
  common/include/oclocFunction.h
  common/oclocFunctions.cpp
  common/include/oclocFunctions.h
  common/include/oclocHeader.h
  common/include/oclocStateTracking.h
  common/include/oclocStateDynamic.h
  common/oclocStateDynamic.cpp
  common/include/oclocTools.h
  common/oclocTools.cpp
  common/include/arDecoder.h
  common/arDecoder.cpp
)
include_directories(common/include)

add_library(ocloc_recorder
  recorder/oclocRecorderWrapper.cpp
  recorder/oclocRecorderWrapper.h
  recorder/oclocRecorderWrapperIface.h
)

include_directories(recorder)

list(APPEND ocloc_interceptor_files
  ../common/tools_lite.cpp
  ../common/exception.cpp
  ../common/dynamic_linker.cpp
  ../common/gitsLoader.cpp
  ../common/log.cpp
  interceptor/gitsPluginOcloc.cpp
  interceptor/include/gitsPluginOcloc.h
  interceptor/oclocEntryPoints.cpp
)
if(WIN32)
  list(APPEND ocloc_interceptor_files ../common/tools_windows.cpp)
endif()

add_library(ocloc_interceptor SHARED ${ocloc_interceptor_files})
include_directories(interceptor/include)

if(WIN32)
  set(OCLOC_LIBRARY_NAME ocloc64)
else()
  set(OCLOC_LIBRARY_NAME ocloc)
endif()

set_target_properties(ocloc_interceptor PROPERTIES OUTPUT_NAME ${OCLOC_LIBRARY_NAME})
if ("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
  target_link_libraries(ocloc_interceptor dl pthread)
endif()

add_dependencies(ocloc_common common)

set_target_properties(ocloc_common PROPERTIES FOLDER ocloc)
set_target_properties(ocloc_recorder PROPERTIES FOLDER ocloc)
set_target_properties(ocloc_interceptor PROPERTIES FOLDER ocloc)

if(MSVC)
  install(FILES $<TARGET_PDB_FILE:ocloc_interceptor> RENAME ${OCLOC_LIBRARY_NAME}.pdb DESTINATION Recorder/${L0_INSTALL_SUBDIR} OPTIONAL)
endif()

install(TARGETS ocloc_interceptor
  RUNTIME DESTINATION Recorder/${L0_INSTALL_SUBDIR}
  LIBRARY DESTINATION Recorder/${L0_INSTALL_SUBDIR})
