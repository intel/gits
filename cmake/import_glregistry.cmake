# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

if(NOT DEFINED OPENGL_REGISTRY_ROOT)
  add_thirdparty_arg("--with-glregistry")
  add_thirdparty_arg("--with-eglregistry")

  set(OPENGL_REGISTRY_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/OpenGL-Registry")
  set(EGL_REGISTRY_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/EGL-Registry")
endif()