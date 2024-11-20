// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   gitsPluginL0.h
*
* @brief
*/

#pragma once

#include "gitsLoader.h"
#include "platform.h"
#include "tools.h"

#include <cstdint>
#include <string>

namespace gits {

class CRecorder;
struct Config;

namespace l0 {
class IRecorderWrapper;
struct CustomLoaderCleanup;
class CGitsPlugin {
  static IRecorderWrapper* _recorderWrapper;
  static std::unique_ptr<CGitsLoader, CustomLoaderCleanup> _loader;
  static std::mutex _mutex;
  static bool _initialized;

  CGitsPlugin(const CGitsPlugin&) = delete;
  CGitsPlugin(CGitsPlugin&&) = delete;
  CGitsPlugin& operator=(const CGitsPlugin&) = delete;
  CGitsPlugin& operator=(CGitsPlugin&&) = delete;
  ~CGitsPlugin() = delete;

public:
  static void Initialize();
  static IRecorderWrapper& RecorderWrapper() {
    return *_recorderWrapper;
  }
  static void ProcessTerminationDetected();
  static const Config& Configuration();
  static bool Initialized() {
    return _initialized;
  }
};
struct CustomLoaderCleanup {
  void operator()(CGitsLoader* loader) const {
    CGitsPlugin::ProcessTerminationDetected();
    delete loader;
  }
};
} // namespace l0
} // namespace gits
