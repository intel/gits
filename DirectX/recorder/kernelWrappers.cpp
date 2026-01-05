// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "kernelWrappers.h"
#include "captureManager.h"

namespace gits {
namespace DirectX {

DWORD WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds) {

  auto& manager = CaptureManager::get();
  DWORD ret = manager.getKernel32DispatchTable().WaitForSingleObject(hHandle, dwMilliseconds);

  if (ret == WAIT_OBJECT_0) {
    manager.getFenceService().waitSignaled(hHandle);
  }

  return ret;
}

DWORD WaitForSingleObjectEx(HANDLE hHandle, DWORD dwMilliseconds, BOOL bAlertable) {

  auto& manager = CaptureManager::get();
  DWORD ret =
      manager.getKernel32DispatchTable().WaitForSingleObjectEx(hHandle, dwMilliseconds, bAlertable);

  if (ret == WAIT_OBJECT_0) {
    manager.getFenceService().waitSignaled(hHandle);
  }

  return ret;
}

DWORD WaitForMultipleObjects(DWORD nCount,
                             const HANDLE* lpHandles,
                             BOOL bWaitAll,
                             DWORD dwMilliseconds) {

  auto& manager = CaptureManager::get();
  DWORD ret = manager.getKernel32DispatchTable().WaitForMultipleObjects(nCount, lpHandles, bWaitAll,
                                                                        dwMilliseconds);
  if (ret < WAIT_OBJECT_0 + nCount) {
    if (bWaitAll) {
      manager.getFenceService().waitSignaled(nCount, lpHandles);
    } else {
      manager.getFenceService().waitSignaled(lpHandles[ret - WAIT_OBJECT_0]);
    }
  }
  return ret;
}

DWORD WaitForMultipleObjectsEx(
    DWORD nCount, const HANDLE* lpHandles, BOOL bWaitAll, DWORD dwMilliseconds, BOOL bAlertable) {

  auto& manager = CaptureManager::get();
  DWORD ret = manager.getKernel32DispatchTable().WaitForMultipleObjectsEx(
      nCount, lpHandles, bWaitAll, dwMilliseconds, bAlertable);

  if (ret < WAIT_OBJECT_0 + nCount) {
    if (bWaitAll) {
      manager.getFenceService().waitSignaled(nCount, lpHandles);
    } else {
      manager.getFenceService().waitSignaled(lpHandles[ret - WAIT_OBJECT_0]);
    }
  }
  return ret;
}

DWORD SignalObjectAndWait(HANDLE hObjectToSignal,
                          HANDLE hObjectToWaitOn,
                          DWORD dwMilliseconds,
                          BOOL bAlertable) {

  auto& manager = CaptureManager::get();
  DWORD ret =
      manager.getKernel32DispatchTable().WaitForSingleObject(hObjectToWaitOn, dwMilliseconds);

  if (ret == WAIT_OBJECT_0) {
    manager.getFenceService().waitSignaled(hObjectToWaitOn, hObjectToSignal);
  }

  return ret;
}

HMODULE MyLoadLibraryA(LPCSTR lpLibFileName) {
  HMODULE hModule = CaptureManager::get().getKernel32DispatchTable().LoadLibraryA(lpLibFileName);
  if (!hModule) {
    return hModule;
  }

  std::string libStr = lpLibFileName;
  std::transform(libStr.begin(), libStr.end(), libStr.begin(), ::tolower);
  if (libStr.find("libxess.dll") != std::string::npos) {
    CaptureManager::get().interceptXessFunctions();
  }

  return hModule;
}

HMODULE MyLoadLibraryW(LPCWSTR lpLibFileName) {
  HMODULE hModule = CaptureManager::get().getKernel32DispatchTable().LoadLibraryW(lpLibFileName);
  if (!hModule) {
    return hModule;
  }

  std::wstring libStr = lpLibFileName;
  std::transform(libStr.begin(), libStr.end(), libStr.begin(), ::towlower);
  if (libStr.find(L"libxess.dll") != std::string::npos) {
    CaptureManager::get().interceptXessFunctions();
  }

  return hModule;
}

HMODULE MyLoadLibraryExA(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags) {
  HMODULE hModule = CaptureManager::get().getKernel32DispatchTable().LoadLibraryExA(lpLibFileName,
                                                                                    hFile, dwFlags);
  if (!hModule) {
    return hModule;
  }

  std::string libStr = lpLibFileName;
  std::transform(libStr.begin(), libStr.end(), libStr.begin(), ::tolower);
  if (libStr.find("libxess.dll") != std::string::npos) {
    CaptureManager::get().interceptXessFunctions();
  }

  return hModule;
}

HMODULE MyLoadLibraryExW(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags) {
  HMODULE hModule = CaptureManager::get().getKernel32DispatchTable().LoadLibraryExW(lpLibFileName,
                                                                                    hFile, dwFlags);
  if (!hModule) {
    return hModule;
  }

  std::wstring libStr = lpLibFileName;
  std::transform(libStr.begin(), libStr.end(), libStr.begin(), ::towlower);
  if (libStr.find(L"libxess.dll") != std::string::npos) {
    CaptureManager::get().interceptXessFunctions();
  }

  return hModule;
}

} // namespace DirectX
} // namespace gits
