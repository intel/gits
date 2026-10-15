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

#include <algorithm>
#include <vector>

namespace gits::opengl2 {

void WindowGeometryMetaRunner::Run() {
  ThreadContextService::Get().SwitchTo(command.m_ThreadId);
  auto& manager = PlayerManager::Get();
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }

  if (manager.ExecuteCommands() && !command.m_Skip) {
    const size_t count = (std::min)({command.m_Windows.Data.size(), command.m_X.Data.size(),
                                     command.m_Y.Data.size(), command.m_Width.Data.size(),
                                     command.m_Height.Data.size(), command.m_Visible.Data.size()});
    std::vector<HdcVirtualizationService::WindowSample> samples(count);
    for (size_t i = 0; i < count; ++i) {
      samples[i] = {command.m_Windows.Data[i], command.m_X.Data[i],
                    command.m_Y.Data[i],       command.m_Width.Data[i],
                    command.m_Height.Data[i],  command.m_Visible.Data[i] != 0};
    }
    HdcVirtualizationService::Get().SetGeometry(samples, command.m_CallSamples.Value,
                                                command.m_DestroyedWindows.Data);
  }

  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

} // namespace gits::opengl2
