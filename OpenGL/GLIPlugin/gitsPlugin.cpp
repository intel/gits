// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   gitsPlugin.cpp
 *
 * @brief
 */

#include "platform.h"
#ifdef GITS_PLATFORM_WINDOWS
#include <Windows.h>
#else
#include <dlfcn.h>
#endif

#include "gitsPlugin.h"
#include "openglRecorderWrapperIface.h"
#include "gitsLoader.h"

#include "exception.h"
#include "config.h"
#include "log.h"
#include "diagnostic.h"
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

DISABLE_WARNINGS
#include <boost/property_tree/ptree.hpp>
ENABLE_WARNINGS

void PrePostDisableGL();

namespace gits {
namespace OpenGL {
void* get_proc_address(const char* name);
}
} // namespace gits

namespace gits {
namespace OpenGL {

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

    const char* envConfigPath = getenv("GITS_CONFIG_DIR");

    std::filesystem::path libPath = dl::this_library_path();
    std::filesystem::path configPath = libPath.parent_path();

    if (envConfigPath) {
      configPath = std::filesystem::path(envConfigPath);
    }

    _loader.reset(new CGitsLoader(configPath, "GITSRecorderOpenGL"));
    _recorderWrapper = (decltype(_recorderWrapper))_loader->RecorderWrapperPtr();

    if (!_loader->Configuration().recorder.basic.enabled) {
      PrePostDisableGL();
    }

    CGitsPlugin::_recorderWrapper->StreamFinishedEvent(PrePostDisableGL);
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

void CGitsPlugin::ProcessTerminationDetected() {
  if (_loader) {
    _loader->ProcessTerminationDetected();
  }
}

const Config& CGitsPlugin::Configuration() {
  return _loader->Configuration();
}

} // namespace OpenGL
} // namespace gits

#ifdef GITS_PLATFORM_WINDOWS

#include <direct.h>
#include <crtdbg.h>
#include <psapi.h>

namespace {
std::vector<HMODULE> getProcessModules(HANDLE process) {
  std::vector<HMODULE> modules(1);
  DWORD needed = 0;
  EnumProcessModules(process, &modules[0], sizeof(HMODULE), &needed);

  modules.resize(needed / sizeof(HMODULE));
  EnumProcessModules(
      process, &modules[0],
      gits::ensure_unsigned32bit_representible<size_t>(sizeof(HMODULE) * modules.size()), &needed);
  return modules;
}

void routeEntryPoint(PROC src, PROC dst) {
#if defined GITS_ARCH_X86
  const int ptrOffset = 1;
  unsigned char routingCode[] = {0xB8, 0, 0, 0, 0, 0xFF, 0xE0};
#elif defined GITS_ARCH_X64
  const int ptrOffset = 2;
  unsigned char routingCode[] = {0x48, 0xB8, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xE0};
#endif
  void* addr_value = (void*)dst;
  unsigned char* addr = (unsigned char*)&addr_value;
  std::copy(addr, addr + sizeof(void*), &routingCode[ptrOffset]);
  std::copy(routingCode, routingCode + sizeof(routingCode), (unsigned char*)src);
}

std::vector<PROC> getAllProcsByOrdinal(HMODULE module) {
  std::vector<PROC> procs;
  for (int i = 1;; ++i) {
    PROC ptr = GetProcAddress(module, (LPCSTR)i);
    if (ptr == 0) {
      break;
    }
    procs.push_back(ptr);
  }
  return procs;
}

std::filesystem::path ModuleDir(HMODULE module) {
  char dllName[1024] = {0};
  GetModuleFileName(module, dllName, 1023);
  return std::filesystem::path(dllName).parent_path();
}

void routeEntryPoints(const std::vector<PROC>& src, const std::vector<PROC>& dst) {
  assert(src.size() == dst.size());
  for (size_t i = 0; i < src.size(); ++i) {
    routeEntryPoint(src[i], dst[i]);
  }
}

bool hookToExistingModule(HMODULE hModule, const char* idTokenName) {
  std::vector<HMODULE> modules = getProcessModules(GetCurrentProcess());
  auto this_module_dir = ModuleDir(hModule);

  int firstIntercept = -1;

  // find first loaded glintercept
  for (size_t i = 0; i < modules.size(); ++i) {
    typedef int ident_proc_t();
    ident_proc_t* ident_proc = (ident_proc_t*)GetProcAddress(modules[i], idTokenName);
    if (ident_proc != 0 && firstIntercept == -1) {
      if (ModuleDir(modules[i]) == this_module_dir) {
        firstIntercept = static_cast<int>(i);
      }
    }
  }

  // this is first loaded glintercept, don't reroute anything
  if (modules[firstIntercept] == hModule) {
    return false;
  }

  // reroute all entrypoint from current instance to first instance
  {
    std::vector<PROC> orig = getAllProcsByOrdinal(hModule);
    std::vector<PROC> dest = getAllProcsByOrdinal(modules[firstIntercept]);

    char* max_proc = (char*)*std::max_element(orig.begin(), orig.end());
    char* min_proc = (char*)*std::min_element(orig.begin(), orig.end());

    DWORD oldProtect = 0;
    VirtualProtect(min_proc, (max_proc - min_proc) + 32, PAGE_EXECUTE_READWRITE, &oldProtect);
    routeEntryPoints(orig, dest);
    VirtualProtect(min_proc, (max_proc - min_proc) + 32, oldProtect, &oldProtect);
  }

  return true;
}

} // namespace

BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
  using namespace gits::OpenGL;
  switch (ul_reason_for_call) {
  case DLL_PROCESS_ATTACH: {
    hookToExistingModule((HMODULE)hModule, "GITSIdentificationToken");
    break;
  }
  case DLL_THREAD_ATTACH: {
    break;
  }
  case DLL_THREAD_DETACH: {
    break;
  }
  case DLL_PROCESS_DETACH: {
    // unload due to process termination
    if (lpReserved != NULL) {
      CGitsPlugin::ProcessTerminationDetected();
    }

    break;
  }
  }
  return TRUE;
}

#else

void DllInitialize() __attribute__((constructor(102)));

void DllInitialize() {
  // Initialize lazily, following will be done in GL functions
  // gits::OpenGL::CGitsPlugin::Initialize();
}

#endif

namespace gits {
namespace OpenGL {
static int module_identification_token = 0;

void* get_proc_address(const char* name) {
  static auto thisModule = dl::symbol_library(&module_identification_token);
  void* proc = dl::load_symbol(thisModule, name);

  static const char* exclusionList[] = {
      "glCreateDebugObjectMESA",    "glGetDebugLogLengthMESA",  "glCreateSyncFromCLeventARB",
      "glClearDebugLogMESA",        "glFramebufferTextureFace", "glGetDebugLogMESA",
      "glGetProgramRegisterfvMESA", "glGetQueryObjectui64vARB", "glXGetCurrentContext",
  };
  const char** exclusionListEnd = exclusionList + size(exclusionList);

  auto pos = std::find(exclusionList, exclusionListEnd, std::string(name));
  if (pos != exclusionListEnd) {
    Log(WARN) << "Explicitly ignored function load request for: " << *pos;
    proc = nullptr;
  }

  if (proc != nullptr) {
    return proc;
  }

#if defined(GITS_PLATFORM_LINUX)
  // Can't continue on Linux due to a function being called from dlsym
  // replacement. To handle this correctly we should provide only 'core'
  // function in dlsym, but that requires yet another table of functions.
  return nullptr;
#endif

  // The function isn't provided by GITS wrappers - this will with high
  // probability cause recording error. Log the event, only mention such event
  // once for each function.
  static std::set<std::string> missedFuncs;
  bool firstEvent = (missedFuncs.count(name) == 0);
  missedFuncs.insert(name);

  auto& drv_ = CGitsPlugin::RecorderWrapper().Drivers();
  if (drv_.gl.Api() == CGlDriver::API_NULL) {
    Log(WARN) << "Application queried function address before setting context - API unknown.";
    return nullptr;
  }

  if (drv_.gl.Api() == CGlDriver::API_GL) {
#if defined GITS_PLATFORM_WINDOWS
    proc = drv_.wgl.wglGetProcAddress(name);
#elif defined GITS_PLATFORM_X11
    proc = (void*)drv.glx.glXGetProcAddress((GLubyte*)name);
#endif
  }
  if (drv_.gl.Api() != CGlDriver::API_GL) {
    proc = drv_.egl.eglGetProcAddress(name);
  }

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
} // namespace OpenGL
} // namespace gits
