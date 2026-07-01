# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

set_source_files_properties(
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkanPrePostAuto.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/vkPlugin.def
  PROPERTIES GENERATED 1
)

set(VulkanLegacy_DEF )
if(MSVC)
  set(VulkanLegacy_DEF vkPlugin.def)
endif()

add_definitions(-DBUILD_FOR_VULKAN_INTERCEPTOR)

list(APPEND VulkanLegacy_interceptor_external_cpps
  ../../common/legacy/tools_lite.cpp
  ../../common/legacy/exception.cpp
  ../../common/legacy/dynamic_linker.cpp
  ../../common/legacy/gitsLoader.cpp
)
if(WIN32)
  list(APPEND VulkanLegacy_interceptor_external_cpps ../../common/legacy/tools_windows.cpp)
endif()

add_library(VulkanLegacy_interceptor SHARED
  include/gitsPluginVulkan.h
  include/vulkanExecWrap.h
  include/vulkanThreadSync.h

  ${VulkanLegacy_interceptor_external_cpps}
  include/gitsPluginVulkan.cpp
  vulkanPrePostAuto.cpp
  ${VulkanLegacy_DEF}
)

if(WIN32)
    target_compile_definitions(VulkanLegacy_interceptor PRIVATE PLOG_IMPORT)
else()
    target_compile_definitions(VulkanLegacy_interceptor PRIVATE PLOG_GLOBAL)
endif()

source_group("External Files" FILES ${VulkanLegacy_interceptor_external_cpps})
source_group("Generated Files" FILES ${VulkanLegacy_DEF} REGULAR_EXPRESSION ".*Auto.*")

if(WIN32)
    target_link_options(VulkanLegacy_interceptor PRIVATE "/DELAYLOAD:gitsRecorder.dll")
endif()

target_include_directories(VulkanLegacy_interceptor PRIVATE ${VULKAN_HEADERS_DIR})
target_link_libraries(VulkanLegacy_interceptor PRIVATE gits_recorder gits_recorder_interface common_interface configuration log)
if(WIN32)
  target_link_libraries(VulkanLegacy_interceptor PRIVATE hud)
endif()

if(MSVC)
  set_target_properties(VulkanLegacy_interceptor PROPERTIES OUTPUT_NAME vulkan-1)
else()
  set_target_properties(VulkanLegacy_interceptor PROPERTIES OUTPUT_NAME Vulkan)
  target_link_libraries(VulkanLegacy_interceptor PRIVATE pthread dl)
endif()

set_target_properties(VulkanLegacy_interceptor PROPERTIES FOLDER VulkanLegacy)

if(MSVC)
  install(FILES $<TARGET_PDB_FILE:VulkanLegacy_interceptor> DESTINATION Recorder/${VulkanLegacy_INSTALL_SUBDIR} OPTIONAL)
endif()

install(TARGETS VulkanLegacy_interceptor
  RUNTIME DESTINATION Recorder/${VulkanLegacy_INSTALL_SUBDIR}
  LIBRARY DESTINATION Recorder/${VulkanLegacy_INSTALL_SUBDIR})
