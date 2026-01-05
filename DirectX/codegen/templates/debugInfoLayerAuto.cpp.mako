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

void DebugInfoLayer::pre(CreateDXGIFactoryCommand& command) {
  debugInfo_.createDXGIFactory(command);
}

void DebugInfoLayer::pre(CreateDXGIFactory1Command& command) {
  debugInfo_.createDXGIFactory1(command);
}

void DebugInfoLayer::post(CreateDXGIFactoryCommand& command) {
  command.skip = false;
}

void DebugInfoLayer::post(CreateDXGIFactory1Command& command) {
  command.skip = false;
}

void DebugInfoLayer::pre(CreateDXGIFactory2Command& command) {
  debugInfo_.createDXGIFactory2(command);
}

void DebugInfoLayer::pre(D3D12CreateDeviceCommand& command) {
  debugInfo_.createD3D12DevicePre(command);
}

void DebugInfoLayer::post(D3D12CreateDeviceCommand& command) {
  debugInfo_.createD3D12DevicePost(command);
}

void DebugInfoLayer::pre(DMLCreateDeviceCommand& command) {
  debugInfo_.createDMLDevicePre(command);
}

void DebugInfoLayer::pre(DMLCreateDevice1Command& command) {
  debugInfo_.createDMLDevice1Pre(command);
}

void DebugInfoLayer::post(DStorageGetFactoryCommand& command) {
  if (command.skip || FAILED(command.result_.value) || !command.ppv_.value ||
      !(*command.ppv_.value)) {
    return;
  }
  IDStorageFactory* factory = (IDStorageFactory*)*command.ppv_.value;
  factory->SetDebugFlags(DSTORAGE_DEBUG_SHOW_ERRORS | DSTORAGE_DEBUG_RECORD_OBJECT_NAMES);
}

void DebugInfoLayer::post(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command) {
  if (!command.skip) {
    debugInfo_.checkD3D12DebugInfo(command, command.pCommandList_.value);
    debugInfo_.checkD3D12DeviceRemoval(command, command.pCommandList_.value);
  }
}

void DebugInfoLayer::post(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command) {
  if (!command.skip) {
    debugInfo_.checkD3D12DebugInfo(command, command.pCommandList_.value);
    debugInfo_.checkD3D12DeviceRemoval(command, command.pCommandList_.value);
  }
}

%for interface in interfaces:
%for function in interface.functions:
%if interface.api == Api.D3D12 or interface.api == Api.DML or interface.api == Api.DXGI:
void DebugInfoLayer::post(${interface.name}${function.name}Command& command) {
  if (!command.skip) {
%if interface.api == Api.DXGI:
    debugInfo_.checkDXGIDebugInfo(command, command.object_.value);
%elif interface.api == Api.D3D12 or interface.api == Api.DML:
    debugInfo_.checkD3D12DebugInfo(command, command.object_.value);
%endif
%if interface.api == Api.D3D12 and (not "CommandList" in interface.name):
    debugInfo_.checkD3D12DeviceRemoval(command, command.object_.value);
%endif
  }
}

%endif
%endfor
%endfor
} // namespace DirectX
} // namespace gits
