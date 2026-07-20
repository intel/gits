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
    p.AddArgument(command.m_hWnd);
    p.AddArgument(command.m_width);
    p.AddArgument(command.m_height);
    p.Print(m_Flush);
  }
}

void TraceLayer::Post(CreateWindowMetaCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "CreateWindowMetaCommand");
    p.AddArgument(command.m_hWnd);
    p.AddArgument(command.m_width);
    p.AddArgument(command.m_height);
    p.Print(m_Flush);
  }
}

void TraceLayer::Pre(MappedDataMetaCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "MappedDataMetaCommand");
    p.AddArgument(command.m_resource);
    p.AddArgument(command.m_mappedAddress);
    p.AddArgument(command.m_offset);
    p.AddArgument(command.m_data);
    p.Print(m_Flush);
  }
}

void TraceLayer::Post(MappedDataMetaCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "MappedDataMetaCommand");
    p.AddArgument(command.m_resource);
    p.AddArgument(command.m_mappedAddress);
    p.AddArgument(command.m_offset);
    p.AddArgument(command.m_data);
    p.Print(m_Flush);
  }
}

void TraceLayer::Pre(CreateHeapAllocationMetaCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "CreateHeapAllocationMetaCommand");
    p.AddArgument(command.m_heap);
    p.AddArgument(command.m_address);
    p.AddArgument(command.m_data);
    p.Print(m_Flush);
  }
}

void TraceLayer::Post(CreateHeapAllocationMetaCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "CreateHeapAllocationMetaCommand");
    p.AddArgument(command.m_heap);
    p.AddArgument(command.m_address);
    p.AddArgument(command.m_data);
    p.Print(m_Flush);
  }
}

#pragma endregion

void TraceLayer::Pre(WaitForFenceSignaledDeprecatedCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "WaitForFenceSignaledCommand");
    p.AddArgument(command.m_event);
    p.AddArgument(command.m_fence);
    p.AddArgument(command.m_Value);
    p.Print(m_Flush);
  }
}

void TraceLayer::Post(WaitForFenceSignaledDeprecatedCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "WaitForFenceSignaledCommand");
    p.AddArgument(command.m_event);
    p.AddArgument(command.m_fence);
    p.AddArgument(command.m_Value);
    p.Print(m_Flush);
  }
}

void TraceLayer::Pre(WaitForFenceSignaledCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "WaitForFenceSignaledCommand");
    p.AddArgument(command.m_event);
    p.AddArgument(command.m_fence);
    p.AddArgument(command.m_Value);
    p.Print(m_Flush);
  }
}

void TraceLayer::Post(WaitForFenceSignaledCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "WaitForFenceSignaledCommand");
    p.AddArgument(command.m_event);
    p.AddArgument(command.m_fence);
    p.AddArgument(command.m_Value);
    p.Print(m_Flush);
  }
}

void TraceLayer::Pre(DllContainerMetaCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "DllContainerMetaCommand");
    p.AddArgument(command.m_dllName);
    p.AddArgument(command.m_dllData);
    p.Print(m_Flush);
  }
}

void TraceLayer::Post(DllContainerMetaCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "DllContainerMetaCommand");
    p.AddArgument(command.m_dllName);
    p.AddArgument(command.m_dllData);
    p.Print(m_Flush);
  }
}

void TraceLayer::Pre(IUnknownQueryInterfaceCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "IUnknown::QueryInterface",
                     command.m_Object.Key);
    p.AddArgument(command.m_riid);
    p.AddArgument(command.m_ppvObject);
    p.AddResult(command.m_Result);
    p.Print(m_Flush);
  }
}

void TraceLayer::Post(IUnknownQueryInterfaceCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "IUnknown::QueryInterface",
                     command.m_Object.Key);
    p.AddArgument(command.m_riid);
    p.AddArgument(command.m_ppvObject);
    p.AddResult(command.m_Result);
    p.Print(m_Flush);
  }
}

void TraceLayer::Pre(IUnknownAddRefCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "IUnknown::AddRef", command.m_Object.Key);
    p.AddResult(command.m_Result);
    p.Print(m_Flush);
  }
}

