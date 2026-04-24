// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "traceLayerAuto.h"
#include <unordered_map>

namespace gits {
namespace DirectX {

static std::string uint64MarkerToStr(uint64_t value) {
  static const std::unordered_map<uint64_t, std::string> enumMap = {
      {MarkerUInt64Command::Value::NONE, "NONE"},
      {MarkerUInt64Command::Value::STATE_RESTORE_OBJECTS_BEGIN, "STATE_RESTORE_OBJECTS_BEGIN"},
      {MarkerUInt64Command::Value::STATE_RESTORE_OBJECTS_END, "STATE_RESTORE_OBJECTS_END"},
      {MarkerUInt64Command::Value::STATE_RESTORE_RTAS_BEGIN, "STATE_RESTORE_RTAS_BEGIN"},
      {MarkerUInt64Command::Value::STATE_RESTORE_RTAS_END, "STATE_RESTORE_RTAS_END"},
      {MarkerUInt64Command::Value::STATE_RESTORE_RESOURCES_BEGIN, "STATE_RESTORE_RESOURCES_BEGIN"},
      {MarkerUInt64Command::Value::STATE_RESTORE_RESOURCES_END, "STATE_RESTORE_RESOURCES_END"},
      {MarkerUInt64Command::Value::GPU_EXECUTION_BEGIN, "GPU_EXECUTION_BEGIN"},
      {MarkerUInt64Command::Value::GPU_EXECUTION_END, "GPU_EXECUTION_END"}};

  return enumMap.contains(value) ? enumMap.at(value) : "UNKNOWN";
}

void TraceLayer::Pre(StateRestoreBeginCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "StateRestoreBegin");
    m_StreamPre << "STATE_RESTORE_BEGIN\n";
    if (m_Flush) {
      m_StreamPre.Flush();
    }
  }
}

void TraceLayer::Post(StateRestoreBeginCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "StateRestoreBegin");
    m_StreamPost << "STATE_RESTORE_BEGIN\n";
    if (m_Flush) {
      m_StreamPost.Flush();
    }
  }
}

void TraceLayer::Pre(StateRestoreEndCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "StateRestoreEnd");
    m_StreamPre << "STATE_RESTORE_END\n";
    if (m_Flush) {
      m_StreamPre.Flush();
    }
  }
}

void TraceLayer::Post(StateRestoreEndCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "StateRestoreEnd");
    m_StreamPost << "STATE_RESTORE_END\n";
    if (m_Flush) {
      m_StreamPost.Flush();
    }
  }
}

void TraceLayer::Pre(MarkerUInt64Command& command) {
  if (m_PrintPre) {
    m_StreamPre << "MARKER_" << uint64MarkerToStr(command.m_Value.Value) << "\n";
    if (m_Flush) {
      m_StreamPre.Flush();
    }
  }
}

void TraceLayer::Post(MarkerUInt64Command& command) {
  if (m_PrintPost) {
    m_StreamPost << "MARKER_" << uint64MarkerToStr(command.m_Value.Value) << "\n";
    if (m_Flush) {
      m_StreamPost.Flush();
    }
  }
}

#pragma region MetaCommands

void TraceLayer::Pre(CreateWindowMetaCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "CreateWindowMetaCommand");
    p.addArgument(command.m_hWnd);
    p.addArgument(command.m_width);
    p.addArgument(command.m_height);
    p.print(m_Flush);
  }
}

void TraceLayer::Post(CreateWindowMetaCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "CreateWindowMetaCommand");
    p.addArgument(command.m_hWnd);
    p.addArgument(command.m_width);
    p.addArgument(command.m_height);
    p.print(m_Flush);
  }
}

void TraceLayer::Pre(MappedDataMetaCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "MappedDataMetaCommand");
    p.addArgument(command.m_resource);
    p.addArgument(command.m_mappedAddress);
    p.addArgument(command.m_offset);
    p.addArgument(command.m_data);
    p.print(m_Flush);
  }
}

void TraceLayer::Post(MappedDataMetaCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "MappedDataMetaCommand");
    p.addArgument(command.m_resource);
    p.addArgument(command.m_mappedAddress);
    p.addArgument(command.m_offset);
    p.addArgument(command.m_data);
    p.print(m_Flush);
  }
}

