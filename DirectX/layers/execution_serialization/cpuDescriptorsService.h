// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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
      : recorder_(recorder),
        commandListExecutionService_(commandListExecutionService),
        uavDescriptorHeap_(*this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV),
        rtvDescriptorHeap_(*this, D3D12_DESCRIPTOR_HEAP_TYPE_RTV),
        dsvDescriptorHeap_(*this, D3D12_DESCRIPTOR_HEAP_TYPE_DSV) {}

  void createCommandList(unsigned deviceKey);
  void executeCommandLists(std::vector<unsigned>& commandListKeys);
  void preserveDescriptor(ID3D12GraphicsCommandListOMSetRenderTargetsCommand& c);
  void preserveDescriptor(ID3D12GraphicsCommandListClearDepthStencilViewCommand& c);
  void preserveDescriptor(ID3D12GraphicsCommandListClearRenderTargetViewCommand& c);
  void preserveDescriptor(ID3D12GraphicsCommandListClearUnorderedAccessViewUintCommand& c);
  void preserveDescriptor(ID3D12GraphicsCommandListClearUnorderedAccessViewFloatCommand& c);

private:
  ExecutionSerializationRecorder& recorder_;
  CommandListExecutionService& commandListExecutionService_;
  unsigned deviceKey_{};

  template <unsigned SIZE>
  class DescriptorHeap {
  public:
    DescriptorHeap(CpuDescriptorsService& service, D3D12_DESCRIPTOR_HEAP_TYPE type)
        : service_(service), type_(type) {}
    unsigned preserveDescriptor(unsigned heapKey, unsigned heapIndex);
    void clearDescriptor(unsigned index);

  public:
    unsigned descriptorHeapKey_{};

  private:
    void createDescriptorHeap();

  private:
    CpuDescriptorsService& service_;
    D3D12_DESCRIPTOR_HEAP_TYPE type_;
    std::bitset<SIZE> descriptorUsage_;
  };

  static const unsigned uavDescriptorSize_{0x1000};
  DescriptorHeap<uavDescriptorSize_> uavDescriptorHeap_;
  static const unsigned rtvDsvDescriptorSize_{0x1000};
  DescriptorHeap<rtvDsvDescriptorSize_> rtvDescriptorHeap_;
  DescriptorHeap<rtvDsvDescriptorSize_> dsvDescriptorHeap_;

  struct DescriptorHandle {
    D3D12_DESCRIPTOR_HEAP_TYPE type;
    unsigned index;
  };
  std::unordered_map<unsigned, std::vector<DescriptorHandle>> descriptorsByCommandList_;
};

} // namespace DirectX
} // namespace gits
