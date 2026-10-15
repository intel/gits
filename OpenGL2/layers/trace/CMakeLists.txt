# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

set(GENERATED_SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/traceLayerAuto.h
  ${CMAKE_CURRENT_SOURCE_DIR}/traceLayerAuto.cpp
)
set_source_files_properties(${GENERATED_SRC} PROPERTIES GENERATED TRUE)
source_group("Generated Files" FILES ${GENERATED_SRC})

add_library(OpenGL2_trace STATIC
  ${GENERATED_SRC}
  commandPrinter.h
  commandPrinter.cpp
  printArguments.h
  traceLayerCustom.cpp
  traceLayerGroup.h
  traceLayerGroup.cpp
)
add_dependencies(OpenGL2_trace OpenGL2_codegen)
target_include_directories(OpenGL2_trace PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(OpenGL2_trace PUBLIC OpenGL2_layer_interface fastOutput common_interface)
set_target_properties(OpenGL2_trace PROPERTIES FOLDER OpenGL2)