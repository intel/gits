# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(Vulkan_layer SHARED)

set(COMMON_SRC
  ${CMAKE_SOURCE_DIR}/common/legacy/tools_lite.cpp
  ${CMAKE_SOURCE_DIR}/common/legacy/exception.cpp
  ${CMAKE_SOURCE_DIR}/common/legacy/dynamic_linker.cpp
  ${CMAKE_SOURCE_DIR}/common/legacy/gitsLoader.cpp
)
source_group("common" FILES ${COMMON_SRC})

set(ROOT_SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/VkLayer_vulkan_GITS_recorder.cpp
)
source_group("" FILES ${ROOT_SRC})

target_sources(Vulkan_layer PRIVATE
  ${COMMON_SRC}
  ${ROOT_SRC}
)

target_include_directories(Vulkan_layer PRIVATE
  ${VULKAN_HEADERS_DIR}
  ${VULKAN_DIR}/recorder
)
target_link_libraries(Vulkan_layer PRIVATE
  gits_recorder_interface
  Vulkan_layer_interface
  Vulkan_recorder
)

if(NOT WIN32)
  target_link_libraries(Vulkan_layer PRIVATE pthread dl)
endif()

set_target_properties(Vulkan_layer PROPERTIES OUTPUT_NAME VkLayer_vulkan_GITS_recorder)
set_target_properties(Vulkan_layer PROPERTIES FOLDER Vulkan)

if(MSVC)
  install(FILES $<TARGET_PDB_FILE:Vulkan_layer> DESTINATION Recorder/${VulkanLayer_INSTALL_SUBDIR} OPTIONAL)
endif()
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/VkLayer_vulkan_GITS_recorder.json DESTINATION Recorder/${VulkanLayer_INSTALL_SUBDIR})
install(TARGETS Vulkan_layer
  RUNTIME DESTINATION Recorder/${VulkanLayer_INSTALL_SUBDIR}
  LIBRARY DESTINATION Recorder/${VulkanLayer_INSTALL_SUBDIR})
