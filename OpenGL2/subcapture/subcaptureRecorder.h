// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "arguments.h"
#include "commandSerializer.h"
#include "streamWriter.h"

#include <filesystem>
#include <memory>

namespace gits::opengl2 {

// Keys of commands and objects that state restore writes, distinct from every recorded key.
inline constexpr GITSKey STATE_RESTORE_KEY_MASK = GITSKey{1} << 63;

// Writes the subcapture stream to Common.Player.SubcapturePath.
class SubcaptureRecorder {
public:
  SubcaptureRecorder();
  ~SubcaptureRecorder();
  SubcaptureRecorder(const SubcaptureRecorder&) = delete;
  SubcaptureRecorder& operator=(const SubcaptureRecorder&) = delete;

  void Record(const stream::CommandSerializer& serializer);
  // Idempotent.
  void FinishRecording();

  GITSKey CreateStateRestoreKey() {
    return ++m_NextStateRestoreKey;
  }
  GITSKey CreateStateRestoreObjectKey() {
    return ++m_NextStateRestoreObjectKey;
  }

private:
  std::unique_ptr<stream::StreamWriter> m_Writer;
  std::filesystem::path m_StreamPath;
  GITSKey m_NextStateRestoreKey{STATE_RESTORE_KEY_MASK};
  GITSKey m_NextStateRestoreObjectKey{STATE_RESTORE_KEY_MASK};
};

} // namespace gits::opengl2
