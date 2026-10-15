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
// No Pre/Post layers (the *recorder* side doesn't call its own layers yet either - see
// docs/development/opengl2-player-skeleton-plan.md's Non-goals), no null-run support -
// ExecuteCommands() is unconditionally true for this skeleton.

namespace gits::opengl2 {

class PlayerManager {
public:
  static PlayerManager& Get() {
    static PlayerManager instance;
    return instance;
  }

  bool ExecuteCommands() const {
    return true;
  }

private:
  PlayerManager() = default;
};

} // namespace gits::opengl2
