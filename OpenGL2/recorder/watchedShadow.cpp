// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "watchedShadow.h"

#include "log.h"

#include <windows.h>

namespace gits::opengl2 {

WatchedShadow::WatchedShadow(size_t size) : m_Size(size) {
  m_Data = VirtualAlloc(nullptr, size, MEM_RESERVE | MEM_COMMIT | MEM_WRITE_WATCH, PAGE_READWRITE);
  if (m_Data == nullptr) {
    GITS_ASSERT(false, "Cannot allocate write watch for buffer map (Windows error " +
                           std::to_string(GetLastError()) + ")");
  }
}

WatchedShadow::~WatchedShadow() {
  if (m_Data != nullptr) {
    VirtualFree(m_Data, 0, MEM_RELEASE);
  }
}

void WatchedShadow::ResetWriteWatch() const {
  if (::ResetWriteWatch(m_Data, m_Size) != 0) {
    GITS_ASSERT(false, "Cannot reset write watch for buffer map (Windows error " +
                           std::to_string(GetLastError()) + ")");
  }
}

} // namespace gits::opengl2
