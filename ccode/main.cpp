// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

// Plog
#include <plog/Log.h>
#include <plog/Formatters/MessageOnlyFormatter.h>
#include <plog/Formatters/FuncMessageFormatter.h>
#include <plog/Appenders/DebugOutputAppender.h>
#include <plog/Appenders/ColorConsoleAppender.h>
#include <plog/Init.h>

// Args
#include <args.hxx>

#include "dataService.h"
#include "generated/objects.h"
#include "generated/commands.h"

#include <filesystem>

#ifndef DATA_BIN_PATH
#error "DATA_BIN_PATH must be defined by CMake"
#endif

int main(int argc, char* argv[]) {
  // Initialize logger
  static plog::DebugOutputAppender<plog::FuncMessageFormatter> debugAppender;
  static plog::ColorConsoleAppender<plog::MessageOnlyFormatter> colorConsoleAppender;
  plog::init(plog::verbose, &debugAppender).addAppender(&colorConsoleAppender);

  // Change working directory to executable directory
  std::filesystem::path exeDir = std::filesystem::path(argv[0]).parent_path();
  std::filesystem::current_path(exeDir);

  // Prepare environment
  SetupEnvironment();

  // Locate and open data.bin file
  auto dataFile = std::filesystem::path(DATA_BIN_PATH);
  if (!DataService::Get().Open(dataFile)) {
    dataFile = exeDir / "data.bin";
    if (!DataService::Get().Open(dataFile)) {
      LOG_ERROR << "CCode - data.bin file not found at " << DATA_BIN_PATH << " or "
                << dataFile.string();
      return -1;
    }
  }

  LOG_INFO << "CCode - Starting execution...";
  StateRestore();
  RunFrames();
  LOG_INFO << "CCode - Done!";

  TeardownEnvironment();
  return 0;
}
