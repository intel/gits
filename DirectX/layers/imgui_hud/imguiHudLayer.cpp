// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "imguiHudLayer.h"

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx12.h>

#include "log.h"
#include "gits.h"
#include "imGuiHUD.h"

namespace gits {
namespace DirectX {

ImGuiHUDLayer::ImGuiHUDLayer() : Layer("ImGuiHUDLayer") {
  // nothing
}

ImGuiHUDLayer::~ImGuiHUDLayer() {
  if (!initialized_) {
    return;
  }
  fence_->SetEventOnCompletion(fenceValue_, NULL);
  ImGui_ImplDX12_Shutdown();
}

void ImGuiHUDLayer::post(IDXGIFactoryCreateSwapChainCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }

  initialized_ = initializeResources(c.pDevice_.value, *c.ppSwapChain_.value);
  if (!initialized_) {
    Log(ERR) << "ImGui HUD: Failed to initialize resources";
    return;
  }
}

void ImGuiHUDLayer::post(IDXGIFactory2CreateSwapChainForHwndCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }

  initialized_ = initializeResources(c.pDevice_.value, *c.ppSwapChain_.value);
  if (!initialized_) {
    Log(ERR) << "ImGui HUD: Failed to initialize resources";
    return;
  }
}

void ImGuiHUDLayer::post(IDXGIFactory2CreateSwapChainForCoreWindowCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }

  initialized_ = initializeResources(c.pDevice_.value, *c.ppSwapChain_.value);
  if (!initialized_) {
    Log(ERR) << "ImGui HUD: Failed to initialize resources";
    return;
  }
}

void ImGuiHUDLayer::post(IDXGIFactory2CreateSwapChainForCompositionCommand& c) {
  if (c.result_.value != S_OK) {
    return;
  }

  initialized_ = initializeResources(c.pDevice_.value, *c.ppSwapChain_.value);
  if (!initialized_) {
    Log(ERR) << "ImGui HUD: Failed to initialize resources";
    return;
  }
}

void ImGuiHUDLayer::pre(IDXGISwapChainPresentCommand& command) {
  if (!initialized_) {
    return;
  }

  ImGui_ImplDX12_NewFrame();
  ImGui_ImplWin32_NewFrame();

  ImGui::NewFrame();

  CGits::Instance().GetImGuiHUD()->Render();

  UINT backBufferIdx = swapChain_->GetCurrentBackBufferIndex();
  static UINT frameNumber = 0;
  if (frameNumber > frameContext_.size()) {
    fence_->SetEventOnCompletion(fenceValue_ - frameContext_.size(), NULL);
  }
  ++frameNumber;

  FrameContext& frameCtx = frameContext_[backBufferIdx];
  frameCtx.commandAllocator->Reset();

  D3D12_RESOURCE_BARRIER barrier = {};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  barrier.Transition.pResource = frameContext_[backBufferIdx].rtResource.Get();
  barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

  commandList_->Reset(frameCtx.commandAllocator.Get(), nullptr);
  commandList_->ResourceBarrier(1, &barrier);

  commandList_->OMSetRenderTargets(1, &frameContext_[backBufferIdx].rtvHandle, FALSE, nullptr);
  commandList_->SetDescriptorHeaps(1, srvDescHeap_.GetAddressOf());

  ImGui::Render();
  ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList_.Get());

  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

  commandList_->ResourceBarrier(1, &barrier);
  commandList_->Close();

  commandQueue_->ExecuteCommandLists(
      1, reinterpret_cast<ID3D12CommandList* const*>(commandList_.GetAddressOf()));

  commandQueue_->Signal(fence_.Get(), ++fenceValue_);
}

void ImGuiHUDLayer::pre(IDXGISwapChainResizeBuffersCommand& command) {
  if (!initialized_) {
    return;
  }

  for (auto& frameCtx : frameContext_) {
    frameCtx.rtResource.Reset();
  }
}

void ImGuiHUDLayer::post(IDXGISwapChainResizeBuffersCommand& command) {
  if (!initialized_) {
    return;
  }

  static UINT currentWidth = command.Width_.value;
  static UINT currentHeight = command.Height_.value;

  if (!resizeBuffersWarning_ &&
      (currentWidth != command.Width_.value || currentHeight != command.Height_.value)) {
    Log(WARN) << "ImGui HUD: BackBuffer resized - HUD can appear 'stretched'.";
    resizeBuffersWarning_ = true;
  }

  currentWidth = command.Width_.value;
  currentHeight = command.Height_.value;

  createRenderTarget();

  CGits::Instance().GetImGuiHUD()->SetBackBufferInfo(command.Width_.value, command.Height_.value,
                                                     command.BufferCount_.value);
}

