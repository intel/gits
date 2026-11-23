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

namespace gits::opengl {

// Writes the subcapture stream to Common.Player.SubcapturePath. The stream and the state restore
// keys are shared with the subcaptures of the other APIs of a stream recorded with several APIs.
// See SubcaptureSession.
class SubcaptureRecorder {
public:
  SubcaptureRecorder();
  ~SubcaptureRecorder();
  SubcaptureRecorder(const SubcaptureRecorder&) = delete;
  SubcaptureRecorder& operator=(const SubcaptureRecorder&) = delete;

  void Record(const stream::CommandSerializer& serializer);
  // Idempotent.
  void FinishRecording();

  GITSKey CreateStateRestoreKey();
  GITSKey CreateStateRestoreObjectKey();
};

} // namespace gits::opengl