void TraceLayer::Pre(CreateHeapAllocationMetaCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "CreateHeapAllocationMetaCommand");
    p.addArgument(command.m_heap);
    p.addArgument(command.m_address);
    p.addArgument(command.m_data);
    p.print(m_Flush);
  }
}

void TraceLayer::Post(CreateHeapAllocationMetaCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "CreateHeapAllocationMetaCommand");
    p.addArgument(command.m_heap);
    p.addArgument(command.m_address);
    p.addArgument(command.m_data);
    p.print(m_Flush);
  }
}

#pragma endregion

void TraceLayer::Pre(WaitForFenceSignaledCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "WaitForFenceSignaledCommand");
    p.addArgument(command.m_event);
    p.addArgument(command.m_fence);
    p.addArgument(command.m_Value);
    p.print(m_Flush);
  }
}

void TraceLayer::Post(WaitForFenceSignaledCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "WaitForFenceSignaledCommand");
    p.addArgument(command.m_event);
    p.addArgument(command.m_fence);
    p.addArgument(command.m_Value);
    p.print(m_Flush);
  }
}

void TraceLayer::Pre(DllContainerMetaCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "DllContainerMetaCommand");
    p.addArgument(command.m_dllName);
    p.addArgument(command.m_dllData);
    p.print(m_Flush);
  }
}

void TraceLayer::Post(DllContainerMetaCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "DllContainerMetaCommand");
    p.addArgument(command.m_dllName);
    p.addArgument(command.m_dllData);
    p.print(m_Flush);
  }
}

void TraceLayer::Pre(IUnknownQueryInterfaceCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "IUnknown::QueryInterface",
                     command.m_Object.Key);
    p.addArgument(command.m_riid);
    p.addArgument(command.m_ppvObject);
    p.addResult(command.m_Result);
    p.print(m_Flush);
  }
}

void TraceLayer::Post(IUnknownQueryInterfaceCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "IUnknown::QueryInterface",
                     command.m_Object.Key);
    p.addArgument(command.m_riid);
    p.addArgument(command.m_ppvObject);
    p.addResult(command.m_Result);
    p.print(m_Flush);
  }
}

void TraceLayer::Pre(IUnknownAddRefCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "IUnknown::AddRef", command.m_Object.Key);
    p.addResult(command.m_Result);
    p.print(m_Flush);
  }
}

void TraceLayer::Post(IUnknownAddRefCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "IUnknown::AddRef", command.m_Object.Key);
    p.addResult(command.m_Result);
    p.print(m_Flush);
  }
}

void TraceLayer::Pre(IUnknownReleaseCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "IUnknown::Release", command.m_Object.Key);
    p.addResult(command.m_Result);
    p.print(m_Flush);
  }
}

void TraceLayer::Post(IUnknownReleaseCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "IUnknown::Release", command.m_Object.Key);
    p.addResult(command.m_Result);
    p.print(m_Flush);
  }
}

void TraceLayer::Pre(ID3D12ResourceGetGPUVirtualAddressCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "ID3D12Resource::GetGPUVirtualAddress",
                     command.m_Object.Key);
    p.addResult(reinterpret_cast<Argument<void*>&>(command.m_Result));
    p.print(m_Flush);
  }
}

void TraceLayer::Post(ID3D12ResourceGetGPUVirtualAddressCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "ID3D12Resource::GetGPUVirtualAddress",
                     command.m_Object.Key);
    p.addResult(reinterpret_cast<Argument<void*>&>(command.m_Result));
    p.print(m_Flush);
  }
}

void TraceLayer::Pre(ID3D12DescriptorHeapGetCPUDescriptorHandleForHeapStartCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command,
                     "ID3D12DescriptorHeap::GetCPUDescriptorHandleForHeapStart",
                     command.m_Object.Key);
    p.addResult(reinterpret_cast<Argument<void*>&>(command.m_Result));
    p.print(m_Flush);
  }
}

void TraceLayer::Post(ID3D12DescriptorHeapGetCPUDescriptorHandleForHeapStartCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command,
                     "ID3D12DescriptorHeap::GetCPUDescriptorHandleForHeapStart",
                     command.m_Object.Key);
    p.addResult(reinterpret_cast<Argument<void*>&>(command.m_Result));
    p.print(m_Flush);
  }
}

