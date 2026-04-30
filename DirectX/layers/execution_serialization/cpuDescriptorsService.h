// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandsAuto.h"
#include "commandsCustom.h"
#include "executionSerializationRecorder.h"
#include "commandListExecutionService.h"

#include <bitset>
#include <unordered_map>

namespace gits {
namespace DirectX {

class CpuDescriptorsService {
public:
  CpuDescriptorsService(ExecutionSerializationRecorder& recorder,
                        CommandListExecutionService& commandListExecutionService)
      : m_Recorder(recorder),
        m_CommandListExecutionService(commandListExecutionService),
        m_UavDescriptorHeap(*this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV),
        m_RtvDescriptorHeap(*this, D3D12_DESCRIPTOR_HEAP_TYPE_RTV),
        m_DsvDescriptorHeap(*this, D3D12_DESCRIPTOR_HEAP_TYPE_DSV) {}

  void createCommandList(unsigned DeviceKey);
  void executeCommandLists(std::vector<unsigned>& commandListKeys);
  void preserveDescriptor(ID3D12GraphicsCommandListOMSetRenderTargetsCommand& c);
  void preserveDescriptor(ID3D12GraphicsCommandListClearDepthStencilViewCommand& c);
  void preserveDescriptor(ID3D12GraphicsCommandListClearRenderTargetViewCommand& c);
  void preserveDescriptor(ID3D12GraphicsCommandListClearUnorderedAccessViewUintCommand& c);
  void preserveDescriptor(ID3D12GraphicsCommandListClearUnorderedAccessViewFloatCommand& c);

private:
  ExecutionSerializationRecorder& m_Recorder;
  CommandListExecutionService& m_CommandListExecutionService;
  unsigned m_DeviceKey{};

  template <unsigned SIZE>
  class DescriptorHeap {
  public:
    DescriptorHeap(CpuDescriptorsService& service, D3D12_DESCRIPTOR_HEAP_TYPE type)
        : m_Service(service), m_Type(type) {}
    unsigned preserveDescriptor(unsigned heapKey, unsigned heapIndex);
    void clearDescriptor(unsigned index);

  public:
    unsigned m_DescriptorHeapKey{};

  private:
    void createDescriptorHeap();

  private:
    CpuDescriptorsService& m_Service;
    D3D12_DESCRIPTOR_HEAP_TYPE m_Type;
    std::bitset<SIZE> m_DescriptorUsage;
  };

  static constexpr unsigned UavDescriptorSize = 0x1000;
  DescriptorHeap<UavDescriptorSize> m_UavDescriptorHeap;
  static constexpr unsigned RtvDsvDescriptorSize = 0x1000;
  DescriptorHeap<RtvDsvDescriptorSize> m_RtvDescriptorHeap;
  DescriptorHeap<RtvDsvDescriptorSize> m_DsvDescriptorHeap;

  struct DescriptorHandle {
    D3D12_DESCRIPTOR_HEAP_TYPE Type;
    unsigned Index;
  };
  std::unordered_map<unsigned, std::vector<DescriptorHandle>> m_DescriptorsByCommandList;
};

} // namespace DirectX
} // namespace gits
