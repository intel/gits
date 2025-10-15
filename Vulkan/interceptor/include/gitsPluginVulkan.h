// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   gitsPluginVulkan.h
*
* @brief
*/

#pragma once

#include "gitsLoader.h"

namespace gits {

class CRecorder;

namespace Vulkan {
class IRecorderWrapper;

class CGitsPluginVulkan {
  static IRecorderWrapper* _recorderWrapper;
  static std::unique_ptr<CGitsLoader> _loader;
  static std::mutex _mutex;

public:
  static std::atomic<bool> _recorderFinished;
  static void Initialize();
  static IRecorderWrapper& RecorderWrapper() {
    std::unique_lock<std::mutex> lock(_mutex);
    return *_recorderWrapper;
  }
  static void ProcessTerminationDetected();
  static Config& Configuration();
};
} // namespace Vulkan
} // namespace gits
