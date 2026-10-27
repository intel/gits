// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "subcaptureLayer.h"

#include "stateRestorer.h"

namespace gits::opengl {

SubcaptureLayer::SubcaptureLayer(const SubcaptureRange& range)
    : Layer("Subcapture"), m_Range(range) {}

// The runner overwrites m_Return with the replay result, so the captured one is read in Pre.
void SubcaptureLayer::Pre(wglSwapBuffersCommand& command) {
  m_SwapEndsFrame = command.m_Return.Value != 0;
}

void SubcaptureLayer::Post(wglSwapBuffersCommand& /*command*/) {
  EndFrame();
}

void SubcaptureLayer::Pre(wglSwapLayerBuffersCommand& command) {
  m_SwapEndsFrame = command.m_Return.Value != 0;
}

void SubcaptureLayer::Post(wglSwapLayerBuffersCommand& /*command*/) {
  EndFrame();
}

void SubcaptureLayer::Pre(glXSwapBuffersCommand& /*command*/) {
  m_SwapEndsFrame = true;
}

void SubcaptureLayer::Post(glXSwapBuffersCommand& /*command*/) {
  EndFrame();
}

void SubcaptureLayer::EndFrame() {
  if (m_SwapEndsFrame) {
    m_SwapEndsFrame = false;
    m_Range.FrameEnd();
    if (m_Range.StartFrame() > 1 && m_Range.CurrentFrame() == m_Range.StartFrame()) {
      StateRestorer(m_StateTracking, m_Recorder, m_Analysis).Restore();
    }
  }
}

} // namespace gits::opengl
