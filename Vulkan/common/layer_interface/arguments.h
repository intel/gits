// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "vulkanHeader2.h"

#include <cstdint>
#include <string>
#include <vector>

typedef uint64_t GITSKey;

// During real-time recording every command's key is minted sequentially from 1 by
// CaptureManager::CreateCommandKey() (see wrappersAuto.cpp.mako), before the command is
// ever touched by a Serializer.  Commands synthesized purely for subcapture state restore
// (see Vulkan/subcapture) never go through that path, so without an explicit key they
// default to 0 - printing as an indistinguishable, misleading "0" in the trace log instead
// of a stable per-command identity. Mint those from the top of the GITSKey range instead
// (top bit set), mirroring the DirectX subcapture convention (see
// DirectX/common/utils/keyUtils.h): the range can never collide with a real captured key,
// and the trace layer renders it with a distinguishing 'S' prefix (see keyToStr in
// printCustom.h).
inline constexpr GITSKey STATE_RESTORE_KEY_MASK = GITSKey{1} << 63;

inline bool IsStateRestoreKey(GITSKey key) {
  return (key & STATE_RESTORE_KEY_MASK) != 0;
}

inline GITSKey ExtractStateRestoreKey(GITSKey key) {
  return key & ~STATE_RESTORE_KEY_MASK;
}

