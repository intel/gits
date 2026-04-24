// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "xessStateService.h"
#include "stateTrackingService.h"
#include "commandsAuto.h"
#include "commandSerializersAuto.h"
#include "commandSerializersCustom.h"

namespace gits {
namespace DirectX {

#pragma region XESS

void XessStateService::RestoreState() {
  for (auto& it : m_ContextStatesByContextKey) {
    RestoreContextState(it.second.get());
  }
}

void XessStateService::StoreContextState(ContextState* state) {
  m_ContextStatesByContextKey[state->Key].reset(state);
  m_ContextStatesByDeviceKey[state->DeviceKey] = state;
}

void XessStateService::DestroyDevice(unsigned key) {
  auto it = m_ContextStatesByDeviceKey.find(key);
  if (it != m_ContextStatesByDeviceKey.end()) {
    m_ContextStatesByContextKey.erase(it->second->Key);
    m_ContextStatesByDeviceKey.erase(it);
  }
}

void XessStateService::DestroyContext(unsigned key) {
  auto it = m_ContextStatesByContextKey.find(key);
  if (it != m_ContextStatesByContextKey.end()) {
    m_ContextStatesByDeviceKey.erase(it->second->DeviceKey);
    m_ContextStatesByContextKey.erase(it);
  }
}

void XessStateService::RestoreContextState(ContextState* state) {
  xessD3D12CreateContextCommand c;
  c.Key = m_StateService.GetUniqueCommandKey();
  c.m_phContext.Key = state->Key;
  c.m_pDevice.Key = state->DeviceKey;
  m_Recorder.Record(xessD3D12CreateContextSerializer(c));

  if (state->InitParams) {
    xessD3D12InitCommand c;
    c.Key = m_StateService.GetUniqueCommandKey();
    c.m_hContext.Key = state->Key;
    c.m_pInitParams.Value = state->InitParams.value().Value;
    c.m_pInitParams.Key = state->InitParams.value().Key;
    c.m_pInitParams.TempBufferHeapKey = state->InitParams.value().TempBufferHeapKey;
    c.m_pInitParams.TempTextureHeapKey = state->InitParams.value().TempTextureHeapKey;
    c.m_pInitParams.PipelineLibraryKey = state->InitParams.value().PipelineLibraryKey;
    m_Recorder.Record(xessD3D12InitSerializer(c));
  }

  if (state->JitterScale) {
    xessSetJitterScaleCommand c;
    c.Key = m_StateService.GetUniqueCommandKey();
    c.m_hContext.Key = state->Key;
    c.m_x.Value = state->JitterScale[0];
    c.m_y.Value = state->JitterScale[1];
    m_Recorder.Record(xessSetJitterScaleSerializer(c));
  }

  if (state->ExposureScale) {
    xessSetExposureMultiplierCommand c;
    c.Key = m_StateService.GetUniqueCommandKey();
    c.m_hContext.Key = state->Key;
    c.m_scale.Value = state->ExposureScale.value();
    m_Recorder.Record(xessSetExposureMultiplierSerializer(c));
  }

  if (state->VelocityScale) {
    xessSetVelocityScaleCommand c;
    c.Key = m_StateService.GetUniqueCommandKey();
    c.m_hContext.Key = state->Key;
    c.m_x.Value = state->VelocityScale[0];
    c.m_y.Value = state->VelocityScale[1];
    m_Recorder.Record(xessSetVelocityScaleSerializer(c));
  }

  if (state->ForceLegacyScaleFactors) {
    xessForceLegacyScaleFactorsCommand c;
    c.Key = m_StateService.GetUniqueCommandKey();
    c.m_hContext.Key = state->Key;
    c.m_force.Value = state->ForceLegacyScaleFactors.value();
    m_Recorder.Record(xessForceLegacyScaleFactorsSerializer(c));
  }
}

#pragma endregion

#pragma region XELL

void XellStateService::RestoreState() {
  if (m_Restored) {
    return;
  }
  for (auto& it : m_ContextStatesByContextKey) {
    RestoreContextState(it.second.get());
  }
  m_Restored = true;
}

void XellStateService::StoreContextState(ContextState* state) {
  m_ContextStatesByContextKey[state->Key].reset(state);
  m_ContextStatesByDeviceKey[state->DeviceKey] = state;
}

void XellStateService::TrackMarker(unsigned key,
                                   uint32_t frame,
                                   xell_latency_marker_type_t marker) {
  auto it = m_ContextStatesByContextKey.find(key);
  if (it == m_ContextStatesByContextKey.end()) {
    return;
  }
  auto& state = it->second;
  auto& frameMarkers = state->RegisteredMarkers[frame];
  frameMarkers.push_back(marker);
}

bool XellStateService::AreMarkersRegistered(std::vector<xell_latency_marker_type_t> markers) const {
  std::set<xell_latency_marker_type_t> markerTypes(markers.begin(), markers.end());
  return markerTypes.count(XELL_RENDERSUBMIT_END) && markerTypes.count(XELL_PRESENT_END);
}

void XellStateService::DestroyDevice(unsigned key) {
  auto it = m_ContextStatesByDeviceKey.find(key);
  if (it != m_ContextStatesByDeviceKey.end()) {
    m_ContextStatesByContextKey.erase(it->second->Key);
    m_ContextStatesByDeviceKey.erase(it);
  }
}

void XellStateService::DestroyContext(unsigned key) {
  auto it = m_ContextStatesByContextKey.find(key);
  if (it != m_ContextStatesByContextKey.end()) {
    m_ContextStatesByDeviceKey.erase(it->second->DeviceKey);
    m_ContextStatesByContextKey.erase(it);
  }
}

void XellStateService::RestoreContextState(ContextState* state) {
  xellD3D12CreateContextCommand c;
  c.Key = m_StateService.GetUniqueCommandKey();
  c.m_out_context.Key = state->Key;
  c.m_device.Key = state->DeviceKey;
  m_Recorder.Record(xellD3D12CreateContextSerializer(c));

  if (state->SleepParams) {
    xellSetSleepModeCommand c;
    c.Key = m_StateService.GetUniqueCommandKey();
    c.m_context.Key = state->Key;
    c.m_param.Value = &state->SleepParams.value();
    m_Recorder.Record(xellSetSleepModeSerializer(c));
  }

  for (const auto& [frame, markers] : state->RegisteredMarkers) {
    if (AreMarkersRegistered(markers)) {
      continue;
    }
    for (const auto& marker : markers) {
      xellAddMarkerDataCommand c;
      c.Key = m_StateService.GetUniqueCommandKey();
      c.m_context.Key = state->Key;
      c.m_frame_id.Value = frame;
      c.m_marker.Value = marker;
      m_Recorder.Record(xellAddMarkerDataSerializer(c));
    }
  }
}

#pragma endregion

#pragma region XEFG

void XefgStateService::RestoreState() {
  if (m_Restored) {
    return;
  }
  for (auto& it : m_ContextStatesByContextKey) {
    RestoreContextState(it.second.get());
  }
  m_Restored = true;
}

void XefgStateService::StoreContextState(ContextState* state) {
  m_ContextStatesByContextKey[state->Key].reset(state);
  m_ContextStatesByDeviceKey[state->DeviceKey] = state;
}

void XefgStateService::DestroyDevice(unsigned key) {
  auto it = m_ContextStatesByDeviceKey.find(key);
  if (it != m_ContextStatesByDeviceKey.end()) {
    m_ContextStatesByContextKey.erase(it->second->Key);
    m_ContextStatesByDeviceKey.erase(it);
  }
}

void XefgStateService::DestroyContext(unsigned key) {
  auto it = m_ContextStatesByContextKey.find(key);
  if (it != m_ContextStatesByContextKey.end()) {
    m_ContextStatesByDeviceKey.erase(it->second->DeviceKey);
    m_ContextStatesByContextKey.erase(it);
  }
}

void XefgStateService::RestoreContextState(ContextState* state) {
  xefgSwapChainD3D12CreateContextCommand c;
  c.Key = m_StateService.GetUniqueCommandKey();
  c.m_phSwapChain.Key = state->Key;
  c.m_pDevice.Key = state->DeviceKey;
  m_Recorder.Record(xefgSwapChainD3D12CreateContextSerializer(c));

  xefgSwapChainSetLatencyReductionCommand cLatency;
  cLatency.Key = m_StateService.GetUniqueCommandKey();
  cLatency.m_hSwapChain.Key = state->Key;
  cLatency.m_hXeLLContext.Key = state->XellContext.Key;
  m_Recorder.Record(xefgSwapChainSetLatencyReductionSerializer(cLatency));

  if (state->InitFromSwapChainParams) {
    auto& initParams = state->InitFromSwapChainParams.value();
    xefgSwapChainD3D12InitFromSwapChainCommand c;
    c.Key = m_StateService.GetUniqueCommandKey();
    c.m_hSwapChain.Key = state->Key;
    c.m_pCmdQueue.Key = initParams.CmdQueueKey;
    c.m_pInitParams.Value = initParams.InitParams.Value;
    c.m_pInitParams.Key = initParams.InitParams.Key;
    c.m_pInitParams.ApplicationSwapChainKey = initParams.InitParams.ApplicationSwapChainKey;
    c.m_pInitParams.TempBufferHeapKey = initParams.InitParams.TempBufferHeapKey;
    c.m_pInitParams.TempTextureHeapKey = initParams.InitParams.TempTextureHeapKey;
    c.m_pInitParams.PipelineLibraryKey = initParams.InitParams.PipelineLibraryKey;
    m_Recorder.Record(xefgSwapChainD3D12InitFromSwapChainSerializer(c));
  } else if (state->InitFromSwapChainDescParams) {
    auto& initParams = state->InitFromSwapChainDescParams.value();

    CreateWindowMetaCommand createWindowCommand;
    createWindowCommand.Key = m_StateService.GetUniqueCommandKey();
    createWindowCommand.m_hWnd.Value = initParams.HWnd;
    createWindowCommand.m_width.Value = initParams.SwapChainDesc.Width;
    createWindowCommand.m_height.Value = initParams.SwapChainDesc.Height;
    m_Recorder.Record(CreateWindowMetaSerializer(createWindowCommand));

    xefgSwapChainD3D12InitFromSwapChainDescCommand c;
    c.Key = m_StateService.GetUniqueCommandKey();
    c.m_hSwapChain.Key = state->Key;
    c.m_hWnd.Value = initParams.HWnd;
    c.m_pSwapChainDesc.Value = &initParams.SwapChainDesc;
    c.m_pFullscreenDesc.Value =
        initParams.FullscreenDesc.has_value() ? &initParams.FullscreenDesc.value() : nullptr;
    c.m_pCmdQueue.Key = initParams.CmdQueueKey;
    c.m_pDxgiFactory.Key = initParams.DxgiFactoryKey;
    c.m_pInitParams.Value = initParams.InitParams.Value;
    c.m_pInitParams.Key = initParams.InitParams.Key;
    c.m_pInitParams.ApplicationSwapChainKey = initParams.InitParams.ApplicationSwapChainKey;
    c.m_pInitParams.TempBufferHeapKey = initParams.InitParams.TempBufferHeapKey;
    c.m_pInitParams.TempTextureHeapKey = initParams.InitParams.TempTextureHeapKey;
    c.m_pInitParams.PipelineLibraryKey = initParams.InitParams.PipelineLibraryKey;
    m_Recorder.Record(xefgSwapChainD3D12InitFromSwapChainDescSerializer(c));
  }

  if (state->SwapChain) {
    xefgSwapChainD3D12GetSwapChainPtrCommand c;
    c.Key = m_StateService.GetUniqueCommandKey();
    c.m_hSwapChain.Key = state->Key;
    c.m_riid.Value = state->SwapChain.value().Riid;
    c.m_ppSwapChain.Key = state->SwapChain.value().SwapChainKey;
    m_Recorder.Record(xefgSwapChainD3D12GetSwapChainPtrSerializer(c));
  }

  if (state->DescriptorHeap) {
    xefgSwapChainD3D12SetDescriptorHeapCommand c;
    c.Key = m_StateService.GetUniqueCommandKey();
    c.m_hSwapChain.Key = state->Key;
    c.m_pDescriptorHeap.Key = state->DescriptorHeap.value().DescriptorHeapKey;
    c.m_descriptorHeapOffsetInBytes.Value =
        state->DescriptorHeap.value().DescriptorHeapOffsetInBytes;
    m_Recorder.Record(xefgSwapChainD3D12SetDescriptorHeapSerializer(c));
  }

  if (state->Enabled) {
    xefgSwapChainSetEnabledCommand c;
    c.Key = m_StateService.GetUniqueCommandKey();
    c.m_hSwapChain.Key = state->Key;
    c.m_enable.Value = 1;
    m_Recorder.Record(xefgSwapChainSetEnabledSerializer(c));
  }

  if (state->Threshold) {
    xefgSwapChainSetSceneChangeThresholdCommand c;
    c.Key = m_StateService.GetUniqueCommandKey();
    c.m_hSwapChain.Key = state->Key;
    c.m_threshold.Value = state->Threshold.value();
    m_Recorder.Record(xefgSwapChainSetSceneChangeThresholdSerializer(c));
  }

  if (state->DebugFeature) {
    xefgSwapChainEnableDebugFeatureCommand c;
    c.Key = m_StateService.GetUniqueCommandKey();
    c.m_hSwapChain.Key = state->Key;
    c.m_featureId.Value = state->DebugFeature.value().FeatureId;
    c.m_enable.Value = state->DebugFeature.value().Enable;
    c.m_pArgument.Value = state->DebugFeature.value().Argument;
    m_Recorder.Record(xefgSwapChainEnableDebugFeatureSerializer(c));
  }
}

#pragma endregion

} // namespace DirectX
} // namespace gits
