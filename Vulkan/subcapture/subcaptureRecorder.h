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

namespace gits {
namespace vulkan {

// Owns the output stream for a subcapture.  Wraps stream::StreamWriter with
// Vulkan-specific path construction from config and lifetime management.
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

  // Mints a fresh, unique key for a command synthesized purely for state restore (i.e.
  // one that never passed through CaptureManager::CreateCommandKey() during real-time
  // recording).  Callers must set it on the command's m_Key before constructing the
  // Serializer passed to Record() - the Serializer encodes the command into its byte
  // buffer immediately in its constructor, so setting the key any later has no effect.
  GITSKey CreateStateRestoreKey() {
    return ++m_NextStateRestoreKey;
  }

  // Mints a fresh, unique key for an *object* that has no real captured key of its own
  // and is synthesized purely to support state restore (a temporary command buffer, a
  // staging buffer/memory, a relocated acceleration structure's scratch, etc.) - mirrors
  // DirectX's StateTrackingService::GetUniqueObjectKey(). Deliberately a separate counter
  // from CreateStateRestoreKey() (also mirroring DirectX's independent
  // m_RestoreCommandKey/m_RestoreObjectKey), so object and command numbering do not
  // interleave. Both draw from the same STATE_RESTORE_KEY_MASK-flagged range, which
  // PrintKey()/PrintGitsObjectKey() render as "S<n>"/"O<S<n>>" so they read distinctly
  // from real captured keys - see printCustom.cpp.
  GITSKey CreateStateRestoreObjectKey() {
    return ++m_NextStateRestoreObjectKey;
  }

private:
  std::unique_ptr<stream::StreamWriter> m_Writer;
  std::filesystem::path m_StreamPath;
  bool m_Finished{false};
  GITSKey m_NextStateRestoreKey{STATE_RESTORE_KEY_MASK};
  GITSKey m_NextStateRestoreObjectKey{STATE_RESTORE_KEY_MASK};
};

} // namespace vulkan
} // namespace gits
