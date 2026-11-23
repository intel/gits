// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

// Hand-written recorder wrappers for buffer and GL_INTEL_map_texture map/unmap calls. Mapped
// pointer returns cannot be serialized as plain values, and target-addressed buffer maps need a
// bound-buffer query. Wrappers dispatch to their own driver entry points and guard nested capture;
// MapTrackingService and MappedTextureService own the corresponding capture policies.

namespace gits::opengl {

// Returns a custom wrapper for tracked map/unmap entry points, or nullptr otherwise.
// GetGeneratedRecorderWrapper checks this before generated wrappers.
void* GetCustomRecorderWrapper(const char* name);

} // namespace gits::opengl
