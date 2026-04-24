// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerGroup.h"
#include "subcaptureRecorder.h"
#include "subcaptureRange.h"

#include <memory>

namespace gits {
namespace DirectX {

class SubcaptureLayerGroup : public LayerGroup {
public:
  SubcaptureLayerGroup() = default;
  ~SubcaptureLayerGroup() override = default;

  void LoadLayers() override;

private:
  std::unique_ptr<SubcaptureRecorder> m_Recorder;
  std::unique_ptr<SubcaptureRange> m_SubcaptureRange;
};

} // namespace DirectX
} // namespace gits
