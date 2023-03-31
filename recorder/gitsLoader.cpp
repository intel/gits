// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   gitsLoader.cpp
 *
 * @brief Definition of GITS Loader to be used in proxy DLLs.
 */

#include "pragmas.h"
#include "platform.h"
#include <cstdarg>
#include <iostream>

DISABLE_WARNINGS
#include <boost/filesystem/fstream.hpp>
ENABLE_WARNINGS

#include "gitsLoader.h"
#include "recorderIface.h"
#include "config.h"
#include "exception.h"
#include "log.h"

namespace bfs = boost::filesystem;

#if defined GITS_PLATFORM_WINDOWS
const char* RECORDER_LIB_NAME = "gitsRecorder.dll";
#else
const char* RECORDER_LIB_NAME = "libGitsRecorder.so";
#endif

gits::CGitsLoader::CGitsLoader(const bfs::path& path, const char* recorderWrapperFactoryName)
    : _config(nullptr) {
  CLog::LogFile(path);

  // get GITS binaries path
  // ptree is not read here from config as if there was a typo in config gits
  // would crash with any output. we assume that first "InstallationPath" string
  // in a config file is option name followed by the path to the Recorder.
  auto cfgPath = path / "gits_config.txt";
  std::ifstream cfgFile(cfgPath.string().c_str());
  if (!cfgFile.good()) {
    static const char* msg = "Error: GITS config file not found.\n";
    std::cerr << msg;
    throw std::runtime_error(msg);
  }

  std::string recorderPath;
  while (cfgFile >> recorderPath) {
    if (recorderPath.find("InstallationPath") != std::string::npos) {
      std::getline(cfgFile, recorderPath);
      break;
    }
  }
  // Remove '"' from recorder path
  auto pos_f = recorderPath.find_first_not_of("\"' \t\r\n");
  auto pos_l = recorderPath.find_last_not_of("\"' \t\r\n");

  // If either is npos, both are neceserily npos.
  if (pos_f == pos_l) {
    throw std::runtime_error("invalid InstallationPath");
  }

  // load GITS Recorder DLL
  auto path_cstr = recorderPath.c_str();
  bfs::path gitsPath(path_cstr + pos_f, path_cstr + pos_l + 1);
  gitsPath /= RECORDER_LIB_NAME;

  _sharedLib = dl::open_library(gitsPath.string().c_str());
  if (_sharedLib == nullptr) {
    Log(ERR) << "Cannot load GITS library ('" << gitsPath << "')!!!";
    Log(ERR) << dl::last_error();
    throw std::runtime_error("Failed to load required library");
  }

  // set print handler
  auto printFunc = (FPrintHandlerGet)dl::load_symbol(_sharedLib, "PrintHandlerGet");
  if (printFunc == nullptr) {
    Log(ERR) << "Could not obtain GITS print handler from the library: " << gitsPath << "!!!";
    Log(ERR) << dl::last_error();
    throw std::runtime_error("Failed to load required symbol");
  }

  // call the function
  CLog::LogFunction(printFunc(path.string().c_str()));

  // set GITS configuration
  auto configureFunc = (FConfigure)dl::load_symbol(_sharedLib, "Configure");
  if (configureFunc == nullptr) {
    Log(ERR) << "Could not obtain GITS configuration handle from the library: " << gitsPath
             << "!!!";
    Log(ERR) << dl::last_error();
    throw std::runtime_error("Failed to load required symbol");
  }

  // call the function
  _config = configureFunc(path.string().c_str());
  if (!_config) {
    Log(ERR) << "Parsing configuration file: " << path << " failed!!!";
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
  // Because log can't use config directly, see log.cpp for info.
  CLog::SetLogLevel(_config->common.thresholdLogLevel);

  if (!_config->recorder.basic.enabled) {
    Log(INFO) << "Recording disabled in the configuration file";
  }

  // obtain GITS recorder
  auto gitsRecorderFactory =
      (void*(STDCALL*)())dl::load_symbol(_sharedLib, recorderWrapperFactoryName);
  if (gitsRecorderFactory == nullptr) {
    Log(ERR) << "Couldn't obtain GITS recorder from the library: " << gitsPath << "!!!";
    Log(ERR) << dl::last_error();
    throw std::runtime_error("Couldn't load recorder wrapper factory");
  }
  // call the function
  recorderWrapper = gitsRecorderFactory();
}

gits::CGitsLoader::~CGitsLoader() {
  dl::close_library(_sharedLib);
}

const gits::Config& gits::CGitsLoader::Configuration() const {
  if (_config) {
    return *_config;
  }
  throw ENotInitialized(EXCEPTION_MESSAGE);
}

void gits::CGitsLoader::ProcessTerminationDetected() {
  _config = nullptr;
  dl::close_library(_sharedLib);
  _sharedLib = nullptr;
}
