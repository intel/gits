// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandSerializer.h"
#include "streamWriter.h"

#include <filesystem>
#include <memory>

namespace gits {
namespace vulkan {

// Owns the output stream for a subcapture.  Wraps stream::StreamWriter with
// Vulkan2-specific path construction from config and lifetime management.
//
// The recorder is constructed once per player session.  If subcapture is
// disabled in config (empty frames string) the internal StreamWriter is never
// created and Record() is a no-op.
class SubcaptureRecorder {
public:
  // enabled == false keeps the recorder permanently closed even when subcapture
  // is configured.  Used by the analysis pass, which tracks state and writes the
  // analysis file but must never open (and thus overwrite) the output stream.
  explicit SubcaptureRecorder(bool enabled = true);
  ~SubcaptureRecorder();
  SubcaptureRecorder(const SubcaptureRecorder&) = delete;
  SubcaptureRecorder& operator=(const SubcaptureRecorder&) = delete;

  // Write one command token to the output stream.
  void Record(const stream::CommandSerializer& serializer);

  // Flush and close the stream.  Idempotent -- safe to call from destructor.
  void FinishRecording();

  bool IsOpen() const {
    return m_Writer != nullptr;
  }

private:
  std::unique_ptr<stream::StreamWriter> m_Writer;
  std::filesystem::path m_StreamPath;
  bool m_Finished{false};
};

} // namespace vulkan
} // namespace gits
