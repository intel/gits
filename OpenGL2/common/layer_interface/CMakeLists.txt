# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

# Real STATIC library (not header-only/INTERFACE): commandCodersAuto.cpp is generated,
# non-template code (see OpenGL2/codegen/generator_layer_interface.py) with a genuine
# translation unit to compile, unlike the earlier header-only Command/CommandSerializer
# classes it replaces.
set(GENERATED_SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/commandIdsAuto.h
  ${CMAKE_CURRENT_SOURCE_DIR}/commandsAuto.h
  ${CMAKE_CURRENT_SOURCE_DIR}/commandCodersAuto.h
  ${CMAKE_CURRENT_SOURCE_DIR}/commandCodersAuto.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/commandSerializersAuto.h
)
set_source_files_properties(${GENERATED_SRC} PROPERTIES GENERATED TRUE)
source_group("Generated Files" FILES ${GENERATED_SRC})

add_library(OpenGL2_layer_interface STATIC
  ${GENERATED_SRC}
  arguments.h
  argumentCoders.h
  command.h
  layer.h
  mappedDataMetaTypes.h
  unsupportedCommandTypes.h
  pixelTransferSize.h
  pixelTransferSize.cpp
)

add_dependencies(OpenGL2_layer_interface OpenGL2_codegen)

target_include_directories(OpenGL2_layer_interface PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  # commandsAuto.h uses GLenum/GLfloat/etc. from glTypes.h, which is hand-declared in
  # OpenGL2/interceptor (see that header's own comment) instead of pulling in <GL/gl.h>.
  ${CMAKE_CURRENT_SOURCE_DIR}/../../interceptor
)

target_link_libraries(OpenGL2_layer_interface PUBLIC
  stream
)

# Groups this target under an "OpenGL2_common" virtual folder in the IDE, matching how
# Vulkan's equivalent common/* targets group under "Vulkan/Vulkan_common" (e.g.
# Vulkan/common/layer_interface/CMakeLists.txt) - purely a Solution Explorer grouping, not
# a separate library/project.
set_target_properties(OpenGL2_layer_interface PROPERTIES FOLDER OpenGL2/OpenGL2_common)