void TraceLayer::Post(IUnknownAddRefCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "IUnknown::AddRef", command.m_Object.Key);
    p.AddResult(command.m_Result);
    p.Print(m_Flush);
  }
}

void TraceLayer::Pre(IUnknownReleaseCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "IUnknown::Release", command.m_Object.Key);
    p.AddResult(command.m_Result);
    p.Print(m_Flush);
  }
}

void TraceLayer::Post(IUnknownReleaseCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "IUnknown::Release", command.m_Object.Key);
    p.AddResult(command.m_Result);
    p.Print(m_Flush);
  }
}

void TraceLayer::Pre(ID3D12ResourceGetGPUVirtualAddressCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "ID3D12Resource::GetGPUVirtualAddress",
                     command.m_Object.Key);
    p.AddResult(reinterpret_cast<Argument<void*>&>(command.m_Result));
    p.Print(m_Flush);
  }
}

void TraceLayer::Post(ID3D12ResourceGetGPUVirtualAddressCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "ID3D12Resource::GetGPUVirtualAddress",
                     command.m_Object.Key);
    p.AddResult(reinterpret_cast<Argument<void*>&>(command.m_Result));
    p.Print(m_Flush);
  }
}

void TraceLayer::Pre(ID3D12DescriptorHeapGetCPUDescriptorHandleForHeapStartCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command,
                     "ID3D12DescriptorHeap::GetCPUDescriptorHandleForHeapStart",
                     command.m_Object.Key);
    p.AddResult(reinterpret_cast<Argument<void*>&>(command.m_Result));
    p.Print(m_Flush);
  }
}

void TraceLayer::Post(ID3D12DescriptorHeapGetCPUDescriptorHandleForHeapStartCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command,
                     "ID3D12DescriptorHeap::GetCPUDescriptorHandleForHeapStart",
                     command.m_Object.Key);
    p.AddResult(reinterpret_cast<Argument<void*>&>(command.m_Result));
    p.Print(m_Flush);
  }
}

void TraceLayer::Pre(ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command,
                     "ID3D12DescriptorHeap::GetGPUDescriptorHandleForHeapStart",
                     command.m_Object.Key);
    p.AddResult(reinterpret_cast<Argument<void*>&>(command.m_Result));
    p.Print(m_Flush);
  }
}

void TraceLayer::Post(ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command,
                     "ID3D12DescriptorHeap::GetGPUDescriptorHandleForHeapStart",
                     command.m_Object.Key);
    p.AddResult(reinterpret_cast<Argument<void*>&>(command.m_Result));
    p.Print(m_Flush);
  }
}

void TraceLayer::Pre(ID3D12GraphicsCommandListBeginEventCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "ID3D12GraphicsCommandList::BeginEvent",
                     command.m_Object.Key);
    p.AddArgument(command.m_Metadata);
    LPCWSTR_Argument arg(static_cast<wchar_t*>(command.m_pData.Value));
    p.AddArgument(arg);
    p.Print(m_Flush);
  }
}

void TraceLayer::Post(ID3D12GraphicsCommandListBeginEventCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "ID3D12GraphicsCommandList::BeginEvent",
                     command.m_Object.Key);
    p.AddArgument(command.m_Metadata);
    LPCWSTR_Argument arg(static_cast<wchar_t*>(command.m_pData.Value));
    p.AddArgument(arg);
    p.Print(m_Flush);
  }
}

#pragma region INTCExtension

void TraceLayer::Pre(INTC_D3D12_GetSupportedVersionsCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "INTC_D3D12_GetSupportedVersions");
    p.AddArgument(command.m_pDevice);
    p.AddArgument(command.m_pSupportedExtVersions);
    p.AddArgument(command.m_pSupportedExtVersionsCount);
    p.AddResult(command.m_Result);
    p.Print(m_Flush);
  }
}

void TraceLayer::Post(INTC_D3D12_GetSupportedVersionsCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "INTC_D3D12_GetSupportedVersions");
    p.AddArgument(command.m_pDevice);
    p.AddArgument(command.m_pSupportedExtVersions);
    p.AddArgument(command.m_pSupportedExtVersionsCount);
    p.AddResult(command.m_Result);
    p.Print(m_Flush);
  }
}

