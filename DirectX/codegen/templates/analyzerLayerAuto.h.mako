// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "layerAuto.h"

#include <set>

namespace gits {
namespace DirectX {

class AnalyzerLayer : public Layer {
public:
  AnalyzerLayer();

  void post(IDXGISwapChainPresentCommand& c) override;
  void post(IDXGISwapChain1Present1Command& c) override;
  void post(ID3D12CommandQueueExecuteCommandListsCommand& c) override;
  %for interface in interfaces:
  %for function in interface.functions:
  %if interface.name.startswith('ID3D12GraphicsCommandList') and not function.name.startswith('SetName'):
  void post(${interface.name}${function.name}Command& c) override;
  %endif
  %endfor
  %endfor

  static std::string getAnalysisFileName();

private:
  bool inRange_{};
  std::set<unsigned> commandListsResetBeforeExecution_;
  std::set<unsigned> commandListsExecuted_;
  std::set<unsigned> commandListsForRestore_;
  std::set<unsigned> commandListsReset_;
  unsigned startFrame_;
  unsigned endFrame_;
};

} // namespace DirectX
} // namespace gits
