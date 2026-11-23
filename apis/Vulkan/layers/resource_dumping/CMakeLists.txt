# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(Vulkan_resource_dumping STATIC)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(ROOT_SRC
  ${SRC_DIR}/resourceDumpingLayerGroup.h
  ${SRC_DIR}/resourceDumpingLayerGroup.cpp
)
source_group("" FILES ${ROOT_SRC})

set(SCREENSHOTS_SRC
  ${SRC_DIR}/screenshotsLayer.h
  ${SRC_DIR}/screenshotsLayer.cpp
  ${SRC_DIR}/swapchainImagesDumper.h
  ${SRC_DIR}/swapchainImagesDumper.cpp
)
source_group("screenshots" FILES ${SCREENSHOTS_SRC})

set(RESOURCES_DUMP_SRC
  ${SRC_DIR}/resourcesDumpLayerAuto.h
  ${SRC_DIR}/resourcesDumpLayerAuto.cpp
  ${SRC_DIR}/resourcesDumpLayerCustom.cpp
  ${SRC_DIR}/resourcesDumpService.h
  ${SRC_DIR}/resourcesDumpService.cpp
  ${SRC_DIR}/resourcesDump.h
  ${SRC_DIR}/resourcesDump.cpp
  ${SRC_DIR}/dumpTarget.h
  ${SRC_DIR}/dumpTarget.cpp
  ${SRC_DIR}/imageLayouts.h
  ${SRC_DIR}/imageLayouts.cpp
  ${SRC_DIR}/queueFamilyOwnership.h
  ${SRC_DIR}/queueFamilyOwnership.cpp
  ${SRC_DIR}/submitSignalsDeferral.h
  ${SRC_DIR}/submitSignalsDeferral.cpp
  ${SRC_DIR}/imageDumpWriter.h
  ${SRC_DIR}/imageDumpWriter.cpp
  ${SRC_DIR}/dumpKeys.h
  ${SRC_DIR}/dumpKeys.cpp
)
source_group("resources_dump" FILES ${RESOURCES_DUMP_SRC})

set(ALL_SRC
  ${ROOT_SRC}
  ${SCREENSHOTS_SRC}
  ${RESOURCES_DUMP_SRC}
)

set_auto_generated_property(FILE_LIST  ${ALL_SRC})

target_sources(Vulkan_resource_dumping PRIVATE
  ${ALL_SRC}
)

add_dependencies(Vulkan_resource_dumping Vulkan_codegen)

target_link_libraries(Vulkan_resource_dumping PRIVATE
  stb_implementation
  Vulkan_layer_interface
  Vulkan_trace
  Vulkan_utils
)

target_include_directories(Vulkan_resource_dumping PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(Vulkan_resource_dumping PRIVATE
  ${STB_DIR}
)

set_target_properties(Vulkan_resource_dumping PROPERTIES FOLDER Vulkan/Vulkan_layers)

# Host-only checks of the key parsing, the layout and ownership tracking, the copies and the image
# conversion: no GPU, no driver, no stream. Run the executable; it prints failures and exits non-zero.
add_executable(Vulkan_resource_dumping_test
  ${SRC_DIR}/tests/resourcesDumpTest.cpp
)

target_link_libraries(Vulkan_resource_dumping_test PRIVATE
  Vulkan_resource_dumping
  Vulkan_layer_interface
  Vulkan_utils
)

target_include_directories(Vulkan_resource_dumping_test PRIVATE
  ${STB_DIR}
)

set_target_properties(Vulkan_resource_dumping_test PROPERTIES FOLDER Vulkan)
