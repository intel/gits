// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "objectInfo.h"
#include "directx.h"
#include "gits.h"

#include <unordered_map>

namespace gits {
namespace DirectX {

class RootSignatureObjectInfo : public ObjectInfo {
public:
  void setDescriptorTableHeapType(unsigned parameterIndex, D3D12_DESCRIPTOR_HEAP_TYPE type) {
    descriptorTableHeapTypeByParameterIndex_[parameterIndex] = type;
  }
  D3D12_DESCRIPTOR_HEAP_TYPE getDescriptorTableHeapType(unsigned parameterIndex) {
    auto it = descriptorTableHeapTypeByParameterIndex_.find(parameterIndex);
    GITS_ASSERT(it != descriptorTableHeapTypeByParameterIndex_.end());
    return it->second;
  }

private:
  std::unordered_map<unsigned, D3D12_DESCRIPTOR_HEAP_TYPE> descriptorTableHeapTypeByParameterIndex_;
};

class CommandListObjectInfo : public ObjectInfo {
public:
  RootSignatureObjectInfo* graphicsRootSignatureInfo{nullptr};
  RootSignatureObjectInfo* computeRootSignatureInfo{nullptr};
};

} // namespace DirectX
} // namespace gits
