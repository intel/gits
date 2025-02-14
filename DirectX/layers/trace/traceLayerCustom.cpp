// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "traceLayerAuto.h"

namespace gits {
namespace DirectX {

#pragma region MetaCommands

void TraceLayer::pre(CreateWindowMetaCommand& command) {
  if (printPre_) {
    CommandPrinter p(streamPre_, statePre_, command, "CreateWindowMetaCommand");
    p.addArgument(command.hWnd_);
    p.addArgument(command.width_);
    p.addArgument(command.height_);
    p.print(flush_);
  }
}

void TraceLayer::post(CreateWindowMetaCommand& command) {
  if (printPost_) {
    CommandPrinter p(streamPost_, statePost_, command, "CreateWindowMetaCommand");
    p.addArgument(command.hWnd_);
    p.addArgument(command.width_);
    p.addArgument(command.height_);
    p.print(flush_);
  }
}

void TraceLayer::pre(MappedDataMetaCommand& command) {
  if (printPre_) {
    CommandPrinter p(streamPre_, statePre_, command, "MappedDataMetaCommand");
    p.addArgument(command.resource_);
    p.addArgument(command.mappedAddress_);
    p.addArgument(command.offset_);
    p.addArgument(command.data_);
    p.print(flush_);
  }
}

void TraceLayer::post(MappedDataMetaCommand& command) {
  if (printPost_) {
    CommandPrinter p(streamPost_, statePost_, command, "MappedDataMetaCommand");
    p.addArgument(command.resource_);
    p.addArgument(command.mappedAddress_);
    p.addArgument(command.offset_);
    p.addArgument(command.data_);
    p.print(flush_);
  }
}

void TraceLayer::pre(CreateHeapAllocationMetaCommand& command) {
  if (printPre_) {
    CommandPrinter p(streamPre_, statePre_, command, "CreateHeapAllocationMetaCommand");
    p.addArgument(command.heap_);
    p.addArgument(command.address_);
    p.addArgument(command.data_);
    p.print(flush_);
  }
}

void TraceLayer::post(CreateHeapAllocationMetaCommand& command) {
  if (printPost_) {
    CommandPrinter p(streamPost_, statePost_, command, "CreateHeapAllocationMetaCommand");
    p.addArgument(command.heap_);
    p.addArgument(command.address_);
    p.addArgument(command.data_);
    p.print(flush_);
  }
}

#pragma endregion

void TraceLayer::pre(WaitForFenceSignaledCommand& command) {
  if (printPre_) {
    CommandPrinter p(streamPre_, statePre_, command, "WaitForFenceSignaledCommand");
    p.addArgument(command.event_);
    p.addArgument(command.fence_);
    p.addArgument(command.value_);
    p.print(flush_);
  }
}

void TraceLayer::post(WaitForFenceSignaledCommand& command) {
  if (printPost_) {
    CommandPrinter p(streamPost_, statePost_, command, "WaitForFenceSignaledCommand");
    p.addArgument(command.event_);
    p.addArgument(command.fence_);
    p.addArgument(command.value_);
    p.print(flush_);
  }
}

void TraceLayer::pre(IUnknownQueryInterfaceCommand& command) {
  if (printPre_) {
    CommandPrinter p(streamPre_, statePre_, command, "IUnknown::QueryInterface",
                     command.object_.key);
    p.addArgument(command.riid_);
    p.addArgument(command.ppvObject_);
    p.addResult(command.result_);
    p.print(flush_);
  }
}

void TraceLayer::post(IUnknownQueryInterfaceCommand& command) {
  if (printPost_) {
    CommandPrinter p(streamPost_, statePost_, command, "IUnknown::QueryInterface",
                     command.object_.key);
    p.addArgument(command.riid_);
    p.addArgument(command.ppvObject_);
    p.addResult(command.result_);
    p.print(flush_);
  }
}

void TraceLayer::pre(IUnknownAddRefCommand& command) {
  if (printPre_) {
    CommandPrinter p(streamPre_, statePre_, command, "IUnknown::AddRef", command.object_.key);
    p.addResult(command.result_);
    p.print(flush_);
  }
}

void TraceLayer::post(IUnknownAddRefCommand& command) {
  if (printPost_) {
    CommandPrinter p(streamPost_, statePost_, command, "IUnknown::AddRef", command.object_.key);
    p.addResult(command.result_);
    p.print(flush_);
  }
}

void TraceLayer::pre(IUnknownReleaseCommand& command) {
  if (printPre_) {
    CommandPrinter p(streamPre_, statePre_, command, "IUnknown::Release", command.object_.key);
    p.addResult(command.result_);
    p.print(flush_);
  }
}

void TraceLayer::post(IUnknownReleaseCommand& command) {
  if (printPost_) {
    CommandPrinter p(streamPost_, statePost_, command, "IUnknown::Release", command.object_.key);
    p.addResult(command.result_);
    p.print(flush_);
  }
}

void TraceLayer::pre(ID3D12ResourceGetGPUVirtualAddressCommand& command) {
  if (printPre_) {
    CommandPrinter p(streamPre_, statePre_, command, "ID3D12Resource::GetGPUVirtualAddress",
                     command.object_.key);
    p.addResult(reinterpret_cast<Argument<void*>&>(command.result_));
    p.print(flush_);
  }
}

void TraceLayer::post(ID3D12ResourceGetGPUVirtualAddressCommand& command) {
  if (printPost_) {
    CommandPrinter p(streamPost_, statePost_, command, "ID3D12Resource::GetGPUVirtualAddress",
                     command.object_.key);
    p.addResult(reinterpret_cast<Argument<void*>&>(command.result_));
    p.print(flush_);
  }
}

void TraceLayer::pre(ID3D12DescriptorHeapGetCPUDescriptorHandleForHeapStartCommand& command) {
  if (printPre_) {
    CommandPrinter p(streamPre_, statePre_, command,
                     "ID3D12DescriptorHeap::GetCPUDescriptorHandleForHeapStart",
                     command.object_.key);
    p.addResult(reinterpret_cast<Argument<void*>&>(command.result_));
    p.print(flush_);
  }
}

void TraceLayer::post(ID3D12DescriptorHeapGetCPUDescriptorHandleForHeapStartCommand& command) {
  if (printPost_) {
    CommandPrinter p(streamPost_, statePost_, command,
                     "ID3D12DescriptorHeap::GetCPUDescriptorHandleForHeapStart",
                     command.object_.key);
    p.addResult(reinterpret_cast<Argument<void*>&>(command.result_));
    p.print(flush_);
  }
}

void TraceLayer::pre(ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& command) {
  if (printPre_) {
    CommandPrinter p(streamPre_, statePre_, command,
                     "ID3D12DescriptorHeap::GetGPUDescriptorHandleForHeapStart",
                     command.object_.key);
    p.addResult(reinterpret_cast<Argument<void*>&>(command.result_));
    p.print(flush_);
  }
}

void TraceLayer::post(ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& command) {
  if (printPost_) {
    CommandPrinter p(streamPost_, statePost_, command,
                     "ID3D12DescriptorHeap::GetGPUDescriptorHandleForHeapStart",
                     command.object_.key);
    p.addResult(reinterpret_cast<Argument<void*>&>(command.result_));
    p.print(flush_);
  }
}

#pragma region INTCExtension

void TraceLayer::pre(INTC_D3D12_GetSupportedVersionsCommand& command) {
  if (printPre_) {
    CommandPrinter p(streamPre_, statePre_, command, "INTC_D3D12_GetSupportedVersions");
    p.addArgument(command.pDevice_);
    p.addArgument(command.pSupportedExtVersions_.value);
    p.addArgument(command.pSupportedExtVersionsCount_);
    p.addResult(command.result_);
    p.print(flush_);
  }
}

void TraceLayer::post(INTC_D3D12_GetSupportedVersionsCommand& command) {
  if (printPost_) {
    CommandPrinter p(streamPost_, statePost_, command, "INTC_D3D12_GetSupportedVersions");
    p.addArgument(command.pDevice_);
    p.addArgument(command.pSupportedExtVersions_.value);
    p.addArgument(command.pSupportedExtVersionsCount_);
    p.addResult(command.result_);
    p.print(flush_);
  }
}

void TraceLayer::pre(INTC_D3D12_CreateDeviceExtensionContextCommand& command) {
  if (printPre_) {
    CommandPrinter p(streamPre_, statePre_, command, "INTC_D3D12_CreateDeviceExtensionContext");
    p.addArgument(command.pDevice_);
    p.addArgument(command.ppExtensionContext_);
    p.addArgument(command.pExtensionInfo_);
    p.addArgument(command.pExtensionAppInfo_);
    p.addResult(command.result_);
    p.print(flush_);
  }
}

void TraceLayer::post(INTC_D3D12_CreateDeviceExtensionContextCommand& command) {
  if (printPost_) {
    CommandPrinter p(streamPost_, statePost_, command, "INTC_D3D12_CreateDeviceExtensionContext");
    p.addArgument(command.pDevice_);
    p.addArgument(command.ppExtensionContext_);
    p.addArgument(command.pExtensionInfo_);
    p.addArgument(command.pExtensionAppInfo_);
    p.addResult(command.result_);
    p.print(flush_);
  }
}

void TraceLayer::pre(INTC_D3D12_CreateDeviceExtensionContext1Command& command) {
  if (printPre_) {
    CommandPrinter p(streamPre_, statePre_, command, "INTC_D3D12_CreateDeviceExtensionContext1");
    p.addArgument(command.pDevice_);
    p.addArgument(command.ppExtensionContext_);
    p.addArgument(command.pExtensionInfo_);
    p.addArgument(command.pExtensionAppInfo_);
    p.addResult(command.result_);
    p.print(flush_);
  }
}

void TraceLayer::post(INTC_D3D12_CreateDeviceExtensionContext1Command& command) {
  if (printPost_) {
    CommandPrinter p(streamPost_, statePost_, command, "INTC_D3D12_CreateDeviceExtensionContext1");
    p.addArgument(command.pDevice_);
    p.addArgument(command.ppExtensionContext_);
    p.addArgument(command.pExtensionInfo_);
    p.addArgument(command.pExtensionAppInfo_);
    p.addResult(command.result_);
    p.print(flush_);
  }
}

void TraceLayer::pre(INTC_DestroyDeviceExtensionContextCommand& command) {
  if (printPre_) {
    CommandPrinter p(streamPre_, statePre_, command, "INTC_DestroyDeviceExtensionContext");
    p.addArgument(command.ppExtensionContext_);
    p.addResult(command.result_);
    p.print(flush_);
  }
}

void TraceLayer::post(INTC_DestroyDeviceExtensionContextCommand& command) {
  if (printPost_) {
    CommandPrinter p(streamPost_, statePost_, command, "INTC_DestroyDeviceExtensionContext");
    p.addArgument(command.ppExtensionContext_);
    p.addResult(command.result_);
    p.print(flush_);
  }
}

void TraceLayer::pre(INTC_D3D12_CheckFeatureSupportCommand& command) {
  if (printPre_) {
    CommandPrinter p(streamPre_, statePre_, command, "INTC_D3D12_CheckFeatureSupport");
    p.addArgument(command.pExtensionContext_);
    p.addArgument(command.Feature_);
    p.addArgument(command.pFeatureSupportData_);
    p.addArgument(command.FeatureSupportDataSize_);
    p.addResult(command.result_);
    p.print(flush_);
  }
}

void TraceLayer::post(INTC_D3D12_CheckFeatureSupportCommand& command) {
  if (printPost_) {
    CommandPrinter p(streamPost_, statePost_, command, "INTC_D3D12_CheckFeatureSupport");
    p.addArgument(command.pExtensionContext_);
    p.addArgument(command.Feature_);
    p.addArgument(command.pFeatureSupportData_);
    p.addArgument(command.FeatureSupportDataSize_);
    p.addResult(command.result_);
    p.print(flush_);
  }
}

void TraceLayer::pre(INTC_D3D12_SetFeatureSupportCommand& command) {
  if (printPre_) {
    CommandPrinter p(streamPre_, statePre_, command, "INTC_D3D12_SetFeatureSupport");
    p.addArgument(command.pExtensionContext_);
    p.addArgument(command.pFeature_);
    p.addResult(command.result_);
    p.print(flush_);
  }
}

void TraceLayer::post(INTC_D3D12_SetFeatureSupportCommand& command) {
  if (printPost_) {
    CommandPrinter p(streamPost_, statePost_, command, "INTC_D3D12_SetFeatureSupport");
    p.addArgument(command.pExtensionContext_);
    p.addArgument(command.pFeature_);
    p.addResult(command.result_);
    p.print(flush_);
  }
}

void TraceLayer::pre(INTC_D3D12_GetResourceAllocationInfoCommand& command) {
  if (printPre_) {
    CommandPrinter p(streamPre_, statePre_, command, "INTC_D3D12_GetResourceAllocationInfo");
    p.addArgument(command.pExtensionContext_);
    p.addArgument(command.visibleMask_);
    p.addArgument(command.numResourceDescs_);
    p.addArgument(command.pResourceDescs_);
    p.addResult(command.result_);
    p.print(flush_);
  }
}

void TraceLayer::post(INTC_D3D12_GetResourceAllocationInfoCommand& command) {
  if (printPost_) {
    CommandPrinter p(streamPost_, statePost_, command, "INTC_D3D12_GetResourceAllocationInfo");
    p.addArgument(command.pExtensionContext_);
    p.addArgument(command.visibleMask_);
    p.addArgument(command.numResourceDescs_);
    p.addArgument(command.pResourceDescs_);
    p.addResult(command.result_);
    p.print(flush_);
  }
}

void TraceLayer::pre(INTC_D3D12_CreatePlacedResourceCommand& command) {
  if (printPre_) {
    CommandPrinter p(streamPre_, statePre_, command, "INTC_D3D12_CreatePlacedResource");
    p.addArgument(command.pExtensionContext_);
    p.addArgument(command.pHeap_);
    p.addArgument(command.HeapOffset_);
    p.addArgument(command.pDesc_);
    p.addArgument(command.InitialState_);
    p.addArgument(command.pOptimizedClearValue_);
    p.addArgument(command.riid_);
    p.addArgument(command.ppvResource_);
    p.addResult(command.result_);
    p.print(flush_);
  }
}

void TraceLayer::post(INTC_D3D12_CreatePlacedResourceCommand& command) {
  if (printPost_) {
    CommandPrinter p(streamPost_, statePost_, command, "INTC_D3D12_CreatePlacedResource");
    p.addArgument(command.pExtensionContext_);
    p.addArgument(command.pHeap_);
    p.addArgument(command.HeapOffset_);
    p.addArgument(command.pDesc_);
    p.addArgument(command.InitialState_);
    p.addArgument(command.pOptimizedClearValue_);
    p.addArgument(command.riid_);
    p.addArgument(command.ppvResource_);
    p.addResult(command.result_);
    p.print(flush_);
  }
}

void TraceLayer::pre(INTC_D3D12_CreateCommittedResourceCommand& command) {
  if (printPre_) {
    CommandPrinter p(streamPre_, statePre_, command, "INTC_D3D12_CreateCommittedResource");
    p.addArgument(command.pExtensionContext_);
    p.addArgument(command.pHeapProperties_);
    p.addArgument(command.HeapFlags_);
    p.addArgument(command.pDesc_);
    p.addArgument(command.InitialResourceState_);
    p.addArgument(command.pOptimizedClearValue_);
    p.addArgument(command.riidResource_);
    p.addArgument(command.ppvResource_);
    p.addResult(command.result_);
    p.print(flush_);
  }
}

void TraceLayer::post(INTC_D3D12_CreateCommittedResourceCommand& command) {
  if (printPost_) {
    CommandPrinter p(streamPost_, statePost_, command, "INTC_D3D12_CreateCommittedResource");
    p.addArgument(command.pExtensionContext_);
    p.addArgument(command.pHeapProperties_);
    p.addArgument(command.HeapFlags_);
    p.addArgument(command.pDesc_);
    p.addArgument(command.InitialResourceState_);
    p.addArgument(command.pOptimizedClearValue_);
    p.addArgument(command.riidResource_);
    p.addArgument(command.ppvResource_);
    p.addResult(command.result_);
    p.print(flush_);
  }
}

void TraceLayer::pre(INTC_D3D12_CreateComputePipelineStateCommand& command) {
  if (printPre_) {
    CommandPrinter p(streamPre_, statePre_, command, "INTC_D3D12_CreateComputePipelineState");
    p.addArgument(command.pExtensionContext_);
    p.addArgument(command.pDesc_);
    p.addArgument(command.riid_);
    p.addArgument(command.ppPipelineState_);
    p.addResult(command.result_);
    p.print(flush_);
  }
}

void TraceLayer::post(INTC_D3D12_CreateComputePipelineStateCommand& command) {
  if (printPost_) {
    CommandPrinter p(streamPost_, statePost_, command, "INTC_D3D12_CreateComputePipelineState");
    p.addArgument(command.pExtensionContext_);
    p.addArgument(command.pDesc_);
    p.addArgument(command.riid_);
    p.addArgument(command.ppPipelineState_);
    p.addResult(command.result_);
    p.print(flush_);
  }
}

#pragma endregion

} // namespace DirectX
} // namespace gits
