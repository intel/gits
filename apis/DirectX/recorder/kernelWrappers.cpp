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

  auto& manager = CaptureManager::Get();
  DWORD ret = manager.GetKernel32DispatchTable().WaitForSingleObject(hHandle, dwMilliseconds);

  if (ret == WAIT_OBJECT_0) {
    manager.GetFenceService().WaitSignaled(hHandle);
  }

  return ret;
}

DWORD WaitForSingleObjectEx(HANDLE hHandle, DWORD dwMilliseconds, BOOL bAlertable) {

  auto& manager = CaptureManager::Get();
  DWORD ret =
      manager.GetKernel32DispatchTable().WaitForSingleObjectEx(hHandle, dwMilliseconds, bAlertable);

  if (ret == WAIT_OBJECT_0) {
    manager.GetFenceService().WaitSignaled(hHandle);
  }

  return ret;
}

DWORD WaitForMultipleObjects(DWORD nCount,
                             const HANDLE* lpHandles,
                             BOOL bWaitAll,
                             DWORD dwMilliseconds) {

  auto& manager = CaptureManager::Get();
  DWORD ret = manager.GetKernel32DispatchTable().WaitForMultipleObjects(nCount, lpHandles, bWaitAll,
                                                                        dwMilliseconds);
  if (ret < WAIT_OBJECT_0 + nCount) {
    if (bWaitAll) {
      manager.GetFenceService().WaitSignaled(nCount, lpHandles);
    } else {
      manager.GetFenceService().WaitSignaled(lpHandles[ret - WAIT_OBJECT_0]);
    }
  }
  return ret;
}

DWORD WaitForMultipleObjectsEx(
    DWORD nCount, const HANDLE* lpHandles, BOOL bWaitAll, DWORD dwMilliseconds, BOOL bAlertable) {

  auto& manager = CaptureManager::Get();
  DWORD ret = manager.GetKernel32DispatchTable().WaitForMultipleObjectsEx(
      nCount, lpHandles, bWaitAll, dwMilliseconds, bAlertable);

  if (ret < WAIT_OBJECT_0 + nCount) {
    if (bWaitAll) {
      manager.GetFenceService().WaitSignaled(nCount, lpHandles);
    } else {
      manager.GetFenceService().WaitSignaled(lpHandles[ret - WAIT_OBJECT_0]);
    }
  }
  return ret;
}

DWORD SignalObjectAndWait(HANDLE hObjectToSignal,
                          HANDLE hObjectToWaitOn,
                          DWORD dwMilliseconds,
                          BOOL bAlertable) {

  auto& manager = CaptureManager::Get();
  auto& fenceService = manager.GetFenceService();
  fenceService.WaitSignaled(hObjectToSignal);

  DWORD ret = manager.GetKernel32DispatchTable().SignalObjectAndWait(
      hObjectToSignal, hObjectToWaitOn, dwMilliseconds, bAlertable);

  if (ret == WAIT_OBJECT_0) {
    fenceService.WaitSignaled(hObjectToWaitOn);
  }

  return ret;
}

HMODULE MyLoadLibraryA(LPCSTR lpLibFileName) {
  HMODULE hModule = CaptureManager::Get().GetKernel32DispatchTable().LoadLibraryA(lpLibFileName);
  if (!hModule) {
    return hModule;
  }

  std::string libStr = lpLibFileName;
  std::transform(libStr.begin(), libStr.end(), libStr.begin(), ::tolower);
  if (libStr.find("libxess.dll") != std::string::npos) {
    CaptureManager::Get().InterceptXessFunctions();
  } else if (libStr.find("libxell.dll") != std::string::npos) {
    CaptureManager::Get().InterceptXellFunctions();
  } else if (libStr.find("libxess_fg.dll") != std::string::npos) {
    CaptureManager::Get().InterceptXefgFunctions();
  }

  return hModule;
}

HMODULE MyLoadLibraryW(LPCWSTR lpLibFileName) {
  HMODULE hModule = CaptureManager::Get().GetKernel32DispatchTable().LoadLibraryW(lpLibFileName);
  if (!hModule) {
    return hModule;
  }

  std::wstring libStr = lpLibFileName;
  std::transform(libStr.begin(), libStr.end(), libStr.begin(), ::towlower);
  if (libStr.find(L"libxess.dll") != std::string::npos) {
    CaptureManager::Get().InterceptXessFunctions();
  } else if (libStr.find(L"libxell.dll") != std::string::npos) {
    CaptureManager::Get().InterceptXellFunctions();
  } else if (libStr.find(L"libxess_fg.dll") != std::string::npos) {
    CaptureManager::Get().InterceptXefgFunctions();
  }

  return hModule;
}

HMODULE MyLoadLibraryExA(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags) {
  HMODULE hModule = CaptureManager::Get().GetKernel32DispatchTable().LoadLibraryExA(lpLibFileName,
                                                                                    hFile, dwFlags);
  if (!hModule) {
    return hModule;
  }

  std::string libStr = lpLibFileName;
  std::transform(libStr.begin(), libStr.end(), libStr.begin(), ::tolower);
  if (libStr.find("libxess.dll") != std::string::npos) {
    CaptureManager::Get().InterceptXessFunctions();
  } else if (libStr.find("libxell.dll") != std::string::npos) {
    CaptureManager::Get().InterceptXellFunctions();
  } else if (libStr.find("libxess_fg.dll") != std::string::npos) {
    CaptureManager::Get().InterceptXefgFunctions();
  }

  return hModule;
}

HMODULE MyLoadLibraryExW(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags) {
  HMODULE hModule = CaptureManager::Get().GetKernel32DispatchTable().LoadLibraryExW(lpLibFileName,
                                                                                    hFile, dwFlags);
  if (!hModule) {
    return hModule;
  }

  std::wstring libStr = lpLibFileName;
  std::transform(libStr.begin(), libStr.end(), libStr.begin(), ::towlower);
  if (libStr.find(L"libxess.dll") != std::string::npos) {
    CaptureManager::Get().InterceptXessFunctions();
  } else if (libStr.find(L"libxell.dll") != std::string::npos) {
    CaptureManager::Get().InterceptXellFunctions();
  } else if (libStr.find(L"libxess_fg.dll") != std::string::npos) {
    CaptureManager::Get().InterceptXefgFunctions();
  }

  return hModule;
}

} // namespace DirectX
} // namespace gits