void TraceLayer::Pre(ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command,
                     "ID3D12DescriptorHeap::GetGPUDescriptorHandleForHeapStart",
                     command.m_Object.Key);
    p.addResult(reinterpret_cast<Argument<void*>&>(command.m_Result));
    p.print(m_Flush);
  }
}

void TraceLayer::Post(ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command,
                     "ID3D12DescriptorHeap::GetGPUDescriptorHandleForHeapStart",
                     command.m_Object.Key);
    p.addResult(reinterpret_cast<Argument<void*>&>(command.m_Result));
    p.print(m_Flush);
  }
}

void TraceLayer::Pre(ID3D12GraphicsCommandListBeginEventCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "ID3D12GraphicsCommandList::BeginEvent",
                     command.m_Object.Key);
    p.addArgument(command.m_Metadata);
    LPCWSTR_Argument arg(static_cast<wchar_t*>(command.m_pData.Value));
    p.addArgument(arg);
    p.print(m_Flush);
  }
}

void TraceLayer::Post(ID3D12GraphicsCommandListBeginEventCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "ID3D12GraphicsCommandList::BeginEvent",
                     command.m_Object.Key);
    p.addArgument(command.m_Metadata);
    LPCWSTR_Argument arg(static_cast<wchar_t*>(command.m_pData.Value));
    p.addArgument(arg);
    p.print(m_Flush);
  }
}

#pragma region INTCExtension

void TraceLayer::Pre(INTC_D3D12_GetSupportedVersionsCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "INTC_D3D12_GetSupportedVersions");
    p.addArgument(command.m_pDevice);
    p.addArgument(command.m_pSupportedExtVersions);
    p.addArgument(command.m_pSupportedExtVersionsCount);
    p.addResult(command.m_Result);
    p.print(m_Flush);
  }
}

void TraceLayer::Post(INTC_D3D12_GetSupportedVersionsCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "INTC_D3D12_GetSupportedVersions");
    p.addArgument(command.m_pDevice);
    p.addArgument(command.m_pSupportedExtVersions);
    p.addArgument(command.m_pSupportedExtVersionsCount);
    p.addResult(command.m_Result);
    p.print(m_Flush);
  }
}

void TraceLayer::Pre(INTC_D3D12_CreateDeviceExtensionContextCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "INTC_D3D12_CreateDeviceExtensionContext");
    p.addArgument(command.m_pDevice);
    p.addArgument(command.m_ppExtensionContext);
    p.addArgument(command.m_pExtensionInfo);
    p.addArgument(command.m_pExtensionAppInfo);
    p.addResult(command.m_Result);
    p.print(m_Flush);
  }
}

void TraceLayer::Post(INTC_D3D12_CreateDeviceExtensionContextCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "INTC_D3D12_CreateDeviceExtensionContext");
    p.addArgument(command.m_pDevice);
    p.addArgument(command.m_ppExtensionContext);
    p.addArgument(command.m_pExtensionInfo);
    p.addArgument(command.m_pExtensionAppInfo);
    p.addResult(command.m_Result);
    p.print(m_Flush);
  }
}

void TraceLayer::Pre(INTC_D3D12_CreateDeviceExtensionContext1Command& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "INTC_D3D12_CreateDeviceExtensionContext1");
    p.addArgument(command.m_pDevice);
    p.addArgument(command.m_ppExtensionContext);
    p.addArgument(command.m_pExtensionInfo);
    p.addArgument(command.m_pExtensionAppInfo);
    p.addResult(command.m_Result);
    p.print(m_Flush);
  }
}

void TraceLayer::Post(INTC_D3D12_CreateDeviceExtensionContext1Command& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command,
                     "INTC_D3D12_CreateDeviceExtensionContext1");
    p.addArgument(command.m_pDevice);
    p.addArgument(command.m_ppExtensionContext);
    p.addArgument(command.m_pExtensionInfo);
    p.addArgument(command.m_pExtensionAppInfo);
    p.addResult(command.m_Result);
    p.print(m_Flush);
  }
}

void TraceLayer::Pre(INTC_D3D12_SetApplicationInfoCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "INTC_D3D12_SetApplicationInfo");
    p.addArgument(command.m_pExtensionAppInfo);
    p.addResult(command.m_Result);
    p.print(m_Flush);
  }
}

void TraceLayer::Post(INTC_D3D12_SetApplicationInfoCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "INTC_D3D12_SetApplicationInfo");
    p.addArgument(command.m_pExtensionAppInfo);
    p.addResult(command.m_Result);
    p.print(m_Flush);
  }
}

