# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

set(CMAKE_INCLUDE_CURRENT_DIR ON)

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
)
include_directories(common/include)

add_library(ocloc_recorder
  recorder/oclocRecorderWrapper.cpp
  recorder/oclocRecorderWrapper.h
  recorder/oclocRecorderWrapperIface.h
)
target_link_libraries(ocloc_recorder Boost::filesystem)

include_directories(recorder)

add_library(ocloc_interceptor SHARED
  ../common/exception.cpp
  ../common/dynamic_linker.cpp
  ../recorder/gitsLoader.cpp
  interceptor/gitsPluginOcloc.cpp
  interceptor/include/gitsPluginOcloc.h
  interceptor/oclocEntryPoints.cpp
  ../common/log.cpp
)
include_directories(interceptor/include)
target_link_libraries(ocloc_interceptor Boost::thread Boost::filesystem)

set_target_properties(ocloc_interceptor PROPERTIES OUTPUT_NAME ocloc)
if ("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
  target_link_libraries(ocloc_interceptor dl pthread)
endif()

add_dependencies(ocloc_common common)

set_target_properties(ocloc_common PROPERTIES FOLDER ocloc)
set_target_properties(ocloc_recorder PROPERTIES FOLDER ocloc)
set_target_properties(ocloc_interceptor PROPERTIES FOLDER ocloc)

if(MSVC)
  install(FILES $<TARGET_PDB_FILE:ocloc_interceptor> DESTINATION Recorder/${L0_INSTALL_SUBDIR} OPTIONAL)
endif()

install(TARGETS ocloc_interceptor
  RUNTIME DESTINATION Recorder/${L0_INSTALL_SUBDIR}
  LIBRARY DESTINATION Recorder/${L0_INSTALL_SUBDIR})
