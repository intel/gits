// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
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
#include "configurationLib.h"

#include <string>

namespace gits {

class CRecorder;

namespace OpenCL {
class IRecorderWrapper;
class CustomLoaderCleanup;
class CGitsPluginOpenCL {
  static IRecorderWrapper* _recorderWrapper;
  static std::unique_ptr<CGitsLoader, CustomLoaderCleanup> _loader;
  static std::mutex _mutex;

  CGitsPluginOpenCL(const CGitsPluginOpenCL&) = delete;
  CGitsPluginOpenCL(CGitsPluginOpenCL&&) = delete;
  CGitsPluginOpenCL& operator=(const CGitsPluginOpenCL&) = delete;
  CGitsPluginOpenCL& operator=(CGitsPluginOpenCL&&) = delete;
  ~CGitsPluginOpenCL() = delete;

public:
  static void Initialize();
  static IRecorderWrapper& RecorderWrapper() {
    return *_recorderWrapper;
  }
  static void ProcessTerminationDetected();
  static const Config& Configuration();
};

struct CustomLoaderCleanup {
  void operator()(CGitsLoader* loader) const {
    CGitsPluginOpenCL::ProcessTerminationDetected();
    delete loader;
  }
};
} // namespace OpenCL
} // namespace gits
