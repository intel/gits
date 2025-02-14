// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"
#include "fastOStream.h"

#include <mutex>

namespace gits {
namespace DirectX {

class DebugInfo {

public:
  DebugInfo(FastOStream& traceFile, std::mutex& mutex, bool debugInfoWarning);

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
  static void __stdcall debugMessageCallback(D3D12_MESSAGE_CATEGORY category,
                                             D3D12_MESSAGE_SEVERITY severity,
                                             D3D12_MESSAGE_ID id,
                                             LPCSTR description,
                                             void* context);
  void printMessage(unsigned commandKey,
                    D3D12_MESSAGE_SEVERITY severity,
                    const std::string& message);
  void initDredLog();
  void logDredBreadcrumbs(const D3D12_AUTO_BREADCRUMB_NODE* headNode);
  void logDredPageFaults(const D3D12_DRED_PAGE_FAULT_OUTPUT& pageFaultOutput);

private:
  FastOStream& traceFile_;
  std::mutex& mutex_;
  FastOFileStream dredFile_;
  bool debugInfoWarning_;
  bool d3d12CallbackRegistered_{};
  bool print_{};
};

} // namespace DirectX
} // namespace gits
