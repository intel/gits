// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

// Player-local counterpart of recorder/include/realDriver.h, same signatures (the generated
// openglDriversAuto.cpp instantiated for the player - see codegen/generator_player.py - calls
// GetRealProcAddress exactly like the recorder's copy does). Namespaced under ::player,
// matching the player's own instantiation of openglDriversAuto.h/.cpp under
// gits::opengl2::player (see docs/development/opengl2-player-skeleton-plan.md's Decision 3).
//
// Simpler than the recorder's version: the player is a normal, unhooked process, so there is
// no private-handle bypass to worry about (contrast recorder/realDriver.cpp's comment on why
// it LoadLibraryA's its own private handle to opengl32.dll).

namespace gits::opengl2::player {

void InitializeRealDriver();
void FinalizeRealDriver();
void* GetRealProcAddress(const char* name);

} // namespace gits::opengl2::player
