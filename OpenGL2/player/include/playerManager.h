// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

// Deliberately thin, unlike Vulkan's PlayerManager (Vulkan/player/playerManager.h): OpenGL2
// has exactly one dispatch table (WGL/GL are process-global, not object-scoped APIs like
// Vulkan's per-device functions), so there is nothing for this class to select between yet.
// No null-run support - ExecuteCommands() is unconditionally true for this skeleton.
//
// GetPreLayers()/GetPostLayers() are wired into commandRunnersAuto.cpp.mako's generated Run()
// (see docs/development/opengl2-layer-architecture-alignment-plan.md), matching CaptureManager's
// own vectors on the recorder side, but both start - and currently remain - permanently empty:
// no concrete Layer subclass registers into either one yet on the player side (the player has
// no encoding step, so it has no EncoderLayer-equivalent to register; see encoderLayerAuto.h's
// own header comment). A real PlayerLayerManager with its own LoadLayers() is out of scope for
// now (see the plan's "Non-goals").
//
// Layer is only forward-declared, not #include "layerAuto.h" - this header only ever stores/
// returns Layer* (never dereferences one), so a forward declaration is sufficient, mirroring
// CaptureManager's own header comment on why it does the same (see
// recorder/include/captureManager.h) - layerAuto.h pulls in commandsAuto.h and therefore
// glTypes.h, which must not coexist with a real <windows.h> include in the same translation
// unit (see glTypes.h's own header comment), and player window-management code does include
// real <windows.h>.

#include <vector>

namespace gits::opengl2 {

class Layer;

class PlayerManager {
public:
  static PlayerManager& Get() {
    static PlayerManager instance;
    return instance;
  }

  bool ExecuteCommands() const {
    return true;
  }

  std::vector<Layer*>& GetPreLayers() {
    return m_PreLayers;
  }
  std::vector<Layer*>& GetPostLayers() {
    return m_PostLayers;
  }

private:
  PlayerManager() = default;

  std::vector<Layer*> m_PreLayers;
  std::vector<Layer*> m_PostLayers;
};

} // namespace gits::opengl2
