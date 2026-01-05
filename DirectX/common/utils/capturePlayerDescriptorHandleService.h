// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <map>
#include <unordered_map>
#include <memory>
#include <d3d12.h>
#include <mutex>

namespace gits {
namespace DirectX {

class CapturePlayerDescriptorHandleService {
public:
  struct DescriptorHeapInfo {
    UINT64 captureStart;
    UINT64 playerStart;
    UINT64 size;
    unsigned key;
  };
  struct DescriptorMapping {
    UINT64 captureStart;
    UINT64 playerStart;
    UINT64 size;
  };

public:
  void addCaptureHandle(ID3D12DescriptorHeap* heap,
                        unsigned heapKey,
                        D3D12_GPU_DESCRIPTOR_HANDLE captureHandle);
  void addPlayerHandle(unsigned heapKey, D3D12_GPU_DESCRIPTOR_HANDLE playerHandle);
  void destroyHeap(unsigned heapKey);
  bool getViewMappings(std::vector<DescriptorMapping>& mappings);
  bool getSamplerMappings(std::vector<DescriptorMapping>& mappings);
  DescriptorHeapInfo* getViewDescriptorHeapInfoByCaptureHandle(UINT64 handle) {
    return getViewDescriptorHeapInfo(viewHeapsByCaptureHandle_, handle, true);
  }
  DescriptorHeapInfo* getSamplerDescriptorHeapInfoByCaptureHandle(UINT64 handle) {
    std::lock_guard<std::mutex> lock(mutex_);
    return getViewDescriptorHeapInfo(samplerHeapsByCaptureHandle_, handle, true);
  }
  DescriptorHeapInfo* getViewDescriptorHeapInfoByPlayerHandle(UINT64 handle) {
    std::lock_guard<std::mutex> lock(mutex_);
    return getViewDescriptorHeapInfo(viewHeapsByPlayerHandle_, handle, false);
  }
  DescriptorHeapInfo* getSamplerDescriptorHeapInfoByPlayerHandle(UINT64 handle) {
    std::lock_guard<std::mutex> lock(mutex_);
    return getViewDescriptorHeapInfo(samplerHeapsByPlayerHandle_, handle, false);
  }
  unsigned viewHeapIncrement() {
    return viewHeapIncrement_;
  }
  unsigned samplerHeapIncrement() {
    return samplerHeapIncrement_;
  }
  void enablePlayerHandleLookup() {
    dumpLookup_ = true;
  }

private:
  using DescriptorHandleMap = std::map<UINT64, DescriptorHeapInfo*>;
  DescriptorHandleMap viewHeapsByCaptureHandle_;
  DescriptorHandleMap samplerHeapsByCaptureHandle_;
  DescriptorHandleMap viewHeapsByPlayerHandle_;
  DescriptorHandleMap samplerHeapsByPlayerHandle_;
  std::unordered_map<unsigned, std::unique_ptr<DescriptorHeapInfo>> viewHeapsByKey_;
  std::unordered_map<unsigned, std::unique_ptr<DescriptorHeapInfo>> samplerHeapsByKey_;
  unsigned viewHeapIncrement_{};
  unsigned samplerHeapIncrement_{};
  bool viewHeapsChanged_{};
  bool samplerHeapsChanged_{};
  bool dumpLookup_{};
  std::mutex mutex_;

private:
  DescriptorHeapInfo* getViewDescriptorHeapInfo(DescriptorHandleMap& descriptorHandleMap,
                                                UINT64 handle,
                                                bool fromCapture);
};

} // namespace DirectX
} // namespace gits
