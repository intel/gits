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
  ${CMAKE_CURRENT_SOURCE_DIR}/openGLCommandFactoryAuto.cpp
)
set_source_files_properties(${GENERATED_SRC} PROPERTIES GENERATED TRUE)
source_group("Generated Files" FILES ${GENERATED_SRC})

add_library(OpenGL_player STATIC
  ${GENERATED_SRC}
  include/openGLCommandFactory.h
  include/playerLayerManager.h
  include/playerManager.h
  include/realDriver.h
  include/bindlessHandleMapService.h
  include/clientArraysMetaRunner.h
  include/discardedOutputElements.h
  include/glxDrawableService.h
  include/glxFBConfigService.h
  include/hdcVirtualizationService.h
  include/handleArgumentUpdaters.h
  include/mappedDataMetaRunner.h
  include/mappedTextureDataMetaRunner.h
  include/pixelFormatMetaRunner.h
  include/pbufferService.h
  include/programBinaryCheckService.h
  include/replayContextCaps.h
  include/replayCustomizationLayer.h
  include/threadContextService.h
  include/unsupportedCommandRunner.h
  include/uniformArrayMetaRunner.h
  include/uniformLocationMapService.h
  include/windowGeometryMetaRunner.h
  include/commandRunnersCustom.h
  realDriver.cpp
  playerLayerManager.cpp
  playerManager.cpp
  bindlessHandleMapService.cpp
  clientArraysMetaRunner.cpp
  discardedOutputElements.cpp
  glxDrawableService.cpp
  glxFBConfigService.cpp
  hdcVirtualizationService.cpp
  mappedDataMetaRunner.cpp
  mappedTextureDataMetaRunner.cpp
  pixelFormatMetaRunner.cpp
  pbufferService.cpp
  programBinaryCheckService.cpp
  replayContextCaps.cpp
  replayCustomizationLayer.cpp
  threadContextService.cpp
  unsupportedCommandRunner.cpp
  uniformArrayMetaRunner.cpp
  uniformLocationMapService.cpp
  windowGeometryMetaRunner.cpp
  commandRunnersCustom.cpp
)

add_dependencies(OpenGL_player OpenGL_codegen)

target_include_directories(OpenGL_player
  PUBLIC include
  PRIVATE ../interceptor ${CMAKE_SOURCE_DIR}/plugins
)

target_link_libraries(OpenGL_player
  PUBLIC OpenGL_layer_interface
  PRIVATE OpenGL_resource_dumping OpenGL_trace OpenGL_subcapture OpenGL_services stream
          windowing log common_interface
)
if(WIN32)
  target_link_libraries(OpenGL_player PRIVATE gdi32)
else()
  target_link_libraries(OpenGL_player PRIVATE X11)
endif()

set_target_properties(OpenGL_player PROPERTIES FOLDER OpenGL)

# Ad hoc manual-verification targets are not part of the build; add them
# standalone if needed, not via this file.
