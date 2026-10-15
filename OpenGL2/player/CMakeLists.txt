# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

set(GENERATED_SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/include/openglDriversAuto.h
  ${CMAKE_CURRENT_SOURCE_DIR}/openglDriversAuto.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/commandRunnersAuto.h
  ${CMAKE_CURRENT_SOURCE_DIR}/commandRunnersAuto.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/openGL2CommandFactoryAuto.cpp
)
set_source_files_properties(${GENERATED_SRC} PROPERTIES GENERATED TRUE)
source_group("Generated Files" FILES ${GENERATED_SRC})

add_library(OpenGL2_player STATIC
  ${GENERATED_SRC}
  include/openGL2CommandFactory.h
  include/playerLayerManager.h
  include/playerManager.h
  include/realDriver.h
  include/clientArraysMetaRunner.h
  include/discardedOutputElements.h
  include/hdcVirtualizationService.h
  include/handleArgumentUpdaters.h
  include/mappedDataMetaRunner.h
  include/mappedTextureDataMetaRunner.h
  include/mipmapCompletionService.h
  include/pbufferPixelFormatMetaRunner.h
  include/pbufferService.h
  include/replayContextCaps.h
  include/threadContextService.h
  include/unsupportedCommandRunner.h
  include/uniformArrayMetaRunner.h
  include/uniformLocationMapService.h
  include/windowGeometryMetaRunner.h
  include/commandRunnersCustom.h
  realDriver.cpp
  playerLayerManager.cpp
  playerManager.cpp
  clientArraysMetaRunner.cpp
  discardedOutputElements.cpp
  hdcVirtualizationService.cpp
  mappedDataMetaRunner.cpp
  mappedTextureDataMetaRunner.cpp
  mipmapCompletionService.cpp
  pbufferPixelFormatMetaRunner.cpp
  pbufferService.cpp
  replayContextCaps.cpp
  threadContextService.cpp
  unsupportedCommandRunner.cpp
  uniformArrayMetaRunner.cpp
  uniformLocationMapService.cpp
  windowGeometryMetaRunner.cpp
  commandRunnersCustom.cpp
)

add_dependencies(OpenGL2_player OpenGL2_codegen)

target_include_directories(OpenGL2_player
  PUBLIC include
  PRIVATE ../interceptor
)

target_link_libraries(OpenGL2_player
  PUBLIC OpenGL2_layer_interface
  PRIVATE OpenGL2_trace OpenGL2_subcapture OpenGL2_services stream windowing log common_interface
          gdi32
)

set_target_properties(OpenGL2_player PROPERTIES FOLDER OpenGL2)

# Ad hoc manual-verification targets are not part of the build; add them
# standalone if needed, not via this file.