void TraceLayer::Pre(INTC_DestroyDeviceExtensionContextCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "INTC_DestroyDeviceExtensionContext");
    p.addArgument(command.m_ppExtensionContext);
    p.addResult(command.m_Result);
    p.print(m_Flush);
  }
}

void TraceLayer::Post(INTC_DestroyDeviceExtensionContextCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "INTC_DestroyDeviceExtensionContext");
    p.addArgument(command.m_ppExtensionContext);
    p.addResult(command.m_Result);
    p.print(m_Flush);
  }
}

void TraceLayer::Pre(INTC_D3D12_CheckFeatureSupportCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "INTC_D3D12_CheckFeatureSupport");
    p.addArgument(command.m_pExtensionContext);
    p.addArgument(command.m_Feature);
    p.addArgument(command.m_pFeatureSupportData);
    p.addArgument(command.m_FeatureSupportDataSize);
    p.addResult(command.m_Result);
    p.print(m_Flush);
  }
}

void TraceLayer::Post(INTC_D3D12_CheckFeatureSupportCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "INTC_D3D12_CheckFeatureSupport");
    p.addArgument(command.m_pExtensionContext);
    p.addArgument(command.m_Feature);
    p.addArgument(command.m_pFeatureSupportData);
    p.addArgument(command.m_FeatureSupportDataSize);
    p.addResult(command.m_Result);
    p.print(m_Flush);
  }
}

void TraceLayer::Pre(INTC_D3D12_SetFeatureSupportCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "INTC_D3D12_SetFeatureSupport");
    p.addArgument(command.m_pExtensionContext);
    p.addArgument(command.m_pFeature);
    p.addResult(command.m_Result);
    p.print(m_Flush);
  }
}

void TraceLayer::Post(INTC_D3D12_SetFeatureSupportCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "INTC_D3D12_SetFeatureSupport");
    p.addArgument(command.m_pExtensionContext);
    p.addArgument(command.m_pFeature);
    p.addResult(command.m_Result);
    p.print(m_Flush);
  }
}

void TraceLayer::Pre(INTC_D3D12_GetResourceAllocationInfoCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "INTC_D3D12_GetResourceAllocationInfo");
    p.addArgument(command.m_pExtensionContext);
    p.addArgument(command.m_visibleMask);
    p.addArgument(command.m_numResourceDescs);
    p.addArgument(command.m_pResourceDescs);
    p.addResult(command.m_Result);
    p.print(m_Flush);
  }
}

void TraceLayer::Post(INTC_D3D12_GetResourceAllocationInfoCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "INTC_D3D12_GetResourceAllocationInfo");
    p.addArgument(command.m_pExtensionContext);
    p.addArgument(command.m_visibleMask);
    p.addArgument(command.m_numResourceDescs);
    p.addArgument(command.m_pResourceDescs);
    p.addResult(command.m_Result);
    p.print(m_Flush);
  }
}

void TraceLayer::Pre(INTC_D3D12_CreatePlacedResourceCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "INTC_D3D12_CreatePlacedResource");
    p.addArgument(command.m_pExtensionContext);
    p.addArgument(command.m_pHeap);
    p.addArgument(command.m_HeapOffset);
    p.addArgument(command.m_pDesc);
    p.addArgument(command.m_InitialState);
    p.addArgument(command.m_pOptimizedClearValue);
    p.addArgument(command.m_riid);
    p.addArgument(command.m_ppvResource);
    p.addResult(command.m_Result);
    p.print(m_Flush);
  }
}

void TraceLayer::Post(INTC_D3D12_CreatePlacedResourceCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "INTC_D3D12_CreatePlacedResource");
    p.addArgument(command.m_pExtensionContext);
    p.addArgument(command.m_pHeap);
    p.addArgument(command.m_HeapOffset);
    p.addArgument(command.m_pDesc);
    p.addArgument(command.m_InitialState);
    p.addArgument(command.m_pOptimizedClearValue);
    p.addArgument(command.m_riid);
    p.addArgument(command.m_ppvResource);
    p.addResult(command.m_Result);
    p.print(m_Flush);
  }
}

