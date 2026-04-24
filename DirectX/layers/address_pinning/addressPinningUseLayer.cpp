// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "addressPinningUseLayer.h"
#include "configurationLib.h"
#include "log.h"
#include "intelExtensions.h"

#include <fstream>

namespace gits {
namespace DirectX {

AddressPinningUseLayer::AddressPinningUseLayer() : Layer("AddressPinningUse") {
  ReadAddressRanges();
}

void AddressPinningUseLayer::Pre(D3D12CreateDeviceCommand& command) {
  if (command.m_Result.Value != S_OK) {
    return;
  }

  HRESULT hr = D3D12GetInterface(CLSID_D3D12Tools, IID_PPV_ARGS(&m_D3d12Tools));
  GITS_ASSERT(hr == S_OK);
  hr = m_D3d12Tools->ReserveGPUVARangesAtCreate(m_AddressRanges.data(),
                                                static_cast<UINT>(m_AddressRanges.size()));
  GITS_ASSERT(hr == S_OK);
}

void AddressPinningUseLayer::Post(D3D12CreateDeviceCommand& command) {
  if (command.Skip || command.m_Result.Value != S_OK) {
    return;
  }

  ID3D12Device* device = static_cast<ID3D12Device*>(*command.m_ppDevice.Value);
  HRESULT hr = device->QueryInterface(IID_PPV_ARGS(&m_DeviceTools));
  GITS_ASSERT(hr == S_OK);
}

void AddressPinningUseLayer::Pre(ID3D12DeviceCreateHeapCommand& command) {
  PreHeap(command);
}

void AddressPinningUseLayer::Post(ID3D12DeviceCreateHeapCommand& command) {
  PostHeap(command);
}

void AddressPinningUseLayer::Pre(ID3D12Device4CreateHeap1Command& command) {
  PreHeap(command);
}

void AddressPinningUseLayer::Post(ID3D12Device4CreateHeap1Command& command) {
  PostHeap(command);
}

void AddressPinningUseLayer::Pre(INTC_D3D12_CreateHeapCommand& command) {
  if (command.m_Result.Value != S_OK) {
    return;
  }
  D3D12_HEAP_DESC* desc = command.m_pDesc.Value->pD3D12Desc;
  if (desc->Flags & D3D12_HEAP_FLAG_DENY_BUFFERS) {
    return;
  }
  auto it = m_HeapAddressRanges.find(command.m_ppvHeap.Key);
  if (it == m_HeapAddressRanges.end()) {
    return;
  }
  m_DeviceTools->SetNextAllocationAddress(it->second.m_AddressRange.StartAddress);
}

void AddressPinningUseLayer::Post(INTC_D3D12_CreateHeapCommand& command) {
  if (command.m_Result.Value != S_OK) {
    return;
  }
  D3D12_HEAP_DESC* desc = command.m_pDesc.Value->pD3D12Desc;
  if (desc->Flags & D3D12_HEAP_FLAG_DENY_BUFFERS) {
    return;
  }
  auto it = m_HeapAddressRanges.find(command.m_ppvHeap.Key);
  if (it == m_HeapAddressRanges.end()) {
    return;
  }
  ID3D12Heap* heap = static_cast<ID3D12Heap*>(*command.m_ppvHeap.Value);
  ID3D12PageableTools* pPageableTools = nullptr;
  HRESULT hr = heap->QueryInterface(IID_PPV_ARGS(&pPageableTools));
  GITS_ASSERT(hr == S_OK);
  D3D12_GPU_VIRTUAL_ADDRESS_RANGE range{};
  hr = pPageableTools->GetAllocation(&range);
  GITS_ASSERT(hr == S_OK);
  GITS_ASSERT(it->second.m_AddressRange.StartAddress == range.StartAddress);
}

void AddressPinningUseLayer::Pre(CreateHeapAllocationMetaCommand& command) {
  command.Skip = true;
}

void AddressPinningUseLayer::Pre(ID3D12Device3OpenExistingHeapFromAddressCommand& command) {
  PreOpenExistingHeap(command);
}

void AddressPinningUseLayer::Pre(ID3D12Device13OpenExistingHeapFromAddress1Command& command) {
  PreOpenExistingHeap(command);
}

void AddressPinningUseLayer::Pre(ID3D12DeviceCreateCommittedResourceCommand& command) {
  PreResource(command);
}

void AddressPinningUseLayer::Post(ID3D12DeviceCreateCommittedResourceCommand& command) {
  PostResource(command);
}

void AddressPinningUseLayer::Pre(ID3D12Device4CreateCommittedResource1Command& command) {
  PreResource(command);
}

void AddressPinningUseLayer::Post(ID3D12Device4CreateCommittedResource1Command& command) {
  PostResource(command);
}

void AddressPinningUseLayer::Pre(ID3D12Device8CreateCommittedResource2Command& command) {
  PreResource(command);
}

void AddressPinningUseLayer::Post(ID3D12Device8CreateCommittedResource2Command& command) {
  PostResource(command);
}

void AddressPinningUseLayer::Pre(ID3D12Device10CreateCommittedResource3Command& command) {
  PreResource(command);
}

void AddressPinningUseLayer::Post(ID3D12Device10CreateCommittedResource3Command& command) {
  PostResource(command);
}

void AddressPinningUseLayer::Pre(ID3D12DeviceCreateReservedResourceCommand& command) {
  PreResource(command);
}

void AddressPinningUseLayer::Post(ID3D12DeviceCreateReservedResourceCommand& command) {
  PostResource(command);
}

void AddressPinningUseLayer::Pre(ID3D12Device4CreateReservedResource1Command& command) {
  PreResource(command);
}

void AddressPinningUseLayer::Post(ID3D12Device4CreateReservedResource1Command& command) {
  PostResource(command);
}

void AddressPinningUseLayer::Pre(ID3D12Device10CreateReservedResource2Command& command) {
  PreResource(command);
}

void AddressPinningUseLayer::Post(ID3D12Device10CreateReservedResource2Command& command) {
  PostResource(command);
}

void AddressPinningUseLayer::Pre(ID3D12DeviceCreatePlacedResourceCommand& command) {
  if (command.m_Result.Value != S_OK) {
    return;
  }

  if (m_ChangedHeaps.count(command.m_pHeap.Key)) {
    command.m_pDesc.Value->Flags &= ~D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER;
  }
}

void AddressPinningUseLayer::Post(ID3D12DeviceCreatePlacedResourceCommand& command) {
  if (command.m_Result.Value != S_OK) {
    return;
  }

  if (m_ChangedHeaps.count(command.m_pHeap.Key)) {
    command.m_pDesc.Value->Flags |= D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER;
  }
}

void AddressPinningUseLayer::Pre(ID3D12Device8CreatePlacedResource1Command& command) {
  if (command.m_Result.Value != S_OK) {
    return;
  }

  if (m_ChangedHeaps.count(command.m_pHeap.Key)) {
    command.m_pDesc.Value->Flags &= ~D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER;
  }
}

void AddressPinningUseLayer::Post(ID3D12Device8CreatePlacedResource1Command& command) {
  if (command.m_Result.Value != S_OK) {
    return;
  }

  if (m_ChangedHeaps.count(command.m_pHeap.Key)) {
    command.m_pDesc.Value->Flags |= D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER;
  }
}

void AddressPinningUseLayer::Pre(ID3D12Device10CreatePlacedResource2Command& command) {
  if (command.m_Result.Value != S_OK) {
    return;
  }

  if (m_ChangedHeaps.count(command.m_pHeap.Key)) {
    command.m_pDesc.Value->Flags &= ~D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER;
  }
}

void AddressPinningUseLayer::Post(ID3D12Device10CreatePlacedResource2Command& command) {
  if (command.m_Result.Value != S_OK) {
    return;
  }

  if (m_ChangedHeaps.count(command.m_pHeap.Key)) {
    command.m_pDesc.Value->Flags |= D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER;
  }
}

template <typename CommandT>
void AddressPinningUseLayer::PreResource(CommandT& command) {
  if (command.m_Result.Value != S_OK ||
      command.m_pDesc.Value->Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
    return;
  }

  auto it = m_ResourceAddressRanges.find(command.m_ppvResource.Key);
  if (it == m_ResourceAddressRanges.end()) {
    return;
  }
  m_DeviceTools->SetNextAllocationAddress(it->second.StartAddress);
}

template <typename CommandT>
void AddressPinningUseLayer::PostResource(CommandT& command) {
  if (command.m_Result.Value != S_OK ||
      command.m_pDesc.Value->Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
    return;
  }

  auto it = m_ResourceAddressRanges.find(command.m_ppvResource.Key);
  if (it == m_ResourceAddressRanges.end()) {
    return;
  }
  ID3D12Resource* resource = static_cast<ID3D12Resource*>(*command.m_ppvResource.Value);
  GITS_ASSERT(it->second.StartAddress == resource->GetGPUVirtualAddress());
}

template <typename CommandT>
void AddressPinningUseLayer::PreHeap(CommandT& command) {
  if (command.m_Result.Value != S_OK ||
      command.m_pDesc.Value->Flags & D3D12_HEAP_FLAG_DENY_BUFFERS) {
    return;
  }

  auto it = m_HeapAddressRanges.find(command.m_ppvHeap.Key);
  if (it == m_HeapAddressRanges.end()) {
    return;
  }
  m_DeviceTools->SetNextAllocationAddress(it->second.m_AddressRange.StartAddress);
}

template <typename CommandT>
void AddressPinningUseLayer::PostHeap(CommandT& command) {
  if (command.m_Result.Value != S_OK ||
      command.m_pDesc.Value->Flags & D3D12_HEAP_FLAG_DENY_BUFFERS) {
    return;
  }
  auto it = m_HeapAddressRanges.find(command.m_ppvHeap.Key);
  if (it == m_HeapAddressRanges.end()) {
    return;
  }
  ID3D12Heap* heap = static_cast<ID3D12Heap*>(*command.m_ppvHeap.Value);
  ID3D12PageableTools* pPageableTools = nullptr;
  HRESULT hr = heap->QueryInterface(IID_PPV_ARGS(&pPageableTools));
  GITS_ASSERT(hr == S_OK);
  D3D12_GPU_VIRTUAL_ADDRESS_RANGE range{};
  hr = pPageableTools->GetAllocation(&range);
  GITS_ASSERT(hr == S_OK);
  GITS_ASSERT(it->second.m_AddressRange.StartAddress == range.StartAddress);
}

template <typename CommandT>
void AddressPinningUseLayer::PreOpenExistingHeap(CommandT& command) {
  if (command.m_Result.Value != S_OK) {
    return;
  }

  auto it = m_HeapAddressRanges.find(command.m_ppvHeap.Key);
  if (it == m_HeapAddressRanges.end()) {
    return;
  }

  auto* device = command.m_Object.Value;
  ID3D12Heap* heap = nullptr;
  D3D12_HEAP_DESC heapDesc{};
  HRESULT hr{};

  heapDesc.SizeInBytes = it->second.m_AddressRange.SizeInBytes;
  heapDesc.Properties.Type = D3D12_HEAP_TYPE_CUSTOM;
  heapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
  heapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
  heapDesc.Properties.CreationNodeMask = 1;
  heapDesc.Properties.VisibleNodeMask = 1;
  heapDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
  heapDesc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;

  m_DeviceTools->SetNextAllocationAddress(it->second.m_AddressRange.StartAddress);
  hr = device->CreateHeap(&heapDesc, IID_PPV_ARGS(&heap));
  GITS_ASSERT(hr == S_OK);

  ID3D12PageableTools* pPageableTools = nullptr;
  hr = heap->QueryInterface(IID_PPV_ARGS(&pPageableTools));
  GITS_ASSERT(hr == S_OK);
  D3D12_GPU_VIRTUAL_ADDRESS_RANGE range{};
  hr = pPageableTools->GetAllocation(&range);
  GITS_ASSERT(hr == S_OK);
  GITS_ASSERT(it->second.m_AddressRange.StartAddress == range.StartAddress);

  m_ChangedHeaps.insert(command.m_ppvHeap.Key);
  *command.m_ppvHeap.Value = heap;
  command.m_Result.Value = S_OK;
  command.Skip = true;
}

void AddressPinningUseLayer::ReadAddressRanges() {
  const std::filesystem::path inputPath =
      Configurator::Get().common.player.streamDir / "addressRanges.txt";
  std::ifstream inputFile(inputPath);
  GITS_ASSERT(!inputFile.fail());

  enum class Section {
    NONE,
    RESOURCES,
    HEAPS
  };
  std::string line;
  Section currentSection = Section::NONE;
  while (std::getline(inputFile, line)) {
    if (line.empty()) {
      continue;
    }

    if (line == "RESOURCES") {
      currentSection = Section::RESOURCES;
      continue;
    } else if (line == "HEAPS") {
      currentSection = Section::HEAPS;
      continue;
    }

    std::istringstream lineStream(line);
    unsigned key{};
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE range{};
    UINT64 alignment{};

    if (currentSection == Section::RESOURCES) {
      if (lineStream >> key >> range.StartAddress >> range.SizeInBytes) {
        m_AddressRanges.push_back(range);
        m_ResourceAddressRanges[key] = range;
      } else {
        LOG_ERROR << "AddressPinningUseLayer: Failed to parse resource address range line: "
                  << line;
        return;
      }
    } else if (currentSection == Section::HEAPS) {
      if (lineStream >> key >> range.StartAddress >> range.SizeInBytes >> alignment) {
        UINT64 msaaAlignment = D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT;
        if (alignment == msaaAlignment) {
          if (range.SizeInBytes % msaaAlignment != 0) {
            UINT64 alignedSize =
                ((range.SizeInBytes + msaaAlignment - 1) / msaaAlignment) * msaaAlignment;
            LOG_INFO << "AddressPinningUseLayer: Heap with key: " << key
                     << " has alignment set to D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT "
                        "but size "
                     << range.SizeInBytes << " is not aligned to "
                     << (msaaAlignment / static_cast<UINT64>(1024 * 1024)) << "MB, aligning to "
                     << alignedSize;
            range.SizeInBytes = alignedSize;
          }
        }
        m_AddressRanges.push_back(range);
        HeapAllocationInfo heapAllocationInfo{};
        heapAllocationInfo.m_AddressRange = range;
        heapAllocationInfo.m_Alignment = alignment;
        m_HeapAddressRanges[key] = heapAllocationInfo;
      } else {
        LOG_ERROR << "AddressPinningUseLayer: Failed to parse heap address range line: " << line;
        return;
      }
    }
  }
}

} // namespace DirectX
} // namespace gits
