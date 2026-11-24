// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

namespace gits::opengl {

void* GetGeneratedRecorderWrapper(const char* name);
// Common.Recorder.Enabled. While it is false, wrappers run only the plugin layers.
bool RecordingEnabled();
// While recording is disabled, whether any plugin layer is loaded. Loads the plugins on first use.
bool PluginLayersLoaded();

} // namespace gits::opengl
