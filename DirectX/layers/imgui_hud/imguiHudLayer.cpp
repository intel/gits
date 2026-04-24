// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "imguiHudLayer.h"

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx12.h>

#include "keyUtils.h"
#include "gits.h"
#include "imGuiHUD.h"

namespace gits {
namespace DirectX {

ImGuiHUDLayer::ImGuiHUDLayer() : Layer("ImGuiHUDLayer") {
  // nothing
}

ImGuiHUDLayer::~ImGuiHUDLayer() {
  if (!m_Initialized) {
    return;
  }
  m_Fence->SetEventOnCompletion(m_FenceValue, NULL);
  ImGui_ImplDX12_Shutdown();
}

void ImGuiHUDLayer::Post(IDXGISwapChainGetBufferCommand& c) {
  if (c.m_Result.Value == S_OK) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_BackBufferKeys.insert(c.m_ppSurface.Key);
  }
}

void ImGuiHUDLayer::Pre(IUnknownReleaseCommand& c) {
  if (!c.m_Object.Value) {
    return;
  }

  c.m_Object.Value->AddRef();
  auto result = c.m_Object.Value->Release();
  if (result != 1) {
    return;
  }

  std::lock_guard<std::mutex> lock(m_Mutex);
  if (!m_BackBufferKeys.contains(c.m_Object.Key)) {
    return;
  }

  for (unsigned i = 0; i < m_FrameContext.size(); ++i) {
    WaitForFrame(i);
  }

  m_BackBufferKeys.clear();
}

void ImGuiHUDLayer::Pre(ID3D12CommandQueueExecuteCommandListsCommand& c) {
  if (m_SwapChain && m_FirstExecuteInFrame) {
    WaitForCurrentFrame();
    m_FirstExecuteInFrame = false;
  }
}

void ImGuiHUDLayer::Post(IDXGIFactoryCreateSwapChainCommand& c) {
  if (c.m_Result.Value != S_OK) {
    return;
  }

  m_Initialized = InitializeResources(c.m_pDevice.Value, *c.m_ppSwapChain.Value);
  if (!m_Initialized) {
    LOG_ERROR << "ImGui HUD: Failed to initialize resources";
    return;
  }
}

void ImGuiHUDLayer::Post(IDXGIFactory2CreateSwapChainForHwndCommand& c) {
  if (c.m_Result.Value != S_OK) {
    return;
  }

  m_Initialized = InitializeResources(c.m_pDevice.Value, *c.m_ppSwapChain.Value);
  if (!m_Initialized) {
    LOG_ERROR << "ImGui HUD: Failed to initialize resources";
    return;
  }
}

void ImGuiHUDLayer::Post(IDXGIFactory2CreateSwapChainForCoreWindowCommand& c) {
  if (c.m_Result.Value != S_OK) {
    return;
  }

  m_Initialized = InitializeResources(c.m_pDevice.Value, *c.m_ppSwapChain.Value);
  if (!m_Initialized) {
    LOG_ERROR << "ImGui HUD: Failed to initialize resources";
    return;
  }
}

void ImGuiHUDLayer::Post(IDXGIFactory2CreateSwapChainForCompositionCommand& c) {
  if (c.m_Result.Value != S_OK) {
    return;
  }

  m_Initialized = InitializeResources(c.m_pDevice.Value, *c.m_ppSwapChain.Value);
  if (!m_Initialized) {
    LOG_ERROR << "ImGui HUD: Failed to initialize resources";
    return;
  }
}

void ImGuiHUDLayer::Pre(IDXGISwapChainPresentCommand& c) {
  if (c.Skip || c.m_Result.Value != S_OK || c.m_Flags.Value & DXGI_PRESENT_TEST) {
    return;
  }
  OnPrePresent();
}

void ImGuiHUDLayer::Post(IDXGISwapChainPresentCommand& c) {
  if (c.Skip || c.m_Result.Value != S_OK || c.m_Flags.Value & DXGI_PRESENT_TEST ||
      IsStateRestoreKey(c.Key)) {
    return;
  }
  CGits::Instance().FrameCountUp();
  m_FirstExecuteInFrame = true;
}

void ImGuiHUDLayer::Pre(IDXGISwapChain1Present1Command& c) {
  if (c.Skip || c.m_Result.Value != S_OK || c.m_PresentFlags.Value & DXGI_PRESENT_TEST) {
    return;
  }
  OnPrePresent();
}

void ImGuiHUDLayer::Post(IDXGISwapChain1Present1Command& c) {
  if (c.Skip || c.m_Result.Value != S_OK || c.m_PresentFlags.Value & DXGI_PRESENT_TEST ||
      IsStateRestoreKey(c.Key)) {
    return;
  }
  CGits::Instance().FrameCountUp();
  m_FirstExecuteInFrame = true;
}

void ImGuiHUDLayer::Post(IDXGISwapChainResizeBuffersCommand& command) {
  if (!m_Initialized || CGits::InstancePtr() == nullptr) {
    return;
  }

  // ResizeBuffers can be called with BufferCount set to 0 (to keep the current count)
  static unsigned currentBufferCount = m_FrameContext.size();
  unsigned bufferCount = command.m_BufferCount.Value;
  if (bufferCount == 0) {
    bufferCount = currentBufferCount;
  } else {
    currentBufferCount = bufferCount;
  }

  if (!CreateFrameContext(bufferCount)) {
    LOG_ERROR << "ImGui HUD: Failed to correctly create frame context on ResizeBuffers";
  }
  CGits::Instance().GetImGuiHUD()->SetBackBufferInfo(command.m_Width.Value, command.m_Height.Value,
                                                     bufferCount);
}

void ImGuiHUDLayer::Post(IDXGISwapChain3ResizeBuffers1Command& command) {
  if (!m_Initialized || CGits::InstancePtr() == nullptr) {
    return;
  }

  // ResizeBuffers can be called with BufferCount set to 0 (to keep the current count)
  static unsigned currentBufferCount = m_FrameContext.size();
  unsigned bufferCount = command.m_BufferCount.Value;
  if (bufferCount == 0) {
    bufferCount = currentBufferCount;
  } else {
    currentBufferCount = bufferCount;
  }

  if (!CreateFrameContext(bufferCount)) {
    LOG_ERROR << "ImGui HUD: Failed to correctly create frame context on ResizeBuffers";
  }
  CGits::Instance().GetImGuiHUD()->SetBackBufferInfo(command.m_Width.Value, command.m_Height.Value,
                                                     bufferCount);
}

bool ImGuiHUDLayer::CreateFrameContext(unsigned bufferCount) {
  if (!m_SwapChain || !m_Device) {
    return false;
  }

  auto rtvHandleSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  auto rtvHandle = m_RtvDescHeap->GetCPUDescriptorHandleForHeapStart();

  m_FrameContext.resize(bufferCount);
  for (UINT i = 0; i < bufferCount; ++i) {
    WaitForFrame(i);

    FrameContext& frameCtx = m_FrameContext[i];
    frameCtx.fenceValue = 0;

    // Get the backBuffer
    Microsoft::WRL::ComPtr<ID3D12Resource> backBuffer;
    auto hr = m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer));
    if (hr != S_OK) {
      LOG_ERROR << "ImGui HUD: Failed to get back buffer";
      return false;
    }

