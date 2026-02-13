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
#include "commandWritersAuto.h"
#include "commandWritersCustom.h"

namespace gits {
namespace DirectX {

#pragma region XESS

void XessStateService::restoreState() {
  for (auto& it : contextStatesByContextKey_) {
    restoreContextState(it.second.get());
  }
}

void XessStateService::storeContextState(ContextState* state) {
  contextStatesByContextKey_[state->key].reset(state);
  contextStatesByDeviceKey_[state->deviceKey] = state;
}

void XessStateService::destroyDevice(unsigned key) {
  auto it = contextStatesByDeviceKey_.find(key);
  if (it != contextStatesByDeviceKey_.end()) {
    contextStatesByContextKey_.erase(it->second->key);
    contextStatesByDeviceKey_.erase(it);
  }
}

void XessStateService::destroyContext(unsigned key) {
  auto it = contextStatesByContextKey_.find(key);
  if (it != contextStatesByContextKey_.end()) {
    contextStatesByDeviceKey_.erase(it->second->deviceKey);
    contextStatesByContextKey_.erase(it);
  }
}

void XessStateService::restoreContextState(ContextState* state) {
  xessD3D12CreateContextCommand c;
  c.key = stateService_.getUniqueCommandKey();
  c.phContext_.key = state->key;
  c.pDevice_.key = state->deviceKey;
  recorder_.record(new xessD3D12CreateContextWriter(c));

  if (state->initParams) {
    xessD3D12InitCommand c;
    c.key = stateService_.getUniqueCommandKey();
    c.hContext_.key = state->key;
    c.pInitParams_.value = state->initParams.value().value;
    c.pInitParams_.key = state->initParams.value().key;
    c.pInitParams_.tempBufferHeapKey = state->initParams.value().tempBufferHeapKey;
    c.pInitParams_.tempTextureHeapKey = state->initParams.value().tempTextureHeapKey;
    c.pInitParams_.pipelineLibraryKey = state->initParams.value().pipelineLibraryKey;
    recorder_.record(new xessD3D12InitWriter(c));
  }

  if (state->jitterScale) {
    xessSetJitterScaleCommand c;
    c.key = stateService_.getUniqueCommandKey();
    c.hContext_.key = state->key;
    c.x_.value = state->jitterScale[0];
    c.y_.value = state->jitterScale[1];
    recorder_.record(new xessSetJitterScaleWriter(c));
  }

  if (state->exposureScale) {
    xessSetExposureMultiplierCommand c;
    c.key = stateService_.getUniqueCommandKey();
    c.hContext_.key = state->key;
    c.scale_.value = state->exposureScale.value();
  }

  if (state->velocityScale) {
    xessSetVelocityScaleCommand c;
    c.key = stateService_.getUniqueCommandKey();
    c.hContext_.key = state->key;
    c.x_.value = state->velocityScale[0];
    c.y_.value = state->velocityScale[1];
    recorder_.record(new xessSetVelocityScaleWriter(c));
  }

  if (state->forceLegacyScaleFactors) {
    xessForceLegacyScaleFactorsCommand c;
    c.key = stateService_.getUniqueCommandKey();
    c.hContext_.key = state->key;
    c.force_.value = state->forceLegacyScaleFactors.value();
  }
}

#pragma endregion

#pragma region XELL

void XellStateService::restoreState() {
  if (restored_) {
    return;
  }
  for (auto& it : contextStatesByContextKey_) {
    restoreContextState(it.second.get());
  }
  restored_ = true;
}

void XellStateService::storeContextState(ContextState* state) {
  contextStatesByContextKey_[state->key].reset(state);
  contextStatesByDeviceKey_[state->deviceKey] = state;
}

void XellStateService::trackMarker(unsigned key,
                                   uint32_t frame,
                                   xell_latency_marker_type_t marker) {
  auto it = contextStatesByContextKey_.find(key);
  if (it == contextStatesByContextKey_.end()) {
    return;
  }
  auto& state = it->second;
  auto& frameMarkers = state->registeredMarkers[frame];
  frameMarkers.push_back(marker);
}

bool XellStateService::areMarkersRegistered(std::vector<xell_latency_marker_type_t> markers) const {
  std::set<xell_latency_marker_type_t> markerTypes(markers.begin(), markers.end());
  return markerTypes.count(XELL_RENDERSUBMIT_END) && markerTypes.count(XELL_PRESENT_END);
}

void XellStateService::destroyDevice(unsigned key) {
  auto it = contextStatesByDeviceKey_.find(key);
  if (it != contextStatesByDeviceKey_.end()) {
    contextStatesByContextKey_.erase(it->second->key);
    contextStatesByDeviceKey_.erase(it);
  }
}

void XellStateService::destroyContext(unsigned key) {
  auto it = contextStatesByContextKey_.find(key);
  if (it != contextStatesByContextKey_.end()) {
    contextStatesByDeviceKey_.erase(it->second->deviceKey);
    contextStatesByContextKey_.erase(it);
  }
}

void XellStateService::restoreContextState(ContextState* state) {
  xellD3D12CreateContextCommand c;
  c.key = stateService_.getUniqueCommandKey();
  c.out_context_.key = state->key;
  c.device_.key = state->deviceKey;
  recorder_.record(new xellD3D12CreateContextWriter(c));

  if (state->sleepParams) {
    xellSetSleepModeCommand c;
    c.key = stateService_.getUniqueCommandKey();
    c.context_.key = state->key;
    c.param_.value = &state->sleepParams.value();
    recorder_.record(new xellSetSleepModeWriter(c));
  }

  for (const auto& [frame, markers] : state->registeredMarkers) {
    if (areMarkersRegistered(markers)) {
      continue;
    }
    for (const auto& marker : markers) {
      xellAddMarkerDataCommand c;
      c.key = stateService_.getUniqueCommandKey();
      c.context_.key = state->key;
      c.frame_id_.value = frame;
      c.marker_.value = marker;
      recorder_.record(new xellAddMarkerDataWriter(c));
    }
  }
}

#pragma endregion

#pragma region XEFG

void XefgStateService::restoreState() {
  for (auto& it : contextStatesByContextKey_) {
    restoreContextState(it.second.get());
  }
}

void XefgStateService::storeContextState(ContextState* state) {
  contextStatesByContextKey_[state->key].reset(state);
  contextStatesByDeviceKey_[state->deviceKey] = state;
}

void XefgStateService::destroyDevice(unsigned key) {
  auto it = contextStatesByDeviceKey_.find(key);
  if (it != contextStatesByDeviceKey_.end()) {
    contextStatesByContextKey_.erase(it->second->key);
    contextStatesByDeviceKey_.erase(it);
  }
}

void XefgStateService::destroyContext(unsigned key) {
  auto it = contextStatesByContextKey_.find(key);
  if (it != contextStatesByContextKey_.end()) {
    contextStatesByDeviceKey_.erase(it->second->deviceKey);
    contextStatesByContextKey_.erase(it);
  }
}

void XefgStateService::restoreContextState(ContextState* state) {
  xefgSwapChainD3D12CreateContextCommand c;
  c.key = stateService_.getUniqueCommandKey();
  c.phSwapChain_.key = state->key;
  c.pDevice_.key = state->deviceKey;
  recorder_.record(new xefgSwapChainD3D12CreateContextWriter(c));

  xefgSwapChainSetLatencyReductionCommand cLatency;
  cLatency.key = stateService_.getUniqueCommandKey();
  cLatency.hSwapChain_.key = state->key;
  cLatency.hXeLLContext_.key = state->xellContext.key;
  recorder_.record(new xefgSwapChainSetLatencyReductionWriter(cLatency));

  if (state->initFromSwapChainParams) {
    auto& initParams = state->initFromSwapChainParams.value();
    xefgSwapChainD3D12InitFromSwapChainCommand c;
    c.key = stateService_.getUniqueCommandKey();
    c.hSwapChain_.key = state->key;
    c.pCmdQueue_.key = initParams.cmdQueueKey;
    c.pInitParams_.value = initParams.initParams.value;
    c.pInitParams_.key = initParams.initParams.key;
    c.pInitParams_.applicationSwapChainKey = initParams.initParams.applicationSwapChainKey;
    c.pInitParams_.tempBufferHeapKey = initParams.initParams.tempBufferHeapKey;
    c.pInitParams_.tempTextureHeapKey = initParams.initParams.tempTextureHeapKey;
    c.pInitParams_.pipelineLibraryKey = initParams.initParams.pipelineLibraryKey;
    recorder_.record(new xefgSwapChainD3D12InitFromSwapChainWriter(c));
  } else if (state->initFromSwapChainDescParams) {
    auto& initParams = state->initFromSwapChainDescParams.value();

    CreateWindowMetaCommand createWindowCommand;
    createWindowCommand.key = stateService_.getUniqueCommandKey();
    createWindowCommand.hWnd_.value = initParams.hWnd;
    createWindowCommand.width_.value = initParams.swapChainDesc.Width;
    createWindowCommand.height_.value = initParams.swapChainDesc.Height;
    recorder_.record(new CreateWindowMetaWriter(createWindowCommand));

    xefgSwapChainD3D12InitFromSwapChainDescCommand c;
    c.key = stateService_.getUniqueCommandKey();
    c.hSwapChain_.key = state->key;
    c.hWnd_.value = initParams.hWnd;
    c.pSwapChainDesc_.value = &initParams.swapChainDesc;
    c.pFullscreenDesc_.value =
        initParams.fullscreenDesc.has_value() ? &initParams.fullscreenDesc.value() : nullptr;
    c.pCmdQueue_.key = initParams.cmdQueueKey;
    c.pDxgiFactory_.key = initParams.dxgiFactoryKey;
    c.pInitParams_.value = initParams.initParams.value;
    c.pInitParams_.key = initParams.initParams.key;
    c.pInitParams_.applicationSwapChainKey = initParams.initParams.applicationSwapChainKey;
    c.pInitParams_.tempBufferHeapKey = initParams.initParams.tempBufferHeapKey;
    c.pInitParams_.tempTextureHeapKey = initParams.initParams.tempTextureHeapKey;
    c.pInitParams_.pipelineLibraryKey = initParams.initParams.pipelineLibraryKey;
    recorder_.record(new xefgSwapChainD3D12InitFromSwapChainDescWriter(c));
  }

  if (state->swapChain) {
    xefgSwapChainD3D12GetSwapChainPtrCommand c;
    c.key = stateService_.getUniqueCommandKey();
    c.hSwapChain_.key = state->key;
    c.riid_.value = state->swapChain.value().riid;
    c.ppSwapChain_.key = state->swapChain.value().swapChainKey;
    recorder_.record(new xefgSwapChainD3D12GetSwapChainPtrWriter(c));
  }

  if (state->descriptorHeap) {
    xefgSwapChainD3D12SetDescriptorHeapCommand c;
    c.key = stateService_.getUniqueCommandKey();
    c.hSwapChain_.key = state->key;
    c.pDescriptorHeap_.key = state->descriptorHeap.value().descriptorHeapKey;
    c.descriptorHeapOffsetInBytes_.value =
        state->descriptorHeap.value().descriptorHeapOffsetInBytes;
    recorder_.record(new xefgSwapChainD3D12SetDescriptorHeapWriter(c));
  }

  if (state->enabled) {
    xefgSwapChainSetEnabledCommand c;
    c.key = stateService_.getUniqueCommandKey();
    c.hSwapChain_.key = state->key;
    c.enable_.value = 1;
    recorder_.record(new xefgSwapChainSetEnabledWriter(c));
  }

  if (state->threshold) {
    xefgSwapChainSetSceneChangeThresholdCommand c;
    c.key = stateService_.getUniqueCommandKey();
    c.hSwapChain_.key = state->key;
    c.threshold_.value = state->threshold.value();
    recorder_.record(new xefgSwapChainSetSceneChangeThresholdWriter(c));
  }

  if (state->debugFeature) {
    xefgSwapChainEnableDebugFeatureCommand c;
    c.key = stateService_.getUniqueCommandKey();
    c.hSwapChain_.key = state->key;
    c.featureId_.value = state->debugFeature.value().featureId;
    c.enable_.value = state->debugFeature.value().enable;
    c.pArgument_.value = state->debugFeature.value().argument;
    recorder_.record(new xefgSwapChainEnableDebugFeatureWriter(c));
  }
}

#pragma endregion

} // namespace DirectX
} // namespace gits
