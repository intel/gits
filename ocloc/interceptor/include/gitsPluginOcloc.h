// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   gitsPluginOcloc.h
*
* @brief
*/

#pragma once

#include "gitsLoader.h"
#include "platform.h"
#include "tools.h"

#include <string>

namespace gits {

class CRecorder;
struct Config;

namespace ocloc {
class IRecorderWrapper;

class CGitsPlugin {
  static IRecorderWrapper* _recorderWrapper;
  static std::unique_ptr<CGitsLoader> _loader;
  static std::mutex _mutex;

  CGitsPlugin(const CGitsPlugin&) = delete;
  CGitsPlugin(CGitsPlugin&&) = delete;
  CGitsPlugin& operator=(const CGitsPlugin&) = delete;
  CGitsPlugin& operator=(CGitsPlugin&&) = delete;

public:
  static void Initialize();
  static IRecorderWrapper& RecorderWrapper() {
    std::unique_lock<std::mutex> lock(_mutex);
    return *_recorderWrapper;
  }
  static void ProcessTerminationDetected();
  static const Config& Configuration();
};
} // namespace ocloc
} // namespace gits