    // Create the RTV for backBuffer
    frameCtx.rtvHandle = rtvHandle;
    m_Device->CreateRenderTargetView(backBuffer.Get(), nullptr, frameCtx.rtvHandle);
    frameCtx.rtResource = backBuffer.Get();

    // Create the CommandAllocators
    hr = m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                          IID_PPV_ARGS(&frameCtx.commandAllocator));
    if (hr != S_OK) {
      LOG_ERROR << "ImGui HUD: Failed to create CommandAllocator";
      return false;
    }
    frameCtx.commandAllocator->SetName(L"ImGuiHUDLayer Command Allocator");

    // Increment RTV handle
    rtvHandle.ptr += rtvHandleSize;
  }

  return true;
}

bool ImGuiHUDLayer::InitializeResources(IUnknown* device, IDXGISwapChain* swapChain) {
  device->QueryInterface(IID_PPV_ARGS(&m_CommandQueue));
  swapChain->QueryInterface(IID_PPV_ARGS(&m_SwapChain));
  m_SwapChain->GetDevice(IID_PPV_ARGS(&m_Device));
  if (!m_CommandQueue || !m_SwapChain || !m_Device) {
    return false;
  }

  DXGI_SWAP_CHAIN_DESC swapChainDesc;
  m_SwapChain->GetDesc(&swapChainDesc);
  if (swapChainDesc.BufferCount == 0) {
    return false;
  }

  m_Window = swapChainDesc.OutputWindow;

  // Create RTV Descriptor Heap
  D3D12_DESCRIPTOR_HEAP_DESC desc = {};
  desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
  desc.NumDescriptors = DXGI_MAX_SWAP_CHAIN_BUFFERS;
  desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  desc.NodeMask = 1;
  if (m_Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_RtvDescHeap)) != S_OK) {
    LOG_ERROR << "ImGui HUD: Failed to create RTV descriptor heap";
    return false;
  }
  m_RtvDescHeap->SetName(L"ImGuiHUDLayer RTV Descriptor Heap");

  // Create SRV Descriptor Heap
  desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  desc.NumDescriptors = 1;
  desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  if (m_Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_SrvDescHeap)) != S_OK) {
    LOG_ERROR << "ImGui HUD: Failed to create SRV descriptor heap";
    return false;
  }
  m_SrvDescHeap->SetName(L"ImGuiHUDLayer SRV Descriptor Heap");

  if (!CreateFrameContext(swapChainDesc.BufferCount)) {
    return false;
  }

  // Create Fence
  m_FenceValue = 0;
  if (m_Device->CreateFence(m_FenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence)) != S_OK) {
    LOG_ERROR << "ImGui HUD: Failed to create fence";
    return false;
  }
  m_Fence->SetName(L"ImGuiHUDLayer Fence");

  // Create CommandList (using the first CommandAllocator)
  if (m_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                  m_FrameContext[0].commandAllocator.Get(), NULL,
                                  IID_PPV_ARGS(&m_CommandList)) != S_OK) {
    LOG_ERROR << "ImGui HUD: Failed to create command list";
    return false;
  }
  m_CommandList->Close();
  m_CommandList->SetName(L"ImGuiHUDLayer Command List");

  InitializeImGui(swapChainDesc.BufferDesc.Format);
  CGits::Instance().GetImGuiHUD()->SetBackBufferInfo(
      swapChainDesc.BufferDesc.Width, swapChainDesc.BufferDesc.Height, swapChainDesc.BufferCount);

  return true;
}

