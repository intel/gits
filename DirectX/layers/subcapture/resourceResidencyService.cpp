// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "resourceResidencyService.h"
#include "stateTrackingService.h"

namespace gits {
namespace DirectX {

void ResourceResidencyService::AddResource(unsigned resourceKey) {
  if (!resourceKey) {
    return;
  }
  ObjectState* state = m_StateService.GetState(resourceKey);
  if (!state) {
    return;
  }

  switch (state->CreationCommand->GetId()) {
  case CommandId::ID_ID3D12DEVICE_CREATECOMMITTEDRESOURCE: {
    auto* command =
        static_cast<ID3D12DeviceCreateCommittedResourceCommand*>(state->CreationCommand.get());
    if (command->m_HeapFlags.Value & D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT) {
      m_ResidencyKeys.insert(resourceKey);
    }
  } break;
  case CommandId::ID_ID3D12DEVICE4_CREATECOMMITTEDRESOURCE1: {
    auto* command =
        static_cast<ID3D12Device4CreateCommittedResource1Command*>(state->CreationCommand.get());
    if (command->m_HeapFlags.Value & D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT) {
      m_ResidencyKeys.insert(resourceKey);
    }
  } break;
  case CommandId::ID_ID3D12DEVICE8_CREATECOMMITTEDRESOURCE2: {
    auto* command =
        static_cast<ID3D12Device8CreateCommittedResource2Command*>(state->CreationCommand.get());
    if (command->m_HeapFlags.Value & D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT) {
      m_ResidencyKeys.insert(resourceKey);
    }
  } break;
  case CommandId::ID_ID3D12DEVICE10_CREATECOMMITTEDRESOURCE3: {
    auto* command =
        static_cast<ID3D12Device10CreateCommittedResource3Command*>(state->CreationCommand.get());
    if (command->m_HeapFlags.Value & D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT) {
      m_ResidencyKeys.insert(resourceKey);
    }
  } break;
  case CommandId::INTC_D3D12_CREATECOMMITTEDRESOURCE: {
    auto* command =
        static_cast<INTC_D3D12_CreateCommittedResourceCommand*>(state->CreationCommand.get());
    if (command->m_HeapFlags.Value & D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT) {
      m_ResidencyKeys.insert(resourceKey);
    }
  } break;
  case CommandId::ID_ID3D12DEVICE_CREATEPLACEDRESOURCE:
  case CommandId::ID_ID3D12DEVICE8_CREATEPLACEDRESOURCE1:
  case CommandId::ID_ID3D12DEVICE10_CREATEPLACEDRESOURCE2:
  case CommandId::INTC_D3D12_CREATEPLACEDRESOURCE: {
    unsigned heapKey = static_cast<ResourceState*>(state)->HeapKey;
    ObjectState* heapState = m_StateService.GetState(heapKey);
    if (!heapState) {
      return;
    }
    switch (heapState->CreationCommand->GetId()) {
    case CommandId::ID_ID3D12DEVICE_CREATEHEAP: {
      auto* command = static_cast<ID3D12DeviceCreateHeapCommand*>(heapState->CreationCommand.get());
      if (command->m_pDesc.Value->Flags & D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT) {
        m_ResidencyKeys.insert(heapKey);
      }
    } break;
    case CommandId::ID_ID3D12DEVICE4_CREATEHEAP1: {
      auto* command =
          static_cast<ID3D12Device4CreateHeap1Command*>(heapState->CreationCommand.get());
      if (command->m_pDesc.Value->Flags & D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT) {
        m_ResidencyKeys.insert(heapKey);
      }
    } break;
    case CommandId::INTC_D3D12_CREATEHEAP: {
      auto* command = static_cast<INTC_D3D12_CreateHeapCommand*>(heapState->CreationCommand.get());
      if (command->m_pDesc.Value->pD3D12Desc->Flags & D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT) {
        m_ResidencyKeys.insert(heapKey);
      }
    } break;
    }
  } break;
  }
}

void ResourceResidencyService::RecordMakeResident() {
  if (m_ResidencyKeys.empty()) {
    return;
  }
  ID3D12DeviceMakeResidentCommand makeResident;
  makeResident.Key = m_StateService.GetUniqueCommandKey();
  makeResident.m_Object.Key = m_DeviceKey;
  makeResident.m_NumObjects.Value = m_ResidencyKeys.size();
  ID3D12Pageable* fakePtr = reinterpret_cast<ID3D12Pageable*>(1);
  makeResident.m_ppObjects.Value = &fakePtr;
  makeResident.m_ppObjects.Size = m_ResidencyKeys.size();
  for (unsigned key : m_ResidencyKeys) {
    makeResident.m_ppObjects.Keys.push_back(key);
  }
  m_StateService.GetRecorder().Record(ID3D12DeviceMakeResidentSerializer(makeResident));
}

void ResourceResidencyService::RecordEvict() {
  if (m_ResidencyKeys.empty()) {
    return;
  }
  ID3D12DeviceEvictCommand evict;
  evict.Key = m_StateService.GetUniqueCommandKey();
  evict.m_Object.Key = m_DeviceKey;
  evict.m_NumObjects.Value = m_ResidencyKeys.size();
  ID3D12Pageable* fakePtr = reinterpret_cast<ID3D12Pageable*>(1);
  evict.m_ppObjects.Value = &fakePtr;
  evict.m_ppObjects.Size = m_ResidencyKeys.size();
  for (unsigned key : m_ResidencyKeys) {
    evict.m_ppObjects.Keys.push_back(key);
  }
  m_StateService.GetRecorder().Record(ID3D12DeviceEvictSerializer(evict));
}

} // namespace DirectX
} // namespace gits
