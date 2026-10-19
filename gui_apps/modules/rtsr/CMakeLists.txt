# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

set(RTSR_MODULE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# Protocol, transport and state serialization, shared with this module's plugins.
add_subdirectory(shared)
add_subdirectory(toolchain)

list(APPEND RTSR_MODULE_HEADER
  ${RTSR_MODULE_DIR}/ui/rtsrPanel.h
  ${RTSR_MODULE_DIR}/ui/rtsrModule.h
  ${RTSR_MODULE_DIR}/ui/rtsrEvents.h
  ${RTSR_MODULE_DIR}/ui/rtsrLabels.h
  ${RTSR_MODULE_DIR}/ui/rtsrShaderPanel.h
  ${RTSR_MODULE_DIR}/ui/rtsrShaderSourceCache.h
  ${RTSR_MODULE_DIR}/ui/rtsrReplacementService.h
  ${RTSR_MODULE_DIR}/ui/rtsrPerformancePanel.h
  ${RTSR_MODULE_DIR}/ui/rtsrPerformanceStats.h
  ${RTSR_MODULE_DIR}/ui/rtsrPlot.h
  ${RTSR_MODULE_DIR}/ui/rtsrSelection.h
)

list(APPEND RTSR_MODULE_SOURCE
  ${RTSR_MODULE_DIR}/ui/rtsrPanel.cpp
  ${RTSR_MODULE_DIR}/ui/rtsrModule.cpp
  ${RTSR_MODULE_DIR}/ui/rtsrShaderPanel.cpp
  ${RTSR_MODULE_DIR}/ui/rtsrShaderSourceCache.cpp
  ${RTSR_MODULE_DIR}/ui/rtsrReplacementService.cpp
  ${RTSR_MODULE_DIR}/ui/rtsrPerformancePanel.cpp
  ${RTSR_MODULE_DIR}/ui/rtsrPerformanceStats.cpp
  ${RTSR_MODULE_DIR}/ui/rtsrPlot.cpp
  ${RTSR_MODULE_DIR}/ui/rtsrSelection.cpp
)
source_group("UI/Source files" FILES ${RTSR_MODULE_SOURCE})
source_group("UI/Header files" FILES ${RTSR_MODULE_HEADER})

add_library(rtsr_module STATIC ${RTSR_MODULE_HEADER} ${RTSR_MODULE_SOURCE})

target_include_directories(rtsr_module
PUBLIC
  ${VULKAN_HEADERS_DIR}
  ${IMGUI_COLOR_TEXT_EDIT_DIR}
  ${PLOG_SOURCE_DIR}/include
  ${COMMON_HEADER_DIR}
)

target_link_libraries(rtsr_module
PUBLIC
  module_common
  rtsr_module_shared
  imgui_common
  gui_common
PRIVATE
  rtsr_module_toolchain
  log
  common
  stream
  # Only the performance view plots, and no public header of this module exposes
  # an ImPlot type, so it stays private to the implementation.
  implot
)

target_compile_options(rtsr_module PRIVATE
  $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:
    -Wno-deprecated-enum-enum-conversion
    -Wno-error=reorder
    -Wno-error=sign-compare
    -Wno-error=parentheses
    -Wno-error=sequence-point
    -Wno-error=unknown-pragmas
  >
  $<$<CXX_COMPILER_ID:MSVC>:
    /wd4996  # deprecated warnings
    /wd4244  # conversion warnings
    /wd4267  # size_t conversion warnings
    /wd4018  # signed/unsigned mismatch
  >
)

add_dependencies(rtsr_module config_codegen generate_font_header)

set_target_properties(rtsr_module PROPERTIES FOLDER "${GUI_MODULES_FOLDER}/rtsr")

# Announces this module to the applications, which do not name it. The id has to
# match the ModuleId enumerator passed to GITS_GUI_REGISTER_MODULE in
# ui/rtsrPanel.cpp.
gui_module_register(rtsr_module Rtsr)

# GITS plugins shipped by this module. They are ordinary GITS plugins and install
# next to the others, into Plugins/<API>/<name>.
add_subdirectory(gitsPlugins/common)

if(BUILD_DIRECTX_PLUGINS)
  add_subdirectory(gitsPlugins/DirectX/rtsr)
endif()
if(WIN32 AND BUILD_VULKAN_PLUGINS)
  add_subdirectory(gitsPlugins/Vulkan/rtsr)
endif()
