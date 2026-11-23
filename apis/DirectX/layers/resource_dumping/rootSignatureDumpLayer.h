// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"
#include "keyUtils.h"
#include "rootSignatureDump.h"

namespace gits {
namespace DirectX {

class RootSignatureDumpLayer : public Layer {
public:
  RootSignatureDumpLayer();
  void Post(ID3D12DeviceCreateRootSignatureCommand& command) override;

private:
  std::wstring m_DumpPath;
  ConfigKeySet m_RootSignatureKeys;
  RootSignatureDump m_RootSignatureDump;
};

} // namespace DirectX
} // namespace gits
