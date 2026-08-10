// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "raytracingInstancesDump.h"

#include "log.h"

#include <cstring>
#include <map>
#include <set>
#include <vector>

namespace gits {
namespace vulkan {

std::vector<uint64_t> RaytracingInstancesDump::ReadReferencedBlases(
    uint64_t deviceKey,
    uint64_t physDevKey,
    uint64_t queueKey,
    uint64_t poolKey,
    VkDeviceAddress instanceArrayAddress,
    uint32_t instanceCount,
    bool arrayOfPointers) {
  if (instanceArrayAddress == 0 || instanceCount == 0) {
    return {};
  }

  auto& addressTracking = m_StateTracking.GetDeviceAddressTracking();

  // Read the entire tracked buffer containing `address` into outData plus the offset
  // of `address` within it. Returns its key, or 0 on failure. ReadBuffer is a one-shot
  // submit + wait-idle, so read each buffer at most once.
  auto readContainingBuffer = [&](VkDeviceAddress address, std::vector<uint8_t>& outData,
                                  VkDeviceSize& outOffset) -> uint64_t {
    auto found = addressTracking.FindContaining(address);
    if (!found) {
      LOG_WARNING << "Vulkan subcapture: TLAS instance address " << address
                  << " does not resolve to any tracked buffer; skipping TLAS->BLAS discovery for "
                     "this build";
      return 0;
    }
    const uint64_t bufferKey = found->first;
    outOffset = found->second;
    // Destroyed states are retained for AS storage buffers, but their handle is dead:
    // a stale device-address hit must not be read back.
    auto* bufferState = m_StateTracking.GetState<BufferState>(bufferKey);
    if (!bufferState || bufferState->Destroyed || bufferState->BufferSize == 0) {
      return 0;
    }
    if (!m_GpuReadback.ReadBuffer(deviceKey, physDevKey, queueKey, poolKey, bufferKey,
                                  /*srcOffset=*/0, bufferState->BufferSize, outData)) {
      LOG_WARNING << "Vulkan subcapture: failed to read back TLAS instance buffer key="
                  << bufferKey;
      return 0;
    }
    return bufferKey;
  };

  // Resolve one VkAccelerationStructureInstanceKHR at bytes[structOffset] into blasKeys.
  auto resolveInstance = [&](const std::vector<uint8_t>& bytes, VkDeviceSize structOffset,
                             std::set<uint64_t>& blasKeys) {
    if (structOffset + sizeof(VkAccelerationStructureInstanceKHR) > bytes.size()) {
      return;
    }
    VkAccelerationStructureInstanceKHR instance{};
    std::memcpy(&instance, bytes.data() + static_cast<size_t>(structOffset), sizeof(instance));
    if (instance.accelerationStructureReference == 0) {
      return;
    }
    auto blasKey = addressTracking.Find(instance.accelerationStructureReference);
    if (blasKey) {
      blasKeys.insert(*blasKey);
    }
  };

  std::set<uint64_t> blasKeys;

  if (!arrayOfPointers) {
    // Contiguous layout: one buffer holding `instanceCount` packed structs.
    std::vector<uint8_t> bytes;
    VkDeviceSize base = 0;
    if (!readContainingBuffer(instanceArrayAddress, bytes, base)) {
      return {};
    }
    for (uint32_t i = 0; i < instanceCount; ++i) {
      resolveInstance(
          bytes, base + static_cast<VkDeviceSize>(i) * sizeof(VkAccelerationStructureInstanceKHR),
          blasKeys);
    }
    return std::vector<uint64_t>(blasKeys.begin(), blasKeys.end());
  }

  // Array-of-pointers layout. Hop 1: read the pointer array (`instanceCount`
  // VkDeviceAddress values), each pointing to a struct elsewhere.
  std::vector<uint8_t> pointerBytes;
  VkDeviceSize pointerBase = 0;
  if (!readContainingBuffer(instanceArrayAddress, pointerBytes, pointerBase)) {
    return {};
  }
  const VkDeviceSize pointersBytes =
      static_cast<VkDeviceSize>(instanceCount) * sizeof(VkDeviceAddress);
  if (pointerBase + pointersBytes > pointerBytes.size()) {
    LOG_WARNING << "Vulkan subcapture: TLAS array-of-pointers table exceeds its buffer; skipping "
                   "TLAS->BLAS discovery for this build";
    return {};
  }

  // Hop 2: group each pointed-to struct's offset by the buffer that contains it,
  // so we read each distinct struct buffer at most once.
  std::map<uint64_t, std::set<VkDeviceSize>> offsetsByBuffer;
  for (uint32_t i = 0; i < instanceCount; ++i) {
    VkDeviceAddress ptr = 0;
    std::memcpy(&ptr,
                pointerBytes.data() + static_cast<size_t>(pointerBase) +
                    static_cast<size_t>(i) * sizeof(VkDeviceAddress),
                sizeof(ptr));
    if (ptr == 0) {
      continue;
    }
    auto found = addressTracking.FindContaining(ptr);
    if (found) {
      offsetsByBuffer[found->first].insert(found->second);
    }
  }

  for (const auto& [bufferKey, offsets] : offsetsByBuffer) {
    // See the Destroyed note in readContainingBuffer above.
    auto* bufferState = m_StateTracking.GetState<BufferState>(bufferKey);
    if (!bufferState || bufferState->Destroyed || bufferState->BufferSize == 0) {
      continue;
    }
    std::vector<uint8_t> bytes;
    if (!m_GpuReadback.ReadBuffer(deviceKey, physDevKey, queueKey, poolKey, bufferKey,
                                  /*srcOffset=*/0, bufferState->BufferSize, bytes)) {
      LOG_WARNING << "Vulkan subcapture: failed to read back TLAS instance-struct buffer key="
                  << bufferKey;
      continue;
    }
    for (VkDeviceSize structOffset : offsets) {
      resolveInstance(bytes, structOffset, blasKeys);
    }
  }

  return std::vector<uint64_t>(blasKeys.begin(), blasKeys.end());
}

} // namespace vulkan
} // namespace gits
