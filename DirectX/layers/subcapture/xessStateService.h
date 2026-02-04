// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "subcaptureRecorder.h"
#include "commandsAuto.h"

#include <memory>
#include <optional>
#include <map>
#include <d3d12.h>

namespace gits {
namespace DirectX {

class StateTrackingService;

#pragma region XESS

class XessStateService {
public:
  struct ContextState {
    unsigned key{};
    unsigned deviceKey{};
    ID3D12Device* device{};
    std::optional<xess_d3d12_init_params_t_Argument> initParams;
    std::unique_ptr<float[]> jitterScale;
    std::optional<float> exposureScale;
    std::unique_ptr<float[]> velocityScale;
    std::optional<bool> forceLegacyScaleFactors;
  };

public:
  XessStateService(StateTrackingService& stateService, SubcaptureRecorder& recorder)
      : stateService_(stateService), recorder_(recorder) {}
  void restoreState();
  void storeContextState(ContextState* state);
  ContextState* getContextState(unsigned key) {
    return contextStatesByContextKey_[key].get();
  }
  void destroyDevice(unsigned key);
  void destroyContext(unsigned key);

private:
  void restoreContextState(ContextState* state);

private:
  StateTrackingService& stateService_;
  SubcaptureRecorder& recorder_;
  std::map<unsigned, std::unique_ptr<ContextState>> contextStatesByContextKey_;
  std::map<unsigned, ContextState*> contextStatesByDeviceKey_;
};

#pragma endregion

#pragma region XELL

class XellStateService {
public:
  struct ContextState {
    unsigned key{};
    unsigned deviceKey{};
    ID3D12Device* device{};
    std::optional<xell_sleep_params_t> sleepParams;
    std::unordered_map<uint32_t, std::vector<xell_latency_marker_type_t>> registeredMarkers;
  };

public:
  XellStateService(StateTrackingService& stateService, SubcaptureRecorder& recorder)
      : stateService_(stateService), recorder_(recorder) {}
  void restoreState();
  void storeContextState(ContextState* state);
  ContextState* getContextState(unsigned key) {
    return contextStatesByContextKey_[key].get();
  }
  void trackMarker(unsigned key, uint32_t frame, xell_latency_marker_type_t marker);
  void destroyDevice(unsigned key);
  void destroyContext(unsigned key);

private:
  void restoreContextState(ContextState* state);
  bool areMarkersRegistered(std::vector<xell_latency_marker_type_t> markers) const;

private:
  StateTrackingService& stateService_;
  SubcaptureRecorder& recorder_;
  std::map<unsigned, std::unique_ptr<ContextState>> contextStatesByContextKey_;
  std::map<unsigned, ContextState*> contextStatesByDeviceKey_;
};

#pragma endregion

#pragma region XEFG

class XefgStateService {
public:
  struct InitFromSwapChainState {
    xefg_swapchain_d3d12_init_params_t_Argument initParams;
    ID3D12CommandQueue* cmdQueue;
    unsigned cmdQueueKey;
  };

  struct InitFromSwapChainDescState {
    xefg_swapchain_d3d12_init_params_t_Argument initParams;
    HWND hWnd;
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
    std::optional<DXGI_SWAP_CHAIN_FULLSCREEN_DESC> fullscreenDesc;
    ID3D12CommandQueue* cmdQueue;
    unsigned cmdQueueKey;
    unsigned dxgiFactoryKey;
  };

  struct SwapChainPtrState {
    IID riid;
    IDXGISwapChain* swapChain;
    unsigned swapChainKey;
  };

  struct DescriptorHeapState {
    unsigned descriptorHeapKey;
    uint32_t descriptorHeapOffsetInBytes;
  };

  struct DebugFeatureState {
    xefg_swapchain_debug_feature_t featureId;
    uint32_t enable;
    void* argument;
  };

  struct ContextState {
    unsigned key{};
    unsigned deviceKey{};
    ID3D12Device* device{};
    bool enabled{};
    XELLContextArgument xellContext{};
    std::optional<float> threshold;
    std::optional<InitFromSwapChainState> initFromSwapChainParams;
    std::optional<InitFromSwapChainDescState> initFromSwapChainDescParams;
    std::optional<SwapChainPtrState> swapChain;
    std::optional<DescriptorHeapState> descriptorHeap;
    std::optional<DebugFeatureState> debugFeature;
  };

public:
  XefgStateService(StateTrackingService& stateService, SubcaptureRecorder& recorder)
      : stateService_(stateService), recorder_(recorder) {}
  void restoreState();
  void storeContextState(ContextState* state);
  ContextState* getContextState(unsigned key) {
    return contextStatesByContextKey_[key].get();
  }
  void destroyDevice(unsigned key);
  void destroyContext(unsigned key);

private:
  void restoreContextState(ContextState* state);

private:
  StateTrackingService& stateService_;
  SubcaptureRecorder& recorder_;
  std::map<unsigned, std::unique_ptr<ContextState>> contextStatesByContextKey_;
  std::map<unsigned, ContextState*> contextStatesByDeviceKey_;
};

#pragma endregion

} // namespace DirectX
} // namespace gits
