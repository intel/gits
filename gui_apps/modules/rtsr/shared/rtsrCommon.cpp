// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "rtsrCommon.h"

#include "xxhash.h"

namespace gits::rtsr {

uint64_t ComputeBytecodeHash(const void* bytecode, size_t size) {
  return XXH64(bytecode, size, 0);
}

} // namespace gits::rtsr
