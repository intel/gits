// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "screenshotsLayer.h"
#include "keyUtils.h"
#include "log.h"
#include "configurator.h"

#include <wrl/client.h>
#include <filesystem>

namespace gits {
namespace DirectX {

ScreenshotsLayer::ScreenshotsLayer()
    : Layer("Screenshots"),
      m_ScreenshotRange(Configurator::Get().directx.features.screenshots.frames) {
  auto dumpPath = Configurator::Get().common.player.outputDir;
  if (Configurator::IsRecorder()) {
    dumpPath = Configurator::Get().common.recorder.dumpPath / "gitsScreenshots/gitsRecorder";
  } else if (Configurator::IsPlayer() && dumpPath.empty()) {
    dumpPath = Configurator::Get().common.player.streamDir / "gitsScreenshots/gitsPlayer";
  }
  if (!dumpPath.empty() && !std::filesystem::exists(dumpPath)) {
    std::filesystem::create_directories(dumpPath);
  }
  m_DumpPath = dumpPath;

  if (!Configurator::Get().common.player.captureFrames.empty()) {
    m_ScreenshotRange = Configurator::Get().common.player.captureFrames;
  }
}

void ScreenshotsLayer::Post(IDXGIFactoryCreateSwapChainCommand& c) {
  if (c.m_Result.Value != S_OK) {
    return;
  }
  SwapChainCreate(c.m_ppSwapChain.Key, c.m_pDevice.Value);
}

void ScreenshotsLayer::Post(IDXGIFactory2CreateSwapChainForHwndCommand& c) {
  if (c.m_Result.Value != S_OK) {
    return;
  }
  SwapChainCreate(c.m_ppSwapChain.Key, c.m_pDevice.Value);
}

void ScreenshotsLayer::Post(xefgSwapChainD3D12InitFromSwapChainCommand& c) {
  if (c.m_Result.Value != XEFG_SWAPCHAIN_RESULT_SUCCESS) {
    return;
  }
  m_XefgToDeviceMap[c.m_hSwapChain.Key] = c.m_pCmdQueue.Value;
}

void ScreenshotsLayer::Post(xefgSwapChainD3D12InitFromSwapChainDescCommand& c) {
  if (c.m_Result.Value != XEFG_SWAPCHAIN_RESULT_SUCCESS) {
    return;
  }
  m_XefgToDeviceMap[c.m_hSwapChain.Key] = c.m_pCmdQueue.Value;
}

void ScreenshotsLayer::Post(xefgSwapChainD3D12GetSwapChainPtrCommand& c) {
  if (c.m_Result.Value != XEFG_SWAPCHAIN_RESULT_SUCCESS) {
    return;
  }
  auto cmdQueueIt = m_XefgToDeviceMap.find(c.m_hSwapChain.Key);
  GITS_ASSERT(cmdQueueIt != m_XefgToDeviceMap.end());
  SwapChainCreate(c.m_ppSwapChain.Key, cmdQueueIt->second);
}

void ScreenshotsLayer::Pre(IDXGISwapChainPresentCommand& c) {
  if (c.m_Flags.Value & DXGI_PRESENT_TEST || IsStateRestoreKey(c.Key)) {
    return;
  }
  ++m_CurrentFrame;
  if (m_ScreenshotRange[m_CurrentFrame]) {
    SwapChainPresent(c.m_Object.Key, c.m_Object.Value);
  }
}

void ScreenshotsLayer::Pre(IDXGISwapChain1Present1Command& c) {
  if (c.m_PresentFlags.Value & DXGI_PRESENT_TEST || IsStateRestoreKey(c.Key)) {
    return;
  }
  ++m_CurrentFrame;
  if (m_ScreenshotRange[m_CurrentFrame]) {
    SwapChainPresent(c.m_Object.Key, c.m_Object.Value);
  }
}

void ScreenshotsLayer::SwapChainCreate(unsigned swapChainKey, IUnknown* m_CommandQueue) {
  ID3D12CommandQueue* commandQueue{};
  HRESULT hr = m_CommandQueue->QueryInterface(IID_PPV_ARGS(&commandQueue));
  GITS_ASSERT(hr == S_OK);
  m_ScreenshotDump[swapChainKey].reset(new ScreenshotDump(commandQueue));
}

void ScreenshotsLayer::SwapChainPresent(unsigned swapChainKey, IDXGISwapChain* swapChain) {
  auto it = m_ScreenshotDump.find(swapChainKey);
  GITS_ASSERT(it != m_ScreenshotDump.end());

  Microsoft::WRL::ComPtr<IDXGISwapChain3> swapChain3;
  HRESULT hr = swapChain->QueryInterface(IID_PPV_ARGS(&swapChain3));
  GITS_ASSERT(hr == S_OK);

  unsigned bufferIndex = swapChain3->GetCurrentBackBufferIndex();
  Microsoft::WRL::ComPtr<ID3D12Resource> backBuffer;
  hr = swapChain->GetBuffer(bufferIndex, IID_PPV_ARGS(&backBuffer));

  std::wstringstream outputName;
  outputName << m_DumpPath << L"/frame" << std::setw(8) << std::setfill(L'0') << m_CurrentFrame;
  it->second->dump(backBuffer.Get(), outputName.str());
}

} // namespace DirectX
} // namespace gits
