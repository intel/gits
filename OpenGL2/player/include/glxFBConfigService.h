// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "glTypes.h"

#include <cstdint>
#include <map>
#include <mutex>
#include <utility>
#include <vector>

namespace gits::opengl2 {

// Chooses the player's framebuffer configurations for the GLX commands taking one, or a visual.
// The recorder records the attributes of the application's configuration or visual in a
// PixelFormatMetaCommand right before each of them (PixelFormatCaptureLayer), as the recorded
// configuration or visual exists only in the recorded process.
class GlxFBConfigService {
public:
  static GlxFBConfigService& Get();

  // The configuration with the attributes of the PixelFormatMetaCommand replayed last on
  // `threadId`, or null when the player has none with them. Among the matching ones, prefers one
  // whose recorded attributes are all equal and that renders to all the recorded drawable types,
  // so that a context and a pixmap with the same recorded attributes get the same configuration.
  // Only a configuration chosen `forPixmap` needs to support pixmaps.
  GLXFBConfig TakeChosen(uint32_t threadId, bool forPixmap = false);

  // The ID of the visual of `config`; 0 for null or a configuration without one.
  static uint64_t VisualId(GLXFBConfig config);
  // The visual a player window a context is made current on must have, which is that of the
  // context's configuration.
  void SetContextVisual(GLXContext context, uint64_t visualId);
  uint64_t ContextVisual(GLXContext context);

private:
  GlxFBConfigService() = default;

  GLXFBConfig Choose(const std::vector<int32_t>& attributes, bool forPixmap);

  std::mutex m_Mutex;
  std::map<std::pair<std::vector<int32_t>, bool>, GLXFBConfig> m_Chosen;
  std::map<GLXContext, uint64_t> m_ContextVisuals;
};

} // namespace gits::opengl2
