// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "addressPinningUseLayer.h"
#include "configurationLib.h"
#include "tools.h"
#include "log.h"
#include "intelExtensions.h"

#include <fstream>

namespace gits {
namespace DirectX {

AddressPinningUseLayer::AddressPinningUseLayer() : Layer("AddressPinningUseLayer") {
  readAddressRanges();
}

void AddressPinningUseLayer::pre(D3D12CreateDeviceCommand& command) {
  if (command.result_.value != S_OK) {
    return;
  }

  HRESULT hr = D3D12GetInterface(CLSID_D3D12Tools, IID_PPV_ARGS(&d3d12Tools_));
  GITS_ASSERT(hr == S_OK);
  hr = d3d12Tools_->ReserveGPUVARangesAtCreate(addressRanges_.data(),
                                               static_cast<UINT>(addressRanges_.size()));
  GITS_ASSERT(hr == S_OK);
}

void AddressPinningUseLayer::post(D3D12CreateDeviceCommand& command) {
  if (command.result_.value != S_OK) {
    return;
  }

  ID3D12Device* device = static_cast<ID3D12Device*>(*command.ppDevice_.value);
  HRESULT hr = device->QueryInterface(IID_PPV_ARGS(&deviceTools_));
  GITS_ASSERT(hr == S_OK);
}

void AddressPinningUseLayer::pre(ID3D12DeviceCreateHeapCommand& command) {
  preHeap(command);
}

void AddressPinningUseLayer::post(ID3D12DeviceCreateHeapCommand& command) {
  postHeap(command);
}

void AddressPinningUseLayer::pre(ID3D12Device4CreateHeap1Command& command) {
  preHeap(command);
}

void AddressPinningUseLayer::post(ID3D12Device4CreateHeap1Command& command) {
  postHeap(command);
}

void AddressPinningUseLayer::pre(INTC_D3D12_CreateHeapCommand& command) {
  if (command.result_.value != S_OK) {
    return;
  }
  D3D12_HEAP_DESC* desc = command.pDesc_.value->pD3D12Desc;
  if (desc->Flags & D3D12_HEAP_FLAG_DENY_BUFFERS) {
    return;
  }
  auto it = heapAddressRanges_.find(command.ppvHeap_.key);
  if (it == heapAddressRanges_.end()) {
    return;
  }
  deviceTools_->SetNextAllocationAddress(it->second.addressRange.StartAddress);
}

void AddressPinningUseLayer::post(INTC_D3D12_CreateHeapCommand& command) {
  if (command.result_.value != S_OK) {
    return;
  }
  D3D12_HEAP_DESC* desc = command.pDesc_.value->pD3D12Desc;
  if (desc->Flags & D3D12_HEAP_FLAG_DENY_BUFFERS) {
    return;
  }
  auto it = heapAddressRanges_.find(command.ppvHeap_.key);
  if (it == heapAddressRanges_.end()) {
    return;
  }
  ID3D12Heap* heap = static_cast<ID3D12Heap*>(*command.ppvHeap_.value);
  ID3D12PageableTools* pPageableTools = nullptr;
  HRESULT hr = heap->QueryInterface(IID_PPV_ARGS(&pPageableTools));
  GITS_ASSERT(hr == S_OK);
  D3D12_GPU_VIRTUAL_ADDRESS_RANGE range{};
  hr = pPageableTools->GetAllocation(&range);
  GITS_ASSERT(hr == S_OK);
  GITS_ASSERT(it->second.addressRange.StartAddress == range.StartAddress);
}

void AddressPinningUseLayer::pre(CreateHeapAllocationMetaCommand& command) {
  command.skip = true;
}

void AddressPinningUseLayer::pre(ID3D12Device3OpenExistingHeapFromAddressCommand& command) {
  preOpenExistingHeap(command);
}

void AddressPinningUseLayer::pre(ID3D12Device13OpenExistingHeapFromAddress1Command& command) {
  preOpenExistingHeap(command);
}

void AddressPinningUseLayer::pre(ID3D12DeviceCreateCommittedResourceCommand& command) {
  preResource(command);
}

void AddressPinningUseLayer::post(ID3D12DeviceCreateCommittedResourceCommand& command) {
  postResource(command);
}

void AddressPinningUseLayer::pre(ID3D12Device4CreateCommittedResource1Command& command) {
  preResource(command);
}

void AddressPinningUseLayer::post(ID3D12Device4CreateCommittedResource1Command& command) {
  postResource(command);
}

void AddressPinningUseLayer::pre(ID3D12Device8CreateCommittedResource2Command& command) {
  preResource(command);
}

void AddressPinningUseLayer::post(ID3D12Device8CreateCommittedResource2Command& command) {
  postResource(command);
}

void AddressPinningUseLayer::pre(ID3D12Device10CreateCommittedResource3Command& command) {
  preResource(command);
}

void AddressPinningUseLayer::post(ID3D12Device10CreateCommittedResource3Command& command) {
  postResource(command);
}

void AddressPinningUseLayer::pre(ID3D12DeviceCreateReservedResourceCommand& command) {
  preResource(command);
}

void AddressPinningUseLayer::post(ID3D12DeviceCreateReservedResourceCommand& command) {
  postResource(command);
}

void AddressPinningUseLayer::pre(ID3D12Device4CreateReservedResource1Command& command) {
  preResource(command);
}

void AddressPinningUseLayer::post(ID3D12Device4CreateReservedResource1Command& command) {
  postResource(command);
}

void AddressPinningUseLayer::pre(ID3D12Device10CreateReservedResource2Command& command) {
  preResource(command);
}

void AddressPinningUseLayer::post(ID3D12Device10CreateReservedResource2Command& command) {
  postResource(command);
}

void AddressPinningUseLayer::pre(ID3D12DeviceCreatePlacedResourceCommand& command) {
  if (command.result_.value != S_OK) {
    return;
  }

  if (changedHeaps_.count(command.pHeap_.key)) {
    command.pDesc_.value->Flags &= ~D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER;
  }
}

void AddressPinningUseLayer::post(ID3D12DeviceCreatePlacedResourceCommand& command) {
  if (command.result_.value != S_OK) {
    return;
  }

  if (changedHeaps_.count(command.pHeap_.key)) {
    command.pDesc_.value->Flags |= D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER;
  }
}

void AddressPinningUseLayer::pre(ID3D12Device8CreatePlacedResource1Command& command) {
  if (command.result_.value != S_OK) {
    return;
  }

  if (changedHeaps_.count(command.pHeap_.key)) {
    command.pDesc_.value->Flags &= ~D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER;
  }
}

void AddressPinningUseLayer::post(ID3D12Device8CreatePlacedResource1Command& command) {
  if (command.result_.value != S_OK) {
    return;
  }

  if (changedHeaps_.count(command.pHeap_.key)) {
    command.pDesc_.value->Flags |= D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER;
  }
}

void AddressPinningUseLayer::pre(ID3D12Device10CreatePlacedResource2Command& command) {
  if (command.result_.value != S_OK) {
    return;
  }

  if (changedHeaps_.count(command.pHeap_.key)) {
    command.pDesc_.value->Flags &= ~D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER;
  }
}

void AddressPinningUseLayer::post(ID3D12Device10CreatePlacedResource2Command& command) {
  if (command.result_.value != S_OK) {
    return;
  }

  if (changedHeaps_.count(command.pHeap_.key)) {
    command.pDesc_.value->Flags |= D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER;
  }
}

template <typename CommandT>
void AddressPinningUseLayer::preResource(CommandT& command) {
  if (command.result_.value != S_OK ||
      command.pDesc_.value->Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
    return;
  }

  auto it = resourceAddressRanges_.find(command.ppvResource_.key);
  if (it == resourceAddressRanges_.end()) {
    return;
  }
  deviceTools_->SetNextAllocationAddress(it->second.StartAddress);
}

template <typename CommandT>
void AddressPinningUseLayer::postResource(CommandT& command) {
  if (command.result_.value != S_OK ||
      command.pDesc_.value->Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
    return;
  }

  auto it = resourceAddressRanges_.find(command.ppvResource_.key);
  if (it == resourceAddressRanges_.end()) {
    return;
  }
  ID3D12Resource* resource = static_cast<ID3D12Resource*>(*command.ppvResource_.value);
  GITS_ASSERT(it->second.StartAddress == resource->GetGPUVirtualAddress());
}

template <typename CommandT>
void AddressPinningUseLayer::preHeap(CommandT& command) {
  if (command.result_.value != S_OK || command.pDesc_.value->Flags & D3D12_HEAP_FLAG_DENY_BUFFERS) {
    return;
  }

  auto it = heapAddressRanges_.find(command.ppvHeap_.key);
  if (it == heapAddressRanges_.end()) {
    return;
  }
  deviceTools_->SetNextAllocationAddress(it->second.addressRange.StartAddress);
}

template <typename CommandT>
void AddressPinningUseLayer::postHeap(CommandT& command) {
  if (command.result_.value != S_OK || command.pDesc_.value->Flags & D3D12_HEAP_FLAG_DENY_BUFFERS) {
    return;
  }
  auto it = heapAddressRanges_.find(command.ppvHeap_.key);
  if (it == heapAddressRanges_.end()) {
    return;
  }
  ID3D12Heap* heap = static_cast<ID3D12Heap*>(*command.ppvHeap_.value);
  ID3D12PageableTools* pPageableTools = nullptr;
  HRESULT hr = heap->QueryInterface(IID_PPV_ARGS(&pPageableTools));
  GITS_ASSERT(hr == S_OK);
  D3D12_GPU_VIRTUAL_ADDRESS_RANGE range{};
  hr = pPageableTools->GetAllocation(&range);
  GITS_ASSERT(hr == S_OK);
  GITS_ASSERT(it->second.addressRange.StartAddress == range.StartAddress);
}

template <typename CommandT>
void AddressPinningUseLayer::preOpenExistingHeap(CommandT& command) {
  if (command.result_.value != S_OK) {
    return;
  }

  auto it = heapAddressRanges_.find(command.ppvHeap_.key);
  if (it == heapAddressRanges_.end()) {
    return;
  }

  auto* device = command.object_.value;
  ID3D12Heap* heap = nullptr;
  D3D12_HEAP_DESC heapDesc{};
  HRESULT hr{};

  heapDesc.SizeInBytes = it->second.addressRange.SizeInBytes;
  heapDesc.Properties.Type = D3D12_HEAP_TYPE_CUSTOM;
  heapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
  heapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
  heapDesc.Properties.CreationNodeMask = 1;
  heapDesc.Properties.VisibleNodeMask = 1;
  heapDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
  heapDesc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;

  deviceTools_->SetNextAllocationAddress(it->second.addressRange.StartAddress);
  hr = device->CreateHeap(&heapDesc, IID_PPV_ARGS(&heap));
  GITS_ASSERT(hr == S_OK);

  ID3D12PageableTools* pPageableTools = nullptr;
  hr = heap->QueryInterface(IID_PPV_ARGS(&pPageableTools));
  GITS_ASSERT(hr == S_OK);
  D3D12_GPU_VIRTUAL_ADDRESS_RANGE range{};
  hr = pPageableTools->GetAllocation(&range);
  GITS_ASSERT(hr == S_OK);
  GITS_ASSERT(it->second.addressRange.StartAddress == range.StartAddress);

  changedHeaps_.insert(command.ppvHeap_.key);
  *command.ppvHeap_.value = heap;
  command.result_.value = S_OK;
  command.skip = true;
}

void AddressPinningUseLayer::readAddressRanges() {
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
        addressRanges_.push_back(range);
        resourceAddressRanges_[key] = range;
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
        addressRanges_.push_back(range);
        HeapAllocationInfo heapAllocationInfo{};
        heapAllocationInfo.addressRange = range;
        heapAllocationInfo.alignment = alignment;
        heapAddressRanges_[key] = heapAllocationInfo;
      } else {
        LOG_ERROR << "AddressPinningUseLayer: Failed to parse heap address range line: " << line;
        return;
      }
    }
  }
}

} // namespace DirectX
} // namespace gits
