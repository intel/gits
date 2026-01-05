# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(DirectX_imgui_hud STATIC)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(ALL_SRC
  ${SRC_DIR}/imguiHudLayer.h
  ${SRC_DIR}/imguiHudLayer.cpp
)
source_group("" FILES ${ALL_SRC})

set(IMGUI_SRC
  ${IMGUI_SOURCE_DIR}/imgui.cpp
  ${IMGUI_SOURCE_DIR}/imgui_demo.cpp
  ${IMGUI_SOURCE_DIR}/imgui_draw.cpp
  ${IMGUI_SOURCE_DIR}/imgui_tables.cpp
  ${IMGUI_SOURCE_DIR}/imgui_widgets.cpp

  ${IMGUI_SOURCE_DIR}/imgui.h
  ${IMGUI_SOURCE_DIR}/imconfig.h
  ${IMGUI_SOURCE_DIR}/imgui_internal.h
)
source_group("ImGui/" FILES ${IMGUI_SRC})

set(IMGUI_BACKEND_SRC
  ${IMGUI_BACKEND_DIR}/imgui_impl_win32.cpp
  ${IMGUI_BACKEND_DIR}/imgui_impl_dx12.cpp
)
source_group("ImGui/Backends" FILES ${IMGUI_BACKEND_SRC})

target_sources(DirectX_imgui_hud PRIVATE
  ${ALL_SRC}
  ${IMGUI_SRC}
  ${IMGUI_BACKEND_SRC}
)

target_include_directories(DirectX_imgui_hud PRIVATE
  ${IMGUI_SOURCE_DIR}/
  ${IMGUI_BACKEND_DIR}/
)

target_include_directories(DirectX_imgui_hud PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(DirectX_imgui_hud PRIVATE
  hud
  DirectX_layer_interface
  DirectX_utils
)

set_target_properties(DirectX_imgui_hud PROPERTIES FOLDER DirectX/DirectX_layers)