void ImGuiHUDLayer::createRenderTarget() {
  if (!swapChain_ || !device_ || frameContext_.empty()) {
    return;
  }

  for (UINT i = 0; i < frameContext_.size(); ++i) {
    ID3D12Resource* backBuffer = nullptr;
    if (FAILED(swapChain_->GetBuffer(i, IID_PPV_ARGS(&backBuffer)))) {
      Log(ERR) << "ImGui HUD: Failed to get back buffer";
      return;
    }
    device_->CreateRenderTargetView(backBuffer, nullptr, frameContext_[i].rtvHandle);
    frameContext_[i].rtResource = backBuffer;
  }
}

bool ImGuiHUDLayer::initializeResources(IUnknown* device, IDXGISwapChain* swapChain) {
  device->QueryInterface(IID_PPV_ARGS(&commandQueue_));
  swapChain->QueryInterface(IID_PPV_ARGS(&swapChain_));
  if (!SUCCEEDED(swapChain_->GetDevice(IID_PPV_ARGS(&device_)))) {
    return false;
  }

  DXGI_SWAP_CHAIN_DESC sdesc;
  swapChain_->GetDesc(&sdesc);
  window_ = sdesc.OutputWindow;

  frameContext_.resize(sdesc.BufferCount);

  {
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    desc.NumDescriptors = frameContext_.size();
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    desc.NodeMask = 1;
    if (device_->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&rtvDescHeap_)) != S_OK) {
      Log(ERR) << "ImGui HUD: Failed to create RTV descriptor heap";
      return false;
    }
    rtvDescHeap_->SetName(L"ImGuiHUDLayer RTV Descriptor Heap");

    SIZE_T rtvDescriptorSize =
        device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvDescHeap_->GetCPUDescriptorHandleForHeapStart();

    for (auto& frameCtx : frameContext_) {
      frameCtx.rtvHandle = rtvHandle;
      rtvHandle.ptr += rtvDescriptorSize;
    }

    {
      D3D12_DESCRIPTOR_HEAP_DESC desc = {};
      desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
      desc.NumDescriptors = 1;
      desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
      if (device_->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&srvDescHeap_)) != S_OK) {
        Log(ERR) << "ImGui HUD: Failed to create SRV descriptor heap";
        return false;
      }
    }
    srvDescHeap_->SetName(L"ImGuiHUDLayer SRV Descriptor Heap");

    {
      for (auto& frameCtx : frameContext_) {
        device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                        IID_PPV_ARGS(&frameCtx.commandAllocator));
        frameCtx.commandAllocator->SetName(L"ImGuiHUDLayer Command Allocator");
      }

      if (device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                     frameContext_[0].commandAllocator.Get(), NULL,
                                     IID_PPV_ARGS(&commandList_)) != S_OK ||
          commandList_->Close() != S_OK) {
        Log(ERR) << "ImGui HUD: Failed to create command list";
        return false;
      }
      commandList_->SetName(L"ImGuiHUDLayer Command List");
    }

    fenceValue_ = 0;
    if (device_->CreateFence(fenceValue_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_)) != S_OK) {
      Log(ERR) << "ImGui HUD: Failed to create fence";
      return false;
    }
    fence_->SetName(L"ImGuiHUDLayer Fence");

    createRenderTarget();

    initializeImGui(sdesc.BufferDesc.Format);
    CGits::Instance().GetImGuiHUD()->SetBackBufferInfo(sdesc.BufferDesc.Width,
                                                       sdesc.BufferDesc.Height, sdesc.BufferCount);

    return true;
  }
}

void ImGuiHUDLayer::initializeImGui(DXGI_FORMAT format) {
  ImGui::CreateContext();

  ImGui_ImplWin32_Init(window_);
  ImGui_ImplDX12_Init(device_.Get(), frameContext_.size(), format, srvDescHeap_.Get(),
                      srvDescHeap_->GetCPUDescriptorHandleForHeapStart(),
                      srvDescHeap_->GetGPUDescriptorHandleForHeapStart());

  float dpiscale = std::max(1.0f, ImGui_ImplWin32_GetDpiScaleForHwnd(window_));
  CGits::Instance().GetImGuiHUD()->SetupImGUI(dpiscale);
  ImGui_ImplDX12_CreateDeviceObjects();
}

} // namespace DirectX
} // namespace gits
