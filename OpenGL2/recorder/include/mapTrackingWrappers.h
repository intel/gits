// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

// Hand-written recorder wrappers for buffer map/unmap entry points. Mapped pointer returns
// cannot be serialized as plain values, and target-addressed maps need a bound-buffer query.
// Each wrapper forwards to its own real driver entry point, guarding against nested capture;
// range tracking and snapshot policy live in MapTrackingService.

namespace gits::opengl2 {

// Returns a custom wrapper for tracked map/unmap entry points, or nullptr otherwise.
// GetGeneratedRecorderWrapper checks this before generated wrappers.
void* GetCustomRecorderWrapper(const char* name);

} // namespace gits::opengl2
