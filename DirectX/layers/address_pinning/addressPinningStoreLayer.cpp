// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "addressPinningStoreLayer.h"
#include "keyUtils.h"
#include "gits.h"
#include "tools.h"

#include <fstream>

namespace gits {
namespace DirectX {

AddressPinningStoreLayer::AddressPinningStoreLayer() : Layer("AddressPinningStoreLayer") {
  if (Configurator::IsRecorder()) {
    gits::CGits::Instance().GetMessageBus().subscribe(
        {PUBLISHER_RECORDER, TOPIC_END},
        [this](Topic t, const MessagePtr& m) { storeAddressRanges(); });
  }
}

AddressPinningStoreLayer::~AddressPinningStoreLayer() {
  try {
    if (Configurator::IsPlayer()) {
      storeAddressRanges();
    }
  } catch (...) {
    topmost_exception_handler("AddressPinningStoreLayer::~AddressPinningStoreLayer");
  }
}

void AddressPinningStoreLayer::storeAddressRanges() {
  const std::filesystem::path& dumpPath =
      Configurator::IsRecorder()
          ? Configurator::Get().common.recorder.dumpPath / "addressRanges.txt"
          : Configurator::Get().common.player.streamDir / "addressRanges.txt";
  std::lock_guard<std::mutex> lock(mutex_);
  std::ofstream dumpFile(dumpPath);
  GITS_ASSERT(!dumpFile.fail());
  dumpFile << "RESOURCES\n";
  for (const auto& [key, range] : resourceAddressRanges_) {
    if (range.StartAddress == 0) {
      continue;
    }
    dumpFile << key << " " << range.StartAddress << " " << range.SizeInBytes << "\n";
  }
  dumpFile << "HEAPS\n";
  for (const auto& [key, heapAllocationInfo] : heapAddressRanges_) {
    if (heapAllocationInfo.addressRange.StartAddress == 0) {
      continue;
    }
    dumpFile << key << " " << heapAllocationInfo.addressRange.StartAddress << " "
             << heapAllocationInfo.addressRange.SizeInBytes << " " << heapAllocationInfo.alignment
             << "\n";
  }
  dumpFile.flush();
}

void AddressPinningStoreLayer::post(ID3D12DeviceCreateCommittedResourceCommand& command) {
  if (isStateRestoreKey(command.ppvResource_.key)) {
    return;
  }
  handleResource(command);
}

void AddressPinningStoreLayer::post(ID3D12Device4CreateCommittedResource1Command& command) {
  handleResource(command);
}

void AddressPinningStoreLayer::post(ID3D12Device8CreateCommittedResource2Command& command) {
  handleResource(command);
}

void AddressPinningStoreLayer::post(ID3D12Device10CreateCommittedResource3Command& command) {
  handleResource(command);
}

void AddressPinningStoreLayer::post(ID3D12DeviceCreateReservedResourceCommand& command) {
  handleResource(command);
}

void AddressPinningStoreLayer::post(ID3D12Device4CreateReservedResource1Command& command) {
  handleResource(command);
}

void AddressPinningStoreLayer::post(ID3D12Device10CreateReservedResource2Command& command) {
  handleResource(command);
}

void AddressPinningStoreLayer::post(ID3D12DeviceCreatePlacedResourceCommand& command) {
  handlePlacedResource(command);
}

void AddressPinningStoreLayer::post(ID3D12Device8CreatePlacedResource1Command& command) {
  handlePlacedResource(command);
}

void AddressPinningStoreLayer::post(ID3D12Device10CreatePlacedResource2Command& command) {
  handlePlacedResource(command);
}

void AddressPinningStoreLayer::post(ID3D12DeviceCreateHeapCommand& command) {
  handleHeap(command);
}

void AddressPinningStoreLayer::post(ID3D12Device4CreateHeap1Command& command) {
  handleHeap(command);
}

void AddressPinningStoreLayer::post(INTC_D3D12_CreateHeapCommand& command) {
  handleHeap(command);
}

void AddressPinningStoreLayer::post(ID3D12Device3OpenExistingHeapFromAddressCommand& command) {
  handleHeap(command);
}

void AddressPinningStoreLayer::post(ID3D12Device13OpenExistingHeapFromAddress1Command& command) {
  handleHeap(command);
}

void AddressPinningStoreLayer::pre(ID3D12ResourceGetGPUVirtualAddressCommand& command) {
  if (Configurator::IsRecorder()) {
    return;
  }

  if (isStateRestoreKey(command.object_.key)) {
    return;
  }

  handleGetGPUVirtualAddress(command);
}

void AddressPinningStoreLayer::post(ID3D12ResourceGetGPUVirtualAddressCommand& command) {
  if (Configurator::IsPlayer()) {
    return;
  }

  handleGetGPUVirtualAddress(command);
}

template <typename CommandT>
void AddressPinningStoreLayer::handleResource(CommandT& command) {
  if (command.result_.value != S_OK ||
      command.pDesc_.value->Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
    return;
  }
  std::lock_guard<std::mutex> lock(mutex_);
  D3D12_GPU_VIRTUAL_ADDRESS_RANGE range{};
  range.SizeInBytes = command.pDesc_.value->Width;
  resourceAddressRanges_[command.ppvResource_.key] = range;
}

template <typename CommandT>
void AddressPinningStoreLayer::handlePlacedResource(CommandT& command) {
  if (command.result_.value != S_OK ||
      command.pDesc_.value->Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
    return;
  }

  ID3D12Heap* heap = command.pHeap_.value;
  D3D12_HEAP_DESC heapDesc = heap->GetDesc();
  if (heapDesc.Flags & D3D12_HEAP_FLAG_DENY_BUFFERS) {
    return;
  }

  std::lock_guard<std::mutex> lock(mutex_);
  HeapInfo heapInfo{};
  heapInfo.heapKey = command.pHeap_.key;
  heapInfo.offset = command.HeapOffset_.value;
  heapInfoByPlacedResource_[command.ppvResource_.key] = heapInfo;
}

template <typename CommandT>
void AddressPinningStoreLayer::handleHeap(CommandT& command) {
  if (command.result_.value != S_OK) {
    return;
  }

  ID3D12Heap* heap = static_cast<ID3D12Heap*>(*command.ppvHeap_.value);
  D3D12_HEAP_DESC heapDesc = heap->GetDesc();

  if (heapDesc.Flags & D3D12_HEAP_FLAG_DENY_BUFFERS) {
    return;
  }

  std::lock_guard<std::mutex> lock(mutex_);
  HeapAllocationInfo heapAllocationInfo{};
  D3D12_GPU_VIRTUAL_ADDRESS_RANGE range{};
  range.SizeInBytes = heapDesc.SizeInBytes;
  heapAllocationInfo.addressRange = range;
  heapAllocationInfo.alignment = heapDesc.Alignment;
  heapAddressRanges_[command.ppvHeap_.key] = heapAllocationInfo;
}

template <typename CommandT>
void AddressPinningStoreLayer::handleGetGPUVirtualAddress(CommandT& command) {
  if (!command.object_.value || command.result_.value == 0) {
    return;
  }

  std::lock_guard<std::mutex> lock(mutex_);
  auto itResource = resourceAddressRanges_.find(command.object_.key);
  if (itResource != resourceAddressRanges_.end()) {
    if (itResource->second.StartAddress == 0) {
      itResource->second.StartAddress = command.result_.value;
    }
    return;
  }
  auto itHeapInfo = heapInfoByPlacedResource_.find(command.object_.key);
  if (itHeapInfo == heapInfoByPlacedResource_.end()) {
    return;
  }
  auto itHeap = heapAddressRanges_.find(itHeapInfo->second.heapKey);
  if (itHeap == heapAddressRanges_.end()) {
    LOG_ERROR << "AddressPinningStoreLayer: Heap key " << itHeapInfo->second.heapKey
              << " not found in addressRanges for placed resource key " << command.object_.key;
    return;
  }
  if (itHeap->second.addressRange.StartAddress) {
    return;
  }
  itHeap->second.addressRange.StartAddress = command.result_.value - itHeapInfo->second.offset;
}

} // namespace DirectX
} // namespace gits