void TraceLayer::Pre(INTC_D3D12_CreateDeviceExtensionContextCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "INTC_D3D12_CreateDeviceExtensionContext");
    p.AddArgument(command.m_pDevice);
    p.AddArgument(command.m_ppExtensionContext);
    p.AddArgument(command.m_pExtensionInfo);
    p.AddArgument(command.m_pExtensionAppInfo);
    p.AddResult(command.m_Result);
    p.Print(m_Flush);
  }
}

void TraceLayer::Post(INTC_D3D12_CreateDeviceExtensionContextCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "INTC_D3D12_CreateDeviceExtensionContext");
    p.AddArgument(command.m_pDevice);
    p.AddArgument(command.m_ppExtensionContext);
    p.AddArgument(command.m_pExtensionInfo);
    p.AddArgument(command.m_pExtensionAppInfo);
    p.AddResult(command.m_Result);
    p.Print(m_Flush);
  }
}

void TraceLayer::Pre(INTC_D3D12_CreateDeviceExtensionContext1Command& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "INTC_D3D12_CreateDeviceExtensionContext1");
    p.AddArgument(command.m_pDevice);
    p.AddArgument(command.m_ppExtensionContext);
    p.AddArgument(command.m_pExtensionInfo);
    p.AddArgument(command.m_pExtensionAppInfo);
    p.AddResult(command.m_Result);
    p.Print(m_Flush);
  }
}

void TraceLayer::Post(INTC_D3D12_CreateDeviceExtensionContext1Command& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command,
                     "INTC_D3D12_CreateDeviceExtensionContext1");
    p.AddArgument(command.m_pDevice);
    p.AddArgument(command.m_ppExtensionContext);
    p.AddArgument(command.m_pExtensionInfo);
    p.AddArgument(command.m_pExtensionAppInfo);
    p.AddResult(command.m_Result);
    p.Print(m_Flush);
  }
}

void TraceLayer::Pre(INTC_D3D12_SetApplicationInfoCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "INTC_D3D12_SetApplicationInfo");
    p.AddArgument(command.m_pExtensionAppInfo);
    p.AddResult(command.m_Result);
    p.Print(m_Flush);
  }
}

void TraceLayer::Post(INTC_D3D12_SetApplicationInfoCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "INTC_D3D12_SetApplicationInfo");
    p.AddArgument(command.m_pExtensionAppInfo);
    p.AddResult(command.m_Result);
    p.Print(m_Flush);
  }
}

void TraceLayer::Pre(INTC_DestroyDeviceExtensionContextCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "INTC_DestroyDeviceExtensionContext");
    p.AddArgument(command.m_ppExtensionContext);
    p.AddResult(command.m_Result);
    p.Print(m_Flush);
  }
}

void TraceLayer::Post(INTC_DestroyDeviceExtensionContextCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "INTC_DestroyDeviceExtensionContext");
    p.AddArgument(command.m_ppExtensionContext);
    p.AddResult(command.m_Result);
    p.Print(m_Flush);
  }
}

void TraceLayer::Pre(INTC_D3D12_CheckFeatureSupportCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "INTC_D3D12_CheckFeatureSupport");
    p.AddArgument(command.m_pExtensionContext);
    p.AddArgument(command.m_Feature);
    p.AddArgument(command.m_pFeatureSupportData);
    p.AddArgument(command.m_FeatureSupportDataSize);
    p.AddResult(command.m_Result);
    p.Print(m_Flush);
  }
}

void TraceLayer::Post(INTC_D3D12_CheckFeatureSupportCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "INTC_D3D12_CheckFeatureSupport");
    p.AddArgument(command.m_pExtensionContext);
    p.AddArgument(command.m_Feature);
    p.AddArgument(command.m_pFeatureSupportData);
    p.AddArgument(command.m_FeatureSupportDataSize);
    p.AddResult(command.m_Result);
    p.Print(m_Flush);
  }
}

