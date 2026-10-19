// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "rtsrSerdeTypes.h"

#include <cstddef>
#include <cstring>
#include <memory>
#include <new>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace gits::rtsr {

// Backing storage for the descriptors of a single deserialized pipeline state.
//
// RTSR used one process wide memory pool that RTSR_FromJson reset on every
// call, so deserializing a pipeline state silently invalidated every previously
// deserialized one. Here each deserialization allocates from its own storage
// and the caller decides how long it lives.
//
// Every allocation is a separate block so that its address survives moving the
// storage, which is what keeps the descriptor pointers of an already
// deserialized pipeline state valid.
class PipelineStateStorage {
public:
  PipelineStateStorage() = default;
  PipelineStateStorage(const PipelineStateStorage&) = delete;
  PipelineStateStorage& operator=(const PipelineStateStorage&) = delete;
  PipelineStateStorage(PipelineStateStorage&&) noexcept = default;
  PipelineStateStorage& operator=(PipelineStateStorage&&) noexcept = default;

  // Returns nullptr for an empty array, matching descriptors that combine a
  // zero element count with a null array pointer.
  template <typename T>
  T* AllocateArray(size_t count) {
    static_assert(std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>,
                  "PipelineStateStorage never runs destructors");
    static_assert(alignof(T) <= __STDCPP_DEFAULT_NEW_ALIGNMENT__,
                  "PipelineStateStorage only guarantees the default new alignment");
    if (count == 0) {
      return nullptr;
    }

    auto* elements = static_cast<T*>(Allocate(sizeof(T) * count));
    for (size_t i = 0; i < count; ++i) {
      new (elements + i) T{};
    }
    return elements;
  }

  template <typename T>
  T* AllocateObject() {
    return AllocateArray<T>(1);
  }

  const char* CopyString(std::string_view text) {
    char* copy = AllocateArray<char>(text.size() + 1);
    if (!text.empty()) {
      std::memcpy(copy, text.data(), text.size());
    }
    copy[text.size()] = '\0';
    return copy;
  }

private:
  void* Allocate(size_t size) {
    m_Blocks.emplace_back(std::make_unique<std::byte[]>(size));
    return m_Blocks.back().get();
  }

  std::vector<std::unique_ptr<std::byte[]>> m_Blocks;
};

// A deserialized pipeline state together with the storage holding everything it
// points to. Move only: the descriptors are not copied, they are owned. They
// stay valid until this object is destroyed, so several pipeline states can be
// deserialized and used at the same time.
class DeserializedPipelineState {
public:
  DeserializedPipelineState() = default;
  DeserializedPipelineState(const PipelineStateInfo& info, PipelineStateStorage storage)
      : m_Info(info), m_Storage(std::move(storage)) {}

  DeserializedPipelineState(const DeserializedPipelineState&) = delete;
  DeserializedPipelineState& operator=(const DeserializedPipelineState&) = delete;

  DeserializedPipelineState(DeserializedPipelineState&& other) noexcept
      : m_Info(other.m_Info), m_Storage(std::move(other.m_Storage)) {
    other.m_Info = {};
  }

  DeserializedPipelineState& operator=(DeserializedPipelineState&& other) noexcept {
    if (this != &other) {
      m_Info = other.m_Info;
      m_Storage = std::move(other.m_Storage);
      other.m_Info = {};
    }
    return *this;
  }

  bool IsValid() const {
    return m_Info.Type != PipelineStateType::Unknown;
  }
  explicit operator bool() const {
    return IsValid();
  }

  const PipelineStateInfo& Get() const {
    return m_Info;
  }
  const PipelineStateInfo* operator->() const {
    return &m_Info;
  }

private:
  PipelineStateInfo m_Info{};
  PipelineStateStorage m_Storage;
};

} // namespace gits::rtsr
