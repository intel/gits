// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "analyzerLayerAuto.h"

#include "analyzerResults.h"

namespace gits::opengl2 {

void AnalyzerLayer::Pre(MappedDataMetaCommand& command) {
  if (Collecting(command.m_ThreadId)) {
    Use(command.m_Buffer);
  }
}

void AnalyzerLayer::Pre(UniformArrayMetaCommand& command) {
  if (Collecting(command.m_ThreadId)) {
    Use(command.m_Program.Value);
  }
}

void AnalyzerLayer::Pre(MappedTextureDataMetaCommand& command) {
  if (Collecting(command.m_ThreadId)) {
    Use(command.m_Textures);
  }
}

void AnalyzerLayer::Pre(glNewListCommand& command) {
  m_CompilingLists.insert(command.m_ThreadId);
}

void AnalyzerLayer::Pre(glEndListCommand& command) {
  m_CompilingLists.erase(command.m_ThreadId);
}

// The runner overwrites m_Return with the replay result, so the captured one is read in Pre.
void AnalyzerLayer::Pre(wglSwapBuffersCommand& command) {
  m_SwapEndsFrame = command.m_Return.Value != 0;
}

void AnalyzerLayer::Post(wglSwapBuffersCommand& /*command*/) {
  EndFrame();
}

void AnalyzerLayer::Pre(wglSwapLayerBuffersCommand& command) {
  m_SwapEndsFrame = command.m_Return.Value != 0;
}

void AnalyzerLayer::Post(wglSwapLayerBuffersCommand& /*command*/) {
  EndFrame();
}

void AnalyzerLayer::Pre(glXSwapBuffersCommand& /*command*/) {
  m_SwapEndsFrame = true;
}

void AnalyzerLayer::Post(glXSwapBuffersCommand& /*command*/) {
  EndFrame();
}

void AnalyzerLayer::EndFrame() {
  if (!m_SwapEndsFrame) {
    return;
  }
  m_SwapEndsFrame = false;
  const bool wasInRange = m_Range.InRange();
  m_Range.FrameEnd();
  if (wasInRange && !m_Range.InRange()) {
    AnalyzerResults::Write(m_Objects);
  }
}

} // namespace gits::opengl2