void TraceLayer::Pre(INTC_D3D12_SetFeatureSupportCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "INTC_D3D12_SetFeatureSupport");
    p.AddArgument(command.m_pExtensionContext);
    p.AddArgument(command.m_pFeature);
    p.AddResult(command.m_Result);
    p.Print(m_Flush);
  }
}

void TraceLayer::Post(INTC_D3D12_SetFeatureSupportCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "INTC_D3D12_SetFeatureSupport");
    p.AddArgument(command.m_pExtensionContext);
    p.AddArgument(command.m_pFeature);
    p.AddResult(command.m_Result);
    p.Print(m_Flush);
  }
}

void TraceLayer::Pre(INTC_D3D12_GetResourceAllocationInfoCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "INTC_D3D12_GetResourceAllocationInfo");
    p.AddArgument(command.m_pExtensionContext);
    p.AddArgument(command.m_visibleMask);
    p.AddArgument(command.m_numResourceDescs);
    p.AddArgument(command.m_pResourceDescs);
    p.AddResult(command.m_Result);
    p.Print(m_Flush);
  }
}

void TraceLayer::Post(INTC_D3D12_GetResourceAllocationInfoCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "INTC_D3D12_GetResourceAllocationInfo");
    p.AddArgument(command.m_pExtensionContext);
    p.AddArgument(command.m_visibleMask);
    p.AddArgument(command.m_numResourceDescs);
    p.AddArgument(command.m_pResourceDescs);
    p.AddResult(command.m_Result);
    p.Print(m_Flush);
  }
}

void TraceLayer::Pre(INTC_D3D12_CreatePlacedResourceCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "INTC_D3D12_CreatePlacedResource");
    p.AddArgument(command.m_pExtensionContext);
    p.AddArgument(command.m_pHeap);
    p.AddArgument(command.m_HeapOffset);
    p.AddArgument(command.m_pDesc);
    p.AddArgument(command.m_InitialState);
    p.AddArgument(command.m_pOptimizedClearValue);
    p.AddArgument(command.m_riid);
    p.AddArgument(command.m_ppvResource);
    p.AddResult(command.m_Result);
    p.Print(m_Flush);
  }
}

void TraceLayer::Post(INTC_D3D12_CreatePlacedResourceCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "INTC_D3D12_CreatePlacedResource");
    p.AddArgument(command.m_pExtensionContext);
    p.AddArgument(command.m_pHeap);
    p.AddArgument(command.m_HeapOffset);
    p.AddArgument(command.m_pDesc);
    p.AddArgument(command.m_InitialState);
    p.AddArgument(command.m_pOptimizedClearValue);
    p.AddArgument(command.m_riid);
    p.AddArgument(command.m_ppvResource);
    p.AddResult(command.m_Result);
    p.Print(m_Flush);
  }
}

void TraceLayer::Pre(INTC_D3D12_CreateCommittedResourceCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "INTC_D3D12_CreateCommittedResource");
    p.AddArgument(command.m_pExtensionContext);
    p.AddArgument(command.m_pHeapProperties);
    p.AddArgument(command.m_HeapFlags);
    p.AddArgument(command.m_pDesc);
    p.AddArgument(command.m_InitialResourceState);
    p.AddArgument(command.m_pOptimizedClearValue);
    p.AddArgument(command.m_riidResource);
    p.AddArgument(command.m_ppvResource);
    p.AddResult(command.m_Result);
    p.Print(m_Flush);
  }
}

void TraceLayer::Post(INTC_D3D12_CreateCommittedResourceCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "INTC_D3D12_CreateCommittedResource");
    p.AddArgument(command.m_pExtensionContext);
    p.AddArgument(command.m_pHeapProperties);
    p.AddArgument(command.m_HeapFlags);
    p.AddArgument(command.m_pDesc);
    p.AddArgument(command.m_InitialResourceState);
    p.AddArgument(command.m_pOptimizedClearValue);
    p.AddArgument(command.m_riidResource);
    p.AddArgument(command.m_ppvResource);
    p.AddResult(command.m_Result);
    p.Print(m_Flush);
  }
}

