// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "dispatchOutputsDumpLayer.h"
#include "keyUtils.h"
#include "log.h"
#include "exception.h"
#include "configurationLib.h"
#include "yaml-cpp/yaml.h"

#include <fstream>
#include <d3dx12.h>
#include <wrl/client.h>

namespace gits {
namespace DirectX {

DispatchOutputsDumpLayer::DispatchOutputsDumpLayer()
    : Layer("DispatchOutputsDump"),
      m_FrameRange(Configurator::Get().directx.features.dispatchOutputsDump.frames),
      m_DispatchRange(Configurator::Get().directx.features.dispatchOutputsDump.dispatches),
      m_DryRun(Configurator::Get().directx.features.dispatchOutputsDump.dryRun),
      m_DispatchOutputsDumpService(m_DispatchOutputsAnalyzer) {
  m_InAnalysis = !m_DispatchOutputsAnalyzer.IsAnalysisDone();
  if (m_InAnalysis) {
    LOG_INFO << "DISPATCH OUTPUTS DUMP IN ANALYSIS. RUN AGAIN TO DUMP RESOURCES";
  } else {
    m_DispatchOutputsAnalyzer.ReadAnalysisFile();
  }
}

DispatchOutputsDumpLayer::~DispatchOutputsDumpLayer() {
  try {
    if (m_InAnalysis) {
      m_DispatchOutputsAnalyzer.DumpAnalysisFile();
    } else if (m_DryRun) {
      m_DispatchOutputsDumpService.DumpDryRun();
    }
  } catch (const std::exception&) {
    topmost_exception_handler("DispatchOutputsDumpLayer::~DispatchOutputsDumpLayer");
  }
}

void DispatchOutputsDumpLayer::Post(ID3D12DeviceCreateCommittedResourceCommand& c) {
  if (m_InAnalysis) {
    m_DispatchOutputsAnalyzer.CreateResource(static_cast<ID3D12Resource*>(*c.m_ppvResource.Value),
                                             c.m_ppvResource.Key);
  } else {
    m_DispatchOutputsDumpService.CreateResource(
        static_cast<ID3D12Resource*>(*c.m_ppvResource.Value), c.m_ppvResource.Key,
        c.m_InitialResourceState.Value);
  }
}

void DispatchOutputsDumpLayer::Post(ID3D12Device4CreateCommittedResource1Command& c) {
  if (m_InAnalysis) {
    m_DispatchOutputsAnalyzer.CreateResource(static_cast<ID3D12Resource*>(*c.m_ppvResource.Value),
                                             c.m_ppvResource.Key);
  } else {
    m_DispatchOutputsDumpService.CreateResource(
        static_cast<ID3D12Resource*>(*c.m_ppvResource.Value), c.m_ppvResource.Key,
        c.m_InitialResourceState.Value);
  }
}

void DispatchOutputsDumpLayer::Post(ID3D12Device8CreateCommittedResource2Command& c) {
  if (m_InAnalysis) {
    m_DispatchOutputsAnalyzer.CreateResource(static_cast<ID3D12Resource*>(*c.m_ppvResource.Value),
                                             c.m_ppvResource.Key);
  } else {
    m_DispatchOutputsDumpService.CreateResource(
        static_cast<ID3D12Resource*>(*c.m_ppvResource.Value), c.m_ppvResource.Key,
        c.m_InitialResourceState.Value);
  }
}

void DispatchOutputsDumpLayer::Post(ID3D12Device10CreateCommittedResource3Command& c) {
  if (m_InAnalysis) {
    m_DispatchOutputsAnalyzer.CreateResource(static_cast<ID3D12Resource*>(*c.m_ppvResource.Value),
                                             c.m_ppvResource.Key);
  } else {
    m_DispatchOutputsDumpService.CreateResource(
        static_cast<ID3D12Resource*>(*c.m_ppvResource.Value), c.m_ppvResource.Key,
        c.m_InitialLayout.Value);
  }
}

void DispatchOutputsDumpLayer::Post(INTC_D3D12_CreateCommittedResourceCommand& c) {
  if (m_InAnalysis) {
    m_DispatchOutputsAnalyzer.CreateResource(static_cast<ID3D12Resource*>(*c.m_ppvResource.Value),
                                             c.m_ppvResource.Key);
  } else {
    m_DispatchOutputsDumpService.CreateResource(
        static_cast<ID3D12Resource*>(*c.m_ppvResource.Value), c.m_ppvResource.Key,
        c.m_InitialResourceState.Value);
  }
}

void DispatchOutputsDumpLayer::Post(ID3D12DeviceCreatePlacedResourceCommand& c) {
  if (m_InAnalysis) {
    m_DispatchOutputsAnalyzer.CreateResource(static_cast<ID3D12Resource*>(*c.m_ppvResource.Value),
                                             c.m_ppvResource.Key);
  } else {
    m_DispatchOutputsDumpService.CreateResource(
        static_cast<ID3D12Resource*>(*c.m_ppvResource.Value), c.m_ppvResource.Key,
        c.m_InitialState.Value);
  }
}

void DispatchOutputsDumpLayer::Post(ID3D12Device8CreatePlacedResource1Command& c) {
  if (m_InAnalysis) {
    m_DispatchOutputsAnalyzer.CreateResource(static_cast<ID3D12Resource*>(*c.m_ppvResource.Value),
                                             c.m_ppvResource.Key);
  } else {
    m_DispatchOutputsDumpService.CreateResource(
        static_cast<ID3D12Resource*>(*c.m_ppvResource.Value), c.m_ppvResource.Key,
        c.m_InitialState.Value);
  }
}

void DispatchOutputsDumpLayer::Post(ID3D12Device10CreatePlacedResource2Command& c) {
  if (m_InAnalysis) {
    m_DispatchOutputsAnalyzer.CreateResource(static_cast<ID3D12Resource*>(*c.m_ppvResource.Value),
                                             c.m_ppvResource.Key);
  } else {
    m_DispatchOutputsDumpService.CreateResource(
        static_cast<ID3D12Resource*>(*c.m_ppvResource.Value), c.m_ppvResource.Key,
        c.m_InitialLayout.Value);
  }
}

void DispatchOutputsDumpLayer::Post(INTC_D3D12_CreatePlacedResourceCommand& c) {
  if (m_InAnalysis) {
    m_DispatchOutputsAnalyzer.CreateResource(static_cast<ID3D12Resource*>(*c.m_ppvResource.Value),
                                             c.m_ppvResource.Key);
  } else {
    m_DispatchOutputsDumpService.CreateResource(
        static_cast<ID3D12Resource*>(*c.m_ppvResource.Value), c.m_ppvResource.Key,
        c.m_InitialState.Value);
  }
}

void DispatchOutputsDumpLayer::Post(ID3D12DeviceCreateReservedResourceCommand& c) {
  if (m_InAnalysis) {
    m_DispatchOutputsAnalyzer.CreateResource(static_cast<ID3D12Resource*>(*c.m_ppvResource.Value),
                                             c.m_ppvResource.Key);
  } else {
    m_DispatchOutputsDumpService.CreateResource(
        static_cast<ID3D12Resource*>(*c.m_ppvResource.Value), c.m_ppvResource.Key,
        c.m_InitialState.Value);
  }
}

void DispatchOutputsDumpLayer::Post(ID3D12Device4CreateReservedResource1Command& c) {
  if (m_InAnalysis) {
    m_DispatchOutputsAnalyzer.CreateResource(static_cast<ID3D12Resource*>(*c.m_ppvResource.Value),
                                             c.m_ppvResource.Key);
  } else {
    m_DispatchOutputsDumpService.CreateResource(
        static_cast<ID3D12Resource*>(*c.m_ppvResource.Value), c.m_ppvResource.Key,
        c.m_InitialState.Value);
  }
}

void DispatchOutputsDumpLayer::Post(ID3D12Device10CreateReservedResource2Command& c) {
  if (m_InAnalysis) {
    m_DispatchOutputsAnalyzer.CreateResource(static_cast<ID3D12Resource*>(*c.m_ppvResource.Value),
                                             c.m_ppvResource.Key);
  } else {
    m_DispatchOutputsDumpService.CreateResource(
        static_cast<ID3D12Resource*>(*c.m_ppvResource.Value), c.m_ppvResource.Key,
        c.m_InitialLayout.Value);
  }
}

void DispatchOutputsDumpLayer::Post(INTC_D3D12_CreateReservedResourceCommand& c) {
  if (m_InAnalysis) {
    m_DispatchOutputsAnalyzer.CreateResource(static_cast<ID3D12Resource*>(*c.m_ppvResource.Value),
                                             c.m_ppvResource.Key);
  } else {
    m_DispatchOutputsDumpService.CreateResource(
        static_cast<ID3D12Resource*>(*c.m_ppvResource.Value), c.m_ppvResource.Key,
        c.m_InitialState.Value);
  }
}

void DispatchOutputsDumpLayer::Post(IUnknownReleaseCommand& c) {
  if (c.m_Result.Value == 0) {
    if (m_InAnalysis) {
      m_DispatchOutputsAnalyzer.DestroyInterface(c.m_Object.Key);
    } else {
      m_DispatchOutputsDumpService.DestroyInterface(c.m_Object.Key);
    }
  }
}

void DispatchOutputsDumpLayer::Post(ID3D12DeviceCreateDescriptorHeapCommand& c) {
  if (m_InAnalysis) {
    m_DispatchOutputsAnalyzer.CreateDescriptorHeap(c);
  }
}

void DispatchOutputsDumpLayer::Post(ID3D12DeviceCreateRenderTargetViewCommand& c) {
  if (m_InAnalysis) {
    auto* descriptor = new DescriptorHeapTracker::Descriptor{};
    descriptor->HeapKey = c.m_DestDescriptor.InterfaceKey;
    descriptor->DescriptorIndex = c.m_DestDescriptor.Index;
    descriptor->ResourceKey = c.m_pResource.Key;
    descriptor->Type = DescriptorHeapTracker::Descriptor::DescriptorType::RTV;
    if (c.m_pDesc.Value) {
      descriptor->RenderTargetViewDesc = *c.m_pDesc.Value;
      descriptor->IsDesc = true;
    }
    m_DispatchOutputsAnalyzer.CreateDescriptor(descriptor);
  }
}

void DispatchOutputsDumpLayer::Post(ID3D12DeviceCreateDepthStencilViewCommand& c) {
  if (m_InAnalysis) {
    auto* descriptor = new DescriptorHeapTracker::Descriptor{};
    descriptor->HeapKey = c.m_DestDescriptor.InterfaceKey;
    descriptor->DescriptorIndex = c.m_DestDescriptor.Index;
    descriptor->ResourceKey = c.m_pResource.Key;
    descriptor->Type = DescriptorHeapTracker::Descriptor::DescriptorType::DSV;
    if (c.m_pDesc.Value) {
      descriptor->DepthStencilViewDesc = *c.m_pDesc.Value;
      descriptor->IsDesc = true;
    }
    m_DispatchOutputsAnalyzer.CreateDescriptor(descriptor);
  }
}

void DispatchOutputsDumpLayer::Post(ID3D12DeviceCreateShaderResourceViewCommand& c) {
  if (m_InAnalysis) {
    auto* descriptor = new DescriptorHeapTracker::Descriptor{};
    descriptor->HeapKey = c.m_DestDescriptor.InterfaceKey;
    descriptor->DescriptorIndex = c.m_DestDescriptor.Index;
    descriptor->ResourceKey = c.m_pResource.Key;
    descriptor->Type = DescriptorHeapTracker::Descriptor::DescriptorType::SRV;
    if (c.m_pDesc.Value) {
      descriptor->ShaderResourceViewDesc = *c.m_pDesc.Value;
      descriptor->IsDesc = true;
    }
    m_DispatchOutputsAnalyzer.CreateDescriptor(descriptor);
  }
}

void DispatchOutputsDumpLayer::Post(ID3D12DeviceCreateUnorderedAccessViewCommand& c) {
  if (m_InAnalysis) {
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
    m_DispatchOutputsAnalyzer.CreateDescriptor(descriptor);
  }
}

void DispatchOutputsDumpLayer::Post(ID3D12DeviceCreateConstantBufferViewCommand& c) {
  if (m_InAnalysis) {
    auto* descriptor = new DescriptorHeapTracker::Descriptor{};
    descriptor->HeapKey = c.m_DestDescriptor.InterfaceKey;
    descriptor->DescriptorIndex = c.m_DestDescriptor.Index;
    descriptor->ResourceKey = c.m_pDesc.BufferLocationKey;
    descriptor->Type = DescriptorHeapTracker::Descriptor::DescriptorType::CBV;
    if (c.m_pDesc.Value) {
      descriptor->ConstantBufferViewDesc = *c.m_pDesc.Value;
      descriptor->IsDesc = true;
    }
    m_DispatchOutputsAnalyzer.CreateDescriptor(descriptor);
  }
}

void DispatchOutputsDumpLayer::Post(ID3D12DeviceCreateSamplerCommand& c) {
  if (m_InAnalysis) {
    auto* descriptor = new DescriptorHeapTracker::Descriptor{};
    descriptor->HeapKey = c.m_DestDescriptor.InterfaceKey;
    descriptor->DescriptorIndex = c.m_DestDescriptor.Index;
    descriptor->Type = DescriptorHeapTracker::Descriptor::DescriptorType::Sampler;
    GITS_ASSERT(c.m_pDesc.Value);
    descriptor->SamplerDesc = *c.m_pDesc.Value;
    descriptor->IsDesc = true;
    m_DispatchOutputsAnalyzer.CreateDescriptor(descriptor);
  }
}

void DispatchOutputsDumpLayer::Post(ID3D12DeviceCopyDescriptorsSimpleCommand& c) {
  if (m_InAnalysis) {
    m_DispatchOutputsAnalyzer.CopyDescriptors(c);
  }
}

void DispatchOutputsDumpLayer::Post(ID3D12DeviceCopyDescriptorsCommand& c) {
  if (m_InAnalysis) {
    m_DispatchOutputsAnalyzer.CopyDescriptors(c);
  }
}

void DispatchOutputsDumpLayer::Post(ID3D12DeviceCreateRootSignatureCommand& c) {
  if (m_InAnalysis) {
    m_DispatchOutputsAnalyzer.CreateRootSignature(c);
  }
}

void DispatchOutputsDumpLayer::Post(StateRestoreBeginCommand& c) {
  m_CurrentFrame = 0;
}

void DispatchOutputsDumpLayer::Post(StateRestoreEndCommand& c) {
  m_DispatchCount = 0;
  m_ExecuteCount = 0;
  m_CurrentFrame = 1;
}

void DispatchOutputsDumpLayer::Post(IDXGISwapChainPresentCommand& c) {
  if (!(c.m_Flags.Value & DXGI_PRESENT_TEST)) {
    m_DispatchCount = 0;
    m_ExecuteCount = 0;
    if (!IsStateRestoreKey(c.Key)) {
      ++m_CurrentFrame;
    }
  }
}

void DispatchOutputsDumpLayer::Post(IDXGISwapChain1Present1Command& c) {
  if (!(c.m_PresentFlags.Value & DXGI_PRESENT_TEST)) {
    m_DispatchCount = 0;
    m_ExecuteCount = 0;
    if (!IsStateRestoreKey(c.Key)) {
      ++m_CurrentFrame;
    }
  }
}

void DispatchOutputsDumpLayer::Post(ID3D12CommandQueueExecuteCommandListsCommand& c) {
  for (unsigned i = 0; i < c.m_NumCommandLists.Value; ++i) {
    m_DispatchCountByCommandList.erase(c.m_ppCommandLists.Keys[i]);
    if (m_InAnalysis) {
      m_DispatchOutputsAnalyzer.ExecuteCommandLists(c);
    }
  }
  ++m_ExecuteCount;
  if (!m_InAnalysis) {
    m_DispatchOutputsDumpService.ExecuteCommandLists(c, m_CurrentFrame, m_ExecuteCount);
  }
}

void DispatchOutputsDumpLayer::Post(ID3D12CommandQueueWaitCommand& c) {
  if (!m_InAnalysis) {
    m_DispatchOutputsDumpService.CommandQueueWait(c);
  }
}

void DispatchOutputsDumpLayer::Post(ID3D12CommandQueueSignalCommand& c) {
  if (!m_InAnalysis) {
    m_DispatchOutputsDumpService.CommandQueueSignal(c);
  }
}

void DispatchOutputsDumpLayer::Post(ID3D12FenceSignalCommand& c) {
  if (!m_InAnalysis) {
    m_DispatchOutputsDumpService.FenceSignal(c);
  }
}

void DispatchOutputsDumpLayer::Post(ID3D12DeviceCreateFenceCommand& c) {
  if (!m_InAnalysis) {
    m_DispatchOutputsDumpService.CreateFence(c);
  }
}

void DispatchOutputsDumpLayer::Post(ID3D12Device3EnqueueMakeResidentCommand& c) {
  if (!m_InAnalysis) {
    m_DispatchOutputsDumpService.EnqueueMakeResident(c);
  }
}

void DispatchOutputsDumpLayer::Post(ID3D12GraphicsCommandListResetCommand& c) {
  if (m_InAnalysis) {
    m_DispatchOutputsAnalyzer.ClearCommandList(c.m_Object.Key);
  }
}

void DispatchOutputsDumpLayer::Post(ID3D12GraphicsCommandListClearStateCommand& c) {
  if (m_InAnalysis) {
    m_DispatchOutputsAnalyzer.ClearCommandList(c.m_Object.Key);
  }
}

void DispatchOutputsDumpLayer::Post(ID3D12GraphicsCommandListSetComputeRootSignatureCommand& c) {
  if (m_InAnalysis) {
    m_DispatchOutputsAnalyzer.SetComputeRootSignature(c);
  }
}

void DispatchOutputsDumpLayer::Post(
    ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& c) {
  if (m_InAnalysis) {
    m_DispatchOutputsAnalyzer.SetComputeRootUnorderedAccessView(c);
  }
}

void DispatchOutputsDumpLayer::Post(
    ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& c) {
  if (m_InAnalysis) {
    m_DispatchOutputsAnalyzer.SetComputeRootDescriptorTable(c);
  }
}

void DispatchOutputsDumpLayer::Post(ID3D12GraphicsCommandListDispatchCommand& c) {
  ++m_DispatchCount;
  unsigned commandListDispatchCount = ++m_DispatchCountByCommandList[c.m_Object.Key];
  if (!m_FrameRange[m_CurrentFrame]) {
    return;
  }
  if (!m_DispatchRange[m_DispatchCount] && !m_InAnalysis) {
    return;
  }

  if (m_InAnalysis) {
    m_DispatchOutputsAnalyzer.Dispatch(c);
  } else {
    m_DispatchOutputsDumpService.Dispatch(c, m_CurrentFrame, m_DispatchCount,
                                          commandListDispatchCount);
  }
}

void DispatchOutputsDumpLayer::Post(ID3D12GraphicsCommandListResourceBarrierCommand& c) {
  if (!m_InAnalysis) {
    m_DispatchOutputsDumpService.ResourceBarrier(c);
  }
}

void DispatchOutputsDumpLayer::Post(ID3D12GraphicsCommandList7BarrierCommand& c) {
  if (!m_InAnalysis) {
    m_DispatchOutputsDumpService.Barrier(c);
  }
}

} // namespace DirectX
} // namespace gits
