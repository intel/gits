// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "vulkanHeader2.h"
#include "arguments.h"

#include <cstring>

namespace gits {
namespace vulkan {

void WriteData(const char* src, unsigned srcSize, char* dst, unsigned& offset);

template <typename T>
T* AddPtrs(const T* relativeOffset, char* bufferBase) {
  return reinterpret_cast<T*>(std::uintptr_t(relativeOffset) + std::uintptr_t(bufferBase));
}

template <typename T>
const T* AddPtrs(const T* relativeOffset, const char* bufferBase) {
  return reinterpret_cast<const T*>(std::uintptr_t(relativeOffset) + std::uintptr_t(bufferBase));
}

template <typename T>
const T* CastTo(const void* buffer) {
  return reinterpret_cast<const T*>(buffer);
}

template <typename T>
uint32_t GetSizeT(const T* src, uint32_t count) {
  if (!src) {
    return 0;
  }
  return count * static_cast<uint32_t>(sizeof(T));
}

template <typename T>
void EncodeT(const T* src, uint32_t count, char* dst, uint32_t& offset) {
  WriteData(reinterpret_cast<const char*>(src), GetSizeT(src, count), dst, offset);
}

template <typename T>
void DecodeT(const T* /*dst*/, uint32_t count, char* /*src*/, uint32_t& offset) {
  offset += GetSizeT(static_cast<const T*>(reinterpret_cast<const void*>(1u)), count);
  // Note: we need the element size without a valid pointer; use sizeof directly:
  offset += 0; // handled below via specialization / explicit calls
  (void)count;
}

template <typename T>
void SkipT(uint32_t count, uint32_t& offset) {
  offset += count * static_cast<uint32_t>(sizeof(T));
}

template <typename T>
void EncodeHandle(T handle, char* dst, uint32_t& offset) {
  std::memcpy(dst + offset, &handle.Key, sizeof(GITSKey));
  offset += sizeof(GITSKey);
}

inline unsigned HandleKeySize() {
  return sizeof(GITSKey);
}

uint32_t GetPNextChainSizeInput(const void* pNext);
uint32_t GetPNextChainSizeOutput(const void* pNext);

void EncodePNextChainInput(char* dst, uint32_t& offset, const void* pNext);
void EncodePNextChainOutput(char* dst, uint32_t& offset, const void* pNext);

void DecodePNextChainInput(char* src, uint32_t& offset, void** pNext);
void DecodePNextChainOutput(char* src, uint32_t& offset, void** pNext);

//uint32_t GetSize(const VkAllocationCallbacks* src, uint32_t count);
//void Encode(const VkAllocationCallbacks* src, uint32_t count, char* dst, uint32_t& offset);
//void Decode(const VkAllocationCallbacks* dst, uint32_t count, char* src, uint32_t& offset);
uint32_t GetStringSize(const char* s);
void EncodeString(const char* s, char* dst, uint32_t& offset);
void DecodeString(char* src, uint32_t& offset, const char** out);

uint32_t GetStringArraySize(const char* const* arr, uint32_t count);
void EncodeStringArray(const char* const* arr, uint32_t count, char* dst, uint32_t& offset);
void DecodeStringArray(char* src, uint32_t& offset, const char*** outArr, uint32_t count);
// ============================================================================
// Generic encode implementations
// ============================================================================

inline bool EncodeNullPtr(char* dest, uint32_t& offset, const void* ptr) {
  std::memcpy(dest + offset, &ptr, sizeof(void*));
  offset += sizeof(void*);
  return ptr == nullptr;
}

template <typename T>
uint32_t GetSize(const T& arg) {
  return sizeof(arg);
}

template <typename T>
void Encode(char* dest, uint32_t& offset, const T& arg) {
  std::memcpy(dest + offset, &arg, sizeof(T));
  offset += sizeof(T);
}

template <typename T>
uint32_t GetSize(const Argument<T>& arg) {
  return sizeof(arg.Value);
}

template <typename T>
void Encode(char* dest, uint32_t& offset, const Argument<T>& arg) {
  uint32_t size = sizeof(arg.Value);
  std::memcpy(dest + offset, &arg.Value, size);
  offset += size;
}

template <typename T>
uint32_t GetSize(const PointerArgument<T>& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(T);
}

template <typename T>
void Encode(char* dest, uint32_t& offset, const PointerArgument<T>& arg) {
  if (EncodeNullPtr(dest, offset, arg.Value)) {
    return;
  }
  std::memcpy(dest + offset, arg.Value, sizeof(T));
  offset += sizeof(T);
}

template <typename T>
uint32_t GetSize(const ArrayArgument<T>& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(arg.Size) + sizeof(T) * arg.Size;
}

template <typename T>
void Encode(char* dest, uint32_t& offset, const ArrayArgument<T>& arg) {
  if (EncodeNullPtr(dest, offset, arg.Value)) {
    return;
  }
  std::memcpy(dest + offset, &arg.Size, sizeof(arg.Size));
  offset += sizeof(arg.Size);
  std::memcpy(dest + offset, arg.Value, sizeof(T) * arg.Size);
  offset += sizeof(T) * arg.Size;
}

template <typename T>
uint32_t GetSize(const HandleArgument<T>& arg) {
  return sizeof(GITSKey);
}

template <typename T>
void Encode(char* dest, uint32_t& offset, const HandleArgument<T>& arg) {
  std::memcpy(dest + offset, &arg.Key, sizeof(arg.Key));
  offset += sizeof(arg.Key);
}

template <typename T>
uint32_t GetSize(const HandleArrayArgument<T>& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(uint32_t) + sizeof(GITSKey) * arg.Size;
}

template <typename T>
void Encode(char* dest, uint32_t& offset, const HandleArrayArgument<T>& arg) {
  if (EncodeNullPtr(dest, offset, arg.Value)) {
    return;
  }
  std::memcpy(dest + offset, &arg.Size, sizeof(arg.Size));
  offset += sizeof(arg.Size);
  std::memcpy(dest + offset, arg.Keys.data(), sizeof(GITSKey) * arg.Size);
  offset += sizeof(GITSKey) * arg.Size;
}

template <typename T>
uint32_t GetSize(const HandleOutputArgument<T>& arg) {
  return sizeof(GITSKey);
}

template <typename T>
void Encode(char* dest, uint32_t& offset, const HandleOutputArgument<T>& arg) {
  std::memcpy(dest + offset, &arg.Key, sizeof(GITSKey));
  offset += sizeof(GITSKey);
}

template <typename T>
uint32_t GetSize(const HandleArrayOutputArgument<T>& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(uint32_t) + sizeof(GITSKey) * arg.Size;
}

template <typename T>
void Encode(char* dest, uint32_t& offset, const HandleArrayOutputArgument<T>& arg) {
  if (EncodeNullPtr(dest, offset, arg.Value)) {
    return;
  }
  std::memcpy(dest + offset, &arg.Size, sizeof(arg.Size));
  offset += sizeof(arg.Size);
  std::memcpy(dest + offset, arg.Keys.data(), sizeof(GITSKey) * arg.Size);
  offset += sizeof(GITSKey) * arg.Size;
}

inline uint32_t GetSize(const BufferArgument& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(arg.Size) + arg.Size;
}

inline void Encode(char* dest, uint32_t& offset, const BufferArgument& arg) {
  if (EncodeNullPtr(dest, offset, arg.Value)) {
    return;
  }
  std::memcpy(dest + offset, &arg.Size, sizeof(arg.Size));
  offset += sizeof(arg.Size);
  std::memcpy(dest + offset, arg.Value, arg.Size);
  offset += arg.Size;
}

inline uint32_t GetSize(const OpaqueBufferArgument& arg) {
  return sizeof(void*);
}

inline void Encode(char* dest, uint32_t& offset, const OpaqueBufferArgument& arg) {
  EncodeNullPtr(dest, offset, arg.Value);
}

inline uint32_t GetSize(const BufferOutputArgument& arg) {
  return sizeof(void*);
}

inline void Encode(char* dest, uint32_t& offset, const BufferOutputArgument& arg) {
  EncodeNullPtr(dest, offset, arg.Value);
}

template <typename T>
uint32_t GetSize(const ArrayOfArrays<T>& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }

