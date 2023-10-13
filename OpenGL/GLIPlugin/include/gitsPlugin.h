// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   gitsPlugin.h
 *
 * @brief
 */

#pragma once

#include "platform.h"
#include "pragmas.h"
#include "tools.h"
#include "gitsLoader.h"

#include <string>

namespace gits {

class CRecorder;
struct Config;

namespace OpenGL {

class IRecorderWrapper;

class CGitsPlugin {
  static IRecorderWrapper* _recorderWrapper;
  static std::unique_ptr<CGitsLoader> _loader;
  static std::mutex _mutex;

public:
  static void Initialize();
  static IRecorderWrapper& RecorderWrapper() {
    return *_recorderWrapper;
  }
  static void ProcessTerminationDetected();
  static const Config& Configuration();
};

} // namespace OpenGL
} // namespace gits
