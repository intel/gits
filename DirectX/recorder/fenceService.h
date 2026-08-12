// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once
#include "arguments.h"

#include "orderingRecorder.h"
#include "directx.h"

#include <mutex>
#include <unordered_map>
#include <unordered_set>

namespace gits {
namespace DirectX {

class FenceService {
public:
  FenceService(stream::OrderingRecorder& recorder);
  void SetEventOnCompletion(ID3D12Fence* fence, GITSKey fenceKey, UINT64 value, HANDLE event);
  void WaitSignaled(HANDLE handle);
  void WaitSignaled(DWORD count, const HANDLE* handles);
  void DestroyFence(GITSKey fenceKey);

  std::mutex& getGlobalMutex() {
    return m_GlobalMutex;
  }

private:
  struct FenceInfo {
    ID3D12Fence* Fence{};
    GITSKey FenceKey{};
    UINT64 Value{};
    HANDLE Event{};
    bool Signaled{};
  };
  std::unordered_map<HANDLE, std::unordered_map<GITSKey, FenceInfo>> m_FencesByHandle;
  std::unordered_set<GITSKey> m_Fences;

  std::mutex m_Mutex;
  stream::OrderingRecorder& m_Recorder;

  std::mutex m_GlobalMutex;
};

} // namespace DirectX
} // namespace gits
