// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "dispatchTablesHolder.h"

#include <cstdint>

namespace gits {
namespace vulkan {
namespace portability {

inline VkDeviceSize AlignUp(VkDeviceSize value, VkDeviceSize alignment) {
  return (value + alignment - 1) / alignment * alignment;
}

// GPU virtual addresses have 48 bits. Device addresses are canonical (sign-extended),
// opaque capture addresses may be zero-extended.
constexpr uint32_t ADDRESS_BITS = 48;

inline uint64_t ToBits48Address(uint64_t address) {
  return address & ((1ull << ADDRESS_BITS) - 1);
}

inline uint64_t ToCanonicalAddress(uint64_t address) {
  constexpr uint32_t SHIFT = 64 - ADDRESS_BITS;
  return static_cast<uint64_t>(static_cast<int64_t>(address << SHIFT) >> SHIFT);
}

// First device-local memory type out of memoryTypeBits, UINT32_MAX when there is none.
uint32_t FindDeviceLocalMemoryType(DispatchTablesHolder& dispatchTablesHolder,
                                   VkPhysicalDevice physicalDevice,
                                   uint32_t memoryTypeBits);

// GITS-owned buffer bound to its own device-local memory allocation.
struct DeviceLocalBuffer {
  VkBuffer Buffer{VK_NULL_HANDLE};
  VkDeviceMemory Memory{VK_NULL_HANDLE};
  VkDeviceSize AllocationSize{0};
};

// Memory of buffers with VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT is allocated with
// VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT.
bool CreateDeviceLocalBuffer(DispatchTablesHolder& dispatchTablesHolder,
                             VkPhysicalDevice physicalDevice,
                             VkDevice device,
                             VkDeviceSize size,
                             VkBufferUsageFlags usage,
                             DeviceLocalBuffer& buffer);
void DestroyDeviceLocalBuffer(DispatchTablesHolder& dispatchTablesHolder,
                              VkDevice device,
                              DeviceLocalBuffer& buffer);

// vkGetBufferDeviceAddress or its KHR alias, nullptr when the device has neither.
PFN_vkGetBufferDeviceAddress GetBufferDeviceAddressFunction(
    const VkDeviceLevelDispatchTable& dispatchTable);

} // namespace portability
} // namespace vulkan
} // namespace gits
