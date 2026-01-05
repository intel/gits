// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
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

CommonEventsHandler::CommonEventsHandler() {}

void CommonEventsHandler::RegisterEvents() {
  auto eventHandler = [](Topic t, const MessagePtr& m) {
    auto msg = std::dynamic_pointer_cast<GitsEventMessage>(m);
    if (!msg) {
      return;
    }

    auto& data = msg->getData();
    switch (data.Id) {
    case CToken::TId::ID_INIT_START:
      stateRestoreBegin();
      break;
    case CToken::TId::ID_INIT_END:
      stateRestoreEnd();
      break;
    case CToken::TId::ID_FRAME_END:
      frameEnd(data.FrameEndData.FrameNumber);
      break;
    case CToken::TId::ID_MARKER_UINT64:
      markerUInt64(data.MarkerUint64Data.Value);
      break;
    default:
      break;
    }
  };

  gits::CGits::Instance().GetMessageBus().subscribe({PUBLISHER_PLAYER, TOPIC_GITS_EVENT},
                                                    eventHandler);
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

void CommonEventsHandler::frameEnd(int frameNumber) {
  FrameEndCommand command(frameNumber);
  auto& manager = PlayerManager::get();
  for (Layer* layer : manager.getPreLayers()) {
    layer->pre(command);
  }
  for (Layer* layer : manager.getPostLayers()) {
    layer->post(command);
  }
}

void CommonEventsHandler::markerUInt64(uint64_t value) {
  MarkerUInt64Command command(value);
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
