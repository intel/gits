# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(Vulkan_trace STATIC)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(TRACE_SRC
  ${SRC_DIR}/commandPrinter.h
  ${SRC_DIR}/commandPrinter.cpp
  ${SRC_DIR}/enumToStrAuto.h
  ${SRC_DIR}/enumToStrAuto.cpp
  ${SRC_DIR}/printArguments.h
  ${SRC_DIR}/printCustom.h
  ${SRC_DIR}/printCustom.cpp
  ${SRC_DIR}/printBitmasksAuto.h
  ${SRC_DIR}/printBitmasksAuto.cpp
  ${SRC_DIR}/printEnumsAuto.h
  ${SRC_DIR}/printEnumsAuto.cpp
  ${SRC_DIR}/printUnionsAuto.h
  ${SRC_DIR}/printUnionsAuto.cpp
  ${SRC_DIR}/printStructuresCustom.h
  ${SRC_DIR}/printStructuresCustom.cpp
  ${SRC_DIR}/printStructuresAuto.h
  ${SRC_DIR}/printStructuresAuto.cpp
  ${SRC_DIR}/traceLayerCustom.cpp
  ${SRC_DIR}/traceLayerAuto.h
  ${SRC_DIR}/traceLayerAuto.cpp
  ${SRC_DIR}/traceLayerGroup.h
  ${SRC_DIR}/traceLayerGroup.cpp
)
source_group("trace" FILES ${TRACE_SRC})

set(ALL_SRC
  ${TRACE_SRC}
)

set_auto_generated_property(FILE_LIST ${ALL_SRC})

target_sources(Vulkan_trace PRIVATE
  ${ALL_SRC}
)

target_include_directories(Vulkan_trace PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(Vulkan_trace PRIVATE
  Vulkan_layer_interface
  fastOutput
)

set_target_properties(Vulkan_trace PROPERTIES FOLDER Vulkan2/Vulkan_layers)
