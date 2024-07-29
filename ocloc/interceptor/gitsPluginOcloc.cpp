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

#include "gitsPluginOcloc.h"
#include "oclocRecorderWrapperIface.h"

#include "gitsLoader.h"

#include "exception.h"
#include "config.h"
#include "log.h"
#include "diagnostic.h"
#include "platform.h"
#include "tools.h"

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

void PrePostDisableOcloc();

namespace gits {
namespace ocloc {
void* get_proc_address(const char* name);

IRecorderWrapper* CGitsPlugin::_recorderWrapper;
std::unique_ptr<CGitsLoader> CGitsPlugin::_loader;
std::mutex CGitsPlugin::_mutex;

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
  static bool initialized = false;
  if (initialized) {
    return;
  }

  try {
    std::unique_lock<std::mutex> lock(_mutex);
    if (initialized) {
      return;
    }

    _loader = std::make_unique<CGitsLoader>("GITSRecorderOcloc");
    _recorderWrapper = (decltype(_recorderWrapper))_loader->GetRecorderWrapperPtr();

    if (!_loader->GetConfiguration().common.recorder.enabled) {
      PrePostDisableOcloc();
    } else {
      CGitsPlugin::_recorderWrapper->StreamFinishedEvent(PrePostDisableOcloc);
    }
    initialized = true;

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

const Config& CGitsPlugin::Configuration() {
  return _loader->GetConfiguration();
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
} // namespace ocloc
} // namespace gits
