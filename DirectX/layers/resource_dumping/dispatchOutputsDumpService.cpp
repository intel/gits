// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "dispatchOutputsDumpService.h"
#include "yaml-cpp/yaml.h"

#include <d3dx12.h>
#include <fstream>

namespace gits {
namespace DirectX {

DispatchOutputsDumpService::DispatchOutputsDumpService(
    DispatchOutputsAnalyzer& dispatchOutputsAnalyzer)
    : m_DispatchOutputsAnalyzer(dispatchOutputsAnalyzer),
      m_ResourceDump(Configurator::Get().directx.features.dispatchOutputsDump.format),
      m_SlotResourcesRange(
          Configurator::Get().directx.features.dispatchOutputsDump.dispatchSlotRange) {
  const auto& config = Configurator::Get();
  m_SkipUnboundedHeaps = config.directx.features.dispatchOutputsDump.skipUnboundedHeaps;
  m_DryRun = config.directx.features.dispatchOutputsDump.dryRun;

  auto& dumpPath = config.common.player.outputDir.empty()
                       ? config.common.player.streamDir / "dispatch_outputs"
                       : config.common.player.outputDir;
  if (!dumpPath.empty() && !std::filesystem::exists(dumpPath)) {
    std::filesystem::create_directories(dumpPath);
  }
  m_DumpPath = dumpPath;
}

void DispatchOutputsDumpService::CreateResource(ID3D12Resource* resource,
                                                unsigned resourceKey,
                                                D3D12_RESOURCE_STATES initialState) {
  m_ResourceByKey[resourceKey] = resource;
  m_ResourceStateTracker.AddResource(resource, resourceKey, initialState);
}

void DispatchOutputsDumpService::CreateResource(ID3D12Resource* resource,
                                                unsigned resourceKey,
                                                D3D12_BARRIER_LAYOUT initialLayout) {
  m_ResourceByKey[resourceKey] = resource;
  m_ResourceStateTracker.AddResource(resource, resourceKey, initialLayout);
}

void DispatchOutputsDumpService::DestroyInterface(unsigned interfaceKey) {
  m_ResourceByKey.erase(interfaceKey);
}

void DispatchOutputsDumpService::ExecuteCommandLists(
    ID3D12CommandQueueExecuteCommandListsCommand& c, unsigned frame, unsigned execution) {
  m_ResourceStateTracker.ExecuteCommandLists(
      reinterpret_cast<ID3D12GraphicsCommandList**>(c.m_ppCommandLists.Value),
      c.m_NumCommandLists.Value);
  m_ResourceDump.ExecuteCommandLists(c.Key, c.m_Object.Key, c.m_Object.Value,
                                     c.m_ppCommandLists.Value, c.m_NumCommandLists.Value, frame,
                                     execution);
}

void DispatchOutputsDumpService::Dispatch(ID3D12GraphicsCommandListDispatchCommand& c,
                                          unsigned frame,
                                          unsigned frameDispatch,
                                          unsigned commandListDispatch) {
  std::vector<DispatchOutputsAnalyzer::Bindings>* dispatchBindings =
      m_DispatchOutputsAnalyzer.GetDispatchBindings(c.Key);
  if (dispatchBindings) {
    for (DispatchOutputsAnalyzer::Bindings& slotBindings : *dispatchBindings) {
      if (slotBindings.Unbounded && m_SkipUnboundedHeaps && !m_DryRun) {
        LOG_WARNING << "Skipped dumping " << slotBindings.Resources.size()
                    << " resources from unbounded descriptor heap in frame " << frame
                    << " dispatch " << frameDispatch << " slot " << slotBindings.Slot;
        continue;
      }
      unsigned resourceIndex{};
      unsigned resourceCount{};
      for (unsigned resourceKey : slotBindings.Resources) {
        ++resourceIndex;
        if (m_SlotResourcesRange[resourceIndex]) {
          ID3D12Resource* resource = m_ResourceByKey[resourceKey];
          GITS_ASSERT(resource);
          if (DumpComputeOutput(c.m_Object.Value, resourceKey, resource, slotBindings.Slot, frame,
                                frameDispatch, commandListDispatch)) {
            ++resourceCount;
            m_DispatchOutputsDryRun.AddDispatch(frame, frameDispatch);
          }
        }
      }
      if (!m_DryRun) {
        LOG_INFO << "Dumping " << resourceCount << " resources from "
                 << (slotBindings.Unbounded ? "unbounded" : "bounded")
                 << " descriptor heap in frame " << frame << " dispatch " << frameDispatch
                 << " slot " << slotBindings.Slot;
      }
    }
  }
}

void DispatchOutputsDumpService::CommandQueueWait(ID3D12CommandQueueWaitCommand& c) {
  m_ResourceDump.CommandQueueWait(c.Key, c.m_Object.Key, c.m_pFence.Key, c.m_Value.Value);
}

void DispatchOutputsDumpService::CommandQueueSignal(ID3D12CommandQueueSignalCommand& c) {
  m_ResourceDump.CommandQueueSignal(c.Key, c.m_Object.Key, c.m_pFence.Key, c.m_Value.Value);
}

void DispatchOutputsDumpService::FenceSignal(ID3D12FenceSignalCommand& c) {
  m_ResourceDump.FenceSignal(c.Key, c.m_Object.Key, c.m_Value.Value);
}

void DispatchOutputsDumpService::CreateFence(ID3D12DeviceCreateFenceCommand& c) {
  m_ResourceDump.FenceSignal(c.Key, c.m_ppFence.Key, c.m_InitialValue.Value);
}

void DispatchOutputsDumpService::EnqueueMakeResident(ID3D12Device3EnqueueMakeResidentCommand& c) {
  m_ResourceDump.FenceSignal(c.Key, c.m_pFenceToSignal.Key, c.m_FenceValueToSignal.Value);
}

void DispatchOutputsDumpService::ResourceBarrier(
    ID3D12GraphicsCommandListResourceBarrierCommand& c) {
  m_ResourceStateTracker.ResourceBarrier(c.m_Object.Value, c.m_pBarriers.Value,
                                         c.m_NumBarriers.Value, c.m_pBarriers.ResourceKeys.data());
}

void DispatchOutputsDumpService::Barrier(ID3D12GraphicsCommandList7BarrierCommand& c) {
  m_ResourceStateTracker.ResourceBarrier(c.m_Object.Value, c.m_pBarrierGroups.Value,
                                         c.m_NumBarrierGroups.Value,
                                         c.m_pBarrierGroups.ResourceKeys.data());
}

bool DispatchOutputsDumpService::DumpComputeOutput(ID3D12GraphicsCommandList* commandList,
                                                   unsigned resourceKey,
                                                   ID3D12Resource* resource,
                                                   unsigned slot,
                                                   unsigned frame,
                                                   unsigned frameDispatch,
                                                   unsigned commandListDispatch) {
  D3D12_RESOURCE_DESC desc = resource->GetDesc();
  DXGI_FORMAT format = desc.Format;

  unsigned arraySize =
      desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D ? 1 : desc.DepthOrArraySize;
  unsigned minArrayIndex = 0;
  unsigned maxArrayIndex =
      desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D ? 0 : desc.DepthOrArraySize - 1;
  unsigned mipLevel = 0;
  unsigned minSlice = 0;
  unsigned maxSlice =
      desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D ? desc.DepthOrArraySize - 1 : 0;

  std::string formatName = m_ResourceDump.FormatToString(format);
  std::wstring formatNameW(formatName.begin(), formatName.end());

  Microsoft::WRL::ComPtr<ID3D12Device> device;
  HRESULT hr = resource->GetDevice(IID_PPV_ARGS(&device));
  GITS_ASSERT(hr == S_OK);
  unsigned planeCount = D3D12GetFormatPlaneCount(device.Get(), format);

  bool dumped = false;

  for (unsigned arrayIndex = minArrayIndex; arrayIndex <= maxArrayIndex; ++arrayIndex) {
    for (unsigned planeSlice = 0; planeSlice < planeCount; ++planeSlice) {

      std::wstring dumpName =
          m_DumpPath + L"/dispatch_e_" + m_ResourceDump.m_DumpNameExecutionMarker + L"_f_" +
          std::to_wstring(frame) + L"_d_" + std::to_wstring(frameDispatch) + L"_s_" +
          std::to_wstring(slot) + L"_O" + std::to_wstring(resourceKey) + L"_" + formatNameW;
      if (planeCount > 1) {
        dumpName += L"_plane_" + std::to_wstring(planeSlice);
      }
      if (arraySize > 1) {
        dumpName += L"_array_" + std::to_wstring(arrayIndex);
      }
      if (desc.MipLevels > 1) {
        dumpName += L"_mip_" + std::to_wstring(mipLevel);
      }

      unsigned subresource =
          D3D12CalcSubresource(mipLevel, arrayIndex, planeSlice, desc.MipLevels, arraySize);
      BarrierState resourceState =
          m_ResourceStateTracker.GetSubresourceState(commandList, resourceKey, subresource);
      if (IsUav(resourceState)) {
        if (!m_DryRun) {
          m_ResourceDump.DumpResource(commandList, resource, subresource, resourceState, dumpName,
                                      mipLevel, format, commandListDispatch);
        }
        dumped = true;
      }
    }
  }
  return dumped;
}

bool DispatchOutputsDumpService::IsUav(BarrierState resourceState) const {
  if (!resourceState.Enhanced) {
    return resourceState.State == D3D12_RESOURCE_STATE_UNORDERED_ACCESS ||
           resourceState.State == D3D12_RESOURCE_STATE_COMMON;
  } else {
    return resourceState.Layout == D3D12_BARRIER_LAYOUT_COMMON ||
           resourceState.Layout == D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_UNORDERED_ACCESS ||
           resourceState.Layout == D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_UNORDERED_ACCESS;
  }
}

void DispatchOutputsDumpService::DumpDryRun() {
  std::filesystem::path dumpDir(m_DumpPath);
  m_DispatchOutputsDryRun.DumpFile(dumpDir);
}

void DispatchOutputsDumpService::DispatchOutputsDryRun::DumpFile(std::filesystem::path& dumpDir) {
  YAML::Node output;
  output["DispatchesWithTextureByFrame"] = YAML::Node();
  for (const auto& [frame, dispatchNumbers] : m_DispatchesWithTextureByFrame) {
    for (unsigned dispatchNumber : dispatchNumbers) {
      output["DispatchesWithTextureByFrame"][frame].push_back(dispatchNumber);
    }
    output["DispatchesWithTextureByFrame"][frame].SetStyle(YAML::EmitterStyle::Flow);
  }
  std::ofstream file(dumpDir / "DispatchOutputsDumpDryRun.yaml");
  file << output;
}

} // namespace DirectX
} // namespace gits
