// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

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
  void setEventOnCompletion(ID3D12Fence* fence, unsigned fenceKey, UINT64 value, HANDLE event);
  void waitSignaled(HANDLE handle);
  void waitSignaled(HANDLE hObjectToWaitOn, HANDLE hObjectToSignal);
  void waitSignaled(DWORD count, const HANDLE* handles);
  void destroyFence(unsigned fenceKey);

  std::mutex& getGlobalMutex() {
    return m_GlobalMutex;
  }

private:
  struct FenceInfo {
    ID3D12Fence* Fence;
    unsigned FenceKey;
    UINT64 Value;
    HANDLE Event;
    bool Signaled;
  };
  std::unordered_map<HANDLE, std::unordered_map<unsigned, FenceInfo>> m_FencesByHandle;
  std::unordered_set<unsigned> m_Fences;

  std::mutex m_Mutex;
  stream::OrderingRecorder& m_Recorder;

  std::mutex m_GlobalMutex;
};

} // namespace DirectX
} // namespace gits
