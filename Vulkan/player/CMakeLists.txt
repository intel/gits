# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(Vulkan_player STATIC)

set(ROOT_SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/playerManager.h
  ${CMAKE_CURRENT_SOURCE_DIR}/playerManager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/playerLayerManager.h
  ${CMAKE_CURRENT_SOURCE_DIR}/playerLayerManager.cpp
)
source_group("" FILES ${ROOT_SRC})

set(PLAYER_SRC
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
  ${CMAKE_CURRENT_SOURCE_DIR}/swapchainImageSyncService.h
  ${CMAKE_CURRENT_SOURCE_DIR}/swapchainImageSyncService.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/fencePendingSignalService.h
  ${CMAKE_CURRENT_SOURCE_DIR}/fencePendingSignalService.cpp
)
source_group("services" FILES ${SERVICES_SRC})

set(RUNNER_SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/commandRunnersAuto.h
  ${CMAKE_CURRENT_SOURCE_DIR}/commandRunnersAuto.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/commandRunnersCustom.h
  ${CMAKE_CURRENT_SOURCE_DIR}/commandRunnersCustom.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkanCommandFactory.h
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkanCommandFactoryAuto.cpp
)
source_group("runner" FILES ${RUNNER_SRC})

set(ALL_SRC
  ${ROOT_SRC}
  ${PLAYER_SRC}
  ${SERVICES_SRC}
  ${RUNNER_SRC}
)

set_auto_generated_property(FILE_LIST ${ALL_SRC})

target_sources(Vulkan_player PRIVATE
  ${ALL_SRC}
)

target_include_directories(Vulkan_player PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(Vulkan_player PRIVATE
  ${VULKAN_DIR}/layers/trace
)


target_link_libraries(Vulkan_player PRIVATE
  stream
  Vulkan_layer_interface
  Vulkan_coders
  Vulkan_services
  Vulkan_trace
  Vulkan_subcapture
  Vulkan_resource_dumping
  Vulkan_utils
  Vulkan_api_debug
)

if(UNIX)
  find_program(WAYLAND_SCANNER wayland-scanner)
  if(NOT WAYLAND_SCANNER)
    message(FATAL_ERROR "wayland-scanner not found")
  endif()

  find_path(WAYLAND_PROTOCOLS_DIR
    NAMES stable/xdg-shell/xdg-shell.xml
    PATHS
      /usr/share/wayland-protocols
      /usr/local/share/wayland-protocols
  )
  if(NOT WAYLAND_PROTOCOLS_DIR)
    message(FATAL_ERROR "wayland-protocols not found")
  endif()

  set(XDG_SHELL_XML ${WAYLAND_PROTOCOLS_DIR}/stable/xdg-shell/xdg-shell.xml)
  set(XDG_SHELL_HEADER ${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-client-protocol.h)
  set(XDG_SHELL_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-protocol.c)

  add_custom_target(generate_xdg_shell_protocol
    COMMAND ${WAYLAND_SCANNER} client-header ${XDG_SHELL_XML} ${XDG_SHELL_HEADER}
    COMMAND ${WAYLAND_SCANNER} private-code ${XDG_SHELL_XML} ${XDG_SHELL_SOURCE}
    BYPRODUCTS
      ${XDG_SHELL_HEADER}
      ${XDG_SHELL_SOURCE}
    COMMENT "Generating xdg-shell protocol files"
  )

  target_sources(Vulkan_player PRIVATE
    ${XDG_SHELL_SOURCE}
    ${XDG_SHELL_HEADER}
  )

  target_include_directories(Vulkan_player PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
  )

  add_dependencies(Vulkan_codegen generate_xdg_shell_protocol)
  add_dependencies(Vulkan_player generate_xdg_shell_protocol)

  target_link_libraries(Vulkan_player PRIVATE
    wayland-client
  )
endif()

set_target_properties(Vulkan_player PROPERTIES FOLDER Vulkan)
