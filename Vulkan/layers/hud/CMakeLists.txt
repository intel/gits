# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(Vulkan_hud STATIC)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(HUD_SRC
  ${SRC_DIR}/hudLayer.h
  ${SRC_DIR}/hudLayer.cpp
)

list(APPEND HUD_SRC
  ${IMGUI_BACKEND_DIR}/imgui_impl_vulkan.h
  ${IMGUI_BACKEND_DIR}/imgui_impl_vulkan.cpp
)

if(WIN32)
  list(APPEND HUD_SRC
    ${IMGUI_BACKEND_DIR}/imgui_impl_win32.h
    ${IMGUI_BACKEND_DIR}/imgui_impl_win32.cpp
  )
endif()

source_group("" FILES ${HUD_SRC})


target_sources(Vulkan_hud PRIVATE
  ${HUD_SRC}
)

target_link_libraries(Vulkan_hud PRIVATE
  Vulkan_layer_interface
)

target_include_directories(Vulkan_hud PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(Vulkan_hud PUBLIC ${IMGUI_SOURCE_DIR} ${IMGUI_BACKEND_DIR})
target_compile_definitions(Vulkan_hud PUBLIC IMGUI_IMPL_VULKAN_NO_PROTOTYPES)
target_link_libraries(Vulkan_hud PUBLIC hud)

set_target_properties(Vulkan_hud PROPERTIES FOLDER Vulkan/Vulkan_layers)
