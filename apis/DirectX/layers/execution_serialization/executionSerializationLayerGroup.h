// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerGroup.h"
#include "executionSerializationRecorder.h"

#include <memory>

namespace gits {
namespace DirectX {

class ExecutionSerializationLayerGroup : public LayerGroup {
public:
  ExecutionSerializationLayerGroup() = default;
  ~ExecutionSerializationLayerGroup() override = default;

  void LoadLayers() override;

private:
  std::unique_ptr<ExecutionSerializationRecorder> m_Recorder;
};

} // namespace DirectX
} // namespace gits