void ImGuiHUDLayer::InitializeImGui(DXGI_FORMAT format) {
  ImGui::CreateContext();

  ImGui_ImplWin32_Init(m_Window);
  ImGui_ImplDX12_Init(m_Device.Get(), m_FrameContext.size(), format, m_SrvDescHeap.Get(),
                      m_SrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
                      m_SrvDescHeap->GetGPUDescriptorHandleForHeapStart());

  float dpiscale = std::max(1.0f, ImGui_ImplWin32_GetDpiScaleForHwnd(m_Window));
  CGits::Instance().GetImGuiHUD()->SetupImGUI(dpiscale);
  ImGui_ImplDX12_CreateDeviceObjects();
}

void ImGuiHUDLayer::OnPrePresent() {
  static bool firstRun{true};
  if (firstRun) {
    firstRun = false;
    Present(); // assures Hud in a first frame of a stream
  }
  WaitForCurrentFrame();
  Present();
}

void ImGuiHUDLayer::WaitForCurrentFrame() {
  UINT backBufferIdx = m_SwapChain->GetCurrentBackBufferIndex();
  WaitForFrame(backBufferIdx);
}

void ImGuiHUDLayer::WaitForFrame(unsigned bufferIndex) {
  FrameContext& frameCtx = m_FrameContext[bufferIndex];

  if (frameCtx.fenceValue) {
    if (m_Fence->GetCompletedValue() < frameCtx.fenceValue) {
      m_Fence->SetEventOnCompletion(frameCtx.fenceValue, NULL);
    }
  }
}

void ImGuiHUDLayer::Present() {
  if (!m_Initialized || CGits::InstancePtr() == nullptr) {
    return;
  }

  ImGui_ImplDX12_NewFrame();
  ImGui_ImplWin32_NewFrame();

  ImGui::NewFrame();

  CGits::Instance().GetImGuiHUD()->Render();

  UINT backBufferIdx = m_SwapChain->GetCurrentBackBufferIndex();
  FrameContext& frameCtx = m_FrameContext[backBufferIdx];

  frameCtx.commandAllocator->Reset();

  D3D12_RESOURCE_BARRIER barrier = {};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  barrier.Transition.pResource = frameCtx.rtResource;
  barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

  m_CommandList->Reset(frameCtx.commandAllocator.Get(), nullptr);
  m_CommandList->ResourceBarrier(1, &barrier);

  m_CommandList->OMSetRenderTargets(1, &frameCtx.rtvHandle, FALSE, nullptr);
  m_CommandList->SetDescriptorHeaps(1, m_SrvDescHeap.GetAddressOf());

  ImGui::Render();
  ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_CommandList.Get());

  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

  m_CommandList->ResourceBarrier(1, &barrier);
  m_CommandList->Close();

  m_CommandQueue->ExecuteCommandLists(
      1, reinterpret_cast<ID3D12CommandList* const*>(m_CommandList.GetAddressOf()));

  frameCtx.fenceValue = ++m_FenceValue;
  m_CommandQueue->Signal(m_Fence.Get(), frameCtx.fenceValue);
}

} // namespace DirectX
} // namespace gits
