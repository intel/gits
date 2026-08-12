// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once
#include "arguments.h"

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

  void CreateCommandList(GITSKey deviceKey);
  void ExecuteCommandLists(std::vector<GITSKey>& commandListKeys);
  void PreserveDescriptor(ID3D12GraphicsCommandListOMSetRenderTargetsCommand& c);
  void PreserveDescriptor(ID3D12GraphicsCommandListClearDepthStencilViewCommand& c);
  void PreserveDescriptor(ID3D12GraphicsCommandListClearRenderTargetViewCommand& c);
  void PreserveDescriptor(ID3D12GraphicsCommandListClearUnorderedAccessViewUintCommand& c);
  void PreserveDescriptor(ID3D12GraphicsCommandListClearUnorderedAccessViewFloatCommand& c);

private:
  ExecutionSerializationRecorder& m_Recorder;
  CommandListExecutionService& m_CommandListExecutionService;
  GITSKey m_DeviceKey{};

  template <unsigned SIZE>
  class DescriptorHeap {
  public:
    DescriptorHeap(CpuDescriptorsService& service, D3D12_DESCRIPTOR_HEAP_TYPE type)
        : m_Service(service), m_Type(type) {}
    unsigned PreserveDescriptor(GITSKey heapKey, unsigned heapIndex);
    void ClearDescriptor(unsigned index);

  public:
    GITSKey m_DescriptorHeapKey{};

  private:
    void CreateDescriptorHeap();

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
    D3D12_DESCRIPTOR_HEAP_TYPE Type{};
    unsigned Index{};
  };
  std::unordered_map<GITSKey, std::vector<DescriptorHandle>> m_DescriptorsByCommandList;
};

} // namespace DirectX
} // namespace gits
