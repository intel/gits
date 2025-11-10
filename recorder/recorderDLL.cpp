// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "platform.h"
#include "tools_lite.h"
#include "recorder.h"
#include "log.h"

#ifdef GITS_PLATFORM_WINDOWS
#include <Windows.h>

// Following is a hack to allow for graceful termination of GITS recorder.
// GITS recorder spawns threads to offload IO from application thread.
// This causes problems for cases where stream is recorded until the end
// of the application, because ExitProcess in first kills all threads
// other then current thread and then calls DLL main functions of loaded
// modules. GITS can't handle such situation correctly because threads are
// terminated without a chance to leave consistent state - in general
// cleanup after them will be impossible from DLL main function.
// To work around, we overwrite ExitProcess function with a to jump
// to our custom procedure which deals with GITS shutdown when its still
// possible cleanly.
char exitProcessHead[12];
FARPROC exitProcessAddr;

void STDCALL GitsExitProcess(UINT uExitCode) {
  if (gits::CRecorder::InstancePtr()) {
    gits::CRecorder::Instance().Close();
  }
  LOG_INFO << "Recording done";

  // Restore code of ExitProcess, now that we intercepted it.
  DWORD oldProtect;
  VirtualProtect(exitProcessAddr, 32, PAGE_EXECUTE_READWRITE, &oldProtect);
  std::copy_n(exitProcessHead, sizeof(exitProcessHead), (char*)exitProcessAddr);
  VirtualProtect(exitProcessAddr, 32, oldProtect, &oldProtect);

  ExitProcess(uExitCode);
}

char terminateProcessHead[12];
FARPROC terminateProcessAddr;

namespace {

void RestoreTerminateProcess();
void InstrumentTerminateProcess();

} // namespace

void STDCALL GitsTerminateProcess(_In_ HANDLE hProcess, _In_ UINT uExitCode) {
  if (GetProcessId(hProcess) == GetCurrentProcessId()) {
    if (gits::CRecorder::InstancePtr()) {
      gits::CRecorder::Instance().Close();
    }
    LOG_INFO << "Recording done by TerminateProcess() ";
  }
  RestoreTerminateProcess();
  TerminateProcess(hProcess, uExitCode);
  InstrumentTerminateProcess();
}

namespace {

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

void InstrumentExitProcess() {
  HMODULE kern = GetModuleHandle("Kernel32.dll");
  exitProcessAddr = GetProcAddress(kern, "ExitProcess");

  DWORD oldProtect;
  VirtualProtect(exitProcessAddr, 32, PAGE_EXECUTE_READWRITE, &oldProtect);
  // Save what we will overwrite.
  std::copy_n((char*)exitProcessAddr, sizeof(exitProcessHead), exitProcessHead);
  // Route to gits cleanup function.
  routeEntryPoint(exitProcessAddr, (PROC)&GitsExitProcess);
  VirtualProtect(exitProcessAddr, 32, oldProtect, &oldProtect);
}

void InstrumentTerminateProcess() {
  HMODULE kern = GetModuleHandle("Kernel32.dll");
  terminateProcessAddr = GetProcAddress(kern, "TerminateProcess");

  DWORD oldProtect;
  VirtualProtect(terminateProcessAddr, 32, PAGE_EXECUTE_READWRITE, &oldProtect);
  // Save what we will overwrite.
  std::copy_n((char*)terminateProcessAddr, sizeof(terminateProcessHead), terminateProcessHead);
  // Route to gits cleanup function.
  routeEntryPoint(terminateProcessAddr, (PROC)&GitsTerminateProcess);
  VirtualProtect(terminateProcessAddr, 32, oldProtect, &oldProtect);
}

void RestoreTerminateProcess() {
  DWORD oldProtect;
  VirtualProtect(terminateProcessAddr, 32, PAGE_EXECUTE_READWRITE, &oldProtect);
  std::copy_n(terminateProcessHead, sizeof(terminateProcessHead), (char*)terminateProcessAddr);
  VirtualProtect(terminateProcessAddr, 32, oldProtect, &oldProtect);
}

} // namespace

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
  switch (ul_reason_for_call) {
  case DLL_PROCESS_ATTACH:
    InstrumentExitProcess();
    InstrumentTerminateProcess();
    break;
  case DLL_THREAD_ATTACH:
    break;
  case DLL_THREAD_DETACH:
    break;
  case DLL_PROCESS_DETACH:
    break;
  }
  return TRUE;
}

#endif // GITS_PLATFORM_WINDOWS
