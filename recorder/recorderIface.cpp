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
#include "config.h"
#include "log.h"
#include "gits.h"
#include "diagnostic.h"
#include "recorderUtils.h"

#include <fstream>

namespace {

void PrintHandler(const char* text) {
  gits::CLog(gits::LogLevel::OFF, gits::LogStyle::RAW) << text;
}

} // namespace

gits::FPrintHandler STDCALL PrintHandlerGet(const char* dir) {
  try {
    gits::CLog::LogFile(dir);
    return PrintHandler;
  } catch (const std::exception& ex) {
    Log(ERR) << "Cannot establish GITS logging: " << ex.what() << std::endl;
    exit(EXIT_FAILURE);
  }
}

gits::Configuration* STDCALL Configure(const char* cfgDir) {
  try {
    if (!gits::ConfigureRecorder(std::filesystem::path(cfgDir) /
                                 gits::Configurator::ConfigFileName())) {
      return nullptr;
    }
    return &gits::Configurator::GetMutable();
  } catch (const std::exception& ex) {
    Log(ERR) << "Cannot configure GITS: " << ex.what() << std::endl;
    exit(EXIT_FAILURE);
  }
}
