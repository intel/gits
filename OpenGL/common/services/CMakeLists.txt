# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

# Small, hand-written services shared by the recorder and the player (HandleMapService<T> - see
# that header's own comment for why ContextHandleMapService/SyncHandleMapService are now just
# its two type aliases, not two separate classes - ObjectNameMapService, ShareGroupService and
# PluginService) -
# mirrors Vulkan/common/services's precedent for exactly this kind of recorder+player-shared
# class. Recorder-only or player-only services (e.g. MapTrackingService,
# UniformLocationMapService) live under recorder/ or player/ instead - see those headers' own
# comments.
add_library(OpenGL_services STATIC
  handleMapService.h
  handleMapService.cpp
  objectNameMapService.h
  objectNameMapService.cpp
  pluginService.h
  pluginService.cpp
  shareGroupService.h
  shareGroupService.cpp
)

target_include_directories(OpenGL_services PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/plugins
)

target_link_libraries(OpenGL_services PRIVATE
  OpenGL_layer_interface
  common_interface
  log
)

# Groups this target under an "OpenGL_common" virtual folder in the IDE, matching how
# OpenGL_layer_interface (OpenGL/common/layer_interface/CMakeLists.txt) and Vulkan's
# equivalent common/* targets already group.
set_target_properties(OpenGL_services PROPERTIES FOLDER OpenGL/OpenGL_common)
