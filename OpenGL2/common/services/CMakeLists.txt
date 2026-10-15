# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

# Small, hand-written services shared by the recorder and the player (HandleMapService<T> - see
# that header's own comment for why ContextHandleMapService/SyncHandleMapService are now just
# its two type aliases, not two separate classes - and ObjectNameMapService) - mirrors
# Vulkan/common/services's precedent for exactly this kind of recorder+player-shared class.
# Recorder-only or player-only services (e.g. MapTrackingService, ShareGroupService,
# UniformLocationMapService) live under recorder/ or player/ instead - see those headers' own
# comments.
add_library(OpenGL2_services STATIC
  handleMapService.h
  handleMapService.cpp
  objectNameMapService.h
  objectNameMapService.cpp
)

target_include_directories(OpenGL2_services PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(OpenGL2_services PRIVATE
  OpenGL2_layer_interface
  log
)

# Groups this target under an "OpenGL2_common" virtual folder in the IDE, matching how
# OpenGL2_layer_interface (OpenGL2/common/layer_interface/CMakeLists.txt) and Vulkan's
# equivalent common/* targets already group.
set_target_properties(OpenGL2_services PROPERTIES FOLDER OpenGL2/OpenGL2_common)
