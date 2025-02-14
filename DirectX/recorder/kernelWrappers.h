// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <windows.h>

namespace gits {
namespace DirectX {

DWORD WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds);

DWORD WaitForSingleObjectEx(HANDLE hHandle, DWORD dwMilliseconds, BOOL bAlertable);

DWORD WaitForMultipleObjects(DWORD nCount,
                             const HANDLE* lpHandles,
                             BOOL bWaitAll,
                             DWORD dwMilliseconds);

DWORD WaitForMultipleObjectsEx(
    DWORD nCount, const HANDLE* lpHandles, BOOL bWaitAll, DWORD dwMilliseconds, BOOL bAlertable);

DWORD SignalObjectAndWait(HANDLE hObjectToSignal,
                          HANDLE hObjectToWaitOn,
                          DWORD dwMilliseconds,
                          BOOL bAlertable);

HMODULE MyLoadLibraryA(LPCSTR lpLibFileName);

HMODULE MyLoadLibraryW(LPCWSTR lpLibFileName);

HMODULE MyLoadLibraryExA(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);

HMODULE MyLoadLibraryExW(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);

} // namespace DirectX
} // namespace gits
