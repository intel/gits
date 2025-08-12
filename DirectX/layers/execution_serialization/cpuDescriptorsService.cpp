// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "cpuDescriptorsService.h"
#include "commandsAuto.h"
#include "commandWritersAuto.h"
#include "to_string/enumToStrAuto.h"
#include "gits.h"
#include "log2.h"

namespace gits {
namespace DirectX {

void CpuDescriptorsService::createCommandList(unsigned deviceKey) {
  if (deviceKey_ && deviceKey_ != deviceKey) {
    LOG_ERROR << "Execution serialization - multiple devices not handled";
  }
  deviceKey_ = deviceKey;
}

void CpuDescriptorsService::executeCommandLists(std::vector<unsigned>& commandListKeys) {
  for (unsigned key : commandListKeys) {
    auto it = descriptorsByCommandList_.find(key);
    if (it != descriptorsByCommandList_.end()) {
      for (DescriptorHandle& descriptor : it->second) {
        switch (descriptor.type) {
        case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
          uavDescriptorHeap_.clearDescriptor(descriptor.index);
          break;
        case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
          rtvDescriptorHeap_.clearDescriptor(descriptor.index);
          break;
        case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
          dsvDescriptorHeap_.clearDescriptor(descriptor.index);
          break;
        }
      }
      descriptorsByCommandList_.erase(it);
    }
  }
}

void CpuDescriptorsService::preserveDescriptor(
    ID3D12GraphicsCommandListOMSetRenderTargetsCommand& c) {

  unsigned heapKey{};
  unsigned heapIndex{};
  for (unsigned i = 0; i < c.NumRenderTargetDescriptors_.value; ++i) {
    if (i == 0 || !c.RTsSingleHandleToDescriptorRange_.value) {
      heapKey = c.pRenderTargetDescriptors_.interfaceKeys[i];
      heapIndex = c.pRenderTargetDescriptors_.indexes[i];
    } else {
      ++heapIndex;
    }
    unsigned preservedIndex = rtvDescriptorHeap_.preserveDescriptor(heapKey, heapIndex);
    c.pRenderTargetDescriptors_.interfaceKeys[i] = rtvDescriptorHeap_.descriptorHeapKey_;
    c.pRenderTargetDescriptors_.indexes[i] = preservedIndex;
    descriptorsByCommandList_[c.object_.key].push_back(
        DescriptorHandle{D3D12_DESCRIPTOR_HEAP_TYPE_RTV, preservedIndex});
  }
  c.RTsSingleHandleToDescriptorRange_.value = FALSE;

  if (c.pDepthStencilDescriptor_.value) {
    unsigned preservedIndex = dsvDescriptorHeap_.preserveDescriptor(
        c.pDepthStencilDescriptor_.interfaceKeys[0], c.pDepthStencilDescriptor_.indexes[0]);
    c.pDepthStencilDescriptor_.interfaceKeys[0] = dsvDescriptorHeap_.descriptorHeapKey_;
    c.pDepthStencilDescriptor_.indexes[0] = preservedIndex;
    descriptorsByCommandList_[c.object_.key].push_back(
        DescriptorHandle{D3D12_DESCRIPTOR_HEAP_TYPE_DSV, preservedIndex});
  }
}

void CpuDescriptorsService::preserveDescriptor(
    ID3D12GraphicsCommandListClearDepthStencilViewCommand& c) {
  unsigned preservedIndex = dsvDescriptorHeap_.preserveDescriptor(c.DepthStencilView_.interfaceKey,
                                                                  c.DepthStencilView_.index);
  c.DepthStencilView_.interfaceKey = dsvDescriptorHeap_.descriptorHeapKey_;
  c.DepthStencilView_.index = preservedIndex;
  descriptorsByCommandList_[c.object_.key].push_back(
      DescriptorHandle{D3D12_DESCRIPTOR_HEAP_TYPE_DSV, preservedIndex});
}

void CpuDescriptorsService::preserveDescriptor(
    ID3D12GraphicsCommandListClearRenderTargetViewCommand& c) {
  unsigned preservedIndex = rtvDescriptorHeap_.preserveDescriptor(c.RenderTargetView_.interfaceKey,
                                                                  c.RenderTargetView_.index);
  c.RenderTargetView_.interfaceKey = rtvDescriptorHeap_.descriptorHeapKey_;
  c.RenderTargetView_.index = preservedIndex;
  descriptorsByCommandList_[c.object_.key].push_back(
      DescriptorHandle{D3D12_DESCRIPTOR_HEAP_TYPE_RTV, preservedIndex});
}

void CpuDescriptorsService::preserveDescriptor(
    ID3D12GraphicsCommandListClearUnorderedAccessViewUintCommand& c) {
  unsigned preservedIndex =
      uavDescriptorHeap_.preserveDescriptor(c.ViewCPUHandle_.interfaceKey, c.ViewCPUHandle_.index);
  c.ViewCPUHandle_.interfaceKey = uavDescriptorHeap_.descriptorHeapKey_;
  c.ViewCPUHandle_.index = preservedIndex;
  descriptorsByCommandList_[c.object_.key].push_back(
      DescriptorHandle{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, preservedIndex});
}

void CpuDescriptorsService::preserveDescriptor(
    ID3D12GraphicsCommandListClearUnorderedAccessViewFloatCommand& c) {
  unsigned preservedIndex =
      uavDescriptorHeap_.preserveDescriptor(c.ViewCPUHandle_.interfaceKey, c.ViewCPUHandle_.index);
  c.ViewCPUHandle_.interfaceKey = uavDescriptorHeap_.descriptorHeapKey_;
  c.ViewCPUHandle_.index = preservedIndex;
  descriptorsByCommandList_[c.object_.key].push_back(
      DescriptorHandle{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, preservedIndex});
}

template <unsigned SIZE>
unsigned CpuDescriptorsService::DescriptorHeap<SIZE>::preserveDescriptor(unsigned heapKey,
                                                                         unsigned heapIndex) {
  if (!descriptorHeapKey_) {
    createDescriptorHeap();
  }

  unsigned preservedIndex = 0;
  for (; preservedIndex < descriptorUsage_.size(); ++preservedIndex) {
    if (!descriptorUsage_.test(preservedIndex)) {
      break;
    }
  }
  if (preservedIndex == descriptorUsage_.size()) {
    LOG_ERROR << "Execution serialization - auxiliary descriptor heap " << toStr(type_)
              << " too small!";
  }

  descriptorUsage_.set(preservedIndex);

  ID3D12DeviceCopyDescriptorsSimpleCommand copy;
  copy.key = service_.commandListExecutionService_.getUniqueCommandKey();
  copy.object_.key = service_.deviceKey_;
  copy.NumDescriptors_.value = 1;
  copy.DestDescriptorRangeStart_.interfaceKey = descriptorHeapKey_;
  copy.DestDescriptorRangeStart_.index = preservedIndex;
  copy.SrcDescriptorRangeStart_.interfaceKey = heapKey;
  copy.SrcDescriptorRangeStart_.index = heapIndex;
  copy.DescriptorHeapsType_.value = type_;
  service_.recorder_.record(new ID3D12DeviceCopyDescriptorsSimpleWriter(copy));

  return preservedIndex;
}

template <unsigned SIZE>
void CpuDescriptorsService::DescriptorHeap<SIZE>::createDescriptorHeap() {
  descriptorHeapKey_ = service_.commandListExecutionService_.getUniqueObjectKey();

  D3D12_DESCRIPTOR_HEAP_DESC desc{};
  desc.Type = type_;
  desc.NumDescriptors = descriptorUsage_.size();
  desc.NodeMask = 0;
  desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

  ID3D12DeviceCreateDescriptorHeapCommand create;
  create.key = service_.commandListExecutionService_.getUniqueCommandKey();
  create.object_.key = service_.deviceKey_;
  create.pDescriptorHeapDesc_.value = &desc;
  create.riid_.value = IID_ID3D12DescriptorHeap;
  create.ppvHeap_.key = descriptorHeapKey_;
  service_.recorder_.record(new ID3D12DeviceCreateDescriptorHeapWriter(create));
}

template <unsigned SIZE>
void CpuDescriptorsService::DescriptorHeap<SIZE>::clearDescriptor(unsigned index) {
  descriptorUsage_.reset(index);
}

} // namespace DirectX
} // namespace gits
