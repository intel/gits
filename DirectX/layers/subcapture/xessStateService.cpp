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

namespace gits {
namespace DirectX {

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

} // namespace DirectX
} // namespace gits