void TraceLayer::Pre(INTC_D3D12_CreateCommittedResourceCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "INTC_D3D12_CreateCommittedResource");
    p.addArgument(command.m_pExtensionContext);
    p.addArgument(command.m_pHeapProperties);
    p.addArgument(command.m_HeapFlags);
    p.addArgument(command.m_pDesc);
    p.addArgument(command.m_InitialResourceState);
    p.addArgument(command.m_pOptimizedClearValue);
    p.addArgument(command.m_riidResource);
    p.addArgument(command.m_ppvResource);
    p.addResult(command.m_Result);
    p.print(m_Flush);
  }
}

void TraceLayer::Post(INTC_D3D12_CreateCommittedResourceCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "INTC_D3D12_CreateCommittedResource");
    p.addArgument(command.m_pExtensionContext);
    p.addArgument(command.m_pHeapProperties);
    p.addArgument(command.m_HeapFlags);
    p.addArgument(command.m_pDesc);
    p.addArgument(command.m_InitialResourceState);
    p.addArgument(command.m_pOptimizedClearValue);
    p.addArgument(command.m_riidResource);
    p.addArgument(command.m_ppvResource);
    p.addResult(command.m_Result);
    p.print(m_Flush);
  }
}

void TraceLayer::Pre(INTC_D3D12_CreateComputePipelineStateCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "INTC_D3D12_CreateComputePipelineState");
    p.addArgument(command.m_pExtensionContext);
    p.addArgument(command.m_pDesc);
    p.addArgument(command.m_riid);
    p.addArgument(command.m_ppPipelineState);
    p.addResult(command.m_Result);
    p.print(m_Flush);
  }
}

void TraceLayer::Post(INTC_D3D12_CreateComputePipelineStateCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "INTC_D3D12_CreateComputePipelineState");
    p.addArgument(command.m_pExtensionContext);
    p.addArgument(command.m_pDesc);
    p.addArgument(command.m_riid);
    p.addArgument(command.m_ppPipelineState);
    p.addResult(command.m_Result);
    p.print(m_Flush);
  }
}

void TraceLayer::Pre(INTC_D3D12_CreateCommandQueueCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "INTC_D3D12_CreateCommandQueue");
    p.addArgument(command.m_pExtensionContext);
    p.addArgument(command.m_pDesc);
    p.addArgument(command.m_riid);
    p.addArgument(command.m_ppCommandQueue);
    p.addResult(command.m_Result);
    p.print(m_Flush);
  }
}

void TraceLayer::Post(INTC_D3D12_CreateCommandQueueCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "INTC_D3D12_CreateCommandQueue");
    p.addArgument(command.m_pExtensionContext);
    p.addArgument(command.m_pDesc);
    p.addArgument(command.m_riid);
    p.addArgument(command.m_ppCommandQueue);
    p.addResult(command.m_Result);
    p.print(m_Flush);
  }
}
void TraceLayer::Pre(INTC_D3D12_CreateReservedResourceCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "INTC_D3D12_CreateReservedResource");
    p.addArgument(command.m_pExtensionContext);
    p.addArgument(command.m_pDesc);
    p.addArgument(command.m_InitialState);
    p.addArgument(command.m_pOptimizedClearValue);
    p.addArgument(command.m_riid);
    p.addArgument(command.m_ppvResource);
    p.addResult(command.m_Result);
    p.print(m_Flush);
  }
}

void TraceLayer::Post(INTC_D3D12_CreateReservedResourceCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "INTC_D3D12_CreateReservedResource");
    p.addArgument(command.m_pExtensionContext);
    p.addArgument(command.m_pDesc);
    p.addArgument(command.m_InitialState);
    p.addArgument(command.m_pOptimizedClearValue);
    p.addArgument(command.m_riid);
    p.addArgument(command.m_ppvResource);
    p.addResult(command.m_Result);
    p.print(m_Flush);
  }
}

void TraceLayer::Pre(INTC_D3D12_CreateHeapCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "INTC_D3D12_CreateHeap");
    p.addArgument(command.m_pExtensionContext);
    p.addArgument(command.m_pDesc);
    p.addArgument(command.m_riid);
    p.addArgument(command.m_ppvHeap);
    p.addResult(command.m_Result);
    p.print(m_Flush);
  }
}

void TraceLayer::Post(INTC_D3D12_CreateHeapCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "INTC_D3D12_CreateHeap");
    p.addArgument(command.m_pExtensionContext);
    p.addArgument(command.m_pDesc);
    p.addArgument(command.m_riid);
    p.addArgument(command.m_ppvHeap);
    p.addResult(command.m_Result);
    p.print(m_Flush);
  }
}

