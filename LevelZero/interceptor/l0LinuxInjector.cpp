// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "gitsPluginL0.h"
#include "l0RecorderWrapperIface.h"
#include "platform.h"

#include <dlfcn.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#if defined(__cplusplus)
extern "C" {
#endif
typedef ssize_t (*real_read_t)(int, void*, size_t);
ssize_t real_read(int fd, void* buf, size_t count) {
  return ((real_read_t)dlsym(RTLD_NEXT, "read"))(fd, buf, count);
}

VISIBLE ssize_t read(int fd, void* buf, size_t count) {
  const auto& wrapper = gits::l0::CGitsPlugin::RecorderWrapper();
  ssize_t amount_read = 0;
  if (gits::l0::CGitsPlugin::Initialized() && wrapper.IsMemorySnifferInstalled()) {
    wrapper.UnProtectMemoryRegion(buf);
    amount_read = real_read(fd, buf, count);
    wrapper.ProtectMemoryRegion(buf);
  } else {
    amount_read = real_read(fd, buf, count);
  }
  return amount_read;
}

#if defined(__cplusplus)
}
#endif
