# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

set(STAMP_FILE ${CMAKE_CURRENT_BINARY_DIR}/DirectX_codegen.stamp)

add_custom_target(DirectX_codegen DEPENDS ${STAMP_FILE})
set_target_properties(DirectX_codegen PROPERTIES FOLDER DirectX)

set(SCRIPTS_SRC
  header_preprocessor.py
  intermediates.py
  intermediates_creator.py
  command_ids.json
  command_ids.py
  generator_helpers.py
  generator.py
  generator_recorder.py
  generator_coders.py
  generator_trace.py
  generator_layer.py
  generator_player.py
  generator_subcapture.py
  generator_execution_serialization.py
  generator_resource_dumping.py
  generator_skip_calls.py
  plugin_generator.py
  generator_xess.py
  generator_xess_helpers.py
)
source_group("scripts" FILES ${SCRIPTS_SRC})

file(GLOB_RECURSE MAKO_TEMPLATES "templates/*.mako")
source_group(mako FILES ${MAKO_TEMPLATES})

target_sources(DirectX_codegen PRIVATE
  ${MAKO_TEMPLATES}
  ${SCRIPTS_SRC}
)

set(DIRECTX_SCRIPTS_DIR ${CMAKE_SOURCE_DIR}/DirectX/codegen)
set(DIRECTX_SCRIPT_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/DirectX)

find_program(CLANG_FORMAT_EXE clang-format)

add_custom_command(
  OUTPUT ${STAMP_FILE}
  COMMAND ${CMAKE_COMMAND} -E echo "Running DirectX code generation script..."
  COMMAND ${Python3_EXECUTABLE} ${DIRECTX_SCRIPTS_DIR}/generator.py ${D3D12_HEADERS_DIR} ${DML_HEADERS_DIR} ${XESS_HEADERS_DIR} ${DSTORAGE_HEADERS_DIR} ${DIRECTX_SCRIPT_OUTPUT_DIR}
  COMMAND ${CMAKE_COMMAND} -DOUT_DIR=${DIRECTX_SCRIPT_OUTPUT_DIR} -DCLANG_FORMAT=${CLANG_FORMAT_EXE} -P ${DIRECTX_SCRIPTS_DIR}/run_clang_format.cmake
  COMMAND ${CMAKE_COMMAND} -E touch ${STAMP_FILE}
  COMMAND ${CMAKE_COMMAND} -E echo "DirectX code generation completed (${STAMP_FILE})."
  WORKING_DIRECTORY ${DIRECTX_SCRIPTS_DIR}
)

unset(DirectX_codegen CACHE)
