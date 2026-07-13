# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

if(NOT DEFINED OPENGL_HEADERS_DIR)
  add_thirdparty_arg("--with-glregistry")
  add_thirdparty_arg("--with-eglregistry")

  set(OPENGL_REGISTRY_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/OpenGL-Registry")
  set(OPENGL_HEADERS_DIR "${OPENGL_REGISTRY_ROOT}/api")
  set(OPENGL_XML_DIR "${OPENGL_REGISTRY_ROOT}/xml")

  set(EGL_REGISTRY_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/EGL-Registry")
  set(EGL_HEADERS_DIR "${EGL_REGISTRY_ROOT}/api")
  set(EGL_XML_DIR "${EGL_REGISTRY_ROOT}/api")
endif()