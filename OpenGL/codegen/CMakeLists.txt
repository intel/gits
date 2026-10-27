# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

set(STAMP_FILE ${CMAKE_CURRENT_BINARY_DIR}/OpenGL_codegen.stamp)

add_custom_target(OpenGL_codegen DEPENDS ${STAMP_FILE})
set_target_properties(OpenGL_codegen PROPERTIES FOLDER OpenGL)

set(SCRIPTS_SRC
  array_of_arrays_metadata.py
  buffer_content_hooks.py
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

set(OPENGL_PLUGINS_DIR ${CMAKE_SOURCE_DIR}/plugins/OpenGL)
file(GLOB PLUGIN_MAKO_TEMPLATES "${OPENGL_PLUGINS_DIR}/*/codegen/templates/*.mako")
source_group("plugin mako" FILES ${PLUGIN_MAKO_TEMPLATES})

target_sources(OpenGL_codegen PRIVATE
  ${MAKO_TEMPLATES}
  ${PLUGIN_MAKO_TEMPLATES}
  ${PERSISTENT_DATA_SRC}
  ${SCRIPTS_SRC}
)

set(OPENGL_SCRIPTS_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(OPENGL_LAYER_INTERFACE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../common/layer_interface)
set(OPENGL_GENERATED_LAYER_INTERFACE_FILES
  ${OPENGL_LAYER_INTERFACE_DIR}/commandIdsAuto.h
  ${OPENGL_LAYER_INTERFACE_DIR}/commandsAuto.h
  ${OPENGL_LAYER_INTERFACE_DIR}/layerAuto.h
  ${OPENGL_LAYER_INTERFACE_DIR}/commandCodersAuto.h
  ${OPENGL_LAYER_INTERFACE_DIR}/commandCodersAuto.cpp
  ${OPENGL_LAYER_INTERFACE_DIR}/commandSerializersAuto.h
  ${OPENGL_LAYER_INTERFACE_DIR}/commandSerializersAuto.cpp
  ${OPENGL_LAYER_INTERFACE_DIR}/glEnumsAuto.h
  ${OPENGL_LAYER_INTERFACE_DIR}/glEnumsAuto.cpp
  ${OPENGL_LAYER_INTERFACE_DIR}/pnameValueCountsAuto.cpp
)
set(OPENGL_INTERCEPTOR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../interceptor)
set(OPENGL_GENERATED_INTERCEPTOR_FILES
  ${OPENGL_INTERCEPTOR_DIR}/interceptorAuto.cpp
  ${OPENGL_INTERCEPTOR_DIR}/interceptorAuto.def
)
set(OPENGL_RECORDER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../recorder)
set(OPENGL_GENERATED_RECORDER_FILES
  ${OPENGL_RECORDER_DIR}/include/openglDriversAuto.h
  ${OPENGL_RECORDER_DIR}/openglDriversAuto.cpp
  ${OPENGL_RECORDER_DIR}/encoderLayerAuto.h
  ${OPENGL_RECORDER_DIR}/encoderLayerAuto.cpp
  ${OPENGL_RECORDER_DIR}/clientArraysLayerAuto.h
  ${OPENGL_RECORDER_DIR}/clientArraysLayerAuto.cpp
  ${OPENGL_RECORDER_DIR}/recorderWrappersAuto.cpp
)
set(OPENGL_PLAYER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../player)
set(OPENGL_GENERATED_PLAYER_FILES
  ${OPENGL_PLAYER_DIR}/include/openglDriversAuto.h
  ${OPENGL_PLAYER_DIR}/openglDriversAuto.cpp
  ${OPENGL_PLAYER_DIR}/include/commandRunnersAuto.h
  ${OPENGL_PLAYER_DIR}/commandRunnersAuto.cpp
  ${OPENGL_PLAYER_DIR}/openGLCommandFactoryAuto.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../layers/trace/traceLayerAuto.h
  ${CMAKE_CURRENT_SOURCE_DIR}/../layers/trace/traceLayerAuto.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../subcapture/recordingLayerAuto.h
  ${CMAKE_CURRENT_SOURCE_DIR}/../subcapture/recordingLayerAuto.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../subcapture/analyzerLayerAuto.h
  ${CMAKE_CURRENT_SOURCE_DIR}/../subcapture/analyzerLayerAuto.cpp
)
set(OPENGL_GENERATED_PLUGIN_FILES
  ${OPENGL_PLUGINS_DIR}/statistics/statisticsLayerAuto.h
  ${OPENGL_PLUGINS_DIR}/statistics/statisticsLayerAuto.cpp
)

add_custom_command(
  OUTPUT ${STAMP_FILE} ${OPENGL_GENERATED_LAYER_INTERFACE_FILES}
         ${OPENGL_GENERATED_INTERCEPTOR_FILES} ${OPENGL_GENERATED_RECORDER_FILES}
         ${OPENGL_GENERATED_PLAYER_FILES} ${OPENGL_GENERATED_PLUGIN_FILES}
  COMMAND ${CMAKE_COMMAND} -E echo "Running OpenGL code generation script..."
  COMMAND ${Python3_EXECUTABLE} ${OPENGL_SCRIPTS_DIR}/generator.py
  COMMAND ${CMAKE_COMMAND} -E touch ${STAMP_FILE}
  WORKING_DIRECTORY ${OPENGL_SCRIPTS_DIR}
  DEPENDS ${SCRIPTS_SRC} ${PERSISTENT_DATA_SRC} ${MAKO_TEMPLATES} ${PLUGIN_MAKO_TEMPLATES}
)

unset(OpenGL_codegen CACHE)
