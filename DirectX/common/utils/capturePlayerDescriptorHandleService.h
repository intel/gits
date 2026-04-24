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
    UINT64 CaptureStart{};
    UINT64 PlayerStart{};
    UINT64 Size{};
    unsigned Key{};
  };
  struct DescriptorMapping {
    UINT64 CaptureStart{};
    UINT64 PlayerStart{};
    UINT64 Size{};
  };

public:
  void AddCaptureHandle(ID3D12DescriptorHeap* heap,
                        unsigned heapKey,
                        D3D12_GPU_DESCRIPTOR_HANDLE captureHandle);
  void AddPlayerHandle(unsigned heapKey, D3D12_GPU_DESCRIPTOR_HANDLE playerHandle);
  void DestroyHeap(unsigned heapKey);
  bool GetViewMappings(std::vector<DescriptorMapping>& mappings);
  bool GetSamplerMappings(std::vector<DescriptorMapping>& mappings);
  DescriptorHeapInfo* GetViewDescriptorHeapInfoByCaptureHandle(UINT64 handle) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return GetViewDescriptorHeapInfo(m_ViewHeapsByCaptureHandle, handle, true);
  }
  DescriptorHeapInfo* GetSamplerDescriptorHeapInfoByCaptureHandle(UINT64 handle) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return GetViewDescriptorHeapInfo(m_SamplerHeapsByCaptureHandle, handle, true);
  }
  DescriptorHeapInfo* GetViewDescriptorHeapInfoByPlayerHandle(UINT64 handle) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return GetViewDescriptorHeapInfo(m_ViewHeapsByPlayerHandle, handle, false);
  }
  DescriptorHeapInfo* GetSamplerDescriptorHeapInfoByPlayerHandle(UINT64 handle) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return GetViewDescriptorHeapInfo(m_SamplerHeapsByPlayerHandle, handle, false);
  }
  unsigned ViewHeapIncrement() {
    return m_ViewHeapIncrement;
  }
  unsigned SamplerHeapIncrement() {
    return m_SamplerHeapIncrement;
  }
  void EnablePlayerHandleLookup() {
    m_DumpLookup = true;
  }

private:
  using DescriptorHandleMap = std::map<UINT64, DescriptorHeapInfo*>;
  DescriptorHandleMap m_ViewHeapsByCaptureHandle;
  DescriptorHandleMap m_SamplerHeapsByCaptureHandle;
  DescriptorHandleMap m_ViewHeapsByPlayerHandle;
  DescriptorHandleMap m_SamplerHeapsByPlayerHandle;
  std::unordered_map<unsigned, std::unique_ptr<DescriptorHeapInfo>> m_ViewHeapsByKey;
  std::unordered_map<unsigned, std::unique_ptr<DescriptorHeapInfo>> m_SamplerHeapsByKey;
  unsigned m_ViewHeapIncrement{};
  unsigned m_SamplerHeapIncrement{};
  bool m_ViewHeapsChanged{};
  bool m_SamplerHeapsChanged{};
  bool m_DumpLookup{};
  std::mutex m_Mutex;

private:
  DescriptorHeapInfo* GetViewDescriptorHeapInfo(DescriptorHandleMap& descriptorHandleMap,
                                                UINT64 handle,
                                                bool fromCapture);
};

} // namespace DirectX
} // namespace gits
