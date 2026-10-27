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

GLXFBConfig GlxFBConfigService::TakeChosen(uint32_t threadId, bool forPixmap) {
  return Choose(HdcVirtualizationService::Get().TakePendingPixelFormat(threadId), forPixmap);
}

GLXFBConfig GlxFBConfigService::Choose(const std::vector<int32_t>& attributes, bool forPixmap) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto key = std::make_pair(attributes, forPixmap);
  if (const auto it = m_Chosen.find(key); it != m_Chosen.end()) {
    return it->second;
  }
  auto& drawables = GlxDrawableService::Get();
  Display* display = drawables.GetDisplay();
  int recordedDrawableType = 0;
  std::vector<int> requested;
  for (size_t i = 0; i + 1 < attributes.size(); i += 2) {
    int value = attributes[i + 1];
    // Requiring pixmap support would rule out configurations for the windows and pbuffers the
    // configuration may also be used with, e.g. double-buffered ones, so it is only preferred.
    if (attributes[i] == GLX_DRAWABLE_TYPE) {
      recordedDrawableType = value & (GLX_WINDOW_BIT | GLX_PBUFFER_BIT | GLX_PIXMAP_BIT);
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
  // Contexts and pixmaps with the same recorded attributes are chosen from the same list by the
  // same preference, so that they get the same configuration whenever one supports everything the
  // recorded one did: making a context current on a pixmap of another configuration, or of
  // another visual, may fail with BadMatch.
  GLXFBConfig firstEqualSupportingRecorded = nullptr;
  GLXFBConfig firstEqual = nullptr;
  GLXFBConfig firstEqualPixmap = nullptr;
  GLXFBConfig firstPixmap = nullptr;
  for (int i = 0; configs != nullptr && i < count; ++i) {
    bool equal = true;
    for (size_t j = 0; equal && j + 1 < attributes.size(); j += 2) {
      int value = 0;
      equal = attributes[j] == GLX_DRAWABLE_TYPE ||
              (getAttribute(display, configs[i], attributes[j], &value) == 0 &&
               value == attributes[j + 1]);
    }
    int drawableType = 0;
    if (getAttribute(display, configs[i], GLX_DRAWABLE_TYPE, &drawableType) != 0) {
      drawableType = 0;
    }
    const bool pixmap = (drawableType & GLX_PIXMAP_BIT) != 0;
    if (equal && (drawableType & recordedDrawableType) == recordedDrawableType &&
        firstEqualSupportingRecorded == nullptr) {
      firstEqualSupportingRecorded = configs[i];
    }
    if (equal && firstEqual == nullptr) {
      firstEqual = configs[i];
    }
    if (equal && pixmap && firstEqualPixmap == nullptr) {
      firstEqualPixmap = configs[i];
    }
    if (pixmap && firstPixmap == nullptr) {
      firstPixmap = configs[i];
    }
  }
  GLXFBConfig forDrawables = firstEqualSupportingRecorded != nullptr ? firstEqualSupportingRecorded
                             : firstEqual != nullptr                 ? firstEqual
                             : configs != nullptr && count > 0       ? configs[0]
                                                                     : nullptr;
  GLXFBConfig chosen = forDrawables;
  if (forPixmap) {
    chosen = firstEqualPixmap != nullptr ? firstEqualPixmap : firstPixmap;
  }
  GlxDrawableService::Free(configs);
  if (chosen == nullptr) {
    static std::atomic<bool> warned{false};
    if (!warned.exchange(true)) {
      LOG_ERROR << "GLX replay: no framebuffer configuration has the recorded attributes";
    }
  } else if (chosen != forDrawables) {
    static std::atomic<bool> warned{false};
    if (!warned.exchange(true)) {
      LOG_ERROR << "GLX replay: no framebuffer configuration with the recorded attributes renders "
                   "to both pixmaps and the other recorded drawable types - making a context "
                   "current on a GLX pixmap may fail; replay on a system whose driver has one";
    }
  }
  m_Chosen.emplace(std::move(key), chosen);
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