  uint32_t size = sizeof(void*) + sizeof(arg.Size);
  for (uint32_t i = 0; i < arg.Size; ++i) {
    size += sizeof(void*);
    size += sizeof(uint32_t);
    size += sizeof(T) * static_cast<uint32_t>(arg.Data[i].size());
  }
  return size;
}

template <typename T>
void Encode(char* dest, uint32_t& offset, const ArrayOfArrays<T>& arg) {
  if (EncodeNullPtr(dest, offset, arg.Value)) {
    return;
  }
  std::memcpy(dest + offset, &arg.Size, sizeof(arg.Size));
  offset += sizeof(arg.Size);
  for (uint32_t i = 0; i < arg.Size; ++i) {
    const void* ptr = arg.Pointers[i];
    std::memcpy(dest + offset, &ptr, sizeof(void*));
    offset += sizeof(void*);
    uint32_t count = static_cast<uint32_t>(arg.Data[i].size());
    std::memcpy(dest + offset, &count, sizeof(count));
    offset += sizeof(count);
    std::memcpy(dest + offset, arg.Data[i].data(), sizeof(T) * count);
    offset += sizeof(T) * count;
  }
}

template <typename T>
uint32_t GetSize(const OpaquePointerArgument<T>& arg) {
  return sizeof(void*);
}

template <typename T>
void Encode(char* dest, uint32_t& offset, const OpaquePointerArgument<T>& arg) {
  EncodeNullPtr(dest, offset, arg.Value);
}

// ============================================================================
// Generic decode implementations
// ============================================================================

template <typename T>
void Decode(char* src, uint32_t& offset, T& arg) {
  std::memcpy(&arg, src + offset, sizeof(T));
  offset += sizeof(T);
}

template <typename T>
void Decode(char* src, uint32_t& offset, Argument<T>& arg) {
  std::memcpy(&arg.Value, src + offset, sizeof(T));
  offset += sizeof(T);
}

template <typename T>
bool DecodeNullPtr(char* src, uint32_t& offset, T& arg) {
  std::memcpy(&arg.Value, src + offset, sizeof(void*));
  offset += sizeof(void*);
  if (!arg.Value) {
    return true;
  }
  return false;
}

template <typename T>
void Decode(char* src, uint32_t& offset, PointerArgument<T>& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }
  arg.Value = reinterpret_cast<T*>(src + offset);
  offset += sizeof(T);
}

