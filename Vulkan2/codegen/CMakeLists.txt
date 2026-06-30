# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

set(STAMP_FILE ${CMAKE_CURRENT_BINARY_DIR}/Vulkan_codegen_2.stamp)

add_custom_target(Vulkan_codegen_2 DEPENDS ${STAMP_FILE})
set_target_properties(Vulkan_codegen_2 PROPERTIES FOLDER Vulkan2)

set(SCRIPTS_SRC
  intermediates.py
  intermediates_creator.py
  command_ids.json
  command_ids.py
  generator_helpers.py
  generator.py
  generator_recorder.py
  generator_layer.py
  generator_coders.py
  generator_player.py
)
source_group("scripts" FILES ${SCRIPTS_SRC})

file(GLOB_RECURSE MAKO_TEMPLATES "templates/*.mako")
source_group(mako FILES ${MAKO_TEMPLATES})

target_sources(Vulkan_codegen_2 PRIVATE
  ${MAKO_TEMPLATES}
  ${SCRIPTS_SRC}
)

set(VULKAN_SCRIPTS_DIR ${CMAKE_SOURCE_DIR}/Vulkan2/codegen)
set(VULKAN_SCRIPT_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/Vulkan2)

find_program(CLANG_FORMAT_EXE clang-format)

add_custom_command(
  OUTPUT ${STAMP_FILE}
  COMMAND ${CMAKE_COMMAND} -E echo "Running Vulkan code generation script..."
  COMMAND ${Python3_EXECUTABLE} ${VULKAN_SCRIPTS_DIR}/generator.py --xml ${VK_XML_PATH} --output ${VULKAN_SCRIPT_OUTPUT_DIR}
  COMMAND ${CMAKE_COMMAND} -DOUT_DIR=${VULKAN_SCRIPT_OUTPUT_DIR} -DCLANG_FORMAT=${CLANG_FORMAT_EXE} -P ${VULKAN_SCRIPTS_DIR}/run_clang_format.cmake
  COMMAND ${CMAKE_COMMAND} -E touch ${STAMP_FILE}
  COMMAND ${CMAKE_COMMAND} -E echo "Vulkan generation completed - ${STAMP_FILE}"
  WORKING_DIRECTORY ${VULKAN_SCRIPTS_DIR}
  DEPENDS ${SCRIPTS_SRC} ${MAKO_TEMPLATES} ${VK_XML_PATH}
)

unset(Vulkan_codegen_2 CACHE)
