// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   recorderIface.cpp
 *
 * @brief Definition of GITS recorder interface.
 */

#include "recorderIface.h"
#include "gits.h"
#include "diagnostic.h"
#include "recorderUtils.h"

#include <fstream>

gits::Configuration* STDCALL Configure(const char* cfgDir) {
  try {
    if (!gits::ConfigureRecorder(std::filesystem::path(cfgDir) /
                                 gits::Configurator::ConfigFileName())) {
      return nullptr;
    }
    return &gits::Configurator::GetMutable();
  } catch (const std::exception& ex) {
    std::cerr << "Cannot configure GITS: " << ex.what() << std::endl;
    exit(EXIT_FAILURE);
  }
}