template <typename T>
void Decode(char* src, uint32_t& offset, ArrayArgument<T>& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    arg.Size = 0;
    return;
  }

  std::memcpy(&arg.Size, src + offset, sizeof(arg.Size));
  offset += sizeof(arg.Size);

  arg.Value = reinterpret_cast<T*>(src + offset);
  if constexpr (!std::is_void_v<T>) {
    offset += sizeof(T) * arg.Size;
  }
}

template <typename T>
void Decode(char* src, uint32_t& offset, HandleArgument<T>& arg) {
  std::memcpy(&arg.Key, src + offset, sizeof(arg.Key));
  offset += sizeof(arg.Key);
}

template <typename T>
void Decode(char* src, uint32_t& offset, HandleArrayArgument<T>& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    arg.Size = 0;
    arg.Keys.clear();
    return;
  }
  std::memcpy(&arg.Size, src + offset, sizeof(arg.Size));
  offset += sizeof(arg.Size);
  arg.Keys.resize(arg.Size);
  std::memcpy(arg.Keys.data(), src + offset, sizeof(GITSKey) * arg.Size);
  offset += sizeof(GITSKey) * arg.Size;
  arg.Value = new T[arg.Size]{};
}

template <typename T>
void Decode(char* src, uint32_t& offset, HandleOutputArgument<T>& arg) {
  std::memcpy(&arg.Key, src + offset, sizeof(GITSKey));
  offset += sizeof(GITSKey);
  arg.Value = &arg.Data;
}

template <typename T>
void Decode(char* src, uint32_t& offset, HandleArrayOutputArgument<T>& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    arg.Size = 0;
    arg.Keys.clear();
    return;
  }
  std::memcpy(&arg.Size, src + offset, sizeof(arg.Size));
  offset += sizeof(arg.Size);
  arg.Keys.resize(arg.Size);
  std::memcpy(arg.Keys.data(), src + offset, sizeof(GITSKey) * arg.Size);
  offset += sizeof(GITSKey) * arg.Size;
  arg.Value = new T[arg.Size]{};
}

inline void Decode(char* src, uint32_t& offset, BufferArgument& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    arg.Size = 0;
    return;
  }
  std::memcpy(&arg.Size, src + offset, sizeof(arg.Size));
  offset += sizeof(arg.Size);
  arg.Value = src + offset;
  offset += arg.Size;
}

inline void Decode(char* src, uint32_t& offset, OpaqueBufferArgument& arg) {
  DecodeNullPtr(src, offset, arg);
}

inline void Decode(char* src, uint32_t& offset, BufferOutputArgument& arg) {
  DecodeNullPtr(src, offset, arg);
  arg.Value = &arg.Data;
}

template <typename T>
void Decode(char* src, uint32_t& offset, ArrayOfArrays<T>& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    arg.Size = 0;
    arg.Data.clear();
    arg.Pointers.clear();
    return;
  }
  std::memcpy(&arg.Size, src + offset, sizeof(arg.Size));
  offset += sizeof(arg.Size);
  arg.Data.resize(arg.Size);
  arg.Pointers.resize(arg.Size);
  for (uint32_t i = 0; i < arg.Size; ++i) {
    std::memcpy(&arg.Pointers[i], src + offset, sizeof(void*));
    offset += sizeof(void*);
    uint32_t count{};
    std::memcpy(&count, src + offset, sizeof(count));
    offset += sizeof(count);
    arg.Data[i].resize(count);
    std::memcpy(arg.Data[i].data(), src + offset, sizeof(T) * count);
    offset += sizeof(T) * count;
  }
}

template <typename T>
void Decode(char* src, uint32_t& offset, OpaquePointerArgument<T>& arg) {
  DecodeNullPtr(src, offset, arg);
}

} // namespace vulkan
} // namespace gits
