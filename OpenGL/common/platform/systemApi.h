// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <cstdint>

// Operating system calls for translation units that include glTypes.h, which conflicts with the
// system headers declaring them.
namespace gits::opengl {

uint32_t CurrentThreadId();

// The calling thread's Win32 last error on Windows, errno elsewhere.
uint32_t GetLastSystemError();
void SetLastSystemError(uint32_t error);

// Restores the calling thread's last system error when it goes out of scope.
class LastSystemErrorGuard {
public:
  LastSystemErrorGuard() = default;
  ~LastSystemErrorGuard() {
    SetLastSystemError(m_Error);
  }
  LastSystemErrorGuard(const LastSystemErrorGuard&) = delete;
  LastSystemErrorGuard& operator=(const LastSystemErrorGuard&) = delete;

private:
  uint32_t m_Error = GetLastSystemError();
};

// The client-area size of the window that `dc` (an HDC) draws to, or false if it draws to none.
bool GetWindowClientSize(void* dc, int32_t& width, int32_t& height);

} // namespace gits::opengl
