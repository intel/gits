# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(Vulkan_coders STATIC)

set(CODERS_SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/commandCodersAuto.h
  ${CMAKE_CURRENT_SOURCE_DIR}/commandCodersAuto.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/argumentCoders.h
  ${CMAKE_CURRENT_SOURCE_DIR}/argumentCoders.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/argumentCodersAuto.h
  ${CMAKE_CURRENT_SOURCE_DIR}/argumentCodersAuto.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/commandCodersCustom.h
  ${CMAKE_CURRENT_SOURCE_DIR}/commandCodersCustom.cpp
)
source_group("coders" FILES ${CODERS_SRC})

set(WRITERS_SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/commandWritersFactory.h
  ${CMAKE_CURRENT_SOURCE_DIR}/commandWritersFactoryAuto.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/commandWriter.h
  ${CMAKE_CURRENT_SOURCE_DIR}/commandWritersAuto.h
  ${CMAKE_CURRENT_SOURCE_DIR}/commandWritersCustom.h
)
source_group("writers" FILES ${WRITERS_SRC})

set(ALL_SRC
  ${CODERS_SRC}
  ${WRITERS_SRC}
)

set_auto_generated_property(FILE_LIST ${ALL_SRC})

target_sources(Vulkan_coders PRIVATE
  ${ALL_SRC}
)

target_include_directories(Vulkan_coders PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(Vulkan_coders PUBLIC
  Vulkan_layer_interface
  Vulkan_services
)

set_target_properties(Vulkan_coders PROPERTIES FOLDER Vulkan2/Vulkan_common)
