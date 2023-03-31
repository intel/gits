// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   gitsPluginOpenCL.h
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

namespace OpenCL {
class IRecorderWrapper;

class CGitsPluginOpenCL {
  static IRecorderWrapper* _recorderWrapper;
  static std::unique_ptr<CGitsLoader> _loader;
  static boost::mutex _mutex;

  CGitsPluginOpenCL(const CGitsPluginOpenCL&) = delete;
  CGitsPluginOpenCL(CGitsPluginOpenCL&&) = delete;
  CGitsPluginOpenCL& operator=(const CGitsPluginOpenCL&) = delete;
  CGitsPluginOpenCL& operator=(CGitsPluginOpenCL&&) = delete;

public:
  ~CGitsPluginOpenCL();
  static void Initialize();
  static IRecorderWrapper& RecorderWrapper() {
    return *_recorderWrapper;
  }
  static void ProcessTerminationDetected();
  static const Config& Configuration();
};
} // namespace OpenCL
} // namespace gits