namespace gits {
namespace vulkan {

template <typename T>
struct Argument {
  T Value{};
  Argument() {}
  Argument(T v) : Value(v) {}
};

template <typename T>
struct PointerArgument {
  PointerArgument(const T* v) : Value(const_cast<T*>(v)) {}
  PointerArgument() {}
  PointerArgument(const PointerArgument<T>& arg) {
    if (arg.Value) {
      Value = new T();
      *Value = *arg.Value;
    }
    HandleKeys = arg.HandleKeys;
    HandleData = arg.HandleData;
    Copy = true;
  }
  ~PointerArgument() {
    if (Copy) {
      delete Value;
    }
  }
  T* Value{};
  std::vector<GITSKey> HandleKeys{};
  std::vector<uint64_t> HandleData{};
  bool Copy{};
};

template <typename T>
struct ArrayArgument {
  T* Value{};
  uint32_t Size{};
  std::vector<GITSKey> HandleKeys{};
  std::vector<uint64_t> HandleData{};
  ArrayArgument() {}
  ArrayArgument(const T* v, uint32_t s) : Value(const_cast<T*>(v)), Size(s) {}
  ArrayArgument(const T* v, uint32_t* s) : Value(const_cast<T*>(v)), Size(*s) {}
  ArrayArgument(const T* v, int s) : Value(const_cast<T*>(v)), Size(static_cast<uint32_t>(s)) {}
  ArrayArgument(const T* v, size_t s) : Value(const_cast<T*>(v)), Size(static_cast<uint32_t>(s)) {}
};

template <typename T>
struct ArrayOutputArgument {
  T** Value{};
  T* Data{};
  uint32_t Size{};
  ArrayOutputArgument() {}
  ArrayOutputArgument(T** v, uint32_t s) : Value(v), Size(s) {}
  ArrayOutputArgument(T** v, uint32_t* s) : Value(v), Size(*s) {}
};

template <typename T>
struct ConstArrayOutputArgument {
  const T** Value{};
  T* Data{};
  // Replay-owned inner pointer; Decode sets Value = &Pointer (see BufferOutputArgument).
  const T* Pointer{};
  uint32_t Size{};
  // When set, element count is read in GetSize/Encode (not in the constructor).
  uint32_t* CountPtr{};
  ConstArrayOutputArgument() {}
  ConstArrayOutputArgument(const T** v, uint32_t s) : Value(v), Size(s) {}
  ConstArrayOutputArgument(const T** v, uint32_t* countPtr) : Value(v), CountPtr(countPtr) {}
};

// UpdateHandle(ArrayArgument<VkAccelerationStructureBuildGeometryInfoKHR>&) collects every
// element's [src, dst] acceleration structure pair before any of the variable length pNext payload,
// so element i keeps a fixed pair position whatever its geometries carry.
inline size_t AsBuildSrcKeyIndex(uint32_t infoIndex) {
  return 2 * static_cast<size_t>(infoIndex);
}

inline size_t AsBuildDstKeyIndex(uint32_t infoIndex) {
  return 2 * static_cast<size_t>(infoIndex) + 1;
}

inline bool HasAsBuildKeys(const std::vector<GITSKey>& keys, uint32_t infoCount) {
  return keys.size() >= 2 * static_cast<size_t>(infoCount);
}

template <>
struct ArrayArgument<VkRayTracingPipelineCreateInfoKHR> {
  VkRayTracingPipelineCreateInfoKHR* Value{};
  uint32_t Size{};
  std::vector<GITSKey> HandleKeys{};
  std::vector<uint64_t> HandleData{};
  uint32_t CaptureReplayHandleSize{};
  std::vector<uint8_t> CaptureReplayHandlesData;
  ArrayArgument() {}
  ArrayArgument(const VkRayTracingPipelineCreateInfoKHR* v, uint32_t s)
      : Value(const_cast<VkRayTracingPipelineCreateInfoKHR*>(v)), Size(s) {}
};

template <typename T>
struct ArrayOfArrays {
  T** Value{};
  uint32_t Size{};
  std::vector<std::vector<T>> Data{};
  std::vector<const T*> Pointers{};
  ArrayOfArrays() {}
  ArrayOfArrays(const T* const* v,
                uint32_t s,
                const VkAccelerationStructureBuildGeometryInfoKHR* infos)
      : Value(const_cast<T**>(v)), Size(s) {
    if (v && infos) {
      Data.resize(Size);
      Pointers.resize(Size);
      for (uint32_t i = 0; i < Size; ++i) {
        uint32_t count = infos[i].geometryCount;
        Data[i].assign(v[i], v[i] + count);
        Pointers[i] = v[i];
      }
    }
  }
};

template <typename T, uint32_t N>
struct StaticArrayArgument {
  T Value[N]{};
  StaticArrayArgument(T* value_) {
    for (uint32_t i = 0; i < N; ++i) {
      Value[i] = value_[i];
    }
  }
  StaticArrayArgument(const T* value_) {
    for (uint32_t i = 0; i < N; ++i) {
      Value[i] = value_[i];
    }
  }
  StaticArrayArgument() {}
};

struct BufferArgument {
  void* Value{};
  size_t Size{};
  BufferArgument() {}
  // Templated size parameter so the generated brace-init `m_pData{ptr, length}`
  // does not trip MSVC's C2398 narrowing check when `length` is VkDeviceSize
  // (unsigned long long) but Size is size_t (unsigned __int64).  Both are
  // 64-bit unsigned on Windows x64 but MSVC treats them as distinct types in
  // brace-init.  The only affected codegen site today is vkCmdUpdateBuffer
  // (commandsAuto.h ~3149) but several other vk* commands take VkDeviceSize
  // lengths so the templated form is more robust than per-call casts.
  template <typename SizeT>
  BufferArgument(void* v, SizeT s) : Value(v), Size(static_cast<size_t>(s)) {}
  template <typename SizeT>
  BufferArgument(void* v, SizeT* s) : Value(v), Size(static_cast<size_t>(*s)) {}
  template <typename SizeT>
  BufferArgument(const void* v, SizeT s)
      : Value(const_cast<void*>(v)), Size(static_cast<size_t>(s)) {}
  template <typename SizeT>
  BufferArgument(const void* v, SizeT* s)
      : Value(const_cast<void*>(v)), Size(static_cast<size_t>(*s)) {}
};

struct OpaqueBufferArgument {
  void* Value{};
  OpaqueBufferArgument() {}
  OpaqueBufferArgument(void* v) : Value(v) {}
  OpaqueBufferArgument(const void* v) : Value(const_cast<void*>(v)) {}
};

// Argument for vkUpdateDescriptorSetWithTemplate pData: holds the serialized
// descriptor data buffer so it can be properly encoded/decoded on the stream.
struct DescriptorTemplateDataArgument {
  void* Value{};            // points into Data after decode, or into app memory during capture
  std::vector<char> Data{}; // owned serialized buffer (populated during capture Pre / decode)
  // Player-side: patched copy of Data where GITSKeys are replaced with player handles.
  // Populated by DescriptorUpdateTemplateService::RemapHandles.  Value is set to point here
  // so the Vulkan call receives the correct handles while Data is left intact for serialization
  // (e.g. RecordingLayer still serializes original GITSKeys into the subcapture stream).
  std::vector<char> PatchedData{};
  DescriptorTemplateDataArgument() {}
  DescriptorTemplateDataArgument(const void* v) : Value(const_cast<void*>(v)) {}
};

struct BufferOutputArgument {
  void** Value{};
  void* Data{};
  BufferOutputArgument() {}
  BufferOutputArgument(void** v) : Value(v) {}
};

template <typename T>
struct HandleArgument {
  T Value{};
  GITSKey Key{};
  HandleArgument() {}
  HandleArgument(T v) : Value(v), Key(0) {}
};

template <typename T>
struct HandleArrayArgument {
  T* Value{};
  uint32_t Size{};
  std::vector<GITSKey> Keys{};
  HandleArrayArgument() {}
  HandleArrayArgument(T* v, uint32_t s) : Value(v) {
    if (v) {
      Size = s;
      Keys.resize(s);
    }
  }
  HandleArrayArgument(const T* v, uint32_t s) : Value(const_cast<T*>(v)) {
    if (v) {
      Size = s;
      Keys.resize(s);
    }
  }
};

template <typename T>
struct HandleOutputArgument {
  T* Value{};
  T Data{};
  GITSKey Key{};
  HandleOutputArgument() {}
  HandleOutputArgument(T* v) : Value(v) {}
};

template <typename T>
struct HandleArrayOutputArgument {
  T* Value{};
  std::vector<GITSKey> Keys{};
  uint32_t Size{};
  HandleArrayOutputArgument() {}
  HandleArrayOutputArgument(T* v, uint32_t s) : Value(v) {
    if (v) {
      Size = s;
      Keys.resize(s);
    }
  }
  HandleArrayOutputArgument(T* v, uint32_t* s) : Value(v) {
    if (v) {
      Size = *s;
      Keys.resize(Size);
    }
  }
};

template <typename T>
struct OpaquePointerArgument {
  T* Value{};
  OpaquePointerArgument() {}
  OpaquePointerArgument(T* v) : Value(v) {}
};

struct MemoryRegions {
  struct Region {
    uint64_t Offset;
    uint64_t Size;
    char* Data;
  };

  uint32_t Size{};
  std::vector<Region> Regions{};
};

struct OutputCStringArrayArgument {
  char*** Value{};
  // Replay-owned char**; Decode sets Value = &PointerSlot (see BufferOutputArgument).
  char** PointerSlot{};
  // Capture: points at the API's name-count output parameter; never read in the constructor.
  uint32_t* CountPtr{};
  // Replay / stream: element count (Encode writes *CountPtr after the call; Decode sets this).
  uint32_t Size{};

  std::vector<std::string> OwnedStrings{};
  std::vector<char*> Pointers{};

  OutputCStringArrayArgument() = default;
  OutputCStringArrayArgument(char*** v, uint32_t* countPtr) : Value(v), CountPtr(countPtr) {}
};

} // namespace vulkan
} // namespace gits
