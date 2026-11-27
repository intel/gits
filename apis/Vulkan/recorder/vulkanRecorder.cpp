// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "vulkanRecorder.h"
#include "captureManager.h"
#include "log.h"
#include "configurator.h"
#include "platform.h"

#ifdef GITS_PLATFORM_LINUX
#include "messageBus.h"

#include <cstdlib>
#include <cstring>
#include <mutex>

#include <dlfcn.h>
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace gits {
namespace vulkan {

void RecorderWrapper::LoadGlobalLevelFunctions(PFN_vkGetInstanceProcAddr getProcAddr) {
  CaptureManager::Get().LoadGlobalFunctions(getProcAddr);
}

void RecorderWrapper::LoadInstanceLevelFunctions(PFN_vkGetInstanceProcAddr getProcAddr,
                                                 VkInstance instance) {
  CaptureManager::Get().LoadInstanceFunctions(getProcAddr, instance);
}

void RecorderWrapper::LoadDeviceLevelFunctions(PFN_vkGetDeviceProcAddr getProcAddr,
                                               VkDevice device) {
  CaptureManager::Get().LoadDeviceFunctions(getProcAddr, device);
}

void RecorderWrapper::LoadDeviceLevelFunctions(void* dispatchKey, VkDevice device) {
  CaptureManager::Get().LoadDeviceFunctions(dispatchKey, device);
}

PFN_vkVoidFunction RecorderWrapper::GetFunctionWrapper(const char* name) {
  return CaptureManager::Get().GetFunctionWrapper(name);
}

} // namespace vulkan
} // namespace gits

std::unique_ptr<gits::vulkan::RecorderWrapper> g_RecorderWrapper;

