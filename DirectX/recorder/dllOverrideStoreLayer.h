// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"

#include <mutex>

namespace gits {
namespace DirectX {

class CaptureManager;
class GitsRecorder;

class DllOverrideStoreLayer : public Layer {
public:
  DllOverrideStoreLayer(CaptureManager& manager, GitsRecorder& recorder);
  ~DllOverrideStoreLayer() = default;

  void post(D3D12CreateDeviceCommand& command) override;
  void post(ID3D12DeviceFactoryCreateDeviceCommand& command) override;

  void pre(xessGetVersionCommand& command) override;
  void pre(xessD3D12CreateContextCommand& command) override;

private:
  void captureAgilitySDKDll(Command& command);
  void captureXessDll(Command& command);

  bool agilitySDKDllchecked_{};
  std::mutex agilitySDKDllMutex_{};
  bool xessDllchecked_{};
  std::mutex xessDllMutex_{};
  CaptureManager& manager_;
  GitsRecorder& recorder_;
};

} // namespace DirectX
} // namespace gits