void TraceLayer::Pre(NvAPI_InitializeCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "NvAPI_Initialize");
    p.addResult(command.m_Result);
    p.print(m_Flush);
  }
}

void TraceLayer::Post(NvAPI_InitializeCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "NvAPI_Initialize");
    p.addResult(command.m_Result);
    p.print(m_Flush);
  }
}

void TraceLayer::Pre(NvAPI_UnloadCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "NvAPI_Unload");
    p.addResult(command.m_Result);
    p.print(m_Flush);
  }
}

void TraceLayer::Post(NvAPI_UnloadCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "NvAPI_Unload");
    p.addResult(command.m_Result);
    p.print(m_Flush);
  }
}

void TraceLayer::Pre(NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "NvAPI_D3D12_SetCreatePipelineStateOptions");
    p.addArgument(command.m_pDevice);
    p.addArgument(command.m_pState);
    p.addResult(command.m_Result);
    p.print(m_Flush);
  }
}

void TraceLayer::Post(NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command,
                     "NvAPI_D3D12_SetCreatePipelineStateOptions");
    p.addArgument(command.m_pDevice);
    p.addArgument(command.m_pState);
    p.addResult(command.m_Result);
    p.print(m_Flush);
  }
}

void TraceLayer::Pre(NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "NvAPI_D3D12_SetNvShaderExtnSlotSpace");
    p.addArgument(command.m_pDev);
    p.addArgument(command.m_uavSlot);
    p.addArgument(command.m_uavSpace);
    p.addResult(command.m_Result);
    p.print(m_Flush);
  }
}

void TraceLayer::Post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "NvAPI_D3D12_SetNvShaderExtnSlotSpace");
    p.addArgument(command.m_pDev);
    p.addArgument(command.m_uavSlot);
    p.addArgument(command.m_uavSpace);
    p.addResult(command.m_Result);
    p.print(m_Flush);
  }
}

void TraceLayer::Pre(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command,
                     "NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThread");
    p.addArgument(command.m_pDev);
    p.addArgument(command.m_uavSlot);
    p.addArgument(command.m_uavSpace);
    p.addResult(command.m_Result);
    p.print(m_Flush);
  }
}

void TraceLayer::Post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command,
                     "NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThread");
    p.addArgument(command.m_pDev);
    p.addArgument(command.m_uavSlot);
    p.addArgument(command.m_uavSpace);
    p.addResult(command.m_Result);
    p.print(m_Flush);
  }
}

void TraceLayer::Pre(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command,
                     "NvAPI_D3D12_BuildRaytracingAccelerationStructureEx");
    p.addArgument(command.m_pCommandList);
    p.addArgument(command.m_pParams);
    p.addResult(command.m_Result);
    p.print(m_Flush);
  }
}

void TraceLayer::Post(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command,
                     "NvAPI_D3D12_BuildRaytracingAccelerationStructureEx");
    p.addArgument(command.m_pCommandList);
    p.addArgument(command.m_pParams);
    p.addResult(command.m_Result);
    p.print(m_Flush);
  }
}

void TraceLayer::Pre(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command,
                     "NvAPI_D3D12_BuildRaytracingOpacityMicromapArray");
    p.addArgument(command.m_pCommandList);
    p.addArgument(command.m_pParams);
    p.addResult(command.m_Result);
    p.print(m_Flush);
  }
}

void TraceLayer::Post(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command,
                     "NvAPI_D3D12_BuildRaytracingOpacityMicromapArray");
    p.addArgument(command.m_pCommandList);
    p.addArgument(command.m_pParams);
    p.addResult(command.m_Result);
    p.print(m_Flush);
  }
}

void TraceLayer::Pre(NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command,
                     "NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperation");
    p.addArgument(command.m_pCommandList);
    p.addArgument(command.m_pParams);
    p.addResult(command.m_Result);
    p.print(m_Flush);
  }
}

void TraceLayer::Post(NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command,
                     "NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperation");
    p.addArgument(command.m_pCommandList);
    p.addArgument(command.m_pParams);
    p.addResult(command.m_Result);
    p.print(m_Flush);
  }
}

#pragma endregion

} // namespace DirectX
} // namespace gits
