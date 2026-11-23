// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "raytracingBuildInputsHash.h"

namespace gits {
namespace DirectX {

size_t RaytracingBuildInputsHash::operator()(
    const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs) const {
  size_t hash = 0;
  auto mix = [&hash](uint64_t value) {
    const size_t v = static_cast<size_t>(value);
    hash ^= v + 0x9e3779b97f4a7c15ULL + (hash << 6) + (hash >> 2);
  };

  auto appendValue = [&mix]<typename T>(T value) { mix(static_cast<uint64_t>(value)); };
  auto appendOptionalGpuAddressInUse = [&mix](D3D12_GPU_VIRTUAL_ADDRESS address) {
    mix(address != 0 ? 1u : 0u);
  };

  const auto hashTriangles = [&appendValue, &appendOptionalGpuAddressInUse](
                                 const D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC& desc) {
    appendOptionalGpuAddressInUse(desc.Transform3x4);
    appendValue(desc.IndexFormat);
    appendValue(desc.VertexFormat);
    appendValue(desc.IndexCount);
    appendValue(desc.VertexCount);
  };

  const auto hashAabbs = [&appendValue](const D3D12_RAYTRACING_GEOMETRY_AABBS_DESC& desc) {
    appendValue(desc.AABBCount);
  };

  const auto hashOmmLinkage =
      [&appendValue](const D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC& desc) {
        appendValue(desc.OpacityMicromapIndexFormat);
        appendValue(desc.OpacityMicromapBaseLocation);
      };

  const auto hashGeometry = [&appendValue, &hashTriangles, &hashAabbs,
                             &hashOmmLinkage](const D3D12_RAYTRACING_GEOMETRY_DESC& desc) {
    appendValue(desc.Type);
    appendValue(desc.Flags);
    switch (desc.Type) {
    case D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES:
      hashTriangles(desc.Triangles);
      break;
    case D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS:
      hashAabbs(desc.AABBs);
      break;
    case D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES:
      appendValue(desc.OmmTriangles.pTriangles != nullptr ? 1u : 0u);
      if (desc.OmmTriangles.pTriangles) {
        hashTriangles(*desc.OmmTriangles.pTriangles);
      }
      appendValue(desc.OmmTriangles.pOmmLinkage != nullptr ? 1u : 0u);
      if (desc.OmmTriangles.pOmmLinkage) {
        hashOmmLinkage(*desc.OmmTriangles.pOmmLinkage);
      }
      break;
    default:
      break;
    }
  };

  appendValue(inputs.Type);
  appendValue(inputs.Flags & ~D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE);
  appendValue(inputs.NumDescs);
  appendValue(inputs.DescsLayout);

  if (inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
    for (unsigned i = 0; i < inputs.NumDescs; ++i) {
      if (inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS) {
        appendValue(inputs.ppGeometryDescs != nullptr && inputs.ppGeometryDescs[i] != nullptr ? 1u
                                                                                              : 0u);
        if (!inputs.ppGeometryDescs || !inputs.ppGeometryDescs[i]) {
          continue;
        }
        hashGeometry(*inputs.ppGeometryDescs[i]);
      } else {
        appendValue(inputs.pGeometryDescs != nullptr ? 1u : 0u);
        if (!inputs.pGeometryDescs) {
          continue;
        }
        hashGeometry(inputs.pGeometryDescs[i]);
      }
    }
  } else if (inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_OPACITY_MICROMAP_ARRAY) {
    appendValue(inputs.pOpacityMicromapArrayDesc != nullptr ? 1u : 0u);
    if (inputs.pOpacityMicromapArrayDesc) {
      const D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC& desc = *inputs.pOpacityMicromapArrayDesc;
      appendValue(desc.NumOmmHistogramEntries);
      appendValue(desc.pOmmHistogram != nullptr ? 1u : 0u);
      if (desc.pOmmHistogram) {
        for (unsigned i = 0; i < desc.NumOmmHistogramEntries; ++i) {
          appendValue(desc.pOmmHistogram[i].Count);
          appendValue(desc.pOmmHistogram[i].SubdivisionLevel);
          appendValue(desc.pOmmHistogram[i].Format);
        }
      }
    }
  }

  return hash;
}

size_t RaytracingBuildInputsHash::operator()(
    const PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS>& inputs) const {
  return operator()(*inputs.Value);
}

bool RaytracingBuildInputsEqual::operator()(
    const PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS>& lhsPtr,
    const PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS>& rhsPtr) const {
  const auto lhs = *lhsPtr.Value;
  const auto rhs = *rhsPtr.Value;
  auto gpuAddressInUseEqual = [](D3D12_GPU_VIRTUAL_ADDRESS a, D3D12_GPU_VIRTUAL_ADDRESS b) {
    return (a != 0) == (b != 0);
  };

  const auto equalTriangles =
      [&gpuAddressInUseEqual](const D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC& a,
                              const D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC& b) {
        return gpuAddressInUseEqual(a.Transform3x4, b.Transform3x4) &&
               a.IndexFormat == b.IndexFormat && a.VertexFormat == b.VertexFormat &&
               a.IndexCount == b.IndexCount && a.VertexCount == b.VertexCount;
      };

  const auto equalAabbs = [](const D3D12_RAYTRACING_GEOMETRY_AABBS_DESC& a,
                             const D3D12_RAYTRACING_GEOMETRY_AABBS_DESC& b) {
    return a.AABBCount == b.AABBCount;
  };

  const auto equalOmmLinkage = [](const D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC& a,
                                  const D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC& b) {
    return a.OpacityMicromapIndexFormat == b.OpacityMicromapIndexFormat &&
           a.OpacityMicromapBaseLocation == b.OpacityMicromapBaseLocation;
  };

  const auto equalGeometry = [&equalTriangles, &equalAabbs,
                              &equalOmmLinkage](const D3D12_RAYTRACING_GEOMETRY_DESC& a,
                                                const D3D12_RAYTRACING_GEOMETRY_DESC& b) {
    if (a.Type != b.Type || a.Flags != b.Flags) {
      return false;
    }
    switch (a.Type) {
    case D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES:
      return equalTriangles(a.Triangles, b.Triangles);
    case D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS:
      return equalAabbs(a.AABBs, b.AABBs);
    case D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES:
      if ((a.OmmTriangles.pTriangles != nullptr) != (b.OmmTriangles.pTriangles != nullptr)) {
        return false;
      }
      if (a.OmmTriangles.pTriangles &&
          !equalTriangles(*a.OmmTriangles.pTriangles, *b.OmmTriangles.pTriangles)) {
        return false;
      }
      if ((a.OmmTriangles.pOmmLinkage != nullptr) != (b.OmmTriangles.pOmmLinkage != nullptr)) {
        return false;
      }
      if (a.OmmTriangles.pOmmLinkage &&
          !equalOmmLinkage(*a.OmmTriangles.pOmmLinkage, *b.OmmTriangles.pOmmLinkage)) {
        return false;
      }
      return true;
    default:
      return true;
    }
  };

  if (lhs.Type != rhs.Type) {
    return false;
  }
  const auto maskedBuildFlags = [](UINT flags) {
    return flags & ~D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
  };
  if (maskedBuildFlags(lhs.Flags) != maskedBuildFlags(rhs.Flags) || lhs.NumDescs != rhs.NumDescs ||
      lhs.DescsLayout != rhs.DescsLayout) {
    return false;
  }

  if (lhs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
    for (unsigned i = 0; i < lhs.NumDescs; ++i) {
      if (lhs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS) {
        const bool lhsHas = lhs.ppGeometryDescs != nullptr && lhs.ppGeometryDescs[i] != nullptr;
        const bool rhsHas = rhs.ppGeometryDescs != nullptr && rhs.ppGeometryDescs[i] != nullptr;
        if (lhsHas != rhsHas) {
          return false;
        }
        if (!lhsHas) {
          continue;
        }
        if (!equalGeometry(*lhs.ppGeometryDescs[i], *rhs.ppGeometryDescs[i])) {
          return false;
        }
      } else {
        const bool lhsHas = lhs.pGeometryDescs != nullptr;
        const bool rhsHas = rhs.pGeometryDescs != nullptr;
        if (lhsHas != rhsHas) {
          return false;
        }
        if (!lhsHas) {
          continue;
        }
        if (!equalGeometry(lhs.pGeometryDescs[i], rhs.pGeometryDescs[i])) {
          return false;
        }
      }
    }
  } else if (lhs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_OPACITY_MICROMAP_ARRAY) {
    const bool lhsHas = lhs.pOpacityMicromapArrayDesc != nullptr;
    const bool rhsHas = rhs.pOpacityMicromapArrayDesc != nullptr;
    if (lhsHas != rhsHas) {
      return false;
    }
    if (!lhsHas) {
      return true;
    }
    const D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC& la = *lhs.pOpacityMicromapArrayDesc;
    const D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC& ra = *rhs.pOpacityMicromapArrayDesc;
    if (la.NumOmmHistogramEntries != ra.NumOmmHistogramEntries) {
      return false;
    }
    const bool lhsHist = la.pOmmHistogram != nullptr;
    const bool rhsHist = ra.pOmmHistogram != nullptr;
    if (lhsHist != rhsHist) {
      return false;
    }
    if (lhsHist) {
      for (unsigned i = 0; i < la.NumOmmHistogramEntries; ++i) {
        if (la.pOmmHistogram[i].Count != ra.pOmmHistogram[i].Count ||
            la.pOmmHistogram[i].SubdivisionLevel != ra.pOmmHistogram[i].SubdivisionLevel ||
            la.pOmmHistogram[i].Format != ra.pOmmHistogram[i].Format) {
          return false;
        }
      }
    }
  }

  return true;
}

} // namespace DirectX
} // namespace gits
