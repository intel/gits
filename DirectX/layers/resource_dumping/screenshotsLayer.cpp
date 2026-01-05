// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "screenshotsLayer.h"
#include "gits.h"
#include "configurationLib.h"

#include <wrl/client.h>
#include <filesystem>

namespace gits {
namespace DirectX {

ScreenshotsLayer::ScreenshotsLayer()
    : Layer("Screenshots"),
      screenshotRange_(Configurator::Get().directx.features.screenshots.frames) {
  auto dumpPath = Configurator::Get().common.player.outputDir;
  if (Configurator::IsRecorder()) {
    dumpPath = Configurator::Get().common.recorder.dumpPath / "gitsScreenshots/gitsRecorder";
  } else if (Configurator::IsPlayer() && dumpPath.empty()) {
    dumpPath = Configurator::Get().common.player.streamDir / "gitsScreenshots/gitsPlayer";
  }
  if (!dumpPath.empty() && !std::filesystem::exists(dumpPath)) {
    std::filesystem::create_directories(dumpPath);
  }
  dumpPath_ = dumpPath;

  if (!Configurator::Get().common.player.captureFrames.empty()) {
    screenshotRange_ = Configurator::Get().common.player.captureFrames;
  }
}

void ScreenshotsLayer::post(IDXGIFactoryCreateSwapChainCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  swapChainCreate(c.ppSwapChain_.key, c.pDevice_.value);
}

void ScreenshotsLayer::post(IDXGIFactory2CreateSwapChainForHwndCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }
  swapChainCreate(c.ppSwapChain_.key, c.pDevice_.value);
}

void ScreenshotsLayer::pre(IDXGISwapChainPresentCommand& c) {
  if (c.Flags_.value & DXGI_PRESENT_TEST) {
    return;
  }
  if (screenshotRange_[CGits::Instance().CurrentFrame()]) {
    swapChainPresent(c.object_.key, c.object_.value);
  }
}

void ScreenshotsLayer::pre(IDXGISwapChain1Present1Command& c) {
  if (c.PresentFlags_.value & DXGI_PRESENT_TEST) {
    return;
  }
  if (screenshotRange_[CGits::Instance().CurrentFrame()]) {
    swapChainPresent(c.object_.key, c.object_.value);
  }
}

void ScreenshotsLayer::swapChainCreate(unsigned swapChainKey, IUnknown* commandQueue_) {
  ID3D12CommandQueue* commandQueue{};
  HRESULT hr = commandQueue_->QueryInterface(IID_PPV_ARGS(&commandQueue));
  GITS_ASSERT(hr == S_OK);
  screenshotDump_[swapChainKey].reset(new ScreenshotDump(commandQueue));
}

void ScreenshotsLayer::swapChainPresent(unsigned swapChainKey, IDXGISwapChain* swapChain) {
  auto it = screenshotDump_.find(swapChainKey);
  GITS_ASSERT(it != screenshotDump_.end());

  Microsoft::WRL::ComPtr<IDXGISwapChain3> swapChain3;
  HRESULT hr = swapChain->QueryInterface(IID_PPV_ARGS(&swapChain3));
  GITS_ASSERT(hr == S_OK);

  unsigned bufferIndex = swapChain3->GetCurrentBackBufferIndex();
  Microsoft::WRL::ComPtr<ID3D12Resource> backBuffer;
  hr = swapChain->GetBuffer(bufferIndex, IID_PPV_ARGS(&backBuffer));

  unsigned currentFrame = CGits::Instance().CurrentFrame();

  std::wstringstream outputName;
  outputName << dumpPath_ << L"/frame" << std::setw(8) << std::setfill(L'0') << currentFrame;
  it->second->dump(backBuffer.Get(), outputName.str());
}

} // namespace DirectX
} // namespace gits
