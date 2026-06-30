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
  ${CMAKE_CURRENT_SOURCE_DIR}/argumentCodersCustom.h
  ${CMAKE_CURRENT_SOURCE_DIR}/argumentCodersCustom.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/commandCodersCustom.h
  ${CMAKE_CURRENT_SOURCE_DIR}/commandCodersCustom.cpp
)
source_group("coders" FILES ${CODERS_SRC})

set(SERIALIZERS_SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/commandSerializersAuto.h
  ${CMAKE_CURRENT_SOURCE_DIR}/commandSerializersCustom.h
  ${CMAKE_CURRENT_SOURCE_DIR}/commandSerializersFactory.h
  ${CMAKE_CURRENT_SOURCE_DIR}/commandSerializersFactoryAuto.cpp
)
source_group("serializers" FILES ${SERIALIZERS_SRC})

set(ALL_SRC
  ${CODERS_SRC}
  ${SERIALIZERS_SRC}
)

set_auto_generated_property(FILE_LIST ${ALL_SRC})

target_sources(Vulkan_coders PRIVATE
  ${ALL_SRC}
)

target_include_directories(Vulkan_coders PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(Vulkan_coders PRIVATE
  stream
  Vulkan_layer_interface
  Vulkan_services
)

set_target_properties(Vulkan_coders PROPERTIES FOLDER Vulkan2/Vulkan_common)
