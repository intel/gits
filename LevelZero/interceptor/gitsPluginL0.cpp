// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   gitsPluginVulkan.cpp
*
* @brief
*/

#include "gitsPluginL0.h"
#include "l0RecorderWrapperIface.h"

#include "gitsLoader.h"

#include "exception.h"
#include "config.h"
#include "log.h"
#include "diagnostic.h"
#include "platform.h"
#include "tools.h"

#include <memory>
#include <sstream>
#include <cctype>
#include <algorithm>
#include <cstring>
#include <iterator>
#include <cstdio>
#include <new>
#include <cstdarg>
#include <thread>
#include <mutex>

DISABLE_WARNINGS
#include <boost/property_tree/ptree.hpp>
ENABLE_WARNINGS

void PrePostDisableLevelZero();

namespace gits {
namespace l0 {
void* get_proc_address(const char* name);

IRecorderWrapper* CGitsPlugin::_recorderWrapper;
std::unique_ptr<CGitsPlugin> CGitsPlugin::_loader;
std::mutex CGitsPlugin::_mutex;
bool CGitsPlugin::_initialized = false;

namespace {
void fast_exit(int code) {
#if defined GITS_PLATFORM_WINDOWS
  _exit(code);
#else
  _Exit(code);
#endif
}
} // namespace

void CGitsPlugin::Initialize() {
  if (_initialized) {
    return;
  }

  // Sleep on initialization to allow for easier attaching to the process
  // during debugging.
  if (getenv("GITS_SLEEP")) {
    std::this_thread::sleep_for(std::chrono::seconds(10));
  }

  try {
    std::unique_lock<std::mutex> lock(_mutex);
    if (_initialized) {
      return;
    }

    const char* envConfigPath = getenv("GITS_CONFIG_DIR");

    std::filesystem::path libPath = dl::this_library_path();
    std::filesystem::path configPath = libPath.parent_path();

    if (envConfigPath) {
      configPath = std::filesystem::path(envConfigPath);
    }

    _loader.reset(new CGitsPlugin(configPath, "GITSRecorderL0"));
    _recorderWrapper = (decltype(_recorderWrapper))_loader->RecorderWrapperPtr();

    if (!_loader->Configuration().recorder.basic.enabled) {
      PrePostDisableLevelZero();
    } else {
      CGitsPlugin::_recorderWrapper->StreamFinishedEvent(PrePostDisableLevelZero);
    }

    _initialized = true;

  } catch (const Exception& ex) {
    Log(ERR) << "Unhandled GITS exception: " << ex.what();
    _loader.reset();
    fast_exit(EXIT_FAILURE);
  } catch (const std::exception& ex) {
    Log(ERR) << "Unhandled STD exception: " << ex.what();
    _loader.reset();
    fast_exit(EXIT_FAILURE);
  } catch (...) {
    Log(ERR) << "Unhandled Unknown exception caught";
    _loader.reset();
    fast_exit(EXIT_FAILURE);
  }
}

void CGitsPlugin::ProcessTerminationDetected() {
  static_cast<CGitsLoader*>(_loader.get())->ProcessTerminationDetected();
}

const Config& CGitsPlugin::Configuration() {
  return static_cast<CGitsLoader*>(_loader.get())->Configuration();
}

CGitsPlugin::~CGitsPlugin() {
  _recorderWrapper->MarkRecorderForDeletion();
  _recorderWrapper->CloseRecorderIfRequired();
  _initialized = false;
  static_cast<CGitsLoader*>(_loader.get())->~CGitsLoader();
}

static int module_identification_token = 0;

void* get_proc_address(const char* name) {
  static auto thisModule = dl::symbol_library(&module_identification_token);
  void* proc = dl::load_symbol(thisModule, name);

  if (proc != nullptr) {
    return proc;
  }

  // The function isn't provided by GITS wrappers - this will with high probability cause
  // recording error. Log the event, only mention such event once for each function.
  static std::set<std::string> missedFuncs;
  bool firstEvent = (missedFuncs.count(name) == 0);
  missedFuncs.insert(name);

  if (firstEvent) {
    if (proc == nullptr) {
      Log(WARN) << "Application queried address of unavailable function " << name << " returned 0";
    } else {
      Log(WARN) << "Application queried address of unavailable function " << name
                << " returned underlying implementation";
    }
  }

  return proc;
}
} // namespace l0
} // namespace gits
