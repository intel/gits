// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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
#include <fstream>

#include "gitsLoader.h"
#include "recorderIface.h"
#include "config.h"
#include "exception.h"
#include "log.h"

#if defined GITS_PLATFORM_WINDOWS
#include "Windows.h"
const char* RECORDER_LIB_NAME = "gitsRecorder.dll";
#else
const char* RECORDER_LIB_NAME = "libGitsRecorder.so";
#endif

namespace gits {

CGitsLoader::CGitsLoader(const char* recorderWrapperFactoryName)
    : config_(nullptr), recorderWrapper_(nullptr) {

  // Give the user some time to attach the debugger...
  if (getenv("GITS_SLEEP")) {
    std::this_thread::sleep_for(std::chrono::seconds(10));
  }

  const char* envConfigPath = getenv("GITS_CONFIG_DIR");
  std::filesystem::path libPath = dl::this_library_path();
  std::filesystem::path configPath = libPath.parent_path();
  if (envConfigPath) {
    configPath = std::filesystem::path(envConfigPath);
  }

  CLog::LogFile(libPath);

  // get GITS binaries path
  // ptree is not read here from config as if there was a typo in config gits
  // would crash with any output. we assume that first "InstallationPath" string
  // in a config file is option name followed by the path to the Recorder.
  auto cfgPath = configPath / "gits_config.yml";
  std::ifstream cfgFile(cfgPath);
  if (!cfgFile.good()) {
    static const char* msg = "Error: GITS config file not found.\n";
    std::cerr << msg;
    throw std::runtime_error(msg);
  }

  std::string installationPath;
  while (cfgFile >> installationPath) {
    if (installationPath.find("InstallationPath") != std::string::npos) {
      std::getline(cfgFile, installationPath);
      break;
    }
  }
  // Remove '"' from recorder path
  auto pos_f = installationPath.find_first_not_of("\"' \t\r\n");
  auto pos_l = installationPath.find_last_not_of("\"' \t\r\n");

  // If either is npos, both are necessarily npos.
  if (pos_f == pos_l) {
    throw std::runtime_error("invalid InstallationPath");
  }

  auto* pathStr = installationPath.c_str();
  gitsPath_ = std::filesystem::path(pathStr + pos_f, pathStr + pos_l + 1);

  // load GITS Recorder DLL
  auto recorderPath = gitsPath_ / RECORDER_LIB_NAME;
  if (!std::filesystem::exists(recorderPath)) {
    Log(ERR) << "GITS library not found ('" << recorderPath << "')!!!";
#ifdef GITS_PLATFORM_WINDOWS
    MessageBoxA(nullptr, "GITS library not found in installationPath, check configuration.",
                "Error", MB_OK | MB_ICONERROR);
#endif
    throw std::runtime_error("Failed to find required library");
  }

  recorderLib_ = dl::open_library(recorderPath.string().c_str());
  if (recorderLib_ == nullptr) {
    Log(ERR) << "Cannot load GITS library ('" << recorderPath << "')!!!";
    Log(ERR) << dl::last_error();
    throw std::runtime_error("Failed to load required library");
  }

  // set print handler
  auto printFunc = (FPrintHandlerGet)dl::load_symbol(recorderLib_, "PrintHandlerGet");
  if (printFunc == nullptr) {
    Log(ERR) << "Could not obtain GITS print handler from the library: " << recorderPath << "!!!";
    Log(ERR) << dl::last_error();
    throw std::runtime_error("Failed to load required symbol");
  }

  // call the function
  CLog::LogFunction(printFunc(configPath.string().c_str()));

  // set GITS configuration
  auto configureFunc = (FConfigure)dl::load_symbol(recorderLib_, "Configure");
  if (configureFunc == nullptr) {
    Log(ERR) << "Could not obtain GITS configuration handle from the library: " << recorderPath
             << "!!!";
    Log(ERR) << dl::last_error();
    throw std::runtime_error("Failed to load required symbol");
  }

  // call the function
  config_ = configureFunc(configPath.string().c_str());
  if (!config_) {
    Log(ERR) << "Parsing configuration file: " << configPath << " failed!!!";
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
  // Because log can't use config directly, see log.cpp for info.
  CLog::SetLogLevel(config_->common.shared.thresholdLogLevel);

  if (!config_->common.recorder.enabled) {
    Log(INFO) << "Recording disabled in the configuration file";
  }

  // obtain GITS recorder
  auto gitsRecorderFactory =
      (void*(STDCALL*)())dl::load_symbol(recorderLib_, recorderWrapperFactoryName);
  if (gitsRecorderFactory == nullptr) {
    Log(ERR) << "Couldn't obtain GITS recorder from the library: " << recorderPath << "!!!";
    Log(ERR) << dl::last_error();
    throw std::runtime_error("Couldn't load recorder wrapper factory");
  }
  // call the function
  recorderWrapper_ = gitsRecorderFactory();
}

CGitsLoader::~CGitsLoader() {
  dl::close_library(recorderLib_);
}

std::filesystem::path CGitsLoader::GetGitsPath() const {
  return gitsPath_;
}

const Config& CGitsLoader::GetConfiguration() const {
  if (config_) {
    return *config_;
  }
  throw ENotInitialized(EXCEPTION_MESSAGE);
}

void* CGitsLoader::GetRecorderWrapperPtr() const {
  return recorderWrapper_;
}

void CGitsLoader::ProcessTerminationDetected() {
  config_ = nullptr;
  dl::close_library(recorderLib_);
  recorderLib_ = nullptr;
}

} // namespace gits
