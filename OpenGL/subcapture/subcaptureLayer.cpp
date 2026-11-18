// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "subcaptureLayer.h"

#include "stateRestorer.h"

#include <stdexcept>
#include <string>

namespace gits::opengl {

SubcaptureLayer::SubcaptureLayer(const SubcaptureRange& range)
    : Layer("Subcapture"), m_Range(range) {
  if (stream::SubcaptureSession::IsShared()) {
    m_Range.SetCurrentFrame(
        stream::SubcaptureSession::Get().AddParticipant(this, 1, m_Range.StartFrame()));
  }
}

SubcaptureLayer::~SubcaptureLayer() {
  stream::SubcaptureSession::Get().RemoveParticipant(this);
}

// The runner overwrites m_Return with the replay result, so the captured one is read in Pre.
void SubcaptureLayer::Pre(wglSwapBuffersCommand& command) {
  m_SwapEndsFrame = command.m_Return.Value != 0;
}

void SubcaptureLayer::Post(wglSwapBuffersCommand& /*command*/) {
  SwapEnded();
}

void SubcaptureLayer::Pre(wglSwapLayerBuffersCommand& command) {
  m_SwapEndsFrame = command.m_Return.Value != 0;
}

void SubcaptureLayer::Post(wglSwapLayerBuffersCommand& /*command*/) {
  SwapEnded();
}

void SubcaptureLayer::Pre(glXSwapBuffersCommand& /*command*/) {
  m_SwapEndsFrame = true;
}

void SubcaptureLayer::Post(glXSwapBuffersCommand& /*command*/) {
  SwapEnded();
}

void SubcaptureLayer::SwapEnded() {
  if (!m_SwapEndsFrame) {
    return;
  }
  m_SwapEndsFrame = false;
  if (stream::SubcaptureSession::IsShared()) {
    stream::SubcaptureSession::Get().EndFrame(stream::StreamHeader::Api::API_OPENGL);
    return;
  }
  EndFrame();
  if (m_Range.StartFrame() > 1 && m_Range.CurrentFrame() == m_Range.StartFrame()) {
    RestoreState();
  }
}

void SubcaptureLayer::PrepareRestoreState() {
  for (const auto& [key, object] : m_StateTracking.Objects()) {
    if (object.ImportedExport != 0 &&
        !stream::SubcaptureSession::Get().IsExportRestored(object.ImportedExport)) {
      throw std::runtime_error(
          "Subcapture of this range is not supported: the OpenGL object " + std::to_string(key) +
          " imports the payload that command " + std::to_string(object.ImportedExport) +
          " exported, and the object it was exported from was destroyed before the range, so "
          "the subcapture cannot export it again. Subcapture a range that starts before that "
          "object is destroyed.");
    }
    // The contents of storage in a memory object are the exporting API's to restore.
    const ObjectState* memory =
        object.MemoryStorage != nullptr ? m_StateTracking.Object(object.StorageMemory) : nullptr;
    if (memory == nullptr || memory->ImportedExport == 0) {
      continue;
    }
    const stream::SubcaptureSession& session = stream::SubcaptureSession::Get();
    // The check of the memory object itself rejects an export that is not restored.
    if (!session.IsExportRestored(memory->ImportedExport)) {
      continue;
    }
    const std::string storage =
        "the OpenGL " + std::string(object.Kind == GLObjectKind::Buffer ? "buffer " : "texture ") +
        std::to_string(key) + " has its storage in the memory that command " +
        std::to_string(memory->ImportedExport) + " exported";
    if (const std::string* reason = session.ContentsNotRestored(memory->ImportedExport)) {
      throw std::runtime_error("Subcapture of this range is not supported: " + storage +
                               ", whose contents the subcapture cannot restore: " + *reason + ".");
    }
    const stream::RestoredContents* contents = session.GetRestoredContents(memory->ImportedExport);
    if (contents == nullptr) {
      throw std::runtime_error("Subcapture of this range is not supported: " + storage +
                               ", and the subcapture does not restore its contents.");
    }
    const bool buffer = object.Kind == GLObjectKind::Buffer;
    const stream::ContentRange range =
        buffer ? stream::ContentRange{object.StorageOffset, object.StorageSize}
               : stream::TextureRange(*contents, object.StorageOffset);
    for (const stream::ContentRange& unrestored : stream::UnrestoredRanges(*contents, range)) {
      // Only GL accesses the bytes of a live buffer that no Vulkan resource is bound over, so the
      // OpenGL restore reads them back itself. A deleted one has no name to read them through.
      if (buffer && !object.DeletePending && !stream::IsBound(*contents, unrestored)) {
        continue;
      }
      throw std::runtime_error(
          "Subcapture of this range is not supported: " + storage + ", at bytes " +
          std::to_string(range.Offset) + " to " + std::to_string(range.Offset + range.Size) +
          ", and the subcapture does not restore the contents of bytes " +
          std::to_string(unrestored.Offset) + " to " +
          std::to_string(unrestored.Offset + unrestored.Size) +
          (buffer && object.DeletePending
               ? ": the buffer is deleted, so they cannot be read back from OpenGL, and no Vulkan "
                 "buffer or image whose contents the subcapture reads back covers them."
               : ": no Vulkan buffer or image whose contents the subcapture reads back covers "
                 "them."));
    }
    stream::SubcaptureSession::Get().RequireContents(memory->ImportedExport, range);
  }
  StateRestorer(m_StateTracking, m_Recorder, m_Analysis).FinishContexts();
}

void SubcaptureLayer::RestoreState() {
  StateRestorer(m_StateTracking, m_Recorder, m_Analysis).Restore();
}

void SubcaptureLayer::EndFrame() {
  m_Range.FrameEnd();
}

} // namespace gits::opengl
