// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "objectInfos.h"
#include "playerManager.h"

namespace gits {
namespace DirectX {

HeapObjectInfo::~HeapObjectInfo() {
  try {
    if (replayHeapAllocationAddress) {
      PlayerManager::get().getHeapAllocationService().destroyHeapAllocation(
          replayHeapAllocationAddress);
    }
  } catch (...) {
    topmost_exception_handler("HeapObjectInfo::~HeapObjectInfo()");
  }
}

} // namespace DirectX
} // namespace gits
