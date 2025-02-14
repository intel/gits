// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "gitsRecorder.h"
#include "directx.h"

#include <mutex>
#include <unordered_map>
#include <unordered_set>

namespace gits {
namespace DirectX {

class FenceService {
public:
  FenceService(GitsRecorder& recorder);
  void setEventOnCompletion(ID3D12Fence* fence, unsigned fenceKey, UINT64 value, HANDLE event);
  void waitSignaled(HANDLE handle);
  void waitSignaled(HANDLE hObjectToWaitOn, HANDLE hObjectToSignal);
  void waitSignaled(DWORD count, const HANDLE* handles);
  void destroyFence(unsigned fenceKey);

  std::mutex& getGlobalMutex() {
    return globalMutex_;
  }

private:
  struct FenceInfo {
    ID3D12Fence* fence;
    unsigned fenceKey;
    UINT64 value;
    HANDLE event;
    bool signaled;
  };
  std::unordered_map<HANDLE, std::unordered_map<unsigned, FenceInfo>> fencesByHandle_;
  std::unordered_set<unsigned> fences_;

  std::mutex mutex_;
  GitsRecorder& recorder_;

  std::mutex globalMutex_;
};

} // namespace DirectX
} // namespace gits