#ifdef GITS_PLATFORM_LINUX
namespace {

// Serializes finish + real _exit/_Exit so a second exiting thread waits until Close completes
// and the first thread has entered the real exit (which does not return).
std::mutex g_FinalizeMutex;
bool g_RecordingFinished = false;
pid_t g_RecordingProcessId = 0;

void FinishRecordingUnlocked() {
  if (g_RecordingFinished) {
    return;
  }
  g_RecordingFinished = true;

  // MessageBus must still be alive: atexit(FinishRecordingImpl) is registered only after
  // MessageBus::get() so this handler runs before the MessageBus static destructor.
  gits::MessageBus::get().publish({gits::PUBLISHER_RECORDER, gits::TOPIC_PROGRAM_EXIT},
                                  std::make_shared<gits::ProgramMessage>());
  LOG_INFO << "Recording done";
}

void FinishRecordingImpl() {
  std::lock_guard<std::mutex> lock(g_FinalizeMutex);
  FinishRecordingUnlocked();
}

// Linux counterpart of recorderDLL.cpp ExitProcess instrumentation: the process can call
// _exit/_Exit and skip atexit and C++ destructors, so the stream never finishes.
constexpr size_t kExitPatchSize = 12;

// libc .text is mapped r-x; temporarily elevate to rwx only while patching.
constexpr int kLibcTextProt = PROT_READ | PROT_EXEC;

struct ExitHook {
  void* address = nullptr;
  unsigned char original[kExitPatchSize] = {};
  bool installed = false;
};

ExitHook g_ExitHook;      // "_exit"
ExitHook g_ExitUpperHook; // "_Exit"

void RouteEntryPoint(void* src, void* dst) {
  const int ptrOffset = 2;
  unsigned char routingCode[] = {0x48, 0xB8, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xE0};
  auto addrValue = reinterpret_cast<uintptr_t>(dst);
  std::memcpy(&routingCode[ptrOffset], &addrValue, sizeof(void*));
  std::memcpy(src, routingCode, sizeof(routingCode));
}

bool SetProtection(void* address, size_t size, int prot) {
  const long pageSize = sysconf(_SC_PAGESIZE);
  if (pageSize <= 0 || address == nullptr || size == 0) {
    return false;
  }
  auto start = reinterpret_cast<uintptr_t>(address) & ~(static_cast<uintptr_t>(pageSize) - 1);
  auto end = reinterpret_cast<uintptr_t>(address) + size;
  return mprotect(reinterpret_cast<void*>(start), end - start, prot) == 0;
}

bool InstallExitHook(ExitHook& hook, const char* name, void* replacement) {
  void* address = dlsym(RTLD_DEFAULT, name);
  if (address == nullptr) {
    LOG_WARNING << "Failed to find libc symbol for exit hook: " << name;
    return false;
  }
  if (!SetProtection(address, kExitPatchSize, PROT_READ | PROT_WRITE | PROT_EXEC)) {
    LOG_WARNING << "Failed to make libc " << name << " writable for exit hook";
    return false;
  }
  std::memcpy(hook.original, address, kExitPatchSize);
  hook.address = address;
  RouteEntryPoint(address, replacement);
  hook.installed = SetProtection(address, kExitPatchSize, kLibcTextProt);
  if (!hook.installed) {
    LOG_WARNING << "Failed to restore libc code page protection after exit hook patch";
    // Best effort: put original bytes back while still writable.
    std::memcpy(address, hook.original, kExitPatchSize);
    return false;
  }
  return true;
}

void RestoreExitHook(ExitHook& hook) {
  if (!hook.installed || hook.address == nullptr) {
    return;
  }
  if (!SetProtection(hook.address, kExitPatchSize, PROT_READ | PROT_WRITE | PROT_EXEC)) {
    return;
  }
  std::memcpy(hook.address, hook.original, kExitPatchSize);
  if (!SetProtection(hook.address, kExitPatchSize, kLibcTextProt)) {
    LOG_WARNING << "Failed to restore libc code page protection after exit hook patch";
  }
}

void ReinstallExitHook(ExitHook& hook, void* replacement) {
  if (!hook.installed || hook.address == nullptr) {
    return;
  }
  if (!SetProtection(hook.address, kExitPatchSize, PROT_READ | PROT_WRITE | PROT_EXEC)) {
    return;
  }
  RouteEntryPoint(hook.address, replacement);
  if (!SetProtection(hook.address, kExitPatchSize, kLibcTextProt)) {
    LOG_WARNING << "Failed to restore libc code page protection after exit hook patch";
  }
}

using LibcExitFn = void (*)(int);

void CallRealExit(ExitHook& hook, int status, void* replacement) {
  RestoreExitHook(hook);
  auto realExit = reinterpret_cast<LibcExitFn>(hook.address);
  realExit(status);
  ReinstallExitHook(hook, replacement);
}

void GitsExit(int status) {
  // Forked child: do not take the mutex (can deadlock after multithreaded fork()) or finalize.
  if (getpid() != g_RecordingProcessId) {
    CallRealExit(g_ExitHook, status, reinterpret_cast<void*>(&GitsExit));
    return;
  }

  std::lock_guard<std::mutex> lock(g_FinalizeMutex);
  // _Exit may call _exit; log only on the first finish path.
  if (!g_RecordingFinished) {
    LOG_INFO << "Closing recorder due to _exit(" << status << ")";
  }
  FinishRecordingUnlocked();
  CallRealExit(g_ExitHook, status, reinterpret_cast<void*>(&GitsExit));
}

void GitsExitUpper(int status) {
  if (getpid() != g_RecordingProcessId) {
    CallRealExit(g_ExitUpperHook, status, reinterpret_cast<void*>(&GitsExitUpper));
    return;
  }

  std::lock_guard<std::mutex> lock(g_FinalizeMutex);
  // Log only on the first finish path (_exit may already have run).
  if (!g_RecordingFinished) {
    LOG_INFO << "Closing recorder due to _Exit(" << status << ")";
  }
  FinishRecordingUnlocked();
  CallRealExit(g_ExitUpperHook, status, reinterpret_cast<void*>(&GitsExitUpper));
}

void InstrumentLibcExits() {
  // exit() already runs atexit handlers; only _exit/_Exit skip them.
  void* const exitAddr = dlsym(RTLD_DEFAULT, "_exit");
  void* const exitUpperAddr = dlsym(RTLD_DEFAULT, "_Exit");
  InstallExitHook(g_ExitHook, "_exit", reinterpret_cast<void*>(&GitsExit));
  // On many libcs these are aliases of the same function; hooking twice corrupts the restore path.
  if (exitUpperAddr != nullptr && exitUpperAddr != exitAddr) {
    InstallExitHook(g_ExitUpperHook, "_Exit", reinterpret_cast<void*>(&GitsExitUpper));
  }
}

} // namespace
#endif

gits::vulkan::IRecorderWrapper* STDCALL GITSRecorderVulkan2() {
  if (!g_RecorderWrapper) {
    try {
      g_RecorderWrapper = std::make_unique<gits::vulkan::RecorderWrapper>();
#ifdef GITS_PLATFORM_LINUX
      // Construct MessageBus before atexit so its __cxa_atexit destructor is registered after
      // FinishRecordingImpl; otherwise exit destroys the bus first and publish is use-after-free.
      gits::MessageBus::get();
      g_RecordingProcessId = getpid();
      // atexit covers exit(); hook _exit/_Exit which skip atexit (Windows uses ExitProcess).
      std::atexit(FinishRecordingImpl);
      InstrumentLibcExits();
#endif
    } catch (const std::exception& e) {
      LOG_ERROR << "Cannot initialize recorder: " << e.what() << std::endl;
      exit(EXIT_FAILURE);
    }
  }
  gits::Configurator::Instance().LogUnspportedConfigUsage(gits::ApiBool::VK);
  return g_RecorderWrapper.get();
}
