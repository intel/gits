// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   gitsLoader.h
 *
 * @brief Declaration of GITS Loader to be used in proxy DLLs.
 */

#pragma once

#include "tools.h"
#include "pragmas.h"
#include "dynamic_linker.h"
#include "configurationLib.h"

namespace gits {
std::filesystem::path GetConfigPathForLoader();
class CGitsLoader : public gits::noncopyable {
public:
  CGitsLoader(const char* recorderWrapperFactoryName);
  ~CGitsLoader();

  std::filesystem::path GetGitsPath() const;
  Configuration& GetConfiguration();
  void* GetRecorderWrapperPtr() const;

  void ProcessTerminationDetected();

private:
  Configuration* config_;
  dl::SharedObject recorderLib_;
  std::filesystem::path gitsPath_;
  void* recorderWrapper_;
};

} // namespace gits
