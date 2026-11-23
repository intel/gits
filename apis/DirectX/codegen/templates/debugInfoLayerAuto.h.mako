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

  void Pre(CreateDXGIFactoryCommand& command) override;
  void Post(CreateDXGIFactoryCommand& command) override;
  void Pre(CreateDXGIFactory1Command& command) override;
  void Post(CreateDXGIFactory1Command& command) override;
  void Pre(CreateDXGIFactory2Command& command) override;
  void Pre(D3D12CreateDeviceCommand& command) override;
  void Post(D3D12CreateDeviceCommand& command) override;
  void Pre(DMLCreateDeviceCommand& command) override;
  void Pre(DMLCreateDevice1Command& command) override;
  void Post(DStorageGetFactoryCommand& command) override;
  void Post(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command) override;
  void Post(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command) override;

  %for interface in interfaces:
  %for function in interface.functions:
  %if interface.api == Api.DXGI or interface.api == Api.D3D12 or interface.api == Api.DML:
  void Post(${interface.name}${function.name}Command& command) override;
  %endif
  %endfor
  %endfor

private:
  DebugInfo m_DebugInfo;
};

} // namespace DirectX
} // namespace gits
