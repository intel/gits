// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
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

namespace gits {

struct Config;
class CGitsLoader {
  const Config* _config;
  dl::SharedObject _sharedLib;
  void* recorderWrapper;

  CGitsLoader(const CGitsLoader&);            /**< @brief Disallowed */
  CGitsLoader& operator=(const CGitsLoader&); /**< @brief Disallowed */

public:
  CGitsLoader(const std::filesystem::path& path, const char* recorderWrapperFactoryName);
  ~CGitsLoader();

  const Config& Configuration() const;
  void* RecorderWrapperPtr() const {
    return recorderWrapper;
  }

  void ProcessTerminationDetected();
};

} // namespace gits
