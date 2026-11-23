// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
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
#include "dynamic_linker.h"
#include "configurationLib.h"

namespace gits {
class CGitsLoader : public gits::noncopyable {
public:
  explicit CGitsLoader(const char* recorderWrapperFactoryName);
  ~CGitsLoader();

  Configuration& GetConfiguration();
  void* GetRecorderWrapperPtr() const;

private:
  Configuration* config_{};
  dl::SharedObject tbbLib_{};
  dl::SharedObject recorderLib_{};
  void* recorderWrapper_{};
};

} // namespace gits
