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
    unsigned HeapKey{};
    unsigned DescriptorIndex{};
    unsigned ResourceKey{};
    enum class DescriptorKind {
      Unknown,
      RTV,
      DSV,
      SRV,
      UAV,
      CBV,
      Sampler
    };
    DescriptorKind Kind{};
  };

  void CreateDescriptor(DescriptorInfo* descriptorInfo);
  void DestroyObject(unsigned key);
  void CopyDescriptors(ID3D12DeviceCopyDescriptorsSimpleCommand& command);
  void CopyDescriptors(ID3D12DeviceCopyDescriptorsCommand& command);
  DescriptorInfo* GetDescriptorInfo(unsigned heapKey, unsigned descriptorIndex) {
    return m_DescriptorsByHeapIndex[heapKey][descriptorIndex].get();
  }

private:
  DescriptorInfo* CopyDescriptor(DescriptorInfo* state,
                                 unsigned destHeapKey,
                                 unsigned destDescriptorIndex);

private:
  std::map<unsigned, std::map<unsigned, std::unique_ptr<DescriptorInfo>>> m_DescriptorsByHeapIndex;
};

} // namespace DirectX
} // namespace gits
