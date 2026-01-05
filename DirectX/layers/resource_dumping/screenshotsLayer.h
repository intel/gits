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
  void post(IDXGIFactoryCreateSwapChainCommand& c) override;
  void post(IDXGIFactory2CreateSwapChainForHwndCommand& c) override;
  void pre(IDXGISwapChainPresentCommand& c) override;
  void pre(IDXGISwapChain1Present1Command& c) override;

private:
  void swapChainCreate(unsigned swapChainKey, IUnknown* commandQueue);
  void swapChainPresent(unsigned swapChainKey, IDXGISwapChain* swapChain);

private:
  std::map<unsigned, std::unique_ptr<ScreenshotDump>> screenshotDump_;
  BitRange screenshotRange_;
  std::wstring dumpPath_;
};

} // namespace DirectX
} // namespace gits
