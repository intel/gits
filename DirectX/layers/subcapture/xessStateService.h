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
    xell_sleep_params_t* sleepParams{nullptr};
  };

public:
  XellStateService(StateTrackingService& stateService, SubcaptureRecorder& recorder)
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
