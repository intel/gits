// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <map>
#include <unordered_map>

namespace gits {
namespace DirectX {

class HeapAllocationService {
public:
  void createHeapAllocation(unsigned heapKey, void* captureAddress, void* data, size_t size);
  void* getHeapAllocation(void* captureAddress);
  void destroyHeapAllocation(unsigned heapKey);

private:
  std::map<void*, void*> heapAllocationsByCaptureAddress_;
  std::map<void*, void*> heapAllocationsByReplayAddress_;
  std::unordered_map<unsigned, void*> heapAllocationsCaptureAddressByHeapKey_;
};

} // namespace DirectX
} // namespace gits
