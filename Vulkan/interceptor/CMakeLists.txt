# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(Vulkan_interceptor SHARED)

set(COMMON_SRC
  ${CMAKE_SOURCE_DIR}/common/legacy/tools_lite.cpp
  ${CMAKE_SOURCE_DIR}/common/legacy/exception.cpp
  ${CMAKE_SOURCE_DIR}/common/legacy/dynamic_linker.cpp
  ${CMAKE_SOURCE_DIR}/common/legacy/gitsLoader.cpp
)
source_group("common" FILES ${COMMON_SRC})

set(ROOT_SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/interceptorAuto.cpp
)
source_group("" FILES ${ROOT_SRC})

set(ALL_SRC
  ${COMMON_SRC}
  ${ROOT_SRC}
)

set_auto_generated_property(FILE_LIST ${ALL_SRC})

target_sources(Vulkan_interceptor PRIVATE
  ${ALL_SRC}
)

target_include_directories(Vulkan_interceptor PRIVATE
  ${VULKAN_HEADERS_DIR}
  ${VULKAN_DIR}/recorder
)
target_link_libraries(Vulkan_interceptor PRIVATE
  gits_recorder_interface
  Vulkan_layer_interface
  Vulkan_recorder
)

if(NOT WIN32)
  target_link_libraries(Vulkan_interceptor PRIVATE pthread dl)
endif()

if(MSVC)
  set_target_properties(Vulkan_interceptor PROPERTIES OUTPUT_NAME vulkan-1)
else()
  set_target_properties(Vulkan_interceptor PROPERTIES OUTPUT_NAME Vulkan)
  target_link_libraries(Vulkan_interceptor PRIVATE pthread dl)
endif()

set_target_properties(Vulkan_interceptor PROPERTIES FOLDER Vulkan)

if(MSVC)
  install(FILES $<TARGET_PDB_FILE:Vulkan_interceptor> DESTINATION Recorder/${Vulkan_INSTALL_SUBDIR} OPTIONAL)
endif()

install(TARGETS Vulkan_interceptor
  RUNTIME DESTINATION Recorder/${Vulkan_INSTALL_SUBDIR}
  LIBRARY DESTINATION Recorder/${Vulkan_INSTALL_SUBDIR})
