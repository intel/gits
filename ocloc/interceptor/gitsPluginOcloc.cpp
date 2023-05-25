// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
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

DISABLE_WARNINGS
#include <boost/property_tree/ptree.hpp>
#include <boost/thread.hpp>
ENABLE_WARNINGS

namespace bfs = boost::filesystem;
void PrePostDisableOcloc();

namespace gits {
namespace ocloc {
void* get_proc_address(const char* name);

IRecorderWrapper* CGitsPlugin::_recorderWrapper;
std::unique_ptr<CGitsPlugin> CGitsPlugin::_loader;
boost::mutex CGitsPlugin::_mutex;
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
    using namespace boost;
    this_thread::sleep(posix_time::millisec(10000));
  }

  try {
    boost::unique_lock<boost::mutex> lock(_mutex);
    if (_initialized) {
      return;
    }

    const char* envConfigPath = getenv("GITS_CONFIG_DIR");

    bfs::path libPath = dl::this_library_path();
    bfs::path configPath = libPath.parent_path();

    if (envConfigPath) {
      configPath = bfs::path(envConfigPath);
    }

    _loader.reset(new CGitsPlugin(configPath, "GITSRecorderOcloc"));
    _recorderWrapper = (decltype(_recorderWrapper))_loader->RecorderWrapperPtr();

    if (!_loader->Configuration().recorder.basic.enabled) {
      PrePostDisableOcloc();
    } else {
      CGitsPlugin::_recorderWrapper->StreamFinishedEvent(PrePostDisableOcloc);
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

CGitsPlugin::~CGitsPlugin() {
  _recorderWrapper->MarkRecorderForDeletion();
  _recorderWrapper->CloseRecorderIfRequired();
  _initialized = false;
  static_cast<CGitsLoader*>(_loader.get())->~CGitsLoader();
}

const Config& CGitsPlugin::Configuration() {
  return static_cast<CGitsLoader*>(_loader.get())->Configuration();
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
