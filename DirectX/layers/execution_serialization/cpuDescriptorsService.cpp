// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "cpuDescriptorsService.h"
#include "commandsAuto.h"
#include "commandSerializersAuto.h"
#include "to_string/enumToStrAuto.h"
#include "log.h"

namespace gits {
namespace DirectX {

void CpuDescriptorsService::createCommandList(unsigned DeviceKey) {
  if (m_DeviceKey && m_DeviceKey != DeviceKey) {
    LOG_ERROR << "Execution serialization - multiple devices not handled";
  }
  m_DeviceKey = DeviceKey;
}

void CpuDescriptorsService::executeCommandLists(std::vector<unsigned>& commandListKeys) {
  for (unsigned key : commandListKeys) {
    auto it = m_DescriptorsByCommandList.find(key);
    if (it != m_DescriptorsByCommandList.end()) {
      for (DescriptorHandle& descriptor : it->second) {
        switch (descriptor.Type) {
        case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
          m_UavDescriptorHeap.clearDescriptor(descriptor.Index);
          break;
        case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
          m_RtvDescriptorHeap.clearDescriptor(descriptor.Index);
          break;
        case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
          m_DsvDescriptorHeap.clearDescriptor(descriptor.Index);
          break;
        }
      }
      m_DescriptorsByCommandList.erase(it);
    }
  }
}

void CpuDescriptorsService::preserveDescriptor(
    ID3D12GraphicsCommandListOMSetRenderTargetsCommand& c) {

  unsigned heapKey{};
  unsigned heapIndex{};
  for (unsigned i = 0; i < c.m_NumRenderTargetDescriptors.Value; ++i) {
    if (i == 0 || !c.m_RTsSingleHandleToDescriptorRange.Value) {
      heapKey = c.m_pRenderTargetDescriptors.InterfaceKeys[i];
      heapIndex = c.m_pRenderTargetDescriptors.Indexes[i];
    } else {
      ++heapIndex;
    }
    unsigned preservedIndex = m_RtvDescriptorHeap.preserveDescriptor(heapKey, heapIndex);
    m_DescriptorsByCommandList[c.m_Object.Key].push_back(
        DescriptorHandle{D3D12_DESCRIPTOR_HEAP_TYPE_RTV, preservedIndex});
    if (i == 0 || !c.m_RTsSingleHandleToDescriptorRange.Value) {
      c.m_pRenderTargetDescriptors.InterfaceKeys[i] = m_RtvDescriptorHeap.m_DescriptorHeapKey;
      c.m_pRenderTargetDescriptors.Indexes[i] = preservedIndex;
    }
  }

  if (c.m_pDepthStencilDescriptor.Value) {
    unsigned preservedIndex = m_DsvDescriptorHeap.preserveDescriptor(
        c.m_pDepthStencilDescriptor.InterfaceKeys[0], c.m_pDepthStencilDescriptor.Indexes[0]);
    c.m_pDepthStencilDescriptor.InterfaceKeys[0] = m_DsvDescriptorHeap.m_DescriptorHeapKey;
    c.m_pDepthStencilDescriptor.Indexes[0] = preservedIndex;
    m_DescriptorsByCommandList[c.m_Object.Key].push_back(
        DescriptorHandle{D3D12_DESCRIPTOR_HEAP_TYPE_DSV, preservedIndex});
  }
}

void CpuDescriptorsService::preserveDescriptor(
    ID3D12GraphicsCommandListClearDepthStencilViewCommand& c) {
  unsigned preservedIndex = m_DsvDescriptorHeap.preserveDescriptor(
      c.m_DepthStencilView.InterfaceKey, c.m_DepthStencilView.Index);
  c.m_DepthStencilView.InterfaceKey = m_DsvDescriptorHeap.m_DescriptorHeapKey;
  c.m_DepthStencilView.Index = preservedIndex;
  m_DescriptorsByCommandList[c.m_Object.Key].push_back(
      DescriptorHandle{D3D12_DESCRIPTOR_HEAP_TYPE_DSV, preservedIndex});
}

void CpuDescriptorsService::preserveDescriptor(
    ID3D12GraphicsCommandListClearRenderTargetViewCommand& c) {
  unsigned preservedIndex = m_RtvDescriptorHeap.preserveDescriptor(
      c.m_RenderTargetView.InterfaceKey, c.m_RenderTargetView.Index);
  c.m_RenderTargetView.InterfaceKey = m_RtvDescriptorHeap.m_DescriptorHeapKey;
  c.m_RenderTargetView.Index = preservedIndex;
  m_DescriptorsByCommandList[c.m_Object.Key].push_back(
      DescriptorHandle{D3D12_DESCRIPTOR_HEAP_TYPE_RTV, preservedIndex});
}

void CpuDescriptorsService::preserveDescriptor(
    ID3D12GraphicsCommandListClearUnorderedAccessViewUintCommand& c) {
  unsigned preservedIndex = m_UavDescriptorHeap.preserveDescriptor(c.m_ViewCPUHandle.InterfaceKey,
                                                                   c.m_ViewCPUHandle.Index);
  c.m_ViewCPUHandle.InterfaceKey = m_UavDescriptorHeap.m_DescriptorHeapKey;
  c.m_ViewCPUHandle.Index = preservedIndex;
  m_DescriptorsByCommandList[c.m_Object.Key].push_back(
      DescriptorHandle{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, preservedIndex});
}

void CpuDescriptorsService::preserveDescriptor(
    ID3D12GraphicsCommandListClearUnorderedAccessViewFloatCommand& c) {
  unsigned preservedIndex = m_UavDescriptorHeap.preserveDescriptor(c.m_ViewCPUHandle.InterfaceKey,
                                                                   c.m_ViewCPUHandle.Index);
  c.m_ViewCPUHandle.InterfaceKey = m_UavDescriptorHeap.m_DescriptorHeapKey;
  c.m_ViewCPUHandle.Index = preservedIndex;
  m_DescriptorsByCommandList[c.m_Object.Key].push_back(
      DescriptorHandle{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, preservedIndex});
}

template <unsigned SIZE>
unsigned CpuDescriptorsService::DescriptorHeap<SIZE>::preserveDescriptor(unsigned heapKey,
                                                                         unsigned heapIndex) {
  if (!m_DescriptorHeapKey) {
    createDescriptorHeap();
  }

  unsigned preservedIndex = 0;
  for (; preservedIndex < m_DescriptorUsage.size(); ++preservedIndex) {
    if (!m_DescriptorUsage.test(preservedIndex)) {
      break;
    }
  }
  if (preservedIndex == m_DescriptorUsage.size()) {
    LOG_ERROR << "Execution serialization - auxiliary descriptor heap " << toStr(m_Type)
              << " too small!";
  }

  m_DescriptorUsage.set(preservedIndex);

  ID3D12DeviceCopyDescriptorsSimpleCommand copy;
  copy.Key = m_Service.m_CommandListExecutionService.getUniqueCommandKey();
  copy.m_Object.Key = m_Service.m_DeviceKey;
  copy.m_NumDescriptors.Value = 1;
  copy.m_DestDescriptorRangeStart.InterfaceKey = m_DescriptorHeapKey;
  copy.m_DestDescriptorRangeStart.Index = preservedIndex;
  copy.m_SrcDescriptorRangeStart.InterfaceKey = heapKey;
  copy.m_SrcDescriptorRangeStart.Index = heapIndex;
  copy.m_DescriptorHeapsType.Value = m_Type;
  m_Service.m_Recorder.Record(ID3D12DeviceCopyDescriptorsSimpleSerializer(copy));

  return preservedIndex;
}

template <unsigned SIZE>
void CpuDescriptorsService::DescriptorHeap<SIZE>::createDescriptorHeap() {
  m_DescriptorHeapKey = m_Service.m_CommandListExecutionService.getUniqueObjectKey();

  D3D12_DESCRIPTOR_HEAP_DESC desc{};
  desc.Type = m_Type;
  desc.NumDescriptors = m_DescriptorUsage.size();
  desc.NodeMask = 0;
  desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

  ID3D12DeviceCreateDescriptorHeapCommand create;
  create.Key = m_Service.m_CommandListExecutionService.getUniqueCommandKey();
  create.m_Object.Key = m_Service.m_DeviceKey;
  create.m_pDescriptorHeapDesc.Value = &desc;
  create.m_riid.Value = IID_ID3D12DescriptorHeap;
  create.m_ppvHeap.Key = m_DescriptorHeapKey;
  m_Service.m_Recorder.Record(ID3D12DeviceCreateDescriptorHeapSerializer(create));
}

template <unsigned SIZE>
void CpuDescriptorsService::DescriptorHeap<SIZE>::clearDescriptor(unsigned index) {
  m_DescriptorUsage.reset(index);
}

} // namespace DirectX
} // namespace gits
