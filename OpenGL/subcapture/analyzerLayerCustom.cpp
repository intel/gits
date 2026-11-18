// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "analyzerLayerAuto.h"

#include "analyzerResults.h"
#include "log.h"

namespace gits::opengl {

AnalyzerLayer::AnalyzerLayer(const SubcaptureRange& range) : Layer("Analyzer"), m_Range(range) {
  if (stream::SubcaptureSession::IsShared()) {
    m_Range.SetCurrentFrame(
        stream::SubcaptureSession::Get().AddParticipant(this, 1, m_Range.StartFrame()));
  }
}

// The frames not played can use objects an interrupted analysis did not see, so it is not
// written as complete.
AnalyzerLayer::~AnalyzerLayer() {
  try {
    if (m_Range.InRange()) {
      LOG_ERROR << "Subcapture: analysis terminated prematurely, inside the subcapture range";
    }
  } catch (...) {
  }
  stream::SubcaptureSession::Get().RemoveParticipant(this);
}

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
  SwapEnded();
}

void AnalyzerLayer::Pre(wglSwapLayerBuffersCommand& command) {
  m_SwapEndsFrame = command.m_Return.Value != 0;
}

void AnalyzerLayer::Post(wglSwapLayerBuffersCommand& /*command*/) {
  SwapEnded();
}

void AnalyzerLayer::Pre(glXSwapBuffersCommand& /*command*/) {
  m_SwapEndsFrame = true;
}

void AnalyzerLayer::Post(glXSwapBuffersCommand& /*command*/) {
  SwapEnded();
}

void AnalyzerLayer::SwapEnded() {
  if (!m_SwapEndsFrame) {
    return;
  }
  m_SwapEndsFrame = false;
  if (stream::SubcaptureSession::IsShared()) {
    stream::SubcaptureSession::Get().EndFrame(stream::StreamHeader::Api::API_OPENGL);
  } else {
    EndFrame();
  }
}

void AnalyzerLayer::EndFrame() {
  const bool wasInRange = m_Range.InRange();
  m_Range.FrameEnd();
  if (wasInRange && !m_Range.InRange()) {
    AnalyzerResults::Write(m_Objects);
  }
}

} // namespace gits::opengl
