# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

set(STAMP_FILE ${CMAKE_CURRENT_BINARY_DIR}/OpenGL2_codegen.stamp)

add_custom_target(OpenGL2_codegen DEPENDS ${STAMP_FILE})
set_target_properties(OpenGL2_codegen PROPERTIES FOLDER OpenGL2)

set(SCRIPTS_SRC
  array_of_arrays_metadata.py
  buffer_content_hooks.py
  capability_suppression.py
  client_array_draws.py
  command_ids.py
  custom_player_runners.py
  intermediates.py
  intermediates_creator.py
  generator_helpers.py
  generator_enums.py
  generator.py
  generator_interceptor.py
  generator_layer_interface.py
  generator_player.py
  generator_plugins.py
  generator_recorder.py
  handle_lifecycle.py
  object_name_lifecycle.py
  pixel_transfer_metadata.py
  pname_value_counts.py
  pointer_length_derivation.py
  pointer_metadata.py
  tex_sampler_parameter_metadata.py
  uniform_setter_metadata.py
  xml_data_exploration.py
  registry.py
)
source_group("scripts" FILES ${SCRIPTS_SRC})

set(PERSISTENT_DATA_SRC
  command_ids.json
)
source_group("persistent data" FILES ${PERSISTENT_DATA_SRC})

file(GLOB_RECURSE MAKO_TEMPLATES "templates/*.mako")
source_group(mako FILES ${MAKO_TEMPLATES})

set(OPENGL2_PLUGINS_DIR ${CMAKE_SOURCE_DIR}/plugins/OpenGL2)
file(GLOB PLUGIN_MAKO_TEMPLATES "${OPENGL2_PLUGINS_DIR}/*/codegen/templates/*.mako")
source_group("plugin mako" FILES ${PLUGIN_MAKO_TEMPLATES})

target_sources(OpenGL2_codegen PRIVATE
  ${MAKO_TEMPLATES}
  ${PLUGIN_MAKO_TEMPLATES}
  ${PERSISTENT_DATA_SRC}
  ${SCRIPTS_SRC}
)

set(OPENGL2_SCRIPTS_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(OPENGL2_LAYER_INTERFACE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../common/layer_interface)
set(OPENGL2_GENERATED_LAYER_INTERFACE_FILES
  ${OPENGL2_LAYER_INTERFACE_DIR}/commandIdsAuto.h
  ${OPENGL2_LAYER_INTERFACE_DIR}/commandsAuto.h
  ${OPENGL2_LAYER_INTERFACE_DIR}/layerAuto.h
  ${OPENGL2_LAYER_INTERFACE_DIR}/commandCodersAuto.h
  ${OPENGL2_LAYER_INTERFACE_DIR}/commandCodersAuto.cpp
  ${OPENGL2_LAYER_INTERFACE_DIR}/commandSerializersAuto.h
  ${OPENGL2_LAYER_INTERFACE_DIR}/glEnumsAuto.h
  ${OPENGL2_LAYER_INTERFACE_DIR}/glEnumsAuto.cpp
  ${OPENGL2_LAYER_INTERFACE_DIR}/pnameValueCountsAuto.cpp
)
set(OPENGL2_INTERCEPTOR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../interceptor)
set(OPENGL2_GENERATED_INTERCEPTOR_FILES
  ${OPENGL2_INTERCEPTOR_DIR}/interceptorAuto.cpp
  ${OPENGL2_INTERCEPTOR_DIR}/interceptorAuto.def
)
set(OPENGL2_RECORDER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../recorder)
set(OPENGL2_GENERATED_RECORDER_FILES
  ${OPENGL2_RECORDER_DIR}/include/openglDriversAuto.h
  ${OPENGL2_RECORDER_DIR}/openglDriversAuto.cpp
  ${OPENGL2_RECORDER_DIR}/encoderLayerAuto.h
  ${OPENGL2_RECORDER_DIR}/encoderLayerAuto.cpp
  ${OPENGL2_RECORDER_DIR}/clientArraysLayerAuto.h
  ${OPENGL2_RECORDER_DIR}/clientArraysLayerAuto.cpp
  ${OPENGL2_RECORDER_DIR}/recorderWrappersAuto.cpp
)
set(OPENGL2_PLAYER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../player)
set(OPENGL2_GENERATED_PLAYER_FILES
  ${OPENGL2_PLAYER_DIR}/include/openglDriversAuto.h
  ${OPENGL2_PLAYER_DIR}/openglDriversAuto.cpp
  ${OPENGL2_PLAYER_DIR}/include/commandRunnersAuto.h
  ${OPENGL2_PLAYER_DIR}/commandRunnersAuto.cpp
  ${OPENGL2_PLAYER_DIR}/openGL2CommandFactoryAuto.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../layers/trace/traceLayerAuto.h
  ${CMAKE_CURRENT_SOURCE_DIR}/../layers/trace/traceLayerAuto.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../subcapture/recordingLayerAuto.h
  ${CMAKE_CURRENT_SOURCE_DIR}/../subcapture/recordingLayerAuto.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../subcapture/analyzerLayerAuto.h
  ${CMAKE_CURRENT_SOURCE_DIR}/../subcapture/analyzerLayerAuto.cpp
)
set(OPENGL2_GENERATED_PLUGIN_FILES
  ${OPENGL2_PLUGINS_DIR}/statistics/statisticsLayerAuto.h
  ${OPENGL2_PLUGINS_DIR}/statistics/statisticsLayerAuto.cpp
)

add_custom_command(
  OUTPUT ${STAMP_FILE} ${OPENGL2_GENERATED_LAYER_INTERFACE_FILES}
         ${OPENGL2_GENERATED_INTERCEPTOR_FILES} ${OPENGL2_GENERATED_RECORDER_FILES}
         ${OPENGL2_GENERATED_PLAYER_FILES} ${OPENGL2_GENERATED_PLUGIN_FILES}
  COMMAND ${CMAKE_COMMAND} -E echo "Running OpenGL2 code generation script..."
  COMMAND ${Python3_EXECUTABLE} ${OPENGL2_SCRIPTS_DIR}/generator.py
  COMMAND ${CMAKE_COMMAND} -E touch ${STAMP_FILE}
  WORKING_DIRECTORY ${OPENGL2_SCRIPTS_DIR}
  DEPENDS ${SCRIPTS_SRC} ${PERSISTENT_DATA_SRC} ${MAKO_TEMPLATES} ${PLUGIN_MAKO_TEMPLATES}
)

unset(OpenGL2_codegen CACHE)
