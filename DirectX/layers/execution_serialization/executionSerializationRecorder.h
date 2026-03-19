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

class ExecutionSerializationRecorder {
public:
  ExecutionSerializationRecorder();
  ~ExecutionSerializationRecorder();

  void record(const stream::CommandSerializer& commandSerializer);
  void finishRecording();

private:
  void copyAuxiliaryFiles();

private:
  std::unique_ptr<stream::StreamWriter> recorder_;
  bool finished_{};
};

} // namespace DirectX
} // namespace gits
