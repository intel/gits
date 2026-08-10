// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "layerAuto.h"
#include "commandsAuto.h"
#include "commandsCustom.h"
#include "analyzerService.h"
#include "analyzerRaytracingService.h"

namespace gits {
namespace vulkan {

// Analysis-pass layer: for every Vulkan command, marks each handle argument as
// "used" in the AnalyzerService when the command executes inside the subcapture
// frame range.  The dependency closure of those handles is dumped to the
// analysis file at range end and consumed by the recording pass to restore only
// the necessary objects.  Mirrors the DirectX AnalyzerLayer.
class AnalyzerLayer : public Layer {
public:
  AnalyzerLayer(AnalyzerService& analyzerService, AnalyzerRaytracingService& raytracingService)
      : Layer("Analyzer"), m_AnalyzerService(analyzerService),
        m_RaytracingService(raytracingService) {}
  ~AnalyzerLayer();

  AnalyzerLayer(const AnalyzerLayer&) = delete;
  AnalyzerLayer& operator=(const AnalyzerLayer&) = delete;

  // Custom-handled commands (implemented in analyzerLayerCustom.cpp).
  void Post(vkCmdBuildAccelerationStructuresKHRCommand& command) override;
  void Post(vkCmdTraceRaysKHRCommand& command) override;
  void Post(vkCmdTraceRaysIndirectKHRCommand& command) override;

  // Auto-generated Post overrides for every other Vulkan command.
  % for command in commands:
  <% define = get_define(command.platform) %>\
  % if define:
#ifdef ${define}
  % endif
  % if command.name not in analyzer_layer_custom_commands:
  void Post(${command.name}Command& command) override;
  % endif
  % if define:
#endif
  % endif
  % endfor

private:
  AnalyzerService& m_AnalyzerService;
  AnalyzerRaytracingService& m_RaytracingService;
};

} // namespace vulkan
} // namespace gits
