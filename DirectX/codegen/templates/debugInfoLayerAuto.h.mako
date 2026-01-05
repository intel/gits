// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "layerAuto.h"
#include "debugInfo.h"

namespace gits {
namespace DirectX {

class DebugInfoLayer : public Layer {

public:
  DebugInfoLayer() : Layer("DebugInfo") {}

  void pre(CreateDXGIFactoryCommand& command) override;
  void post(CreateDXGIFactoryCommand& command) override;
  void pre(CreateDXGIFactory1Command& command) override;
  void post(CreateDXGIFactory1Command& command) override;
  void pre(CreateDXGIFactory2Command& command) override;
  void pre(D3D12CreateDeviceCommand& command) override;
  void post(D3D12CreateDeviceCommand& command) override;
  void pre(DMLCreateDeviceCommand& command) override;
  void pre(DMLCreateDevice1Command& command) override;
  void post(DStorageGetFactoryCommand& command) override;
  void post(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command) override;
  void post(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command) override;

  %for interface in interfaces:
  %for function in interface.functions:
  %if interface.api == Api.DXGI or interface.api == Api.D3D12 or interface.api == Api.DML:
  void post(${interface.name}${function.name}Command& command) override;
  %endif
  %endfor
  %endfor

private:
  DebugInfo debugInfo_;
};

} // namespace DirectX
} // namespace gits
