// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "gpuPatchLayer.h"
#include "log.h"
#include "exception.h"

namespace gits {
namespace DirectX {

GpuPatchLayer::GpuPatchLayer(GpuAddressService& gpuAddressService)
    : Layer("GpuPatch"), m_GpuAddressService(gpuAddressService) {}

void GpuPatchLayer::Post(ID3D12DeviceCreateCommandSignatureCommand& c) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_CommandSignatures[c.m_ppvCommandSignature.Key] = *c.m_pDesc.Value;
  D3D12_COMMAND_SIGNATURE_DESC& desc = m_CommandSignatures[c.m_ppvCommandSignature.Key];
  desc.pArgumentDescs = new D3D12_INDIRECT_ARGUMENT_DESC[desc.NumArgumentDescs];
  std::copy(c.m_pDesc.Value->pArgumentDescs,
            c.m_pDesc.Value->pArgumentDescs + c.m_pDesc.Value->NumArgumentDescs,
            const_cast<D3D12_INDIRECT_ARGUMENT_DESC*>(desc.pArgumentDescs));
}

void GpuPatchLayer::Post(ID3D12GraphicsCommandListExecuteIndirectCommand& c) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_CommandSignatures.find(c.m_pCommandSignature.Key);
  GITS_ASSERT(it != m_CommandSignatures.end());

  bool raytracing = false;
  for (unsigned i = 0; i < it->second.NumArgumentDescs; ++i) {
    D3D12_INDIRECT_ARGUMENT_TYPE type = it->second.pArgumentDescs[i].Type;
    if (type == D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_RAYS) {
      raytracing = true;
    }
  }

  if (raytracing) {
    BarrierState argumentBufferState{};
    argumentBufferState.State = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    BarrierState countBufferState{};
    countBufferState.State = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
    m_GpuPatchDump.DumpArgumentBuffer(c.m_Object.Value, it->second, c.m_MaxCommandCount.Value,
                                      c.m_pArgumentBuffer.Value, c.m_ArgumentBufferOffset.Value,
                                      argumentBufferState, c.m_pCountBuffer.Value,
                                      c.m_CountBufferOffset.Value, countBufferState, c.Key);
  }
}

void GpuPatchLayer::Post(ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  if (c.m_pDesc.Value->Inputs.Type != D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL ||
      c.m_pDesc.Value->Inputs.NumDescs == 0) {
    return;
  }

  std::lock_guard<std::mutex> lock(m_Mutex);
  if (c.m_pDesc.Value->Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS) {
    GpuAddressService::GpuAddressInfo info =
        m_GpuAddressService.getGpuAddressInfo(c.m_pDesc.Value->Inputs.InstanceDescs);
    D3D12_RESOURCE_STATES resourceState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    ID3D12Resource* instancesBuffer = m_ResourcesByKey[info.ResourceKey];
    GITS_ASSERT(instancesBuffer);
    if (m_GenericReadResources.find(info.ResourceKey) != m_GenericReadResources.end()) {
      resourceState = D3D12_RESOURCE_STATE_GENERIC_READ;
    }

    BarrierState state{};
    state.State = resourceState;
    m_GpuPatchDump.DumpInstancesArrayOfPointers(c.m_Object.Value, instancesBuffer, info.Offset,
                                                c.m_pDesc.Value->Inputs.NumDescs, state, c.Key);
  }
}

void GpuPatchLayer::Post(ID3D12CommandQueueExecuteCommandListsCommand& c) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_GpuPatchDump.ExecuteCommandLists(c.Key, c.m_Object.Key, c.m_Object.Value,
                                     c.m_ppCommandLists.Value, c.m_NumCommandLists.Value);
}

void GpuPatchLayer::Post(ID3D12CommandQueueWaitCommand& c) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_GpuPatchDump.CommandQueueWait(c.Key, c.m_Object.Key, c.m_pFence.Key, c.m_Value.Value);
}

void GpuPatchLayer::Post(ID3D12CommandQueueSignalCommand& c) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_GpuPatchDump.CommandQueueSignal(c.Key, c.m_Object.Key, c.m_pFence.Key, c.m_Value.Value);
}

void GpuPatchLayer::Post(ID3D12FenceSignalCommand& c) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_GpuPatchDump.FenceSignal(c.Key, c.m_Object.Key, c.m_Value.Value);
}

void GpuPatchLayer::Post(ID3D12DeviceCreateFenceCommand& c) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_GpuPatchDump.FenceSignal(c.Key, c.m_ppFence.Key, c.m_InitialValue.Value);
}

