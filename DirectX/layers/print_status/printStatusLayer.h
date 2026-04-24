// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"

#include <chrono>

namespace gits {
namespace DirectX {

class PrintStatusLayer : public Layer {
public:
  PrintStatusLayer() : Layer("PrintStatus"){};
  ~PrintStatusLayer() = default;

  void Post(StateRestoreBeginCommand& c) override;
  void Post(StateRestoreEndCommand& c) override;
  void Post(MarkerUInt64Command& c) override;

private:
  std::chrono::steady_clock::time_point m_InitialTime;
};

} // namespace DirectX
} // namespace gits
