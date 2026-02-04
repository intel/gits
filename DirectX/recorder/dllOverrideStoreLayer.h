// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
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
  void post(D3D12GetDebugInterfaceCommand& command) override;
  void post(D3D12CreateRootSignatureDeserializerCommand& command) override;
  void post(D3D12CreateVersionedRootSignatureDeserializerCommand& command) override;
  void post(D3D12EnableExperimentalFeaturesCommand& command) override;
  void post(D3D12GetInterfaceCommand& command) override;
  void post(D3D12SerializeRootSignatureCommand& command) override;
  void post(D3D12SerializeVersionedRootSignatureCommand& command) override;

  void pre(xessGetVersionCommand& command) override;
  void pre(xessD3D12CreateContextCommand& command) override;
  void pre(xellGetVersionCommand& command) override;
  void pre(xellD3D12CreateContextCommand& command) override;

private:
  void captureAgilitySDKDll(Command& command);
  void captureXessDll(Command& command);
  void captureXellDll(Command& command);
  bool captureDll(const std::wstring& dllName, unsigned threadId);

  bool agilitySDKDllchecked_{};
  std::mutex agilitySDKDllMutex_{};
  bool xessDllchecked_{};
  std::mutex xessDllMutex_{};
  bool xellDllchecked_{};
  std::mutex xellDllMutex_{};
  CaptureManager& manager_;
  GitsRecorder& recorder_;
};

} // namespace DirectX
} // namespace gits
