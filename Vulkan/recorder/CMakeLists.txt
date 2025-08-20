# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

set_source_files_properties(
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vulkanRecorderWrapperAuto.h
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkanRecorderWrapperAuto.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/vulkanRecorderWrapperIfaceAuto.h
  PROPERTIES GENERATED 1
)

add_library(Vulkan_recorder
  include/vulkanRecorderSubwrappers.h
  include/vulkanRecorderWrapper.h
  include/vulkanRecorderWrapperAuto.h
  include/vulkanRecorderWrapperIface.h
  include/vulkanRecorderWrapperIfaceAuto.h
  include/vulkanStateRestore.h

  vulkanRecorderWrapper.cpp
  vulkanRecorderWrapperAuto.cpp
  vulkanStateRestore.cpp
)

source_group("Generated Files" REGULAR_EXPRESSION ".*Auto.*")

target_link_libraries(Vulkan_recorder PRIVATE GitsRecorderInterface common configuration Vulkan_common)
set_target_properties(Vulkan_recorder PROPERTIES FOLDER Vulkan)
