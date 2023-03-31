// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
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
struct Config;

namespace Vulkan {
class IRecorderWrapper;

class CGitsPluginVulkan {
  static IRecorderWrapper* _recorderWrapper;
  static std::unique_ptr<CGitsLoader> _loader;
  static boost::mutex _mutex;

public:
  static bool _recorderFinished;
  static void Initialize();
  static IRecorderWrapper& RecorderWrapper() {
    return *_recorderWrapper;
  }
  static void ProcessTerminationDetected();
  static const Config& Configuration();
};
} // namespace Vulkan
} // namespace gits