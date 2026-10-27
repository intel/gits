// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "glxFBConfigService.h"

#include "glEnumsAuto.h"
#include "glxDrawableService.h"
#include "hdcVirtualizationService.h"
#include "log.h"
#include "openglDriversAuto.h"

#include <atomic>

namespace gits::opengl2 {

GlxFBConfigService& GlxFBConfigService::Get() {
  static GlxFBConfigService instance;
  return instance;
}

GLXFBConfig GlxFBConfigService::TakeChosen(uint32_t threadId) {
  return Choose(HdcVirtualizationService::Get().TakePendingPixelFormat(threadId));
}

GLXFBConfig GlxFBConfigService::Choose(const std::vector<int32_t>& attributes) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  if (const auto it = m_Chosen.find(attributes); it != m_Chosen.end()) {
    return it->second;
  }
  auto& drawables = GlxDrawableService::Get();
  Display* display = drawables.GetDisplay();
  std::vector<int> requested;
  for (size_t i = 0; i + 1 < attributes.size(); i += 2) {
    int value = attributes[i + 1];
    // The player replays no GLX pixmaps, so a configuration needs no pixmap support.
    if (attributes[i] == GLX_DRAWABLE_TYPE) {
      value &= GLX_WINDOW_BIT | GLX_PBUFFER_BIT;
    }
    requested.push_back(attributes[i]);
    requested.push_back(value);
  }
  requested.push_back(0);

  auto choose = player::g_DispatchTable.glXChooseFBConfig.load(std::memory_order_acquire);
  auto getAttribute = player::g_DispatchTable.glXGetFBConfigAttrib.load(std::memory_order_acquire);
  int count = 0;
  GLXFBConfig* configs = choose(display, drawables.GetDefaultScreen(), requested.data(), &count);
  GLXFBConfig chosen = configs != nullptr && count > 0 ? configs[0] : nullptr;
  for (int i = 0; configs != nullptr && i < count; ++i) {
    bool equal = true;
    for (size_t j = 0; equal && j + 1 < attributes.size(); j += 2) {
      int value = 0;
      equal = attributes[j] == GLX_DRAWABLE_TYPE ||
              (getAttribute(display, configs[i], attributes[j], &value) == 0 &&
               value == attributes[j + 1]);
    }
    if (equal) {
      chosen = configs[i];
      break;
    }
  }
  GlxDrawableService::Free(configs);
  if (chosen == nullptr) {
    static std::atomic<bool> warned{false};
    if (!warned.exchange(true)) {
      LOG_ERROR << "GLX replay: no framebuffer configuration has the recorded attributes";
    }
  }
  m_Chosen.emplace(attributes, chosen);
  return chosen;
}

uint64_t GlxFBConfigService::VisualId(GLXFBConfig config) {
  if (config == nullptr) {
    return 0;
  }
  auto getAttribute = player::g_DispatchTable.glXGetFBConfigAttrib.load(std::memory_order_acquire);
  int visual = 0;
  if (getAttribute(GlxDrawableService::Get().GetDisplay(), config, GLX_VISUAL_ID, &visual) != 0) {
    return 0;
  }
  return static_cast<uint64_t>(static_cast<unsigned>(visual));
}

void GlxFBConfigService::SetContextVisual(GLXContext context, uint64_t visualId) {
  if (context == nullptr) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_ContextVisuals[context] = visualId;
}

uint64_t GlxFBConfigService::ContextVisual(GLXContext context) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  const auto it = m_ContextVisuals.find(context);
  return it != m_ContextVisuals.end() ? it->second : 0;
}

} // namespace gits::opengl2
