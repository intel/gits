// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "arguments.h"

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

uint32_t GetPNextChainSize(const void* pNext);

void EncodePNextChain(char* dst, uint32_t& offset, const void* pNext);

void DecodePNextChain(char* src, uint32_t& offset, void** pNext);

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
inline uint32_t GetSize(const PointerArgument<T>& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  if constexpr (std::is_void_v<T>) {
    return sizeof(void*);
  } else {
    return sizeof(void*) + sizeof(T);
  }
}

template <typename T>
inline void Encode(char* dest, uint32_t& offset, const PointerArgument<T>& arg) {
  if (EncodeNullPtr(dest, offset, arg.Value)) {
    return;
  }
  if constexpr (!std::is_void_v<T>) {
    std::memcpy(dest + offset, arg.Value, sizeof(T));
    offset += sizeof(T);
  }
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

inline uint32_t GetSize(const BufferOutputArgument& arg) {
  return sizeof(void*);
}

inline void Encode(char* dest, uint32_t& offset, const BufferOutputArgument& arg) {
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
  if constexpr (!std::is_void_v<T>) {
    offset += sizeof(T);
  }
}

template <>
inline void Decode(char* src, uint32_t& offset, PointerArgument<void>& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }
  arg.Value = src + offset;
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

inline void Decode(char* src, uint32_t& offset, BufferOutputArgument& arg) {
  DecodeNullPtr(src, offset, arg);
  arg.Value = &arg.Data;
}

} // namespace vulkan
} // namespace gits
