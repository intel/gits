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
#include <vector>

typedef uint64_t GITSKey;

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
    Copy = true;
  }
  ~PointerArgument() {
    if (Copy) {
      delete Value;
    }
  }
  T* Value{};
  bool Copy{};
};

template <typename T>
struct ArrayArgument {
  T* Value{};
  uint32_t Size{};
  ArrayArgument() {}
  ArrayArgument(const T* v, uint32_t s) : Value(const_cast<T*>(v)), Size(s) {}
  ArrayArgument(const T* v, uint32_t* s) : Value(const_cast<T*>(v)), Size(*s) {}
  ArrayArgument(const T* v, int s) : Value(const_cast<T*>(v)), Size(static_cast<uint32_t>(s)) {}
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
  BufferArgument(void* v, size_t s) : Value(v), Size(s) {}
  BufferArgument(void* v, size_t* s) : Value(v), Size(*s) {}
  BufferArgument(const void* v, size_t s) : Value(const_cast<void*>(v)), Size(s) {}
  BufferArgument(const void* v, size_t* s) : Value(const_cast<void*>(v)), Size(*s) {}
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

} // namespace vulkan
} // namespace gits
