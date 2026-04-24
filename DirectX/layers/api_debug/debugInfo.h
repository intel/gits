// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"
#include "nvapi.h"

#include <fstream>

namespace gits {
namespace DirectX {

class DebugInfo {

public:
  DebugInfo();

  void createDXGIFactory(CreateDXGIFactoryCommand& command);
  void createDXGIFactory1(CreateDXGIFactory1Command& command);
  void createDXGIFactory2(CreateDXGIFactory2Command& command);
  void createD3D12DevicePre(D3D12CreateDeviceCommand& command);
  void createD3D12DevicePost(D3D12CreateDeviceCommand& command);
  void createDMLDevicePre(DMLCreateDeviceCommand& command);
  void createDMLDevice1Pre(DMLCreateDevice1Command& command);
  void checkDXGIDebugInfo(Command& command, IUnknown* object);
  void checkD3D12DebugInfo(Command& command, IUnknown* object);
  void checkD3D12DeviceRemoval(Command& command, IUnknown* object);

private:
  static void __stdcall DebugMessageCallback(D3D12_MESSAGE_CATEGORY category,
                                             D3D12_MESSAGE_SEVERITY severity,
                                             D3D12_MESSAGE_ID id,
                                             LPCSTR description,
                                             void* context);
  static void __stdcall NvApiDebugMessageCallback(
      void* userData,
      NVAPI_D3D12_RAYTRACING_VALIDATION_MESSAGE_SEVERITY severity,
      const char* messageCode,
      const char* message,
      const char* messageDetails);
  void TraceMessage(D3D12_MESSAGE_SEVERITY severity, const char* message);
  void InitDredLog();
  void LogDredBreadcrumbs(const D3D12_AUTO_BREADCRUMB_NODE* headNode);
  void LogDredPageFaults(const D3D12_DRED_PAGE_FAULT_OUTPUT& pageFaultOutput);
  void InitNvApiValidation(ID3D12Device* device);
  void FlushNvApiValidation(IUnknown* object);

  bool m_D3d12CallbackRegistered{};
  bool m_NvApiCallbackRegistered{};
  std::ofstream m_DredFile{};
};

} // namespace DirectX
} // namespace gits
