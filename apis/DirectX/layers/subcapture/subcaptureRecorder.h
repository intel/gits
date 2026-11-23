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

#include <memory>

namespace gits {
namespace DirectX {

class SubcaptureRecorder {
public:
  SubcaptureRecorder();
  ~SubcaptureRecorder();
  SubcaptureRecorder(const SubcaptureRecorder&) = delete;
  SubcaptureRecorder& operator=(const SubcaptureRecorder&) = delete;

  void Record(const stream::CommandSerializer& commandSerializer);
  void FinishRecording();
  bool CommandListSubcapture() const {
    return m_CommandListSubcapture;
  }

private:
  void CopyAuxiliaryFiles();

  std::unique_ptr<stream::StreamWriter> m_Recorder;
  bool m_CommandListSubcapture{};
  bool m_Finished{};
};

} // namespace DirectX
} // namespace gits
