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
namespace stream {

class OrderingRecorder;
}
namespace DirectX {

class CaptureManager;

class DllOverrideStoreLayer : public Layer {
public:
  DllOverrideStoreLayer(CaptureManager& manager, stream::OrderingRecorder& recorder);
  ~DllOverrideStoreLayer() = default;

  void Post(D3D12CreateDeviceCommand& command) override;
  void Post(D3D12GetDebugInterfaceCommand& command) override;
  void Post(D3D12CreateRootSignatureDeserializerCommand& command) override;
  void Post(D3D12CreateVersionedRootSignatureDeserializerCommand& command) override;
  void Post(D3D12EnableExperimentalFeaturesCommand& command) override;
  void Post(D3D12GetInterfaceCommand& command) override;
  void Post(D3D12SerializeRootSignatureCommand& command) override;
  void Post(D3D12SerializeVersionedRootSignatureCommand& command) override;

  void Pre(xessGetVersionCommand& command) override;
  void Pre(xessD3D12CreateContextCommand& command) override;
  void Pre(xellGetVersionCommand& command) override;
  void Pre(xellD3D12CreateContextCommand& command) override;
  void Pre(xefgSwapChainGetVersionCommand& command) override;
  void Pre(xefgSwapChainD3D12CreateContextCommand& command) override;

private:
  void CaptureAgilitySDKDll(Command& command);
  void CaptureXessDll(Command& command);
  void CaptureXellDll(Command& command);
  void CaptureXefgDll(Command& command);
  bool CaptureDll(const std::wstring& dllName, unsigned threadId);

  bool m_AgilitySdkDllChecked{};
  std::mutex m_AgilitySdkDllMutex{};
  bool m_XessDllChecked{};
  std::mutex m_XessDllMutex{};
  bool m_XellDllChecked{};
  std::mutex m_XellDllMutex{};
  bool m_XefgDllChecked{};
  std::mutex m_XefgDllMutex{};
  CaptureManager& m_Manager;
  stream::OrderingRecorder& m_Recorder;
};

} // namespace DirectX
} // namespace gits
