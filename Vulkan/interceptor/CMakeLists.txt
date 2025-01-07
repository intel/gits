# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

set_source_files_properties(
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkanPrePostAuto.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/vkPlugin.def
  PROPERTIES GENERATED 1
)

set(Vulkan_DEF )
if(MSVC)
  set(Vulkan_DEF vkPlugin.def)
endif()

add_definitions(-DBUILD_FOR_VULKAN_INTERCEPTOR)

list(APPEND Vulkan_interceptor_external_cpps
  ../../common/tools_lite.cpp
  ../../common/exception.cpp
  ../../common/dynamic_linker.cpp
  ../../common/gitsLoader.cpp
  ../../common/log.cpp
)
if(WIN32)
  list(APPEND Vulkan_interceptor_external_cpps ../../common/tools_windows.cpp)
endif()

add_library(Vulkan_interceptor SHARED
  include/gitsPluginVulkan.h
  include/vulkanExecWrap.h

  ${Vulkan_interceptor_external_cpps}
  include/gitsPluginVulkan.cpp
  vulkanPrePostAuto.cpp
  ${Vulkan_DEF}
)

source_group("External Files" FILES ${Vulkan_interceptor_external_cpps})
source_group("Generated Files" FILES ${Vulkan_DEF} REGULAR_EXPRESSION ".*Auto.*")

if(MSVC)
  set_target_properties(Vulkan_interceptor PROPERTIES OUTPUT_NAME vulkan-1)
else()
  set_target_properties(Vulkan_interceptor PROPERTIES OUTPUT_NAME Vulkan)
  target_link_libraries(Vulkan_interceptor pthread dl)
endif()

set_target_properties(Vulkan_interceptor PROPERTIES FOLDER Vulkan)

if(MSVC)
  install(FILES $<TARGET_PDB_FILE:Vulkan_interceptor> DESTINATION Recorder/${Vulkan_INSTALL_SUBDIR} OPTIONAL)
endif()

install(TARGETS Vulkan_interceptor
  RUNTIME DESTINATION Recorder/${Vulkan_INSTALL_SUBDIR}
  LIBRARY DESTINATION Recorder/${Vulkan_INSTALL_SUBDIR})
