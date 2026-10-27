# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

if(NOT DEFINED MATERIAL_DESIGN_ICONS_SOURCE_DIR)
  add_thirdparty_arg("--with-material-design-icons")
  set(MATERIAL_DESIGN_ICONS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/material-design-icons")
  set(MATERIAL_DESIGN_FONT_REGULAR "${MATERIAL_DESIGN_ICONS_SOURCE_DIR}/fonts/materialdesignicons-webfont.ttf")
endif()