void TraceLayer::Pre(INTC_D3D12_CreateComputePipelineStateCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "INTC_D3D12_CreateComputePipelineState");
    p.AddArgument(command.m_pExtensionContext);
    p.AddArgument(command.m_pDesc);
    p.AddArgument(command.m_riid);
    p.AddArgument(command.m_ppPipelineState);
    p.AddResult(command.m_Result);
    p.Print(m_Flush);
  }
}

void TraceLayer::Post(INTC_D3D12_CreateComputePipelineStateCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "INTC_D3D12_CreateComputePipelineState");
    p.AddArgument(command.m_pExtensionContext);
    p.AddArgument(command.m_pDesc);
    p.AddArgument(command.m_riid);
    p.AddArgument(command.m_ppPipelineState);
    p.AddResult(command.m_Result);
    p.Print(m_Flush);
  }
}

void TraceLayer::Pre(INTC_D3D12_CreateCommandQueueCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "INTC_D3D12_CreateCommandQueue");
    p.AddArgument(command.m_pExtensionContext);
    p.AddArgument(command.m_pDesc);
    p.AddArgument(command.m_riid);
    p.AddArgument(command.m_ppCommandQueue);
    p.AddResult(command.m_Result);
    p.Print(m_Flush);
  }
}

void TraceLayer::Post(INTC_D3D12_CreateCommandQueueCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "INTC_D3D12_CreateCommandQueue");
    p.AddArgument(command.m_pExtensionContext);
    p.AddArgument(command.m_pDesc);
    p.AddArgument(command.m_riid);
    p.AddArgument(command.m_ppCommandQueue);
    p.AddResult(command.m_Result);
    p.Print(m_Flush);
  }
}
void TraceLayer::Pre(INTC_D3D12_CreateReservedResourceCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "INTC_D3D12_CreateReservedResource");
    p.AddArgument(command.m_pExtensionContext);
    p.AddArgument(command.m_pDesc);
    p.AddArgument(command.m_InitialState);
    p.AddArgument(command.m_pOptimizedClearValue);
    p.AddArgument(command.m_riid);
    p.AddArgument(command.m_ppvResource);
    p.AddResult(command.m_Result);
    p.Print(m_Flush);
  }
}

void TraceLayer::Post(INTC_D3D12_CreateReservedResourceCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "INTC_D3D12_CreateReservedResource");
    p.AddArgument(command.m_pExtensionContext);
    p.AddArgument(command.m_pDesc);
    p.AddArgument(command.m_InitialState);
    p.AddArgument(command.m_pOptimizedClearValue);
    p.AddArgument(command.m_riid);
    p.AddArgument(command.m_ppvResource);
    p.AddResult(command.m_Result);
    p.Print(m_Flush);
  }
}

void TraceLayer::Pre(INTC_D3D12_CreateHeapCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "INTC_D3D12_CreateHeap");
    p.AddArgument(command.m_pExtensionContext);
    p.AddArgument(command.m_pDesc);
    p.AddArgument(command.m_riid);
    p.AddArgument(command.m_ppvHeap);
    p.AddResult(command.m_Result);
    p.Print(m_Flush);
  }
}

void TraceLayer::Post(INTC_D3D12_CreateHeapCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "INTC_D3D12_CreateHeap");
    p.AddArgument(command.m_pExtensionContext);
    p.AddArgument(command.m_pDesc);
    p.AddArgument(command.m_riid);
    p.AddArgument(command.m_ppvHeap);
    p.AddResult(command.m_Result);
    p.Print(m_Flush);
  }
}

void TraceLayer::Pre(NvAPI_InitializeCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "NvAPI_Initialize");
    p.AddResult(command.m_Result);
    p.Print(m_Flush);
  }
}

void TraceLayer::Post(NvAPI_InitializeCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "NvAPI_Initialize");
    p.AddResult(command.m_Result);
    p.Print(m_Flush);
  }
}

void TraceLayer::Pre(NvAPI_UnloadCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "NvAPI_Unload");
    p.AddResult(command.m_Result);
    p.Print(m_Flush);
  }
}

