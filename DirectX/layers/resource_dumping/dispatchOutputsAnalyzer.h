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
#include "descriptorRootSignatureService.h"
#include "descriptorHeapTracker.h"

#include <filesystem>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace gits {
namespace DirectX {

class DispatchOutputsAnalyzer {
public:
  DispatchOutputsAnalyzer();

  void ExecuteCommandLists(ID3D12CommandQueueExecuteCommandListsCommand& c);
  void ClearCommandList(unsigned commandList);
  void CreateDescriptorHeap(ID3D12DeviceCreateDescriptorHeapCommand& c);
  void CreateResource(ID3D12Resource* resource, unsigned resourceKey);
  void CreateRootSignature(ID3D12DeviceCreateRootSignatureCommand& c);
  void CreateDescriptor(DescriptorHeapTracker::Descriptor* descriptor);
  void CopyDescriptors(ID3D12DeviceCopyDescriptorsSimpleCommand& c);
  void CopyDescriptors(ID3D12DeviceCopyDescriptorsCommand& c);
  void SetComputeRootSignature(ID3D12GraphicsCommandListSetComputeRootSignatureCommand& c);
  void SetComputeRootUnorderedAccessView(
      ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& c);
  void SetComputeRootDescriptorTable(
      ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& c);
  void Dispatch(ID3D12GraphicsCommandListDispatchCommand& c);
  void DestroyInterface(unsigned interfaceKey);

  void DumpAnalysisFile();
  void ReadAnalysisFile();
  bool IsAnalysisDone() {
    return std::filesystem::exists(m_AnalysisFilePath);
  }

  struct Bindings {
    unsigned Slot{};
    bool Unbounded{};
    std::vector<unsigned> Resources;
  };
  std::vector<Bindings>* GetDispatchBindings(unsigned dispatchKey);

private:
  DescriptorRootSignatureService m_RootSignatureService;
  DescriptorHeapTracker m_DescriptorService;

  std::filesystem::path m_AnalysisFilePath;
  std::unordered_map<unsigned, std::vector<Bindings>> m_DispatchBindings;

  std::unordered_map<unsigned, ID3D12Resource*> m_ResourceByKey;

  struct DescriptorHeap {
    D3D12_DESCRIPTOR_HEAP_TYPE Type{};
    unsigned NumDescriptors{};
  };
  std::unordered_map<unsigned, DescriptorHeap> m_DescriptorHeaps;

  std::unordered_map<unsigned, unsigned> m_RootSignatureByCommandList;

  std::unordered_map<unsigned, std::unordered_map<unsigned, unsigned>>
      m_DescriptorBySlotByCommandList;

  struct DesciptorTable {
    std::vector<unsigned> Indexes;
    unsigned DescriptorHeap{};
    bool Unbounded{};
  };
  std::unordered_map<unsigned, std::unordered_map<unsigned, DesciptorTable>>
      m_DescriptorTableBySlotByCommandList;
  std::unordered_map<unsigned,
                     std::unordered_map<unsigned, std::unordered_map<unsigned, DesciptorTable>>>
      m_DescriptorTableBySlotByDispatchByCommandList;

  struct AnalysisBindings {
    bool Unbounded{};
    std::unordered_set<unsigned> Resources;
  };
  std::map<unsigned, std::map<unsigned, AnalysisBindings>> m_BindingsBySlotByDispatch;
};

} // namespace DirectX
} // namespace gits
