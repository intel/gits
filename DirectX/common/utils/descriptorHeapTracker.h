// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandsAuto.h"

#include <map>
#include <memory>

namespace gits {
namespace DirectX {

class DescriptorHeapTracker {
public:
  struct DescriptorInfo {
    unsigned heapKey{};
    unsigned descriptorIndex{};
    unsigned resourceKey{};
    enum DescriptorType {
      Unknown,
      RTV,
      DSV,
      SRV,
      UAV,
      CBV,
      Sampler
    } descriptorType{};
  };

  void createDescriptor(DescriptorInfo* descriptorInfo);
  void destroyObject(unsigned key);
  void copyDescriptors(ID3D12DeviceCopyDescriptorsSimpleCommand& c);
  void copyDescriptors(ID3D12DeviceCopyDescriptorsCommand& c);
  DescriptorInfo* getDescriptorInfo(unsigned heapKey, unsigned descriptorIndex) {
    return descriptorsByHeapIndex_[heapKey][descriptorIndex].get();
  }

private:
  DescriptorInfo* copyDescriptor(DescriptorInfo* state,
                                 unsigned destHeapKey,
                                 unsigned destHeapIndex);

private:
  std::map<unsigned, std::map<unsigned, std::unique_ptr<DescriptorInfo>>> descriptorsByHeapIndex_;
};

} // namespace DirectX
} // namespace gits
