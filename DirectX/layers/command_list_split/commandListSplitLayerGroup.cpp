// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "commandListSplitLayerGroup.h"
#include "configurator.h"
#include "commandListSplitLayerAuto.h"

namespace gits {
namespace DirectX {

void CommandListSplitLayerGroup::LoadLayers() {
  if (!Configurator::Get().common.player.subcapture.enabled ||
      Configurator::Get().common.player.subcapture.directx.commandListSplit.empty()) {
    return;
  }

  Configurator::GetMutable().directx.player.execute = false;
  LOG_INFO << "Command list split mode. Execution disabled.";

  m_Recorder = std::make_unique<CommandListSplitRecorder>();
  AddLayer(std::make_unique<CommandListSplitLayer>(*m_Recorder));
}

} // namespace DirectX
} // namespace gits
