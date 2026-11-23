// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "debugInfoLayerAuto.h"

namespace gits {
namespace DirectX {

void DebugInfoLayer::Pre(CreateDXGIFactoryCommand& command) {
  m_DebugInfo.createDXGIFactory(command);
}

void DebugInfoLayer::Pre(CreateDXGIFactory1Command& command) {
  m_DebugInfo.createDXGIFactory1(command);
}

void DebugInfoLayer::Post(CreateDXGIFactoryCommand& command) {
  command.Skip = false;
}

void DebugInfoLayer::Post(CreateDXGIFactory1Command& command) {
  command.Skip = false;
}

void DebugInfoLayer::Pre(CreateDXGIFactory2Command& command) {
  m_DebugInfo.createDXGIFactory2(command);
}

void DebugInfoLayer::Pre(D3D12CreateDeviceCommand& command) {
  m_DebugInfo.createD3D12DevicePre(command);
}

void DebugInfoLayer::Post(D3D12CreateDeviceCommand& command) {
  m_DebugInfo.createD3D12DevicePost(command);
}

void DebugInfoLayer::Pre(DMLCreateDeviceCommand& command) {
  m_DebugInfo.createDMLDevicePre(command);
}

void DebugInfoLayer::Pre(DMLCreateDevice1Command& command) {
  m_DebugInfo.createDMLDevice1Pre(command);
}

void DebugInfoLayer::Post(DStorageGetFactoryCommand& command) {
  if (command.Skip || FAILED(command.m_Result.Value) || !command.m_ppv.Value ||
      !(*command.m_ppv.Value)) {
    return;
  }
  IDStorageFactory* factory = (IDStorageFactory*)*command.m_ppv.Value;
  factory->SetDebugFlags(DSTORAGE_DEBUG_SHOW_ERRORS | DSTORAGE_DEBUG_RECORD_OBJECT_NAMES);
}

void DebugInfoLayer::Post(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command) {
  if (!command.Skip) {
    m_DebugInfo.checkD3D12DebugInfo(command, command.m_pCommandList.Value);
    m_DebugInfo.checkD3D12DeviceRemoval(command, command.m_pCommandList.Value);
  }
}

void DebugInfoLayer::Post(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command) {
  if (!command.Skip) {
    m_DebugInfo.checkD3D12DebugInfo(command, command.m_pCommandList.Value);
    m_DebugInfo.checkD3D12DeviceRemoval(command, command.m_pCommandList.Value);
  }
}

%for interface in interfaces:
%for function in interface.functions:
%if interface.api == Api.D3D12 or interface.api == Api.DML or interface.api == Api.DXGI:
void DebugInfoLayer::Post(${interface.name}${function.name}Command& command) {
  if (!command.Skip) {
%if interface.api == Api.DXGI:
    m_DebugInfo.checkDXGIDebugInfo(command, command.m_Object.Value);
%elif interface.api == Api.D3D12 or interface.api == Api.DML:
    m_DebugInfo.checkD3D12DebugInfo(command, command.m_Object.Value);
%endif
%if interface.api == Api.D3D12 and (not "CommandList" in interface.name):
    m_DebugInfo.checkD3D12DeviceRemoval(command, command.m_Object.Value);
%endif
  }
}

%endif
%endfor
%endfor
} // namespace DirectX
} // namespace gits