void TraceLayer::Post(NvAPI_UnloadCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "NvAPI_Unload");
    p.AddResult(command.m_Result);
    p.Print(m_Flush);
  }
}

void TraceLayer::Pre(NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "NvAPI_D3D12_SetCreatePipelineStateOptions");
    p.AddArgument(command.m_pDevice);
    p.AddArgument(command.m_pState);
    p.AddResult(command.m_Result);
    p.Print(m_Flush);
  }
}

void TraceLayer::Post(NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command,
                     "NvAPI_D3D12_SetCreatePipelineStateOptions");
    p.AddArgument(command.m_pDevice);
    p.AddArgument(command.m_pState);
    p.AddResult(command.m_Result);
    p.Print(m_Flush);
  }
}

void TraceLayer::Pre(NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command, "NvAPI_D3D12_SetNvShaderExtnSlotSpace");
    p.AddArgument(command.m_pDev);
    p.AddArgument(command.m_uavSlot);
    p.AddArgument(command.m_uavSpace);
    p.AddResult(command.m_Result);
    p.Print(m_Flush);
  }
}

void TraceLayer::Post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command, "NvAPI_D3D12_SetNvShaderExtnSlotSpace");
    p.AddArgument(command.m_pDev);
    p.AddArgument(command.m_uavSlot);
    p.AddArgument(command.m_uavSpace);
    p.AddResult(command.m_Result);
    p.Print(m_Flush);
  }
}

void TraceLayer::Pre(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command,
                     "NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThread");
    p.AddArgument(command.m_pDev);
    p.AddArgument(command.m_uavSlot);
    p.AddArgument(command.m_uavSpace);
    p.AddResult(command.m_Result);
    p.Print(m_Flush);
  }
}

void TraceLayer::Post(NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command,
                     "NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThread");
    p.AddArgument(command.m_pDev);
    p.AddArgument(command.m_uavSlot);
    p.AddArgument(command.m_uavSpace);
    p.AddResult(command.m_Result);
    p.Print(m_Flush);
  }
}

void TraceLayer::Pre(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command,
                     "NvAPI_D3D12_BuildRaytracingAccelerationStructureEx");
    p.AddArgument(command.m_pCommandList);
    p.AddArgument(command.m_pParams);
    p.AddResult(command.m_Result);
    p.Print(m_Flush);
  }
}

void TraceLayer::Post(NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command,
                     "NvAPI_D3D12_BuildRaytracingAccelerationStructureEx");
    p.AddArgument(command.m_pCommandList);
    p.AddArgument(command.m_pParams);
    p.AddResult(command.m_Result);
    p.Print(m_Flush);
  }
}

void TraceLayer::Pre(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command,
                     "NvAPI_D3D12_BuildRaytracingOpacityMicromapArray");
    p.AddArgument(command.m_pCommandList);
    p.AddArgument(command.m_pParams);
    p.AddResult(command.m_Result);
    p.Print(m_Flush);
  }
}

void TraceLayer::Post(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command,
                     "NvAPI_D3D12_BuildRaytracingOpacityMicromapArray");
    p.AddArgument(command.m_pCommandList);
    p.AddArgument(command.m_pParams);
    p.AddResult(command.m_Result);
    p.Print(m_Flush);
  }
}

void TraceLayer::Pre(NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& command) {
  if (m_PrintPre) {
    CommandPrinter p(m_StreamPre, m_StatePre, command,
                     "NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperation");
    p.AddArgument(command.m_pCommandList);
    p.AddArgument(command.m_pParams);
    p.AddResult(command.m_Result);
    p.Print(m_Flush);
  }
}

void TraceLayer::Post(NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& command) {
  if (m_PrintPost) {
    CommandPrinter p(m_StreamPost, m_StatePost, command,
                     "NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperation");
    p.AddArgument(command.m_pCommandList);
    p.AddArgument(command.m_pParams);
    p.AddResult(command.m_Result);
    p.Print(m_Flush);
  }
}

#pragma endregion

} // namespace DirectX
} // namespace gits
