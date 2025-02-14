// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <map>

namespace gits {
namespace DirectX {

class HeapAllocationService {
public:
  void createHeapAllocation(void* captureAddress, void* data, size_t size);
  void* getHeapAllocation(void* captureAddress);
  void destroyHeapAllocation(void* captureAddress);

private:
  std::map<void*, void*> heapAllocationsByCaptureAddress_;
  std::map<void*, void*> heapAllocationsByReplayAddress_;
};

} // namespace DirectX
} // namespace gits
