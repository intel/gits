// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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
#include "log2.h"
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

void PrePostDisableLevelZero();

namespace gits {
namespace l0 {
void* get_proc_address(const char* name);

IRecorderWrapper* CGitsPlugin::_recorderWrapper;
std::unique_ptr<CGitsLoader, CustomLoaderCleanup> CGitsPlugin::_loader(nullptr,
                                                                       CustomLoaderCleanup());
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
  try {
    std::unique_lock<std::mutex> lock(_mutex);
    if (_initialized) {
      return;
    }
    _loader.reset(new CGitsLoader("GITSRecorderL0"));
    _recorderWrapper = (decltype(_recorderWrapper))_loader->GetRecorderWrapperPtr();

    if (!_loader->GetConfiguration().common.recorder.enabled) {
      PrePostDisableLevelZero();
    } else {
      CGitsPlugin::_recorderWrapper->StreamFinishedEvent(PrePostDisableLevelZero);
    }

    _initialized = true;

  } catch (const Exception& ex) {
    LOG_ERROR << "Unhandled GITS exception: " << ex.what();
    _loader.reset();
    fast_exit(EXIT_FAILURE);
  } catch (const std::exception& ex) {
    LOG_ERROR << "Unhandled STD exception: " << ex.what();
    _loader.reset();
    fast_exit(EXIT_FAILURE);
  } catch (...) {
    LOG_ERROR << "Unhandled Unknown exception caught";
    _loader.reset();
    fast_exit(EXIT_FAILURE);
  }
}

const Config& CGitsPlugin::Configuration() {
  std::unique_lock<std::mutex> lock(_mutex);
  return _loader->GetConfiguration();
}

void CGitsPlugin::ProcessTerminationDetected() {
  _recorderWrapper->MarkRecorderForDeletion();
  _recorderWrapper->CloseRecorderIfRequired();
  _initialized = false;
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
      LOG_WARNING << "Application queried address of unavailable function " << name
                  << " returned 0";
    } else {
      LOG_WARNING << "Application queried address of unavailable function " << name
                  << " returned underlying implementation";
    }
  }

  return proc;
}
} // namespace l0
} // namespace gits
