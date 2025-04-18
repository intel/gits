// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "objectInfo.h"

#include <windows.h>
#include "tools_lite.h"

namespace gits {
namespace DirectX {

class HeapObjectInfo : public ObjectInfo, gits::noncopyable {
public:
  HeapObjectInfo() = default;
  ~HeapObjectInfo();
  void* replayHeapAllocationAddress{nullptr};
};

class FenceObjectInfo : public ObjectInfo {
public:
  UINT64 lastSignaledValue{};
  bool signaled{false};
};

} // namespace DirectX
} // namespace gits
