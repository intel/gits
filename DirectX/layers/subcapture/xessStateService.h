// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once
#include "arguments.h"

#include "subcaptureRecorder.h"
#include "commandsAuto.h"

#include <memory>
#include <optional>
#include <map>
#include <unordered_map>
#include <d3d12.h>

namespace gits {
namespace DirectX {

class StateTrackingService;

#pragma region XESS

class XessStateService {
public:
  struct ContextState {
    GITSKey Key{};
    GITSKey DeviceKey{};
    ID3D12Device* Device{};
    std::optional<xess_d3d12_init_params_t_Argument> InitParams;
    std::unique_ptr<float[]> JitterScale;
    std::optional<float> ExposureScale;
    std::unique_ptr<float[]> VelocityScale;
    std::optional<bool> ForceLegacyScaleFactors;
  };

public:
  XessStateService(StateTrackingService& stateService, SubcaptureRecorder& recorder)
      : m_StateService(stateService), m_Recorder(recorder) {}
  void RestoreState();
  void StoreContextState(ContextState* state);
  ContextState* GetContextState(GITSKey key) {
    return m_ContextStatesByContextKey[key].get();
  }
  void DestroyDevice(GITSKey key);
  void DestroyContext(GITSKey key);

private:
  void RestoreContextState(ContextState* state);

private:
  StateTrackingService& m_StateService;
  SubcaptureRecorder& m_Recorder;
  std::map<GITSKey, std::unique_ptr<ContextState>> m_ContextStatesByContextKey;
  std::map<GITSKey, ContextState*> m_ContextStatesByDeviceKey;
};

#pragma endregion

#pragma region XELL

class XellStateService {
public:
  struct ContextState {
    GITSKey Key{};
    GITSKey DeviceKey{};
    ID3D12Device* Device{};
    std::optional<xell_sleep_params_t> SleepParams;
    std::unordered_map<uint32_t, std::vector<xell_latency_marker_type_t>> RegisteredMarkers;
  };

public:
  XellStateService(StateTrackingService& stateService, SubcaptureRecorder& recorder)
      : m_StateService(stateService), m_Recorder(recorder) {}
  void RestoreState();
  void StoreContextState(ContextState* state);
  ContextState* GetContextState(GITSKey key) {
    return m_ContextStatesByContextKey[key].get();
  }
  void TrackMarker(GITSKey key, uint32_t frame, xell_latency_marker_type_t marker);
  void DestroyDevice(GITSKey key);
  void DestroyContext(GITSKey key);

private:
  void RestoreContextState(ContextState* state);
  bool AreMarkersRegistered(const std::vector<xell_latency_marker_type_t>& markers) const;

private:
  StateTrackingService& m_StateService;
  SubcaptureRecorder& m_Recorder;
  std::map<GITSKey, std::unique_ptr<ContextState>> m_ContextStatesByContextKey;
  std::map<GITSKey, ContextState*> m_ContextStatesByDeviceKey;
  bool m_Restored{};
};

#pragma endregion

#pragma region XEFG

class XefgStateService {
public:
  struct InitFromSwapChainState {
    InitFromSwapChainState(const xefg_swapchain_d3d12_init_params_t_Argument& initParams_)
        : InitParams(initParams_) {}
    xefg_swapchain_d3d12_init_params_t_Argument InitParams;
    ID3D12CommandQueue* CmdQueue{};
    GITSKey CmdQueueKey{};
  };

  struct InitFromSwapChainDescState {
    InitFromSwapChainDescState(const xefg_swapchain_d3d12_init_params_t_Argument& initParams_)
        : InitParams(initParams_) {}
    xefg_swapchain_d3d12_init_params_t_Argument InitParams;
    HWND HWnd{};
    DXGI_SWAP_CHAIN_DESC1 SwapChainDesc{};
    std::optional<DXGI_SWAP_CHAIN_FULLSCREEN_DESC> FullscreenDesc;
    ID3D12CommandQueue* CmdQueue{};
    GITSKey CmdQueueKey{};
    GITSKey DxgiFactoryKey{};
  };

  struct SwapChainPtrState {
    IID Riid{};
    IDXGISwapChain* SwapChain{};
    GITSKey SwapChainKey{};
  };

  struct DescriptorHeapState {
    GITSKey DescriptorHeapKey;
    uint32_t DescriptorHeapOffsetInBytes;
  };

  struct DebugFeatureState {
    xefg_swapchain_debug_feature_t FeatureId{};
    uint32_t Enable{};
    void* Argument{};
  };

  struct ExternalHeapOnResizeState {
    GITSKey TempBufferHeapKey{};
    uint64_t TempBufferHeapOffset{};
    GITSKey TempTextureHeapKey{};
    uint64_t TempTextureHeapOffset{};
  };

  struct ContextState {
    GITSKey Key{};
    GITSKey DeviceKey{};
    ID3D12Device* Device{};
    bool Enabled{};
    XELLContextArgument XellContext{};
    std::optional<float> Threshold;
    std::optional<InitFromSwapChainState> InitFromSwapChainParams;
    std::optional<InitFromSwapChainDescState> InitFromSwapChainDescParams;
    std::optional<SwapChainPtrState> SwapChain;
    std::optional<DescriptorHeapState> DescriptorHeap;
    std::optional<DebugFeatureState> DebugFeature;
    std::optional<uint32_t> NumInterpolatedFrames;
    std::optional<xefg_swapchain_ui_composition_state_t> UiCompositionState;
    std::optional<ExternalHeapOnResizeState> ExternalHeapOnResize;
  };

public:
  XefgStateService(StateTrackingService& stateService, SubcaptureRecorder& recorder)
      : m_StateService(stateService), m_Recorder(recorder) {}
  void RestoreState();
  void StoreContextState(ContextState* state);
  ContextState* GetContextState(GITSKey key) {
    return m_ContextStatesByContextKey[key].get();
  }
  void DestroyDevice(GITSKey key);
  void DestroyContext(GITSKey key);

private:
  void RestoreContextState(ContextState* state);

private:
  StateTrackingService& m_StateService;
  SubcaptureRecorder& m_Recorder;
  std::map<GITSKey, std::unique_ptr<ContextState>> m_ContextStatesByContextKey;
  std::map<GITSKey, ContextState*> m_ContextStatesByDeviceKey;
  bool m_Restored{};
};

#pragma endregion

} // namespace DirectX
} // namespace gits
