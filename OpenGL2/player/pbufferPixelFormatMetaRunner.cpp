// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "pbufferPixelFormatMetaRunner.h"

#include "layerAuto.h"
#include "pbufferService.h"
#include "playerManager.h"
#include "threadContextService.h"

namespace gits::opengl2 {

void PbufferPixelFormatMetaRunner::Run() {
  ThreadContextService::Get().SwitchTo(command.m_ThreadId);
  auto& manager = PlayerManager::Get();
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }

  if (manager.ExecuteCommands() && !command.m_Skip) {
    PbufferService::Get().SetPendingPixelFormat(command.m_ThreadId, command.m_Attributes.Data);
  }

  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

} // namespace gits::opengl2
