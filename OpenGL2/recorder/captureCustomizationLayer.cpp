// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "captureCustomizationLayer.h"

#include "glEnumsAuto.h"
#include "log.h"

namespace gits::opengl2 {

void CaptureCustomizationLayer::Post(wglCreateContextAttribsARBCommand& command) {
  if (command.m_Return.Value == nullptr) {
    return;
  }
  const auto& attributes = command.m_attribList.Data;
  for (size_t index = 0; index + 1 < attributes.size(); index += 2) {
    if (attributes[index] == WGL_CONTEXT_PROFILE_MASK_ARB) {
      GITS_ASSERT((attributes[index + 1] & WGL_CONTEXT_ES_PROFILE_BIT_EXT) == 0,
                  "OpenGL ES contexts are not supported by the recorder");
    }
  }
}

} // namespace gits::opengl2
