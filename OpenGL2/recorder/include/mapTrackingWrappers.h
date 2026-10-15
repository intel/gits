// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

// Hand-written recorder wrappers for glMapBufferRange/glUnmapBuffer (and, as an additive
// extension - see docs/development/opengl2-buffer-map-tracking-plan.md's Decision 1 "Revisit
// when" clause - glMapBuffer/glMapBufferARB/glUnmapBufferARB and the DSA
// glMapNamedBufferEXT/glMapNamedBufferRangeEXT/glUnmapNamedBufferEXT family), bypassing the
// generated per-command wrapper path entirely - see that plan's Decision 3 for why (a mapped
// pointer return value cannot be expressed in pointer_metadata.py's vocabulary, and resolving
// the bound buffer - for the target-addressed functions - needs an extra driver call no other
// generated wrapper needs). Mirrors unsupportedCommand.h's "small, hand-written, not generated"
// precedent in shape, applied to command *dispatch* instead of command *reporting*.
// The wrappers themselves only dispatch to the real driver and guard against recursion; all
// tracking and capture policy lives in MapTrackingService (mapTrackingService.h).

namespace gits::opengl2 {

// Returns the matching CustomWrapper_* for every name this file intercepts (see this header's
// own top-of-file comment for the full list), nullptr for every other name - checked by
// GetGeneratedRecorderWrapper (recorderWrappersAuto.cpp.mako) before its own generated static
// map, so these names never reach their generated (and, for these names only, unused - see
// Decision 3's Consequences) Wrapper_* counterparts.
void* GetCustomRecorderWrapper(const char* name);

} // namespace gits::opengl2
