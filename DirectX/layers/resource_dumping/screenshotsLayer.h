// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"
#include "screenshotDump.h"
#include "bit_range.h"

#include <map>
#include <memory>

namespace gits {
namespace DirectX {

class ScreenshotsLayer : public Layer {
public:
  ScreenshotsLayer();
  void Post(IDXGIFactoryCreateSwapChainCommand& c) override;
  void Post(IDXGIFactory2CreateSwapChainForHwndCommand& c) override;
  void Post(xefgSwapChainD3D12InitFromSwapChainCommand& c) override;
  void Post(xefgSwapChainD3D12InitFromSwapChainDescCommand& c) override;
  void Post(xefgSwapChainD3D12GetSwapChainPtrCommand& c) override;
  void Pre(IDXGISwapChainPresentCommand& c) override;
  void Pre(IDXGISwapChain1Present1Command& c) override;

private:
  void SwapChainCreate(unsigned swapChainKey, IUnknown* commandQueue);
  void SwapChainPresent(unsigned swapChainKey, IDXGISwapChain* swapChain);

private:
  std::map<unsigned, std::unique_ptr<ScreenshotDump>> m_ScreenshotDump;
  BitRange m_ScreenshotRange;
  std::wstring m_DumpPath;
  unsigned m_CurrentFrame{};
  std::unordered_map<unsigned, ID3D12CommandQueue*> m_XefgToDeviceMap;
};

} // namespace DirectX
} // namespace gits
