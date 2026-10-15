// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "windowGeometryMetaRunner.h"

#include "hdcVirtualizationService.h"
#include "layerAuto.h"
#include "playerManager.h"
#include "threadContextService.h"

namespace gits::opengl2 {

void WindowGeometryMetaRunner::Run() {
  ThreadContextService::Get().SwitchTo(command.m_ThreadId);
  auto& manager = PlayerManager::Get();
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }

  if (manager.ExecuteCommands() && !command.m_Skip) {
    HdcVirtualizationService::Get().SetGeometry(
        command.m_Window.Value, command.m_X.Value, command.m_Y.Value, command.m_Width.Value,
        command.m_Height.Value, command.m_Visible.Value, command.m_ClosedWindows.Data);
  }

  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

} // namespace gits::opengl2
