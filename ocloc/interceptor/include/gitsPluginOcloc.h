// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
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
  static boost::mutex _mutex;

public:
  static void Initialize();
  static IRecorderWrapper& RecorderWrapper() {
    return *_recorderWrapper;
  }
  static void ProcessTerminationDetected();
  static const Config& Configuration();
};
} // namespace ocloc
} // namespace gits
