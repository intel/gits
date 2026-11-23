// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once
#include "arguments.h"

#include <unordered_map>

namespace gits {
namespace DirectX {

struct RaytracingBuildInputsHash {
  size_t operator()(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs) const;
  size_t operator()(
      const PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS>& inputs) const;
};

struct RaytracingBuildInputsEqual {
  bool operator()(
      const PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS>& lhs,
      const PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS>& rhs) const;
};

template <typename Value>
using RaytracingBuildInputsMap =
    std::unordered_map<PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS>,
                       Value,
                       RaytracingBuildInputsHash,
                       RaytracingBuildInputsEqual>;

} // namespace DirectX
} // namespace gits
