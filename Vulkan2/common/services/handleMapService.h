// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "arguments.h"
#include "tools.h"

#include <unordered_map>
#include <mutex>

namespace gits {
namespace vulkan {

class HandleMapService : public gits::noncopyable {
public:
  static HandleMapService& Get() {
    static HandleMapService instance;
    return instance;
  }

  void SetKey(uint64_t handle, GITSKey key);
  GITSKey GetKey(uint64_t handle);
  bool HasKey(uint64_t handle);
  void SetHandle(GITSKey key, uint64_t handle);
  uint64_t GetHandle(GITSKey key);

private:
  HandleMapService() = default;
  ~HandleMapService() = default;

  std::mutex m_Mutex;
  std::unordered_map<uint64_t, GITSKey> m_HandleToKey;
  std::unordered_map<GITSKey, uint64_t> m_KeyToHandle;
};

} // namespace vulkan
} // namespace gits
