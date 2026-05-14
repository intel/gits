// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerGroup.h"
#include "commandListSplitRecorder.h"

#include <memory>

namespace gits {
namespace DirectX {

class CommandListSplitLayerGroup : public LayerGroup {
public:
  CommandListSplitLayerGroup() = default;
  ~CommandListSplitLayerGroup() override = default;

  void LoadLayers() override;

private:
  std::unique_ptr<CommandListSplitRecorder> m_Recorder;
};

} // namespace DirectX
} // namespace gits
