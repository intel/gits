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
#include <functional>
#include <vector>

namespace gits {
namespace vulkan {

// Sparse buffers whose recorded GPU virtual address range is not available on the
// replay device.
//
// A sparse buffer with an opaque capture address reserves its recorded range when
// it is created. When the replay driver's address space doesn't cover that range,
// the buffer can't keep its recorded address, and a driver may also fail to report
// it (Mesa ANV then binds the buffer at address 0, over its own memory). Whether the
// range is available is tested before the buffer is created, by allocating memory
// with the capture addresses of the first and the last page of the range: memory
// allocations report unavailable addresses reliably.
//
// A buffer that can't be placed is created without its capture address. Its new
// device address replaces the recorded one in the shader binding table regions of
// vkCmdTraceRaysKHR and vkCmdTraceRaysIndirectKHR. Addresses of the buffer stored
// in other places (GPU memory, push constants) are not translated.
class SparseBufferRelocation {
public:
  explicit SparseBufferRelocation(DispatchTablesHolder& dispatchTablesHolder)
      : m_DispatchTablesHolder(dispatchTablesHolder) {}

  void SetPhysicalDevice(VkPhysicalDevice physicalDevice);
  // Drop the capture address of a sparse buffer whose recorded range isn't available.
  // toMemoryCaptureAddress converts a recorded address to the form accepted by the
  // replay driver for memory allocations. Returns true when the buffer is relocated.
  bool PrepareBuffer(VkDevice device,
                     VkBufferCreateInfo& createInfo,
                     const std::function<uint64_t(uint64_t)>& toMemoryCaptureAddress);
  // Learn the new address of the buffer prepared by the last PrepareBuffer call.
  void AddBuffer(VkDevice device, VkBuffer buffer);
  void RemoveBuffer(VkBuffer buffer);
  void TranslateRegion(VkStridedDeviceAddressRegionKHR* region);
  void DestroyDevice();

private:
  struct Relocation {
    VkBuffer Buffer{VK_NULL_HANDLE};
    VkDeviceAddress Recorded{0};
    VkDeviceSize Size{0};
    VkDeviceAddress Replayed{0};
  };

  bool IsRangeAvailable(VkDevice device, uint64_t memoryCaptureAddress, VkDeviceSize size);
  bool IsAddressAvailable(VkDevice device, uint64_t memoryCaptureAddress);
  uint32_t ProbeMemoryType(VkDevice device);

  DispatchTablesHolder& m_DispatchTablesHolder;
  VkPhysicalDevice m_PhysicalDevice{VK_NULL_HANDLE};
  uint32_t m_ProbeMemoryType{UINT32_MAX};
  bool m_ProbeMemoryTypeSelected{false};
  std::vector<Relocation> m_Relocations;
  // Buffer prepared by the vkCreateBuffer call in flight
  Relocation m_Pending;
  uint64_t m_TranslatedRegionCount{0};
};

} // namespace vulkan
} // namespace gits
