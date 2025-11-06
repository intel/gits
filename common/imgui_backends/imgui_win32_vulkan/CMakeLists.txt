# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

add_library(imgui_win32_vulkan
${IMGUI_BACKEND_DIR}/imgui_impl_win32.h
${IMGUI_BACKEND_DIR}/imgui_impl_win32.cpp
${IMGUI_BACKEND_DIR}/imgui_impl_vulkan.h
${IMGUI_BACKEND_DIR}/imgui_impl_vulkan.cpp
)

target_include_directories(imgui_win32_vulkan PUBLIC ${IMGUI_SOURCE_DIR} ${IMGUI_BACKEND_DIR})

# Since the patched imgui uses our own Vulkan headers, we also need to link to common_interface for other includes
target_link_libraries(imgui_win32_vulkan PUBLIC common_interface imgui_common)

set_target_properties(imgui_win32_vulkan PROPERTIES FOLDER "common/imgui/backends")
