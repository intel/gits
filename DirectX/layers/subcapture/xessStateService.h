// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "subcaptureRecorder.h"

#include <memory>
#include <optional>
#include <map>
#include <d3d12.h>

namespace gits {
namespace DirectX {

class StateTrackingService;

class XessStateService {
public:
  struct ContextState {
    unsigned key{};
    unsigned deviceKey{};
    ID3D12Device* device{};
    std::unique_ptr<char[]> initParamsEncoded;
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

} // namespace DirectX
} // namespace gits