void GpuPatchLayer::Post(ID3D12Device3EnqueueMakeResidentCommand& c) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_GpuPatchDump.FenceSignal(c.Key, c.m_pFenceToSignal.Key, c.m_FenceValueToSignal.Value);
}

void GpuPatchLayer::Post(IDXGISwapChainPresentCommand& c) {
  if (!(c.m_Flags.Value & DXGI_PRESENT_TEST)) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_GpuPatchDump.Flush();
  }
}

void GpuPatchLayer::Post(IDXGISwapChain1Present1Command& c) {
  if (!(c.m_PresentFlags.Value & DXGI_PRESENT_TEST)) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_GpuPatchDump.Flush();
  }
}

void GpuPatchLayer::Post(IUnknownReleaseCommand& c) {
  if (c.m_Result.Value == 0) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_ResourcesByKey.erase(c.m_Object.Key);
    m_GenericReadResources.erase(c.m_Object.Key);
  }
}

void GpuPatchLayer::Post(ID3D12DeviceCreatePlacedResourceCommand& c) {
  if (c.m_Result.Value != S_OK || c.m_pDesc.Value->Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  if (c.m_InitialState.Value == D3D12_RESOURCE_STATE_GENERIC_READ) {
    m_GenericReadResources.insert(c.m_ppvResource.Key);
  }
  m_ResourcesByKey[c.m_ppvResource.Key] =
      *reinterpret_cast<ID3D12Resource**>(c.m_ppvResource.Value);
}

void GpuPatchLayer::Post(ID3D12Device8CreatePlacedResource1Command& c) {
  if (c.m_Result.Value != S_OK || c.m_pDesc.Value->Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  if (c.m_InitialState.Value == D3D12_RESOURCE_STATE_GENERIC_READ) {
    m_GenericReadResources.insert(c.m_ppvResource.Key);
  }
  m_ResourcesByKey[c.m_ppvResource.Key] =
      *reinterpret_cast<ID3D12Resource**>(c.m_ppvResource.Value);
}

void GpuPatchLayer::Post(ID3D12Device10CreatePlacedResource2Command& c) {
  if (c.m_Result.Value != S_OK || c.m_pDesc.Value->Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  if (c.m_InitialLayout.Value == D3D12_BARRIER_LAYOUT_GENERIC_READ) {
    m_GenericReadResources.insert(c.m_ppvResource.Key);
  }
  m_ResourcesByKey[c.m_ppvResource.Key] =
      *reinterpret_cast<ID3D12Resource**>(c.m_ppvResource.Value);
}

void GpuPatchLayer::Post(ID3D12DeviceCreateCommittedResourceCommand& c) {
  if (c.m_Result.Value != S_OK || c.m_pDesc.Value->Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  if (c.m_InitialResourceState.Value == D3D12_RESOURCE_STATE_GENERIC_READ) {
    m_GenericReadResources.insert(c.m_ppvResource.Key);
  }
  m_ResourcesByKey[c.m_ppvResource.Key] =
      *reinterpret_cast<ID3D12Resource**>(c.m_ppvResource.Value);
}

void GpuPatchLayer::Post(ID3D12Device4CreateCommittedResource1Command& c) {
  if (c.m_Result.Value != S_OK || c.m_pDesc.Value->Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  if (c.m_InitialResourceState.Value == D3D12_RESOURCE_STATE_GENERIC_READ) {
    m_GenericReadResources.insert(c.m_ppvResource.Key);
  }
  m_ResourcesByKey[c.m_ppvResource.Key] =
      *reinterpret_cast<ID3D12Resource**>(c.m_ppvResource.Value);
}

void GpuPatchLayer::Post(ID3D12Device8CreateCommittedResource2Command& c) {
  if (c.m_Result.Value != S_OK || c.m_pDesc.Value->Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  if (c.m_InitialResourceState.Value == D3D12_RESOURCE_STATE_GENERIC_READ) {
    m_GenericReadResources.insert(c.m_ppvResource.Key);
  }
  m_ResourcesByKey[c.m_ppvResource.Key] =
      *reinterpret_cast<ID3D12Resource**>(c.m_ppvResource.Value);
}

void GpuPatchLayer::Post(ID3D12Device10CreateCommittedResource3Command& c) {
  if (c.m_Result.Value != S_OK || c.m_pDesc.Value->Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  if (c.m_InitialLayout.Value == D3D12_BARRIER_LAYOUT_GENERIC_READ) {
    m_GenericReadResources.insert(c.m_ppvResource.Key);
  }
  m_ResourcesByKey[c.m_ppvResource.Key] =
      *reinterpret_cast<ID3D12Resource**>(c.m_ppvResource.Value);
}

} // namespace DirectX
} // namespace gits
