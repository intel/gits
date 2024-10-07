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

#include "gitsPluginOpenCL.h"
#include "openclRecorderWrapperIface.h"

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

void PrePostDisableOpenCL();

namespace gits {
namespace OpenCL {
void* get_proc_address(const char* name);
}
} // namespace gits

namespace gits {
namespace OpenCL {
IRecorderWrapper* CGitsPluginOpenCL::_recorderWrapper;
std::unique_ptr<CGitsLoader, CustomLoaderCleanup> CGitsPluginOpenCL::_loader(nullptr,
                                                                             CustomLoaderCleanup());
std::mutex CGitsPluginOpenCL::_mutex;

namespace {
void fast_exit(int code) {
#if defined GITS_PLATFORM_WINDOWS
  _exit(code);
#else
  _Exit(code);
#endif
}
} // namespace

void CGitsPluginOpenCL::Initialize() {
  static bool initialized = false;
  if (initialized) {
    return;
  }

  // Sleep on initialization to allow for easier attaching to the process
  // during debugging.
  if (getenv("GITS_SLEEP")) {
    std::this_thread::sleep_for(std::chrono::seconds(10));
  }

  try {
    std::unique_lock<std::mutex> lock(_mutex);
    if (initialized) {
      return;
    }

    _loader.reset(new CGitsLoader("GITSRecorderOpenCL"));
    CGitsPluginOpenCL::_recorderWrapper =
        (decltype(_recorderWrapper))CGitsPluginOpenCL::_loader->GetRecorderWrapperPtr();

    if (!CGitsPluginOpenCL::_loader->GetConfiguration().common.recorder.enabled) {
      PrePostDisableOpenCL();
    } else {
      CGitsPluginOpenCL::_recorderWrapper->StreamFinishedEvent(PrePostDisableOpenCL);
    }
    initialized = true;

  } catch (const Exception& ex) {
    Log(ERR) << "Unhandled GITS exception: " << ex.what();
    CGitsPluginOpenCL::_loader.reset();
    fast_exit(EXIT_FAILURE);
  } catch (const std::exception& ex) {
    Log(ERR) << "Unhandled STD exception: " << ex.what();
    CGitsPluginOpenCL::_loader.reset();
    fast_exit(EXIT_FAILURE);
  } catch (...) {
    Log(ERR) << "Unhandled Unknown exception caught";
    CGitsPluginOpenCL::_loader.reset();
    fast_exit(EXIT_FAILURE);
  }
}

void CGitsPluginOpenCL::ProcessTerminationDetected() {
  _recorderWrapper->MarkRecorderForDeletion();
  _recorderWrapper->CloseRecorderIfRequired();
}

const Config& CGitsPluginOpenCL::Configuration() {
  std::unique_lock<std::mutex> lock(_mutex);
  return _loader->GetConfiguration();
}

} // namespace OpenCL
} // namespace gits

namespace gits {
namespace OpenCL {
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
} // namespace OpenCL
} // namespace gits
