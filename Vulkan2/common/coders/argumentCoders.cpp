// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "argumentCoders.h"

namespace gits {
namespace vulkan {

// ============================================================================
// Core helpers
// ============================================================================

void WriteData(const char* src, unsigned srcSize, char* dst, unsigned& offset) {
  std::memcpy(&dst[offset], src, srcSize);
  offset += srcSize;
}

// ============================================================================
// pNext chain  - dropped: encoded as nullptr, decoded as nullptr
// ============================================================================

unsigned GetPNextChainSize(const void* /*pNext*/) {
  return 0;
}

void EncodePNextChain(char* /*dst*/, uint32_t& /*offset*/, const void* /*pNext*/) {}

void DecodePNextChain(char* /*src*/, uint32_t& /*offset*/, void** pNext) {
  *pNext = nullptr;
}

// ============================================================================
// VkAllocationCallbacks - always nullptr
// ============================================================================

//uint32_t GetSize(const VkAllocationCallbacks* /*src*/, uint32_t /*count*/) {
//  return 0;
//}

//void Encode(const VkAllocationCallbacks* /*src*/,
//            uint32_t /*count*/,
//            char* /*dst*/,
//            uint32_t& /*offset*/) {}

//void Decode(const VkAllocationCallbacks* /*dst*/,
//            uint32_t /*count*/,
//            char* /*src*/,
//            uint32_t& /*offset*/) {}

// ============================================================================
// String helpers
// ============================================================================

uint32_t GetStringSize(const char* s) {
  if (!s) {
    return sizeof(uint32_t);
  }
  return sizeof(uint32_t) + static_cast<uint32_t>(std::strlen(s)) + 1u;
}

void EncodeString(const char* s, char* dst, uint32_t& offset) {
  if (!s) {
    uint32_t len = 0;
    std::memcpy(dst + offset, &len, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    return;
  }
  uint32_t len = static_cast<uint32_t>(std::strlen(s)) + 1u;
  std::memcpy(dst + offset, &len, sizeof(uint32_t));
  offset += sizeof(uint32_t);
  std::memcpy(dst + offset, s, len);
  offset += len;
}

void DecodeString(char* src, uint32_t& offset, const char** out) {
  uint32_t len = 0;
  std::memcpy(&len, src + offset, sizeof(uint32_t));
  offset += sizeof(uint32_t);
  if (len == 0) {
    *out = nullptr;
    return;
  }
  *out = src + offset;
  offset += len;
}

uint32_t GetStringArraySize(const char* const* arr, uint32_t count) {
  if (!arr || count == 0) {
    return sizeof(uint32_t);
  }
  // Layout: [uint32_t count] [const char* slot0..slotN-1 (null placeholders)] [encoded string0..N-1]
  uint32_t size = sizeof(uint32_t) + sizeof(const char*) * count;
  for (uint32_t i = 0; i < count; ++i) {
    size += GetStringSize(arr[i]);
  }
  return size;
}

void EncodeStringArray(const char* const* arr, uint32_t count, char* dst, uint32_t& offset) {
  std::memcpy(dst + offset, &count, sizeof(uint32_t));
  offset += sizeof(uint32_t);
  if (!arr || count == 0) {
    return;
  }
  // Write count null pointer placeholders — patched in-place at decode time.
  const char* null_placeholder = nullptr;
  unsigned ptrArrayOffset = offset;
  for (uint32_t i = 0; i < count; ++i) {
    std::memcpy(dst + ptrArrayOffset + i * sizeof(const char*), &null_placeholder,
                sizeof(const char*));
  }
  offset += sizeof(const char*) * count;
  // Encode each string immediately after the pointer array.
  for (uint32_t i = 0; i < count; ++i) {
    EncodeString(arr[i], dst, offset);
  }
}

// outArr: address of the struct field (e.g. &createInfo.ppEnabledExtensionNames).
// On return, *outArr points into the src buffer at the pointer-array region,
// and each pointer slot in that region is patched to point at the string data.
void DecodeStringArray(char* src, uint32_t& offset, const char*** outArr, uint32_t count) {
  uint32_t storedCount = 0;
  std::memcpy(&storedCount, src + offset, sizeof(uint32_t));
  offset += sizeof(uint32_t);
  if (storedCount == 0) {
    if (outArr) {
      *outArr = nullptr;
    }
    return;
  }
  // Point the struct field at the in-buffer pointer array.
  const char** slots = reinterpret_cast<const char**>(src + offset);
  if (outArr) {
    *outArr = slots;
  }
  offset += sizeof(const char*) * storedCount;
  // Patch each slot in-place: point it past the length header to the string bytes.
  for (uint32_t i = 0; i < storedCount; ++i) {
    uint32_t len = 0;
    std::memcpy(&len, src + offset, sizeof(uint32_t));
    slots[i] = (len > 0) ? (src + offset + sizeof(uint32_t)) : nullptr;
    offset += sizeof(uint32_t) + len;
  }
}

// ============================================================================
// Flat POD arrays - simple memcpy encode/decode
// ============================================================================

#define DEFINE_FLAT_CODERS(T)                                                                      \
  uint32_t GetSize(const T* src, uint32_t count) {                                                 \
    return GetSizeT(src, count);                                                                   \
  }                                                                                                \
  void Encode(const T* src, uint32_t count, char* dst, uint32_t& offset) {                         \
    EncodeT(src, count, dst, offset);                                                              \
  }                                                                                                \
  void Decode(const T* /*dst*/, uint32_t count, char* /*src*/, uint32_t& offset) {                 \
    offset += count * static_cast<uint32_t>(sizeof(T));                                            \
  }

DEFINE_FLAT_CODERS(VkAttachmentDescription)
DEFINE_FLAT_CODERS(VkSubpassDependency)
DEFINE_FLAT_CODERS(VkAttachmentReference)
DEFINE_FLAT_CODERS(VkClearValue)
DEFINE_FLAT_CODERS(VkViewport)
DEFINE_FLAT_CODERS(VkRect2D)
DEFINE_FLAT_CODERS(VkPipelineColorBlendAttachmentState)
DEFINE_FLAT_CODERS(VkVertexInputBindingDescription)
DEFINE_FLAT_CODERS(VkVertexInputAttributeDescription)
DEFINE_FLAT_CODERS(VkPushConstantRange)
DEFINE_FLAT_CODERS(VkDynamicState)
DEFINE_FLAT_CODERS(VkSpecializationMapEntry)
DEFINE_FLAT_CODERS(VkDescriptorPoolSize)
DEFINE_FLAT_CODERS(uint32_t)

#undef DEFINE_FLAT_CODERS

} // namespace vulkan
} // namespace gits
