// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "commonEventsHandler.h"
#include "commandsCustom.h"
#include "playerManager.h"
#include "gits.h"

namespace gits {
namespace DirectX {

CommonEventsHandler::CommonEventsHandler() {
  const_cast<gits::Configuration&>(Configurator::Get()).common.shared.useEvents = true;
  gits::Events events{};
  events.frameBegin = frameBegin;
  events.frameEnd = frameEnd;
  events.loopBegin = loopBegin;
  events.loopEnd = loopEnd;
  events.stateRestoreBegin = stateRestoreBegin;
  events.stateRestoreEnd = stateRestoreEnd;
  events.programExit = programExit;
  events.programStart = programStart;
  events.logging = logging;
  CGits::Instance().RegisterPlaybackEvents(events);
}

void CommonEventsHandler::stateRestoreBegin() {
  StateRestoreBeginCommand command;
  auto& manager = PlayerManager::get();
  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }
  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}

void CommonEventsHandler::stateRestoreEnd() {
  StateRestoreEndCommand command;
  auto& manager = PlayerManager::get();
  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }
  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}

void CommonEventsHandler::frameEnd(int frameNum) {
  FrameEndCommand command(frameNum);
  auto& manager = PlayerManager::get();
  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }
  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}

} // namespace DirectX
} // namespace gits
