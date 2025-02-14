// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "gpuPatchLayer.h"
#include "exception.h"
#include "gits.h"

namespace gits {
namespace DirectX {

GpuPatchLayer::GpuPatchLayer(GpuAddressService& gpuAddressService)
    : Layer("GpuPatch"), gpuAddressService_(gpuAddressService) {}

void GpuPatchLayer::post(ID3D12DeviceCreateCommandSignatureCommand& c) {
  D3D12_COMMAND_SIGNATURE_DESC& desc = commandSignatures_[c.ppvCommandSignature_.key] =
      *c.pDesc_.value;
  desc.pArgumentDescs = new D3D12_INDIRECT_ARGUMENT_DESC[desc.NumArgumentDescs];
  std::copy(c.pDesc_.value->pArgumentDescs,
            c.pDesc_.value->pArgumentDescs + c.pDesc_.value->NumArgumentDescs,
            const_cast<D3D12_INDIRECT_ARGUMENT_DESC*>(desc.pArgumentDescs));
}

void GpuPatchLayer::post(ID3D12GraphicsCommandListExecuteIndirectCommand& c) {
  auto it = commandSignatures_.find(c.pCommandSignature_.key);
  GITS_ASSERT(it != commandSignatures_.end());
  D3D12_COMMAND_SIGNATURE_DESC& commandSignature = it->second;

  bool raytracing = false;
  for (unsigned i = 0; i < commandSignature.NumArgumentDescs; ++i) {
    D3D12_INDIRECT_ARGUMENT_TYPE type = commandSignature.pArgumentDescs[i].Type;
    if (type == D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_RAYS) {
      raytracing = true;
    }
  }

  if (raytracing) {
    gpuPatchDump_.dumpArgumentBuffer(c.object_.value, it->second, c.MaxCommandCount_.value,
                                     c.pArgumentBuffer_.value, c.ArgumentBufferOffset_.value,
                                     D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, c.pCountBuffer_.value,
                                     c.CountBufferOffset_.value,
                                     D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, c.key);
  }
}

void GpuPatchLayer::post(ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  if (c.pDesc_.value->Inputs.Type != D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL ||
      c.pDesc_.value->Inputs.NumDescs == 0) {
    return;
  }

  if (c.pDesc_.value->Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS) {
    GpuAddressService::GpuAddressInfo info =
        gpuAddressService_.getGpuAddressInfo(c.pDesc_.value->Inputs.InstanceDescs);
    D3D12_RESOURCE_STATES resourceState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    ID3D12Resource* instancesBuffer{};
    {
      std::lock_guard<std::mutex> lock(mutex_);
      instancesBuffer = resourcesByKey_[info.resourceKey];
      GITS_ASSERT(instancesBuffer);
      if (genericReadResources_.find(info.resourceKey) != genericReadResources_.end()) {
        resourceState = D3D12_RESOURCE_STATE_GENERIC_READ;
      }
    }

    gpuPatchDump_.dumpInstancesArrayOfPointers(c.object_.value, instancesBuffer, info.offset,
                                               c.pDesc_.value->Inputs.NumDescs, resourceState,
                                               c.key);
  }
}

void GpuPatchLayer::post(ID3D12CommandQueueExecuteCommandListsCommand& c) {
  gpuPatchDump_.executeCommandLists(c.key, c.object_.key, c.object_.value, c.ppCommandLists_.value,
                                    c.NumCommandLists_.value);
}

void GpuPatchLayer::post(ID3D12CommandQueueWaitCommand& c) {
  gpuPatchDump_.commandQueueWait(c.key, c.object_.key, c.pFence_.key, c.Value_.value);
}

void GpuPatchLayer::post(ID3D12CommandQueueSignalCommand& c) {
  gpuPatchDump_.commandQueueSignal(c.key, c.object_.key, c.pFence_.key, c.Value_.value);
}

void GpuPatchLayer::post(ID3D12FenceSignalCommand& c) {
  gpuPatchDump_.fenceSignal(c.key, c.object_.key, c.Value_.value);
}

void GpuPatchLayer::post(IDXGISwapChainPresentCommand& c) {
  if (!(c.Flags_.value & DXGI_PRESENT_TEST)) {
    gpuPatchDump_.flush();
  }
}

void GpuPatchLayer::post(IDXGISwapChain1Present1Command& c) {
  if (!(c.PresentFlags_.value & DXGI_PRESENT_TEST)) {
    gpuPatchDump_.flush();
  }
}

void GpuPatchLayer::post(IUnknownReleaseCommand& c) {
  if (c.result_.value == 0) {
    std::lock_guard<std::mutex> lock(mutex_);
    resourcesByKey_.erase(c.object_.key);
    genericReadResources_.erase(c.object_.key);
  }
}

void GpuPatchLayer::post(ID3D12DeviceCreatePlacedResourceCommand& c) {
  if (c.result_.value != S_OK || c.pDesc_.value->Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
    return;
  }
  std::lock_guard<std::mutex> lock(mutex_);
  if (c.InitialState_.value == D3D12_RESOURCE_STATE_GENERIC_READ) {
    genericReadResources_.insert(c.ppvResource_.key);
  }
  resourcesByKey_[c.ppvResource_.key] = *reinterpret_cast<ID3D12Resource**>(c.ppvResource_.value);
}

void GpuPatchLayer::post(ID3D12Device8CreatePlacedResource1Command& c) {
  if (c.result_.value != S_OK || c.pDesc_.value->Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
    return;
  }
  std::lock_guard<std::mutex> lock(mutex_);
  if (c.InitialState_.value == D3D12_RESOURCE_STATE_GENERIC_READ) {
    genericReadResources_.insert(c.ppvResource_.key);
  }
  resourcesByKey_[c.ppvResource_.key] = *reinterpret_cast<ID3D12Resource**>(c.ppvResource_.value);
}

void GpuPatchLayer::post(ID3D12Device10CreatePlacedResource2Command& c) {
  if (c.result_.value != S_OK || c.pDesc_.value->Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
    return;
  }
  std::lock_guard<std::mutex> lock(mutex_);
  if (c.InitialLayout_.value == D3D12_BARRIER_LAYOUT_GENERIC_READ) {
    genericReadResources_.insert(c.ppvResource_.key);
  }
  resourcesByKey_[c.ppvResource_.key] = *reinterpret_cast<ID3D12Resource**>(c.ppvResource_.value);
}

void GpuPatchLayer::post(ID3D12DeviceCreateCommittedResourceCommand& c) {
  if (c.result_.value != S_OK || c.pDesc_.value->Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
    return;
  }
  std::lock_guard<std::mutex> lock(mutex_);
  if (c.InitialResourceState_.value == D3D12_RESOURCE_STATE_GENERIC_READ) {
    genericReadResources_.insert(c.ppvResource_.key);
  }
  resourcesByKey_[c.ppvResource_.key] = *reinterpret_cast<ID3D12Resource**>(c.ppvResource_.value);
}

void GpuPatchLayer::post(ID3D12Device4CreateCommittedResource1Command& c) {
  if (c.result_.value != S_OK || c.pDesc_.value->Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
    return;
  }
  std::lock_guard<std::mutex> lock(mutex_);
  if (c.InitialResourceState_.value == D3D12_RESOURCE_STATE_GENERIC_READ) {
    genericReadResources_.insert(c.ppvResource_.key);
  }
  resourcesByKey_[c.ppvResource_.key] = *reinterpret_cast<ID3D12Resource**>(c.ppvResource_.value);
}

void GpuPatchLayer::post(ID3D12Device8CreateCommittedResource2Command& c) {
  if (c.result_.value != S_OK || c.pDesc_.value->Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
    return;
  }
  std::lock_guard<std::mutex> lock(mutex_);
  if (c.InitialResourceState_.value == D3D12_RESOURCE_STATE_GENERIC_READ) {
    genericReadResources_.insert(c.ppvResource_.key);
  }
  resourcesByKey_[c.ppvResource_.key] = *reinterpret_cast<ID3D12Resource**>(c.ppvResource_.value);
}

void GpuPatchLayer::post(ID3D12Device10CreateCommittedResource3Command& c) {
  if (c.result_.value != S_OK || c.pDesc_.value->Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
    return;
  }
  std::lock_guard<std::mutex> lock(mutex_);
  if (c.InitialLayout_.value == D3D12_BARRIER_LAYOUT_GENERIC_READ) {
    genericReadResources_.insert(c.ppvResource_.key);
  }
  resourcesByKey_[c.ppvResource_.key] = *reinterpret_cast<ID3D12Resource**>(c.ppvResource_.value);
}

} // namespace DirectX
} // namespace gits
