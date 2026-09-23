// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

// Host-only encode -> decode -> compare over the struct members the generated coders reach
// through a pointer-to-pointer (`const T* const*`). Those members need an offset table in the
// blob: the pointers the application handed us are meaningless in the player process, and the
// struct copied into the blob still holds the application's table pointer, so nothing can be
// written through it. Getting that wrong is silent - the decoded values are simply someone
// else's bytes - which is why it is worth a test that needs no driver.
//
// Covers every struct in vk.xml that currently hits those branches:
//   VkAccelerationStructureBuildGeometryInfoKHR::ppGeometries          (complex_struct)
//   VkMicromapBuildInfoEXT::ppUsageCounts                              (basic_struct)
//   VkAccelerationStructureTrianglesOpacityMicromapEXT::ppUsageCounts  (basic_struct)

#include "argumentCoders.h"
#include "argumentCodersAuto.h"

#include <cstdio>
#include <cstring>
#include <vector>

namespace {

int g_Failures = 0;

void Check(bool condition, const char* what) {
  if (!condition) {
    std::printf("FAIL: %s\n", what);
    ++g_Failures;
  }
}

// Encode src into a blob sized by GetSize, then decode it back in place. Returns the blob,
// which owns everything the decoded struct points at, so it must outlive the caller's use.
template <typename T>
std::vector<char> RoundTrip(const T& src, T*& outDecoded) {
  using gits::vulkan::Decode;
  using gits::vulkan::Encode;
  using gits::vulkan::GetSize;

  const uint32_t size = GetSize(&src, 1);
  Check(size >= sizeof(T), "GetSize covers at least the struct itself");
  std::vector<char> blob(size, '\0');

  uint32_t encodeOffset = 0;
  Encode(&src, 1, blob.data(), encodeOffset);
  Check(encodeOffset <= size, "Encode stays within the size GetSize reserved");

  outDecoded = reinterpret_cast<T*>(blob.data());
  uint32_t decodeOffset = 0;
  Decode(outDecoded, 1, blob.data(), decodeOffset);
  Check(decodeOffset == encodeOffset, "Decode consumes exactly what Encode produced");
  return blob;
}

VkMicromapUsageEXT MakeUsage(uint32_t count, uint32_t subdivisionLevel, uint32_t format) {
  VkMicromapUsageEXT usage{};
  usage.count = count;
  usage.subdivisionLevel = subdivisionLevel;
  usage.format = format;
  return usage;
}

bool SameUsage(const VkMicromapUsageEXT& a, const VkMicromapUsageEXT& b) {
  return a.count == b.count && a.subdivisionLevel == b.subdivisionLevel && a.format == b.format;
}

// VkMicromapBuildInfoEXT::ppUsageCounts - the basic_struct pointer-to-pointer branch. The old
// encoder memcpy'd sizeof(VkMicromapUsageEXT) * count bytes straight out of an 8-byte-per-entry
// pointer table, both over-reading the heap and storing pointer bit patterns where values go.
void TestMicromapBuildInfoPpUsageCounts() {
  const VkMicromapUsageEXT u0 = MakeUsage(11, 2, 0);
  const VkMicromapUsageEXT u1 = MakeUsage(22, 3, 1);
  const VkMicromapUsageEXT u2 = MakeUsage(33, 4, 0);
  const VkMicromapUsageEXT* table[3] = {&u0, &u1, &u2};

  VkMicromapBuildInfoEXT src{};
  src.sType = VK_STRUCTURE_TYPE_MICROMAP_BUILD_INFO_EXT;
  src.type = VK_MICROMAP_TYPE_OPACITY_MICROMAP_EXT;
  src.mode = VK_BUILD_MICROMAP_MODE_BUILD_EXT;
  src.usageCountsCount = 3;
  src.ppUsageCounts = table;
  src.data.deviceAddress = 0x1000;
  src.triangleArray.deviceAddress = 0x2000;
  src.triangleArrayStride = sizeof(VkMicromapTriangleEXT);

  VkMicromapBuildInfoEXT* decoded = nullptr;
  std::vector<char> blob = RoundTrip(src, decoded);

  Check(decoded->usageCountsCount == 3, "ppUsageCounts: count round-trips");
  Check(decoded->pUsageCounts == nullptr, "ppUsageCounts: the unused flat array stays null");
  Check(decoded->ppUsageCounts != nullptr, "ppUsageCounts: table pointer is non-null");
  Check(decoded->ppUsageCounts != table,
        "ppUsageCounts: table points into the blob, not at the source table");
  if (decoded->ppUsageCounts) {
    Check(SameUsage(*decoded->ppUsageCounts[0], u0), "ppUsageCounts[0] round-trips");
    Check(SameUsage(*decoded->ppUsageCounts[1], u1), "ppUsageCounts[1] round-trips");
    Check(SameUsage(*decoded->ppUsageCounts[2], u2), "ppUsageCounts[2] round-trips");
    for (uint32_t i = 0; i < 3; ++i) {
      const char* slot = reinterpret_cast<const char*>(decoded->ppUsageCounts[i]);
      Check(slot >= blob.data() && slot < blob.data() + blob.size(),
            "ppUsageCounts entry points inside the blob");
    }
  }

  // The flat form must keep working unchanged - VUID-VkMicromapBuildInfoEXT-pUsageCounts-07516
  // makes exactly one of the two non-null, and this is the one the capture path prefers.
  const VkMicromapUsageEXT flat[2] = {MakeUsage(44, 1, 0), MakeUsage(55, 5, 1)};
  VkMicromapBuildInfoEXT flatSrc{};
  flatSrc.sType = VK_STRUCTURE_TYPE_MICROMAP_BUILD_INFO_EXT;
  flatSrc.usageCountsCount = 2;
  flatSrc.pUsageCounts = flat;

  VkMicromapBuildInfoEXT* flatDecoded = nullptr;
  std::vector<char> flatBlob = RoundTrip(flatSrc, flatDecoded);
  Check(flatDecoded->ppUsageCounts == nullptr, "pUsageCounts: the unused table stays null");
  Check(flatDecoded->pUsageCounts != nullptr, "pUsageCounts: array pointer is non-null");
  if (flatDecoded->pUsageCounts) {
    Check(SameUsage(flatDecoded->pUsageCounts[0], flat[0]), "pUsageCounts[0] round-trips");
    Check(SameUsage(flatDecoded->pUsageCounts[1], flat[1]), "pUsageCounts[1] round-trips");
  }
}

// VkAccelerationStructureTrianglesOpacityMicromapEXT::ppUsageCounts - same branch, and the
// struct the opacity micromap geometry extension chains onto a triangles geometry.
void TestOpacityMicromapPpUsageCounts() {
  const VkMicromapUsageEXT u0 = MakeUsage(7, 2, 0);
  const VkMicromapUsageEXT* table[1] = {&u0};

  VkAccelerationStructureTrianglesOpacityMicromapEXT src{};
  src.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_TRIANGLES_OPACITY_MICROMAP_EXT;
  src.indexType = VK_INDEX_TYPE_UINT32;
  src.indexBuffer.deviceAddress = 0xBEEF0000;
  src.indexStride = 4;
  src.baseTriangle = 2;
  src.usageCountsCount = 1;
  src.ppUsageCounts = table;

  VkAccelerationStructureTrianglesOpacityMicromapEXT* decoded = nullptr;
  std::vector<char> blob = RoundTrip(src, decoded);

  Check(decoded->indexType == VK_INDEX_TYPE_UINT32, "OMM geometry: indexType round-trips");
  Check(decoded->indexBuffer.deviceAddress == 0xBEEF0000,
        "OMM geometry: indexBuffer address round-trips");
  Check(decoded->indexStride == 4, "OMM geometry: indexStride round-trips");
  Check(decoded->baseTriangle == 2, "OMM geometry: baseTriangle round-trips");
  Check(decoded->ppUsageCounts != nullptr && decoded->ppUsageCounts != table,
        "OMM geometry: ppUsageCounts table points into the blob");
  if (decoded->ppUsageCounts && decoded->ppUsageCounts[0]) {
    Check(SameUsage(*decoded->ppUsageCounts[0], u0), "OMM geometry: ppUsageCounts[0] round-trips");
  }
}

// VkAccelerationStructureBuildGeometryInfoKHR::ppGeometries - the complex_struct branch, and
// the one that was already reachable before opacity micromap support: the old encoder wrote its
// offsets through the *application's* geometry table.
void TestAsBuildGeometryInfoPpGeometries() {
  VkAccelerationStructureGeometryKHR g0{};
  g0.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
  g0.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
  g0.geometry.triangles.sType =
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
  g0.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
  g0.geometry.triangles.vertexData.deviceAddress = 0xAAAA0000;
  g0.geometry.triangles.vertexStride = 12;
  g0.geometry.triangles.maxVertex = 99;
  g0.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
  g0.geometry.triangles.indexData.deviceAddress = 0xBBBB0000;

  VkAccelerationStructureGeometryKHR g1{};
  g1.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
  g1.geometryType = VK_GEOMETRY_TYPE_AABBS_KHR;
  g1.geometry.aabbs.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
  g1.geometry.aabbs.data.deviceAddress = 0xCCCC0000;
  g1.geometry.aabbs.stride = 24;

  const VkAccelerationStructureGeometryKHR* table[2] = {&g0, &g1};

  VkAccelerationStructureBuildGeometryInfoKHR src{};
  src.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
  src.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
  src.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
  src.geometryCount = 2;
  src.ppGeometries = table;
  src.scratchData.deviceAddress = 0xDDDD0000;

  // Snapshot the application's table so the encoder writing through it would be visible.
  const VkAccelerationStructureGeometryKHR* tableBefore[2] = {table[0], table[1]};

  VkAccelerationStructureBuildGeometryInfoKHR* decoded = nullptr;
  std::vector<char> blob = RoundTrip(src, decoded);

  Check(tableBefore[0] == table[0] && tableBefore[1] == table[1],
        "ppGeometries: Encode does not write through the application's geometry table");
  Check(decoded->geometryCount == 2, "ppGeometries: geometryCount round-trips");
  Check(decoded->pGeometries == nullptr, "ppGeometries: the unused flat array stays null");
  Check(decoded->ppGeometries != nullptr && decoded->ppGeometries != table,
        "ppGeometries: table points into the blob");
  Check(decoded->scratchData.deviceAddress == 0xDDDD0000, "ppGeometries: scratchData round-trips");
  if (decoded->ppGeometries && decoded->ppGeometries[0] && decoded->ppGeometries[1]) {
    const auto& d0 = *decoded->ppGeometries[0];
    Check(d0.geometryType == VK_GEOMETRY_TYPE_TRIANGLES_KHR, "ppGeometries[0] geometryType");
    Check(d0.geometry.triangles.vertexData.deviceAddress == 0xAAAA0000,
          "ppGeometries[0] vertexData address");
    Check(d0.geometry.triangles.indexData.deviceAddress == 0xBBBB0000,
          "ppGeometries[0] indexData address");
    Check(d0.geometry.triangles.maxVertex == 99, "ppGeometries[0] maxVertex");
    const auto& d1 = *decoded->ppGeometries[1];
    Check(d1.geometryType == VK_GEOMETRY_TYPE_AABBS_KHR, "ppGeometries[1] geometryType");
    Check(d1.geometry.aabbs.data.deviceAddress == 0xCCCC0000, "ppGeometries[1] aabbs address");
    Check(d1.geometry.aabbs.stride == 24, "ppGeometries[1] aabbs stride");
  }

  // A null entry in the table must survive as null, not as an offset into unrelated bytes.
  const VkAccelerationStructureGeometryKHR* sparse[2] = {&g0, nullptr};
  VkAccelerationStructureBuildGeometryInfoKHR sparseSrc = src;
  sparseSrc.ppGeometries = sparse;

  VkAccelerationStructureBuildGeometryInfoKHR* sparseDecoded = nullptr;
  std::vector<char> sparseBlob = RoundTrip(sparseSrc, sparseDecoded);
  Check(sparseDecoded->ppGeometries && sparseDecoded->ppGeometries[0] != nullptr,
        "ppGeometries: populated entry survives alongside a null one");
  Check(sparseDecoded->ppGeometries && sparseDecoded->ppGeometries[1] == nullptr,
        "ppGeometries: null entry stays null");
}

// An opacity micromap geometry chained onto a triangles geometry, itself reached through
// ppGeometries: both pointer-to-pointer branches nested, which is the shape a real
// VK_EXT_opacity_micromap build has.
void TestOpacityMicromapInPNextOfPpGeometries() {
  const VkMicromapUsageEXT u0 = MakeUsage(64, 3, 0);
  const VkMicromapUsageEXT* usageTable[1] = {&u0};

  VkAccelerationStructureTrianglesOpacityMicromapEXT omm{};
  omm.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_TRIANGLES_OPACITY_MICROMAP_EXT;
  omm.indexType = VK_INDEX_TYPE_UINT16;
  omm.indexBuffer.deviceAddress = 0x5150;
  omm.indexStride = 2;
  omm.usageCountsCount = 1;
  omm.ppUsageCounts = usageTable;

  VkAccelerationStructureGeometryKHR g0{};
  g0.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
  g0.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
  g0.geometry.triangles.sType =
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
  g0.geometry.triangles.pNext = &omm;
  g0.geometry.triangles.vertexData.deviceAddress = 0x7000;
  g0.geometry.triangles.vertexStride = 12;
  g0.geometry.triangles.indexType = VK_INDEX_TYPE_NONE_KHR;

  const VkAccelerationStructureGeometryKHR* table[1] = {&g0};

  VkAccelerationStructureBuildGeometryInfoKHR src{};
  src.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
  src.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
  src.geometryCount = 1;
  src.ppGeometries = table;

  VkAccelerationStructureBuildGeometryInfoKHR* decoded = nullptr;
  std::vector<char> blob = RoundTrip(src, decoded);

  const VkAccelerationStructureTrianglesOpacityMicromapEXT* decodedOmm = nullptr;
  if (decoded->ppGeometries && decoded->ppGeometries[0]) {
    for (const auto* node = reinterpret_cast<const VkBaseInStructure*>(
             decoded->ppGeometries[0]->geometry.triangles.pNext);
         node; node = node->pNext) {
      if (node->sType == VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_TRIANGLES_OPACITY_MICROMAP_EXT) {
        decodedOmm =
            reinterpret_cast<const VkAccelerationStructureTrianglesOpacityMicromapEXT*>(node);
        break;
      }
    }
  }
  Check(decodedOmm != nullptr, "nested: the OMM node survives inside ppGeometries[0]'s pNext");
  if (decodedOmm) {
    Check(decodedOmm->indexBuffer.deviceAddress == 0x5150, "nested: OMM indexBuffer address");
    Check(decodedOmm->indexStride == 2, "nested: OMM indexStride");
    Check(decodedOmm->usageCountsCount == 1, "nested: OMM usageCountsCount");
    Check(decodedOmm->ppUsageCounts != nullptr && decodedOmm->ppUsageCounts != usageTable,
          "nested: OMM ppUsageCounts table points into the blob");
    if (decodedOmm->ppUsageCounts && decodedOmm->ppUsageCounts[0]) {
      Check(SameUsage(*decodedOmm->ppUsageCounts[0], u0), "nested: OMM ppUsageCounts[0]");
    }
  }
}

} // namespace

int main() {
  TestMicromapBuildInfoPpUsageCounts();
  TestOpacityMicromapPpUsageCounts();
  TestAsBuildGeometryInfoPpGeometries();
  TestOpacityMicromapInPNextOfPpGeometries();

  if (g_Failures != 0) {
    std::printf("coderRoundTripTest: %d check(s) failed\n", g_Failures);
    return 1;
  }
  std::printf("coderRoundTripTest: all checks passed\n");
  return 0;
}
