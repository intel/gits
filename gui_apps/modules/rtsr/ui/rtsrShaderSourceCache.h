// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "rtsr/shared/server/rtsrSessionStore.h"

#include <condition_variable>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <string_view>

namespace gits::rtsr {
struct ShaderSource;
}

namespace gits::gui {

class RtsrShaderSourceCache {
public:
  enum class State {
    Pending,
    Ready,
    Failed
  };

  struct Entry {
    State Status{State::Pending};
    std::shared_ptr<const rtsr::ShaderSource> Source;
  };

  explicit RtsrShaderSourceCache(const rtsr::SessionStore& store);
  ~RtsrShaderSourceCache();

  RtsrShaderSourceCache(const RtsrShaderSourceCache&) = delete;
  RtsrShaderSourceCache& operator=(const RtsrShaderSourceCache&) = delete;

  // Copies the blob and enqueues it only on the first request for this hash.
  // Subsequent calls copy only a status and shared pointer, not the source text.
  Entry Request(rtsr::RTSR_HASH hash, std::string_view entrypointName);
  void Clear();

private:
  struct Key {
    rtsr::RTSR_HASH Hash{};
    std::string EntrypointName;

    bool operator==(const Key& other) const {
      return Hash == other.Hash && EntrypointName == other.EntrypointName;
    }
  };

  struct KeyHash {
    size_t operator()(const Key& key) const;
  };

  struct Work {
    Key Shader;
    uint64_t Generation{};
    rtsr::SessionStore::ShaderBlob Blob;
    std::filesystem::path PlayerPath;
  };

  void Run();

  const rtsr::SessionStore& m_Store;
  std::mutex m_Mutex;
  std::condition_variable m_Wake;
  std::unordered_map<Key, Entry, KeyHash> m_Entries;
  std::deque<Work> m_Queue;
  uint64_t m_Generation{};
  bool m_Stopping{false};
  // Last: every field the worker can access must exist before it starts.
  std::thread m_Worker;
};

} // namespace gits::gui
