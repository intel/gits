// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "layerAuto.h"
#include "config.h"
#include "messageBus.h"
#include "services/cpuFrameBenchmarkService.h"

namespace gits {
namespace vulkan {

class BenchmarkLayer : public Layer {
public:
  BenchmarkLayer(const BenchmarkConfig& cfg, gits::MessageBus& msgBus);
  ~BenchmarkLayer() = default;

  void Pre(vkCreateInstanceCommand& command) override;
  void Post(vkQueuePresentKHRCommand& command) override;

private:
  BenchmarkConfig m_Cfg;
  CpuFrameBenchmarkService m_CpuFrameBenchmarkService;
};

} // namespace vulkan
} // namespace gits
