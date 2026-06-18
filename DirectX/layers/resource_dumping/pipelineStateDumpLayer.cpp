// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "pipelineStateDumpLayer.h"
#include "configurationLib.h"
#include "log.h"

#include <wrl/client.h>

namespace gits {
namespace DirectX {

PipelineStateDumpLayer::PipelineStateDumpLayer()
    : Layer("PipelineStateDump"),
      m_CommandKeys(Configurator::Get().directx.features.pipelineStateDump.commandKeys) {
  const std::filesystem::path& dumpDir =
      Configurator::Get().common.player.outputDir.empty()
          ? Configurator::Get().common.player.streamDir / "pipelineStates"
          : Configurator::Get().common.player.outputDir;
  if (!dumpDir.empty() && !std::filesystem::exists(dumpDir)) {
    std::filesystem::create_directories(dumpDir);
  }
  m_DumpDir = dumpDir;
}

PipelineStateDumpLayer::~PipelineStateDumpLayer() {
  for (auto& it : m_RootSignatures) {
    D3D12_ROOT_SIGNATURE_DESC2* desc = it.second;
    for (unsigned i = 0; i < desc->NumParameters; ++i) {
      if (desc->pParameters[i].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
        delete[] desc->pParameters[i].DescriptorTable.pDescriptorRanges;
      }
    }
    delete[] desc->pParameters;
    delete[] desc->pStaticSamplers;
  }
}

void PipelineStateDumpLayer::Post(IUnknownReleaseCommand& c) {
  if (c.m_Result.Value == 0) {
    m_DescriptorService.DestroyObject(c.m_Object.Key);
  }
}

void PipelineStateDumpLayer::Post(ID3D12DeviceCreateGraphicsPipelineStateCommand& c) {
  m_GraphicsPipelineStateDescs.emplace(
      c.m_ppPipelineState.Key,
      std::make_unique<D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument>(c.m_pDesc));
}

void PipelineStateDumpLayer::Post(ID3D12DeviceCreateComputePipelineStateCommand& c) {
  m_ComputePipelineStateDescs.emplace(
      c.m_ppPipelineState.Key,
      std::make_unique<D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument>(c.m_pDesc));
}

void PipelineStateDumpLayer::Post(ID3D12Device2CreatePipelineStateCommand& c) {
  m_PipelineStateDescs.emplace(
      c.m_ppPipelineState.Key,
      std::make_unique<D3D12_PIPELINE_STATE_STREAM_DESC_Argument>(c.m_pDesc));
}

void PipelineStateDumpLayer::Post(ID3D12DeviceCreateRootSignatureCommand& c) {
  Microsoft::WRL::ComPtr<ID3D12VersionedRootSignatureDeserializer> deserializer;
  HRESULT hr = D3D12CreateVersionedRootSignatureDeserializer(
      c.m_pBlobWithRootSignature.Value, c.m_blobLengthInBytes.Value, IID_PPV_ARGS(&deserializer));
  GITS_ASSERT(hr == S_OK);
  const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* versionedDesc{};
  hr = deserializer->GetRootSignatureDescAtVersion(D3D_ROOT_SIGNATURE_VERSION_1_2, &versionedDesc);
  GITS_ASSERT(hr == S_OK);
  D3D12_ROOT_SIGNATURE_DESC2* desc = new D3D12_ROOT_SIGNATURE_DESC2(versionedDesc->Desc_1_2);
  desc->pParameters = new D3D12_ROOT_PARAMETER1[desc->NumParameters];
  memcpy(const_cast<D3D12_ROOT_PARAMETER1*>(desc->pParameters), versionedDesc->Desc_1_0.pParameters,
         desc->NumParameters * sizeof(D3D12_ROOT_PARAMETER1));
  for (unsigned i = 0; i < desc->NumParameters; ++i) {
    if (desc->pParameters[i].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
      const_cast<D3D12_ROOT_PARAMETER1*>(desc->pParameters)[i].DescriptorTable.pDescriptorRanges =
          new D3D12_DESCRIPTOR_RANGE1[desc->pParameters[i].DescriptorTable.NumDescriptorRanges];
      memcpy(const_cast<D3D12_DESCRIPTOR_RANGE1*>(
                 desc->pParameters[i].DescriptorTable.pDescriptorRanges),
             versionedDesc->Desc_1_0.pParameters[i].DescriptorTable.pDescriptorRanges,
             desc->pParameters[i].DescriptorTable.NumDescriptorRanges *
                 sizeof(D3D12_DESCRIPTOR_RANGE1));
    }
  }
  desc->pStaticSamplers = new D3D12_STATIC_SAMPLER_DESC1[desc->NumStaticSamplers];
  memcpy(const_cast<D3D12_STATIC_SAMPLER_DESC1*>(desc->pStaticSamplers),
         versionedDesc->Desc_1_0.pStaticSamplers,
         desc->NumStaticSamplers * sizeof(D3D12_STATIC_SAMPLER_DESC1));
  m_RootSignatures[c.m_ppvRootSignature.Key] = desc;
}

void PipelineStateDumpLayer::Post(ID3D12DeviceCreateDescriptorHeapCommand& c) {
  m_DescriptorHeaps[c.m_ppvHeap.Key] = *c.m_pDescriptorHeapDesc.Value;
}

void PipelineStateDumpLayer::Post(ID3D12DeviceCreateRenderTargetViewCommand& c) {
  auto* descriptor = new DescriptorHeapTracker::Descriptor{};
  descriptor->HeapKey = c.m_DestDescriptor.InterfaceKey;
  descriptor->DescriptorIndex = c.m_DestDescriptor.Index;
  descriptor->ResourceKey = c.m_pResource.Key;
  descriptor->Type = DescriptorHeapTracker::Descriptor::DescriptorType::RTV;
  if (c.m_pDesc.Value) {
    descriptor->RenderTargetViewDesc = *c.m_pDesc.Value;
    descriptor->IsDesc = true;
  }
  m_DescriptorService.CreateDescriptor(descriptor);
}

void PipelineStateDumpLayer::Post(ID3D12DeviceCreateDepthStencilViewCommand& c) {
  auto* descriptor = new DescriptorHeapTracker::Descriptor{};
  descriptor->HeapKey = c.m_DestDescriptor.InterfaceKey;
  descriptor->DescriptorIndex = c.m_DestDescriptor.Index;
  descriptor->ResourceKey = c.m_pResource.Key;
  descriptor->Type = DescriptorHeapTracker::Descriptor::DescriptorType::DSV;
  if (c.m_pDesc.Value) {
    descriptor->DepthStencilViewDesc = *c.m_pDesc.Value;
    descriptor->IsDesc = true;
  }
  m_DescriptorService.CreateDescriptor(descriptor);
}

void PipelineStateDumpLayer::Post(ID3D12Device15TryCreateRenderTargetViewCommand& c) {
  if (c.m_Result.Value != S_OK) {
    return;
  }
  auto* descriptor = new DescriptorHeapTracker::Descriptor{};
  descriptor->HeapKey = c.m_DestDescriptor.InterfaceKey;
  descriptor->DescriptorIndex = c.m_DestDescriptor.Index;
  descriptor->ResourceKey = c.m_pResource.Key;
  descriptor->Type = DescriptorHeapTracker::Descriptor::DescriptorType::RTV;
  if (c.m_pDesc.Value) {
    descriptor->RenderTargetViewDesc = *c.m_pDesc.Value;
    descriptor->IsDesc = true;
  }
  m_DescriptorService.CreateDescriptor(descriptor);
}

void PipelineStateDumpLayer::Post(ID3D12Device15TryCreateDepthStencilViewCommand& c) {
  if (c.m_Result.Value != S_OK) {
    return;
  }
  auto* descriptor = new DescriptorHeapTracker::Descriptor{};
  descriptor->HeapKey = c.m_DestDescriptor.InterfaceKey;
  descriptor->DescriptorIndex = c.m_DestDescriptor.Index;
  descriptor->ResourceKey = c.m_pResource.Key;
  descriptor->Type = DescriptorHeapTracker::Descriptor::DescriptorType::DSV;
  if (c.m_pDesc.Value) {
    descriptor->DepthStencilViewDesc = *c.m_pDesc.Value;
    descriptor->IsDesc = true;
  }
  m_DescriptorService.CreateDescriptor(descriptor);
}

void PipelineStateDumpLayer::Post(ID3D12DeviceCreateShaderResourceViewCommand& c) {
  auto* descriptor = new DescriptorHeapTracker::Descriptor{};
  descriptor->HeapKey = c.m_DestDescriptor.InterfaceKey;
  descriptor->DescriptorIndex = c.m_DestDescriptor.Index;
  descriptor->ResourceKey = c.m_pResource.Key;
  descriptor->Type = DescriptorHeapTracker::Descriptor::DescriptorType::SRV;
  if (c.m_pDesc.Value) {
    descriptor->ShaderResourceViewDesc = *c.m_pDesc.Value;
    descriptor->IsDesc = true;
  }
  m_DescriptorService.CreateDescriptor(descriptor);
}

void PipelineStateDumpLayer::Post(ID3D12DeviceCreateUnorderedAccessViewCommand& c) {
  auto* descriptor = new DescriptorHeapTracker::Descriptor{};
  descriptor->HeapKey = c.m_DestDescriptor.InterfaceKey;
  descriptor->DescriptorIndex = c.m_DestDescriptor.Index;
  descriptor->ResourceKey = c.m_pResource.Key;
  descriptor->UavCounterResourceKey = c.m_pCounterResource.Key;
  descriptor->Type = DescriptorHeapTracker::Descriptor::DescriptorType::UAV;
  if (c.m_pDesc.Value) {
    descriptor->UnorderedAccessViewDesc = *c.m_pDesc.Value;
    descriptor->IsDesc = true;
  }
  m_DescriptorService.CreateDescriptor(descriptor);
}

void PipelineStateDumpLayer::Post(ID3D12DeviceCreateConstantBufferViewCommand& c) {
  auto* descriptor = new DescriptorHeapTracker::Descriptor{};
  descriptor->HeapKey = c.m_DestDescriptor.InterfaceKey;
  descriptor->DescriptorIndex = c.m_DestDescriptor.Index;
  descriptor->ResourceKey = c.m_pDesc.BufferLocationKey;
  descriptor->Type = DescriptorHeapTracker::Descriptor::DescriptorType::CBV;
  if (c.m_pDesc.Value) {
    descriptor->ConstantBufferViewDesc = *c.m_pDesc.Value;
    descriptor->IsDesc = true;
  }
  m_DescriptorService.CreateDescriptor(descriptor);
}

void PipelineStateDumpLayer::Post(ID3D12Device15TryCreateShaderResourceViewCommand& c) {
  if (c.m_Result.Value != S_OK) {
    return;
  }
  auto* descriptor = new DescriptorHeapTracker::Descriptor{};
  descriptor->HeapKey = c.m_DestDescriptor.InterfaceKey;
  descriptor->DescriptorIndex = c.m_DestDescriptor.Index;
  descriptor->ResourceKey = c.m_pResource.Key;
  descriptor->Type = DescriptorHeapTracker::Descriptor::DescriptorType::SRV;
  if (c.m_pDesc.Value) {
    descriptor->ShaderResourceViewDesc = *c.m_pDesc.Value;
    descriptor->IsDesc = true;
  }
  m_DescriptorService.CreateDescriptor(descriptor);
}

void PipelineStateDumpLayer::Post(ID3D12Device15TryCreateUnorderedAccessViewCommand& c) {
  if (c.m_Result.Value != S_OK) {
    return;
  }
  auto* descriptor = new DescriptorHeapTracker::Descriptor{};
  descriptor->HeapKey = c.m_DestDescriptor.InterfaceKey;
  descriptor->DescriptorIndex = c.m_DestDescriptor.Index;
  descriptor->ResourceKey = c.m_pResource.Key;
  descriptor->UavCounterResourceKey = c.m_pCounterResource.Key;
  descriptor->Type = DescriptorHeapTracker::Descriptor::DescriptorType::UAV;
  if (c.m_pDesc.Value) {
    descriptor->UnorderedAccessViewDesc = *c.m_pDesc.Value;
    descriptor->IsDesc = true;
  }
  m_DescriptorService.CreateDescriptor(descriptor);
}

void PipelineStateDumpLayer::Post(ID3D12Device15TryCreateConstantBufferViewCommand& c) {
  if (c.m_Result.Value != S_OK) {
    return;
  }
  auto* descriptor = new DescriptorHeapTracker::Descriptor{};
  descriptor->HeapKey = c.m_DestDescriptor.InterfaceKey;
  descriptor->DescriptorIndex = c.m_DestDescriptor.Index;
  descriptor->ResourceKey = c.m_pDesc.BufferLocationKey;
  descriptor->Type = DescriptorHeapTracker::Descriptor::DescriptorType::CBV;
  if (c.m_pDesc.Value) {
    descriptor->ConstantBufferViewDesc = *c.m_pDesc.Value;
    descriptor->IsDesc = true;
  }
  m_DescriptorService.CreateDescriptor(descriptor);
}

void PipelineStateDumpLayer::Post(ID3D12DeviceCreateSamplerCommand& c) {
  auto* descriptor = new DescriptorHeapTracker::Descriptor{};
  descriptor->HeapKey = c.m_DestDescriptor.InterfaceKey;
  descriptor->DescriptorIndex = c.m_DestDescriptor.Index;
  descriptor->Type = DescriptorHeapTracker::Descriptor::DescriptorType::Sampler;
  if (c.m_pDesc.Value) {
    descriptor->SamplerDesc = *c.m_pDesc.Value;
    descriptor->IsDesc = true;
  }
  m_DescriptorService.CreateDescriptor(descriptor);
}

void PipelineStateDumpLayer::Post(ID3D12DeviceCopyDescriptorsSimpleCommand& c) {
  m_DescriptorService.CopyDescriptors(c);
}

void PipelineStateDumpLayer::Post(ID3D12DeviceCopyDescriptorsCommand& c) {
  m_DescriptorService.CopyDescriptors(c);
}

void PipelineStateDumpLayer::Post(ID3D12GraphicsCommandListResetCommand& c) {
  GraphicsPipelineState* graphicsState = GetGraphicsState(c.m_Object.Key);
  ComputePipelineState* computeState = GetComputeState(c.m_Object.Key);
  graphicsState->Reset();
  computeState->Reset();
  if (c.m_pInitialState.Key) {
    auto it = m_GraphicsPipelineStateDescs.find(c.m_pInitialState.Key);
    if (it != m_GraphicsPipelineStateDescs.end()) {
      graphicsState->SetStateDesc(it->second.get());
    } else {
      auto it = m_ComputePipelineStateDescs.find(c.m_pInitialState.Key);
      if (it != m_ComputePipelineStateDescs.end()) {
        computeState->SetStateDesc(it->second.get());
      } else {
        auto it = m_PipelineStateDescs.find(c.m_pInitialState.Key);
        if (it != m_PipelineStateDescs.end()) {
          computeState->SetStateDesc(it->second.get());
          graphicsState->SetStateDesc(it->second.get());
        }
      }
    }
  }
}

void PipelineStateDumpLayer::Post(ID3D12GraphicsCommandListClearStateCommand& c) {}

void PipelineStateDumpLayer::Post(ID3D12GraphicsCommandListSetPipelineStateCommand& c) {
  GraphicsPipelineState* graphicsState = GetGraphicsState(c.m_Object.Key);
  ComputePipelineState* computeState = GetComputeState(c.m_Object.Key);
  if (c.m_pPipelineState.Key) {
    auto it = m_GraphicsPipelineStateDescs.find(c.m_pPipelineState.Key);
    if (it != m_GraphicsPipelineStateDescs.end()) {
      graphicsState->SetStateDesc(it->second.get());
    } else {
      auto it = m_ComputePipelineStateDescs.find(c.m_pPipelineState.Key);
      if (it != m_ComputePipelineStateDescs.end()) {
        computeState->SetStateDesc(it->second.get());
      } else {
        auto it = m_PipelineStateDescs.find(c.m_pPipelineState.Key);
        if (it != m_PipelineStateDescs.end()) {
          computeState->SetStateDesc(it->second.get());
          graphicsState->SetStateDesc(it->second.get());
        }
      }
    }
  }
}

void PipelineStateDumpLayer::Post(ID3D12GraphicsCommandListDrawInstancedCommand& c) {
  if (m_CommandKeys.Contains(c.Key)) {
    GraphicsPipelineState* state = GetGraphicsState(c.m_Object.Key);
    state->DumpState(m_DumpDir, c);
  }
}

void PipelineStateDumpLayer::Post(ID3D12GraphicsCommandListDrawIndexedInstancedCommand& c) {
  if (m_CommandKeys.Contains(c.Key)) {
    GraphicsPipelineState* state = GetGraphicsState(c.m_Object.Key);
    state->DumpState(m_DumpDir, c);
  }
}

void PipelineStateDumpLayer::Post(ID3D12GraphicsCommandListDispatchCommand& c) {
  if (m_CommandKeys.Contains(c.Key)) {
    ComputePipelineState* state = GetComputeState(c.m_Object.Key);
    state->DumpState(m_DumpDir, c);
  }
}

void PipelineStateDumpLayer::Post(ID3D12GraphicsCommandListSetDescriptorHeapsCommand& c) {}

void PipelineStateDumpLayer::Post(ID3D12GraphicsCommandListSetPredicationCommand& c) {}

void PipelineStateDumpLayer::Post(ID3D12GraphicsCommandListIASetIndexBufferCommand& c) {
  GraphicsPipelineState* graphicsState = GetGraphicsState(c.m_Object.Key);
  graphicsState->IASetIndexBuffer(c);
}

void PipelineStateDumpLayer::Post(ID3D12GraphicsCommandListIASetVertexBuffersCommand& c) {
  GraphicsPipelineState* graphicsState = GetGraphicsState(c.m_Object.Key);
  graphicsState->IASetVertexBuffers(c);
}

void PipelineStateDumpLayer::Post(ID3D12GraphicsCommandListIASetPrimitiveTopologyCommand& c) {
  GraphicsPipelineState* graphicsState = GetGraphicsState(c.m_Object.Key);
  graphicsState->IASetPrimitiveTopology(c);
}

void PipelineStateDumpLayer::Post(ID3D12GraphicsCommandListOMSetBlendFactorCommand& c) {
  GraphicsPipelineState* graphicsState = GetGraphicsState(c.m_Object.Key);
  graphicsState->OMSetBlendFactor(c);
}

void PipelineStateDumpLayer::Post(ID3D12GraphicsCommandListOMSetRenderTargetsCommand& c) {
  GraphicsPipelineState* graphicsState = GetGraphicsState(c.m_Object.Key);
  graphicsState->OMSetRenderTargets(c);
}

void PipelineStateDumpLayer::Post(ID3D12GraphicsCommandListOMSetStencilRefCommand& c) {
  GraphicsPipelineState* graphicsState = GetGraphicsState(c.m_Object.Key);
  graphicsState->OMSetStencilRef(c);
}

void PipelineStateDumpLayer::Post(ID3D12GraphicsCommandListRSSetScissorRectsCommand& c) {
  GraphicsPipelineState* graphicsState = GetGraphicsState(c.m_Object.Key);
  graphicsState->RSSetScissorRects(c);
}

void PipelineStateDumpLayer::Post(ID3D12GraphicsCommandListRSSetViewportsCommand& c) {
  GraphicsPipelineState* graphicsState = GetGraphicsState(c.m_Object.Key);
  graphicsState->RSSetViewports(c);
}

void PipelineStateDumpLayer::Post(ID3D12GraphicsCommandListSOSetTargetsCommand& c) {
  GraphicsPipelineState* graphicsState = GetGraphicsState(c.m_Object.Key);
  graphicsState->SOSetTargets(c);
}

void PipelineStateDumpLayer::Post(ID3D12GraphicsCommandListSetGraphicsRootSignatureCommand& c) {
  GraphicsPipelineState* graphicsState = GetGraphicsState(c.m_Object.Key);
  D3D12_ROOT_SIGNATURE_DESC2* desc{};
  if (c.m_pRootSignature.Key) {
    auto it = m_RootSignatures.find(c.m_pRootSignature.Key);
    GITS_ASSERT(it != m_RootSignatures.end());
    desc = it->second;
  }
  graphicsState->SetRootSignature(c.m_pRootSignature.Key, desc);
}

void PipelineStateDumpLayer::Post(ID3D12GraphicsCommandListSetGraphicsRoot32BitConstantCommand& c) {
  GraphicsPipelineState* graphicsState = GetGraphicsState(c.m_Object.Key);
  graphicsState->SetGraphicsRoot32BitConstant(c);
}

void PipelineStateDumpLayer::Post(
    ID3D12GraphicsCommandListSetGraphicsRoot32BitConstantsCommand& c) {
  GraphicsPipelineState* graphicsState = GetGraphicsState(c.m_Object.Key);
  graphicsState->SetGraphicsRoot32BitConstants(c);
}

void PipelineStateDumpLayer::Post(
    ID3D12GraphicsCommandListSetGraphicsRootConstantBufferViewCommand& c) {
  GraphicsPipelineState* graphicsState = GetGraphicsState(c.m_Object.Key);
  graphicsState->SetGraphicsRootConstantBufferView(c);
}

void PipelineStateDumpLayer::Post(
    ID3D12GraphicsCommandListSetGraphicsRootUnorderedAccessViewCommand& c) {
  GraphicsPipelineState* graphicsState = GetGraphicsState(c.m_Object.Key);
  graphicsState->SetGraphicsRootUnorderedAccessView(c);
}

void PipelineStateDumpLayer::Post(
    ID3D12GraphicsCommandListSetGraphicsRootShaderResourceViewCommand& c) {
  GraphicsPipelineState* graphicsState = GetGraphicsState(c.m_Object.Key);
  graphicsState->SetGraphicsRootShaderResourceView(c);
}

void PipelineStateDumpLayer::Post(
    ID3D12GraphicsCommandListSetGraphicsRootDescriptorTableCommand& c) {
  GraphicsPipelineState* graphicsState = GetGraphicsState(c.m_Object.Key);
  graphicsState->SetGraphicsRootDescriptorTable(c);
}

void PipelineStateDumpLayer::Post(ID3D12GraphicsCommandListSetComputeRootSignatureCommand& c) {
  ComputePipelineState* computeState = GetComputeState(c.m_Object.Key);
  D3D12_ROOT_SIGNATURE_DESC2* desc{};
  if (c.m_pRootSignature.Key) {
    auto it = m_RootSignatures.find(c.m_pRootSignature.Key);
    GITS_ASSERT(it != m_RootSignatures.end());
    desc = it->second;
  }
  computeState->SetRootSignature(c.m_pRootSignature.Key, desc);
}

void PipelineStateDumpLayer::Post(ID3D12GraphicsCommandListSetComputeRoot32BitConstantCommand& c) {
  ComputePipelineState* computeState = GetComputeState(c.m_Object.Key);
  computeState->SetComputeRoot32BitConstant(c);
}

void PipelineStateDumpLayer::Post(ID3D12GraphicsCommandListSetComputeRoot32BitConstantsCommand& c) {
  ComputePipelineState* computeState = GetComputeState(c.m_Object.Key);
  computeState->SetComputeRoot32BitConstants(c);
}

void PipelineStateDumpLayer::Post(
    ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand& c) {
  ComputePipelineState* computeState = GetComputeState(c.m_Object.Key);
  computeState->SetComputeRootConstantBufferView(c);
}

void PipelineStateDumpLayer::Post(
    ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& c) {
  ComputePipelineState* computeState = GetComputeState(c.m_Object.Key);
  computeState->SetComputeRootUnorderedAccessView(c);
}

void PipelineStateDumpLayer::Post(
    ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand& c) {
  ComputePipelineState* computeState = GetComputeState(c.m_Object.Key);
  computeState->SetComputeRootShaderResourceView(c);
}

void PipelineStateDumpLayer::Post(
    ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& c) {
  ComputePipelineState* computeState = GetComputeState(c.m_Object.Key);
  computeState->SetComputeRootDescriptorTable(c);
}

} // namespace DirectX
} // namespace gits
