// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "addressPinningStoreLayer.h"
#include "keyUtils.h"
#include "log.h"
#include "messageBus.h"

#include <fstream>

namespace gits {
namespace DirectX {

AddressPinningStoreLayer::AddressPinningStoreLayer() : Layer("AddressPinningStore") {
  if (Configurator::IsRecorder()) {
    gits::MessageBus::get().subscribe(
        {PUBLISHER_RECORDER, TOPIC_STREAM_SAVED},
        [this](Topic t, const MessagePtr& m) { StoreAddressRanges(); });
  }
}

AddressPinningStoreLayer::~AddressPinningStoreLayer() {
  try {
    if (Configurator::IsPlayer()) {
      StoreAddressRanges();
    }
  } catch (...) {
    topmost_exception_handler("AddressPinningStoreLayer::~AddressPinningStoreLayer");
  }
}

void AddressPinningStoreLayer::StoreAddressRanges() {
  const std::filesystem::path& dumpPath =
      Configurator::IsRecorder()
          ? Configurator::Get().common.recorder.dumpPath / "addressRanges.txt"
          : Configurator::Get().common.player.streamDir / "addressRanges.txt";
  std::lock_guard<std::mutex> lock(m_Mutex);
  std::ofstream dumpFile(dumpPath);
  GITS_ASSERT(!dumpFile.fail());
  dumpFile << "RESOURCES\n";
  for (const auto& [key, range] : m_ResourceAddressRanges) {
    if (range.StartAddress == 0) {
      continue;
    }
    dumpFile << key << " " << range.StartAddress << " " << range.SizeInBytes << "\n";
  }
  dumpFile << "HEAPS\n";
  for (const auto& [key, heapAllocationInfo] : m_HeapAddressRanges) {
    if (heapAllocationInfo.m_AddressRange.StartAddress == 0) {
      continue;
    }
    dumpFile << key << " " << heapAllocationInfo.m_AddressRange.StartAddress << " "
             << heapAllocationInfo.m_AddressRange.SizeInBytes << " "
             << heapAllocationInfo.m_Alignment << "\n";
  }
  dumpFile.flush();
}

void AddressPinningStoreLayer::Post(ID3D12DeviceCreateCommittedResourceCommand& command) {
  if (IsStateRestoreKey(command.m_ppvResource.Key)) {
    return;
  }
  HandleResource(command);
}

void AddressPinningStoreLayer::Post(ID3D12Device4CreateCommittedResource1Command& command) {
  HandleResource(command);
}

void AddressPinningStoreLayer::Post(ID3D12Device8CreateCommittedResource2Command& command) {
  HandleResource(command);
}

void AddressPinningStoreLayer::Post(ID3D12Device10CreateCommittedResource3Command& command) {
  HandleResource(command);
}

void AddressPinningStoreLayer::Post(ID3D12DeviceCreateReservedResourceCommand& command) {
  HandleResource(command);
}

void AddressPinningStoreLayer::Post(ID3D12Device4CreateReservedResource1Command& command) {
  HandleResource(command);
}

void AddressPinningStoreLayer::Post(ID3D12Device10CreateReservedResource2Command& command) {
  HandleResource(command);
}

void AddressPinningStoreLayer::Post(ID3D12DeviceCreatePlacedResourceCommand& command) {
  HandlePlacedResource(command);
}

void AddressPinningStoreLayer::Post(ID3D12Device8CreatePlacedResource1Command& command) {
  HandlePlacedResource(command);
}

void AddressPinningStoreLayer::Post(ID3D12Device10CreatePlacedResource2Command& command) {
  HandlePlacedResource(command);
}

void AddressPinningStoreLayer::Post(ID3D12DeviceCreateHeapCommand& command) {
  HandleHeap(command);
}

void AddressPinningStoreLayer::Post(ID3D12Device4CreateHeap1Command& command) {
  HandleHeap(command);
}

void AddressPinningStoreLayer::Post(INTC_D3D12_CreateHeapCommand& command) {
  HandleHeap(command);
}

void AddressPinningStoreLayer::Post(ID3D12Device3OpenExistingHeapFromAddressCommand& command) {
  HandleHeap(command);
}

void AddressPinningStoreLayer::Post(ID3D12Device13OpenExistingHeapFromAddress1Command& command) {
  HandleHeap(command);
}

void AddressPinningStoreLayer::Pre(ID3D12ResourceGetGPUVirtualAddressCommand& command) {
  if (Configurator::IsRecorder()) {
    return;
  }

  if (IsStateRestoreKey(command.m_Object.Key)) {
    return;
  }

  HandleGetGPUVirtualAddress(command);
}

void AddressPinningStoreLayer::Post(ID3D12ResourceGetGPUVirtualAddressCommand& command) {
  if (Configurator::IsPlayer()) {
    return;
  }

  HandleGetGPUVirtualAddress(command);
}

template <typename CommandT>
void AddressPinningStoreLayer::HandleResource(CommandT& command) {
  if (command.m_Result.Value != S_OK ||
      command.m_pDesc.Value->Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  D3D12_GPU_VIRTUAL_ADDRESS_RANGE range{};
  range.SizeInBytes = command.m_pDesc.Value->Width;
  m_ResourceAddressRanges[command.m_ppvResource.Key] = range;
}

template <typename CommandT>
void AddressPinningStoreLayer::HandlePlacedResource(CommandT& command) {
  if (command.m_Result.Value != S_OK ||
      command.m_pDesc.Value->Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
    return;
  }

  ID3D12Heap* heap = command.m_pHeap.Value;
  D3D12_HEAP_DESC heapDesc = heap->GetDesc();
  if (heapDesc.Flags & D3D12_HEAP_FLAG_DENY_BUFFERS) {
    return;
  }

  std::lock_guard<std::mutex> lock(m_Mutex);
  HeapInfo heapInfo{};
  heapInfo.m_HeapKey = command.m_pHeap.Key;
  heapInfo.m_Offset = command.m_HeapOffset.Value;
  m_HeapInfoByPlacedResource[command.m_ppvResource.Key] = heapInfo;
}

template <typename CommandT>
void AddressPinningStoreLayer::HandleHeap(CommandT& command) {
  if (command.m_Result.Value != S_OK) {
    return;
  }

  ID3D12Heap* heap = static_cast<ID3D12Heap*>(*command.m_ppvHeap.Value);
  D3D12_HEAP_DESC heapDesc = heap->GetDesc();

  if (heapDesc.Flags & D3D12_HEAP_FLAG_DENY_BUFFERS) {
    return;
  }

  std::lock_guard<std::mutex> lock(m_Mutex);
  HeapAllocationInfo heapAllocationInfo{};
  D3D12_GPU_VIRTUAL_ADDRESS_RANGE range{};
  range.SizeInBytes = heapDesc.SizeInBytes;
  heapAllocationInfo.m_AddressRange = range;
  heapAllocationInfo.m_Alignment = heapDesc.Alignment;
  m_HeapAddressRanges[command.m_ppvHeap.Key] = heapAllocationInfo;
}

template <typename CommandT>
void AddressPinningStoreLayer::HandleGetGPUVirtualAddress(CommandT& command) {
  if (!command.m_Object.Value || command.m_Result.Value == 0) {
    return;
  }

  std::lock_guard<std::mutex> lock(m_Mutex);
  auto itResource = m_ResourceAddressRanges.find(command.m_Object.Key);
  if (itResource != m_ResourceAddressRanges.end()) {
    if (itResource->second.StartAddress == 0) {
      itResource->second.StartAddress = command.m_Result.Value;
    }
    return;
  }
  auto itHeapInfo = m_HeapInfoByPlacedResource.find(command.m_Object.Key);
  if (itHeapInfo == m_HeapInfoByPlacedResource.end()) {
    return;
  }
  auto itHeap = m_HeapAddressRanges.find(itHeapInfo->second.m_HeapKey);
  if (itHeap == m_HeapAddressRanges.end()) {
    LOG_ERROR << "AddressPinningStoreLayer: Heap key " << itHeapInfo->second.m_HeapKey
              << " not found in addressRanges for placed resource key " << command.m_Object.Key;
    return;
  }
  if (itHeap->second.m_AddressRange.StartAddress) {
    return;
  }
  itHeap->second.m_AddressRange.StartAddress = command.m_Result.Value - itHeapInfo->second.m_Offset;
}

} // namespace DirectX
} // namespace gits
