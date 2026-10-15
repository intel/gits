// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

// Hand-written recorder wrappers for glMapBufferRange/glUnmapBuffer, bypassing the generated
// per-command wrapper path entirely - see docs/development/opengl2-buffer-map-tracking-plan.md's
// Decision 3 for why (a mapped pointer return value cannot be expressed in
// pointer_metadata.py's vocabulary, and resolving the bound buffer needs an extra driver call no
// other generated wrapper needs). Mirrors unsupportedCommand.h's "small, hand-written, not
// generated" precedent in shape, applied to command *dispatch* instead of command *reporting*.
// The wrappers themselves only dispatch to the real driver and guard against recursion; all
// tracking and capture policy lives in MapTrackingService (mapTrackingService.h).

namespace gits::opengl2 {

// Returns &CustomWrapper_glMapBufferRange/&CustomWrapper_glUnmapBuffer for those two names,
// nullptr for every other name - checked by GetGeneratedRecorderWrapper
// (recorderWrappersAuto.cpp.mako) before its own generated static map, so these two names never
// reach their generated (and, for these two names only, unused - see Decision 3's Consequences)
// Wrapper_glMapBufferRange/Wrapper_glUnmapBuffer counterparts.
void* GetCustomRecorderWrapper(const char* name);

} // namespace gits::opengl2
