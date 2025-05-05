// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "enumsAuto.h"
#include "customTypes.h"
#include "configurator.h"

namespace gits {
typedef Configuration Config;

using TRecordingMode = RecordingMode;
using TOpenGLRecorderMode = OpenGLRecorderMode;
using TVulkanRecorderMode = VulkanRecorderMode;
using TOpenCLRecorderMode = OpenCLRecorderMode;
using TLevelZeroRecorderMode = LevelZeroRecorderMode;

using TForcedGLProfile = GLProfile;
using TForcedGLNativeApi = GLNativeApi;
using TBuffersState = BuffersState;
using TTexturesState = TexturesState;
using TMemoryUpdateStates = MemoryUpdateState;
using TMemoryTrackingMode = MemoryTrackingMode;
using TMemoryStateRestoration = MemoryStateRestoration;
using TBufferStateRestoration = BufferStateRestoration;
using TWindowsKeyHandling = WindowsKeyHandling;
using TCaptureGroupType = CaptureGroupType;
using TDeviceType = DeviceType;
using TVkRenderDocCaptureMode = VkRenderDocCaptureMode;
using THashType = HashType;
using VulkanObjectMode = VulkanObjectMode;
using WindowMode = WindowMode;

} // namespace gits
