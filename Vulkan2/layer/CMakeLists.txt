# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(Vulkan_layer_2 SHARED)

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

target_sources(Vulkan_layer_2 PRIVATE
  ${COMMON_SRC}
  ${ROOT_SRC}
)

target_include_directories(Vulkan_layer_2 PRIVATE
  ${VULKAN_HEADERS_DIR}
  ${VULKAN2_DIR}/recorder
)
target_link_libraries(Vulkan_layer_2 PRIVATE
  gits_recorder_interface
  Vulkan_layer_interface
  Vulkan_recorder_2
)

if(NOT WIN32)
  target_link_libraries(Vulkan_layer_2 PRIVATE pthread dl)
endif()

set_target_properties(Vulkan_layer_2 PROPERTIES OUTPUT_NAME VkLayer_vulkan_GITS_recorder)
set_target_properties(Vulkan_layer_2 PROPERTIES FOLDER Vulkan2)

if(MSVC)
  install(FILES $<TARGET_PDB_FILE:Vulkan_layer_2> DESTINATION Recorder/${VulkanLayer2_INSTALL_SUBDIR} OPTIONAL)
endif()
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/gits_recorder.json DESTINATION Recorder/${VulkanLayer2_INSTALL_SUBDIR})
install(TARGETS Vulkan_layer_2
  RUNTIME DESTINATION Recorder/${VulkanLayer2_INSTALL_SUBDIR}
  LIBRARY DESTINATION Recorder/${VulkanLayer2_INSTALL_SUBDIR})
