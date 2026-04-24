// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"
#include "iunknownWrapper.h"

#include <unordered_map>
#include <vector>
#include <mutex>

namespace gits {
namespace DirectX {

class InterceptorCustomizationLayer : public Layer {
public:
  InterceptorCustomizationLayer() : Layer("InterceptorCustomization") {}

  void Post(IDXGISwapChainGetBufferCommand& command) override;
  void Pre(IUnknownReleaseCommand& command) override;

  void Pre(D3D12CreateDeviceCommand& command) override;
  void Post(D3D12CreateDeviceCommand& command) override;

private:
  std::unordered_map<unsigned, unsigned> m_SwapChainByBufferKey;
  std::unordered_map<unsigned, std::unordered_set<IUnknown*>> m_BuffersBySwapChainKey;
  std::mutex m_Mutex;
};

} // namespace DirectX
} // namespace gits
