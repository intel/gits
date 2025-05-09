// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"
#include "executionSerializationRecorder.h"
#include "executionSerializationLayerAuto.h"

#include <memory>

namespace gits {
namespace DirectX {

class ExecutionSerializationFactory {
public:
  ExecutionSerializationFactory();

  std::unique_ptr<Layer> getExecutionSerializationLayer() {
    return std::move(executionSerializationLayer_);
  }

private:
  std::unique_ptr<ExecutionSerializationRecorder> recorder_;
  std::unique_ptr<Layer> executionSerializationLayer_;
};

} // namespace DirectX
} // namespace gits
