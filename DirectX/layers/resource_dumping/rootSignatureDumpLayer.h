// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"
#include "configKeySet.h"
#include "rootSignatureDump.h"

namespace gits {
namespace DirectX {

class RootSignatureDumpLayer : public Layer {
public:
  RootSignatureDumpLayer();
  void post(ID3D12DeviceCreateRootSignatureCommand& command) override;

private:
  std::wstring dumpPath_;
  ConfigKeySet rootSignatureKeys_;
  RootSignatureDump rootSignatureDump_;
};

} // namespace DirectX
} // namespace gits
