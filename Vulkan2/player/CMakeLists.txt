# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(Vulkan_player_2 STATIC)

set(ROOT_SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/playerManager.h
  ${CMAKE_CURRENT_SOURCE_DIR}/playerManager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/playerLayerManager.h
  ${CMAKE_CURRENT_SOURCE_DIR}/playerLayerManager.cpp
)
source_group("" FILES ${ROOT_SRC})

set(PLAYER_SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/dispatchTableAuto.h
  ${CMAKE_CURRENT_SOURCE_DIR}/replayCustomizationLayer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/replayCustomizationLayer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/handleArgumentUpdaters.h
  ${CMAKE_CURRENT_SOURCE_DIR}/handleArgumentUpdatersPlayerAuto.h
  ${CMAKE_CURRENT_SOURCE_DIR}/handleArgumentUpdatersPlayerAuto.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/handleArgumentUpdatersCustom.h
  ${CMAKE_CURRENT_SOURCE_DIR}/handleArgumentUpdatersCustom.cpp
)
source_group("player" FILES ${PLAYER_SRC})

set(SERVICES_SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/windowService.h
  ${CMAKE_CURRENT_SOURCE_DIR}/windowService.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mapTrackingService.h
  ${CMAKE_CURRENT_SOURCE_DIR}/mapTrackingService.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/descriptorUpdateTemplateService.h
  ${CMAKE_CURRENT_SOURCE_DIR}/descriptorUpdateTemplateService.cpp
)
source_group("services" FILES ${SERVICES_SRC})

set(RUNNER_SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkanLibrary2.h
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkanLibrary2Auto.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/commandPlayer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/commandPlayersAuto.h
  ${CMAKE_CURRENT_SOURCE_DIR}/commandPlayersAuto.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/commandPlayersCustom.h
  ${CMAKE_CURRENT_SOURCE_DIR}/commandPlayersCustom.cpp
)
source_group("runner" FILES ${RUNNER_SRC})

set(ALL_SRC
  ${ROOT_SRC}
  ${PLAYER_SRC}
  ${SERVICES_SRC}
  ${RUNNER_SRC}
)

set_auto_generated_property(FILE_LIST ${ALL_SRC})

target_sources(Vulkan_player_2 PRIVATE
  ${ALL_SRC}
)

target_include_directories(Vulkan_player_2 PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(Vulkan_player_2 PRIVATE
  Vulkan_layer_interface
  Vulkan_coders
  Vulkan_services
)

set_target_properties(Vulkan_player_2 PROPERTIES FOLDER Vulkan2)
