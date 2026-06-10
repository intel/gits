// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "dispatchOutputsAnalyzer.h"
#include "configurationLib.h"

#include <fstream>

namespace gits {
namespace DirectX {

DispatchOutputsAnalyzer::DispatchOutputsAnalyzer() {
  const auto& config = Configurator::Get();
  if (config.directx.features.dispatchOutputsDump.analysisFilePath == "") {
    m_AnalysisFilePath = config.common.player.streamDir.filename().string() + "_frames-" +
                         config.directx.features.dispatchOutputsDump.frames +
                         "_DispatchOutputsDumpAnalysis.txt";
  } else {
    m_AnalysisFilePath = config.directx.features.dispatchOutputsDump.analysisFilePath;
    if (!std::filesystem::exists(m_AnalysisFilePath)) {
      LOG_ERROR << "DispatchOutputsDump - provided analysis file path does not exist: "
                << m_AnalysisFilePath;
      GITS_ASSERT(false && "analysis file path set, but does not exist");
    }
  }
}

void DispatchOutputsAnalyzer::DumpAnalysisFile() {
  std::ofstream analysisFile(m_AnalysisFilePath);
  for (auto& [dispatchKey, bindingsBySlot] : m_BindingsBySlotByDispatch) {
    for (auto& [slot, bindings] : bindingsBySlot) {
      analysisFile << dispatchKey << " " << slot << " " << bindings.Resources.size() << " "
                   << (bindings.Unbounded ? 'u' : 'b');

      std::vector<unsigned> sortedResources;
      sortedResources.resize(bindings.Resources.size());
      std::copy(bindings.Resources.cbegin(), bindings.Resources.cend(), sortedResources.begin());
      std::sort(sortedResources.begin(), sortedResources.end());

      for (unsigned resourceKey : sortedResources) {
        analysisFile << " " << resourceKey;
      }
      analysisFile << "\n";
    }
  }
}

void DispatchOutputsAnalyzer::ReadAnalysisFile() {
  std::ifstream analysisFile(m_AnalysisFilePath);
  std::string line;
  while (std::getline(analysisFile, line)) {
    std::istringstream iss(line);
    unsigned dispatchKey{};
    iss >> dispatchKey;
    Bindings bindings{};
    iss >> bindings.Slot;
    unsigned size{};
    iss >> size;
    char unbounded{};
    iss >> unbounded;
    if (unbounded == 'u') {
      bindings.Unbounded = true;
    }
    unsigned resourceKey{};
    while (iss >> resourceKey) {
      bindings.Resources.push_back(resourceKey);
    }
    m_DispatchBindings[dispatchKey].push_back(std::move(bindings));
  }
}

std::vector<DispatchOutputsAnalyzer::Bindings>* DispatchOutputsAnalyzer::GetDispatchBindings(
    unsigned dispatchKey) {
  auto it = m_DispatchBindings.find(dispatchKey);
  if (it != m_DispatchBindings.end()) {
    return &it->second;
  }
  return nullptr;
}

void DispatchOutputsAnalyzer::CreateDescriptorHeap(ID3D12DeviceCreateDescriptorHeapCommand& c) {
  DescriptorHeap& desc = m_DescriptorHeaps[c.m_ppvHeap.Key];
  desc.Type = c.m_pDescriptorHeapDesc.Value->Type;
  desc.NumDescriptors = c.m_pDescriptorHeapDesc.Value->NumDescriptors;
}

void DispatchOutputsAnalyzer::CreateResource(ID3D12Resource* resource, unsigned resourceKey) {
  m_ResourceByKey[resourceKey] = resource;
}

void DispatchOutputsAnalyzer::CreateRootSignature(ID3D12DeviceCreateRootSignatureCommand& c) {
  m_RootSignatureService.CreateRootSignature(c);
}

void DispatchOutputsAnalyzer::CreateDescriptor(DescriptorHeapTracker::Descriptor* descriptor) {
  m_DescriptorService.CreateDescriptor(descriptor);
}

void DispatchOutputsAnalyzer::CopyDescriptors(ID3D12DeviceCopyDescriptorsSimpleCommand& c) {
  m_DescriptorService.CopyDescriptors(c);
}

void DispatchOutputsAnalyzer::CopyDescriptors(ID3D12DeviceCopyDescriptorsCommand& c) {
  m_DescriptorService.CopyDescriptors(c);
}

void DispatchOutputsAnalyzer::SetComputeRootSignature(
    ID3D12GraphicsCommandListSetComputeRootSignatureCommand& c) {
  m_RootSignatureByCommandList[c.m_Object.Key] = c.m_pRootSignature.Key;
}

void DispatchOutputsAnalyzer::SetComputeRootUnorderedAccessView(
    ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& c) {
  ID3D12Resource* resource = m_ResourceByKey[c.m_BufferLocation.InterfaceKey];
  if (!resource || resource->GetDesc().Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
    return;
  }

  m_DescriptorTableBySlotByCommandList[c.m_Object.Key].erase(c.m_RootParameterIndex.Value);
  m_DescriptorBySlotByCommandList[c.m_Object.Key][c.m_RootParameterIndex.Value] =
      c.m_BufferLocation.InterfaceKey;
}

void DispatchOutputsAnalyzer::SetComputeRootDescriptorTable(
    ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& c) {
  if (!c.m_BaseDescriptor.Value.ptr) {
    return;
  }
  unsigned rootSignatureKey = m_RootSignatureByCommandList[c.m_Object.Key];
  GITS_ASSERT(rootSignatureKey);
  unsigned numDescriptors = m_DescriptorHeaps[c.m_BaseDescriptor.InterfaceKey].NumDescriptors;
  GITS_ASSERT(numDescriptors);

  bool unbounded{};
  std::vector<unsigned> indexes = m_RootSignatureService.GetDescriptorTableIndexes(
      rootSignatureKey, c.m_BaseDescriptor.InterfaceKey, c.m_RootParameterIndex.Value,
      c.m_BaseDescriptor.Index, numDescriptors, false, &unbounded);

  DesciptorTable& desciptorTable =
      m_DescriptorTableBySlotByCommandList[c.m_Object.Key][c.m_RootParameterIndex.Value];
  desciptorTable.DescriptorHeap = c.m_BaseDescriptor.InterfaceKey;
  desciptorTable.Unbounded = unbounded;
  desciptorTable.Indexes = indexes;
}

void DispatchOutputsAnalyzer::Dispatch(ID3D12GraphicsCommandListDispatchCommand& c) {
  {
    auto it = m_DescriptorTableBySlotByCommandList.find(c.m_Object.Key);
    if (it != m_DescriptorTableBySlotByCommandList.end()) {
      m_DescriptorTableBySlotByDispatchByCommandList[c.m_Object.Key][c.Key] = it->second;
    }
  }
  {
    auto it = m_DescriptorBySlotByCommandList.find(c.m_Object.Key);
    if (it != m_DescriptorBySlotByCommandList.end()) {
      for (auto& [slot, resourceKey] : it->second) {
        m_BindingsBySlotByDispatch[c.Key][slot].Resources.insert(resourceKey);
      }
    }
  }
}

void DispatchOutputsAnalyzer::ExecuteCommandLists(ID3D12CommandQueueExecuteCommandListsCommand& c) {
  for (unsigned i = 0; i < c.m_NumCommandLists.Value; ++i) {
    const auto it = m_DescriptorTableBySlotByDispatchByCommandList.find(c.m_ppCommandLists.Keys[i]);
    if (it != m_DescriptorTableBySlotByDispatchByCommandList.end()) {
      for (const auto& [dispatchKey, descriptorTableBySlot] : it->second) {
        for (const auto& [slot, descriptorTable] : descriptorTableBySlot) {
          for (unsigned index : descriptorTable.Indexes) {
            const auto* descriptorInfo =
                m_DescriptorService.GetDescriptor(descriptorTable.DescriptorHeap, index);
            if (descriptorInfo &&
                descriptorInfo->Type == DescriptorHeapTracker::Descriptor::DescriptorType::UAV &&
                descriptorInfo->ResourceKey) {
              ID3D12Resource* resource = m_ResourceByKey[descriptorInfo->ResourceKey];
              if (resource && resource->GetDesc().Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
                AnalysisBindings& bindings = m_BindingsBySlotByDispatch[dispatchKey][slot];
                bindings.Unbounded = descriptorTable.Unbounded;
                bindings.Resources.insert(descriptorInfo->ResourceKey);
              }
            }
          }
        }
      }
    }
  }
}

void DispatchOutputsAnalyzer::ClearCommandList(unsigned commandList) {
  m_RootSignatureByCommandList.erase(commandList);
  m_DescriptorBySlotByCommandList.erase(commandList);
  m_DescriptorTableBySlotByCommandList.erase(commandList);
  m_DescriptorTableBySlotByDispatchByCommandList.erase(commandList);
}

void DispatchOutputsAnalyzer::DestroyInterface(unsigned interfaceKey) {
  m_RootSignatureByCommandList.erase(interfaceKey);
  m_DescriptorBySlotByCommandList.erase(interfaceKey);
  m_DescriptorTableBySlotByCommandList.erase(interfaceKey);
  m_DescriptorTableBySlotByDispatchByCommandList.erase(interfaceKey);
  m_DescriptorHeaps.erase(interfaceKey);
  m_DescriptorService.DestroyObject(interfaceKey);
  m_ResourceByKey.erase(interfaceKey);
}

} // namespace DirectX
} // namespace gits
