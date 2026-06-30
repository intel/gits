// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "layer.h"
#include "log.h"

namespace {

uint32_t g_FrameLogCounter = 0;
uint32_t g_QueueSubmitLogCounter = 0;

} // namespace

namespace gits {
namespace vulkan {

HelloPluginLayer::HelloPluginLayer(const HelloPluginConfig& cfg)
    : Layer("HelloPlugin"), m_Cfg(cfg) {}

void HelloPluginLayer::Post(vkQueuePresentKHRCommand& command) {
  (void)command;
  NewFrame();
}

void HelloPluginLayer::Post(vkQueueSubmitCommand& command) {
  (void)command;
  if (!m_Cfg.PrintQueueSubmits) {
    return;
  }
  LOG_INFO << "HelloPlugin - Queue submit: " << ++g_QueueSubmitLogCounter;
}

void HelloPluginLayer::NewFrame() {
  if (!m_Cfg.PrintFrames) {
    return;
  }
  LOG_INFO << "HelloPlugin - Frame: " << ++g_FrameLogCounter;
}

} // namespace vulkan
} // namespace gits
