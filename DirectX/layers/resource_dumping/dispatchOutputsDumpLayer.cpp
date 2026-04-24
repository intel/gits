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
      m_ResourceDump(Configurator::Get().directx.features.dispatchOutputsDump.format),
      m_FrameRange(Configurator::Get().directx.features.dispatchOutputsDump.frames),
      m_DispatchRange(Configurator::Get().directx.features.dispatchOutputsDump.dispatches),
      m_DryRun(Configurator::Get().directx.features.dispatchOutputsDump.dryRun) {
  const auto& config = Configurator::Get();
  auto& dumpPath = config.common.player.outputDir.empty()
                       ? config.common.player.streamDir / "dispatch_outputs"
                       : config.common.player.outputDir;
  if (!dumpPath.empty() && !std::filesystem::exists(dumpPath)) {
    std::filesystem::create_directories(dumpPath);
  }
  m_DumpPath = dumpPath;

  if (config.directx.features.dispatchOutputsDump.analysisFilePath == "") {
    m_AnalysisFilePath = config.common.player.streamDir.filename().string() + "_frames-" +
                         config.directx.features.dispatchOutputsDump.frames +
                         "_DispatchOutputsDumpAnalysis.txt";
  } else {
    m_AnalysisFilePath = config.directx.features.dispatchOutputsDump.analysisFilePath;
    if (!std::filesystem::exists(m_AnalysisFilePath)) {
      LOG_ERROR << "DispatchOutputsDump - provided analysis file path does not exist: "
                << m_AnalysisFilePath;
      GITS_ASSERT(false && "analysis file path set, but does not exist");
    }
  }

  m_InAnalysis = !std::filesystem::exists(m_AnalysisFilePath);
  if (m_InAnalysis) {
    LOG_INFO << "DISPATCH OUTPUTS DUMP IN ANALYSIS. RUN AGAIN TO DUMP RESOURCES";
    m_DryRun = false;
  } else {
    std::ifstream analysisFile(m_AnalysisFilePath);
    std::string line;
    while (std::getline(analysisFile, line)) {
      std::istringstream iss(line);
      unsigned dispatchKey{};
      unsigned slot{};
      unsigned ResourceKey{};
      iss >> dispatchKey;
      iss >> slot;
      auto& ResourceKeys = m_ResourceKeysBySlotByDispatch[dispatchKey][slot];
      while (iss >> ResourceKey) {
        ResourceKeys.insert(ResourceKey);
      }
    }
  }
}

DispatchOutputsDumpLayer::~DispatchOutputsDumpLayer() {
  try {
    if (m_InAnalysis) {
      std::ofstream analysisFile(m_AnalysisFilePath);
      for (const auto& [dispatchKey, resourceKeysBySlot] : m_ResourceKeysBySlotByDispatch) {
        for (const auto& [slot, ResourceKeys] : resourceKeysBySlot) {
          analysisFile << dispatchKey << " " << slot;
          for (unsigned ResourceKey : ResourceKeys) {
            analysisFile << " " << ResourceKey;
          }
          analysisFile << "\n";
        }
      }
    }

    if (m_DryRun) {
      YAML::Node output;
      output["DispatchesWithTextureByFrame"] = YAML::Node();
      for (const auto& [frame, dispatchNumbers] : m_DryRunInfo.dispatchesWithTextureByFrame) {
        for (unsigned dispatchNumber : dispatchNumbers) {
          output["DispatchesWithTextureByFrame"][frame].push_back(dispatchNumber);
        }
        output["DispatchesWithTextureByFrame"][frame].SetStyle(YAML::EmitterStyle::Flow);
      }
      std::ofstream file(std::filesystem::path(m_DumpPath) / "DispatchOutputsDumpDryRun.yaml");
      file << output;
    }

  } catch (const std::exception&) {
    topmost_exception_handler("DispatchOutputsDumpLayer::~DispatchOutputsDumpLayer");
  }
}

void DispatchOutputsDumpLayer::Post(ID3D12DeviceCreateCommittedResourceCommand& c) {
  m_ResourceByKey[c.m_ppvResource.Key] = static_cast<ID3D12Resource*>(*c.m_ppvResource.Value);
}

void DispatchOutputsDumpLayer::Post(ID3D12Device4CreateCommittedResource1Command& c) {
  m_ResourceByKey[c.m_ppvResource.Key] = static_cast<ID3D12Resource*>(*c.m_ppvResource.Value);
}

void DispatchOutputsDumpLayer::Post(ID3D12Device8CreateCommittedResource2Command& c) {
  m_ResourceByKey[c.m_ppvResource.Key] = static_cast<ID3D12Resource*>(*c.m_ppvResource.Value);
}

void DispatchOutputsDumpLayer::Post(ID3D12Device10CreateCommittedResource3Command& c) {
  m_ResourceByKey[c.m_ppvResource.Key] = static_cast<ID3D12Resource*>(*c.m_ppvResource.Value);
}

void DispatchOutputsDumpLayer::Post(INTC_D3D12_CreateCommittedResourceCommand& c) {
  m_ResourceByKey[c.m_ppvResource.Key] = static_cast<ID3D12Resource*>(*c.m_ppvResource.Value);
}

void DispatchOutputsDumpLayer::Post(ID3D12DeviceCreatePlacedResourceCommand& c) {
  m_ResourceByKey[c.m_ppvResource.Key] = static_cast<ID3D12Resource*>(*c.m_ppvResource.Value);
}

void DispatchOutputsDumpLayer::Post(ID3D12Device8CreatePlacedResource1Command& c) {
  m_ResourceByKey[c.m_ppvResource.Key] = static_cast<ID3D12Resource*>(*c.m_ppvResource.Value);
}

void DispatchOutputsDumpLayer::Post(ID3D12Device10CreatePlacedResource2Command& c) {
  m_ResourceByKey[c.m_ppvResource.Key] = static_cast<ID3D12Resource*>(*c.m_ppvResource.Value);
}

void DispatchOutputsDumpLayer::Post(INTC_D3D12_CreatePlacedResourceCommand& c) {
  m_ResourceByKey[c.m_ppvResource.Key] = static_cast<ID3D12Resource*>(*c.m_ppvResource.Value);
}

void DispatchOutputsDumpLayer::Post(ID3D12DeviceCreateReservedResourceCommand& c) {
  m_ResourceByKey[c.m_ppvResource.Key] = static_cast<ID3D12Resource*>(*c.m_ppvResource.Value);
}

void DispatchOutputsDumpLayer::Post(ID3D12Device4CreateReservedResource1Command& c) {
  m_ResourceByKey[c.m_ppvResource.Key] = static_cast<ID3D12Resource*>(*c.m_ppvResource.Value);
}

void DispatchOutputsDumpLayer::Post(ID3D12Device10CreateReservedResource2Command& c) {
  m_ResourceByKey[c.m_ppvResource.Key] = static_cast<ID3D12Resource*>(*c.m_ppvResource.Value);
}

void DispatchOutputsDumpLayer::Post(INTC_D3D12_CreateReservedResourceCommand& c) {
  m_ResourceByKey[c.m_ppvResource.Key] = static_cast<ID3D12Resource*>(*c.m_ppvResource.Value);
}

void DispatchOutputsDumpLayer::Post(IUnknownReleaseCommand& c) {
  if (c.m_Result.Value == 0) {
    m_DescriptorService.DestroyObject(c.m_Object.Key);
    m_ResourceByKey.erase(c.m_Object.Key);
  }
}

void DispatchOutputsDumpLayer::Post(ID3D12DeviceCreateDescriptorHeapCommand& c) {
  m_DescriptorHeapInfos[c.m_ppvHeap.Key].type = c.m_pDescriptorHeapDesc.Value->Type;
  m_DescriptorHeapInfos[c.m_ppvHeap.Key].numDescriptors =
      c.m_pDescriptorHeapDesc.Value->NumDescriptors;
}

void DispatchOutputsDumpLayer::Post(ID3D12DeviceCreateRenderTargetViewCommand& c) {
  CreateDescriptor(c.m_DestDescriptor.InterfaceKey, c.m_DestDescriptor.Index, c.m_pResource.Key,
                   DescriptorHeapTracker::DescriptorInfo::DescriptorKind::RTV);
}

void DispatchOutputsDumpLayer::Post(ID3D12DeviceCreateDepthStencilViewCommand& c) {
  CreateDescriptor(c.m_DestDescriptor.InterfaceKey, c.m_DestDescriptor.Index, c.m_pResource.Key,
                   DescriptorHeapTracker::DescriptorInfo::DescriptorKind::DSV);
}

void DispatchOutputsDumpLayer::Post(ID3D12DeviceCreateShaderResourceViewCommand& c) {
  CreateDescriptor(c.m_DestDescriptor.InterfaceKey, c.m_DestDescriptor.Index, c.m_pResource.Key,
                   DescriptorHeapTracker::DescriptorInfo::DescriptorKind::SRV);
}

void DispatchOutputsDumpLayer::Post(ID3D12DeviceCreateUnorderedAccessViewCommand& c) {
  CreateDescriptor(c.m_DestDescriptor.InterfaceKey, c.m_DestDescriptor.Index, c.m_pResource.Key,
                   DescriptorHeapTracker::DescriptorInfo::DescriptorKind::UAV);
}

void DispatchOutputsDumpLayer::Post(ID3D12DeviceCreateConstantBufferViewCommand& c) {
  CreateDescriptor(c.m_DestDescriptor.InterfaceKey, c.m_DestDescriptor.Index,
                   c.m_pDesc.BufferLocationKey,
                   DescriptorHeapTracker::DescriptorInfo::DescriptorKind::CBV);
}

void DispatchOutputsDumpLayer::Post(ID3D12DeviceCreateSamplerCommand& c) {
  CreateDescriptor(c.m_DestDescriptor.InterfaceKey, c.m_DestDescriptor.Index, 0,
                   DescriptorHeapTracker::DescriptorInfo::DescriptorKind::Sampler);
}

void DispatchOutputsDumpLayer::Post(ID3D12DeviceCopyDescriptorsSimpleCommand& c) {
  m_DescriptorService.CopyDescriptors(c);
}

void DispatchOutputsDumpLayer::Post(ID3D12DeviceCopyDescriptorsCommand& c) {
  m_DescriptorService.CopyDescriptors(c);
}

void DispatchOutputsDumpLayer::Post(ID3D12DeviceCreateRootSignatureCommand& c) {
  m_RootSignatureService.CreateRootSignature(c);
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
      const auto indicesBySlotByDispatchIt =
          m_IndicesBySlotByDispatchByCommandList.find(c.m_ppCommandLists.Keys[i]);
      if (indicesBySlotByDispatchIt != m_IndicesBySlotByDispatchByCommandList.end()) {
        for (const auto& [dispatchKey, indicesBySlot] : indicesBySlotByDispatchIt->second) {
          for (const auto& [slot, indicesInfo] : indicesBySlot) {
            for (unsigned index : indicesInfo.indices) {
              const auto* descriptorInfo =
                  m_DescriptorService.GetDescriptorInfo(indicesInfo.DescriptorHeapKey, index);
              if (descriptorInfo &&
                  descriptorInfo->Kind ==
                      DescriptorHeapTracker::DescriptorInfo::DescriptorKind::UAV &&
                  descriptorInfo->ResourceKey) {
                ID3D12Resource* resource = m_ResourceByKey[descriptorInfo->ResourceKey];
                if (resource && resource->GetDesc().Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
                  m_ResourceKeysBySlotByDispatch[dispatchKey][slot].insert(
                      descriptorInfo->ResourceKey);
                }
              }
            }
          }
        }
      }
    }
  }
  ++m_ExecuteCount;
  m_ResourceDump.executeCommandLists(c.Key, c.m_Object.Key, c.m_Object.Value,
                                     c.m_ppCommandLists.Value, c.m_NumCommandLists.Value,
                                     m_CurrentFrame, m_ExecuteCount);
}

void DispatchOutputsDumpLayer::Post(ID3D12CommandQueueWaitCommand& c) {
  m_ResourceDump.commandQueueWait(c.Key, c.m_Object.Key, c.m_pFence.Key, c.m_Value.Value);
}

void DispatchOutputsDumpLayer::Post(ID3D12CommandQueueSignalCommand& c) {
  m_ResourceDump.commandQueueSignal(c.Key, c.m_Object.Key, c.m_pFence.Key, c.m_Value.Value);
}

void DispatchOutputsDumpLayer::Post(ID3D12FenceSignalCommand& c) {
  m_ResourceDump.fenceSignal(c.Key, c.m_Object.Key, c.m_Value.Value);
}

void DispatchOutputsDumpLayer::Post(ID3D12DeviceCreateFenceCommand& c) {
  m_ResourceDump.fenceSignal(c.Key, c.m_ppFence.Key, c.m_InitialValue.Value);
}

void DispatchOutputsDumpLayer::Post(ID3D12Device3EnqueueMakeResidentCommand& c) {
  m_ResourceDump.fenceSignal(c.Key, c.m_pFenceToSignal.Key, c.m_FenceValueToSignal.Value);
}

void DispatchOutputsDumpLayer::Post(ID3D12GraphicsCommandListResetCommand& c) {
  if (!m_InAnalysis) {
    return;
  }

  m_IndicesBySlotByCommandList[c.m_Object.Key].clear();
  m_IndicesBySlotByDispatchByCommandList[c.m_Object.Key].clear();
  m_ResourceKeyFromSetViewBySlotByCommandList[c.m_Object.Key].clear();
}

void DispatchOutputsDumpLayer::Post(ID3D12GraphicsCommandListSetComputeRootSignatureCommand& c) {
  m_CommandListInfos[c.m_Object.Key].computeRootSignature = c.m_pRootSignature.Key;
}

void DispatchOutputsDumpLayer::Post(
    ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& c) {
  if (!m_InAnalysis) {
    return;
  }

  ID3D12Resource* resource = m_ResourceByKey[c.m_BufferLocation.InterfaceKey];
  if (!resource || resource->GetDesc().Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
    return;
  }

  m_IndicesBySlotByCommandList[c.m_Object.Key][c.m_RootParameterIndex.Value] = {};
  m_ResourceKeyFromSetViewBySlotByCommandList[c.m_Object.Key][c.m_RootParameterIndex.Value] =
      c.m_BufferLocation.InterfaceKey;
}

void DispatchOutputsDumpLayer::Post(
    ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& c) {
  if (!m_InAnalysis || !c.m_BaseDescriptor.Value.ptr) {
    return;
  }
  unsigned RootSignatureKey = m_CommandListInfos[c.m_Object.Key].computeRootSignature;
  GITS_ASSERT(RootSignatureKey);
  unsigned numDescriptors = m_DescriptorHeapInfos[c.m_BaseDescriptor.InterfaceKey].numDescriptors;
  GITS_ASSERT(numDescriptors);

  std::vector<unsigned> indices = m_RootSignatureService.GetDescriptorTableIndexes(
      RootSignatureKey, c.m_BaseDescriptor.InterfaceKey, c.m_RootParameterIndex.Value,
      c.m_BaseDescriptor.Index, numDescriptors, false);

  m_IndicesBySlotByCommandList[c.m_Object.Key][c.m_RootParameterIndex.Value].DescriptorHeapKey =
      c.m_BaseDescriptor.InterfaceKey;
  m_IndicesBySlotByCommandList[c.m_Object.Key][c.m_RootParameterIndex.Value].indices = indices;
}

void DispatchOutputsDumpLayer::Post(ID3D12GraphicsCommandListDispatchCommand& c) {
  unsigned commandListKey = c.m_Object.Key;
  ID3D12GraphicsCommandList* commandList = c.m_Object.Value;

  ++m_DispatchCount;
  unsigned commandListDispatchCount = ++m_DispatchCountByCommandList[commandListKey];
  if (!m_FrameRange[m_CurrentFrame]) {
    return;
  }
  if (!m_DispatchRange[m_DispatchCount] && !m_InAnalysis) {
    return;
  }

  if (m_InAnalysis) {
    m_IndicesBySlotByDispatchByCommandList[commandListKey][c.Key] =
        m_IndicesBySlotByCommandList[commandListKey];
    auto resourceKeyBySlotIt = m_ResourceKeyFromSetViewBySlotByCommandList.find(commandListKey);
    if (resourceKeyBySlotIt != m_ResourceKeyFromSetViewBySlotByCommandList.end()) {
      for (const auto& [slot, ResourceKey] : resourceKeyBySlotIt->second) {
        m_ResourceKeysBySlotByDispatch[c.Key][slot].insert(ResourceKey);
      }
    }
  } else {
    auto resourceKeysBySlotIt = m_ResourceKeysBySlotByDispatch.find(c.Key);
    if (resourceKeysBySlotIt != m_ResourceKeysBySlotByDispatch.end()) {
      for (const auto& [slot, ResourceKeys] : resourceKeysBySlotIt->second) {
        for (unsigned ResourceKey : ResourceKeys) {
          if (m_DryRun) {
            m_DryRunInfo.dispatchesWithTextureByFrame[m_CurrentFrame].insert(m_DispatchCount);
          } else {
            ID3D12Resource* resource = m_ResourceByKey[ResourceKey];
            GITS_ASSERT(resource);
            DispatchOutput dispatchOutput{ResourceKey, resource, slot};
            DumpComputeOutput(commandList, dispatchOutput, m_CurrentFrame,
                              commandListDispatchCount);
          }
        }
      }
    }
  }
}

void DispatchOutputsDumpLayer::CreateDescriptor(
    unsigned heapKey,
    unsigned DescriptorIndex,
    unsigned ResourceKey,
    DescriptorHeapTracker::DescriptorInfo::DescriptorKind descriptorKind) {
  auto* descriptorInfo = new DescriptorHeapTracker::DescriptorInfo;
  descriptorInfo->HeapKey = heapKey;
  descriptorInfo->DescriptorIndex = DescriptorIndex;
  descriptorInfo->ResourceKey = ResourceKey;
  descriptorInfo->Kind = descriptorKind;
  m_DescriptorService.CreateDescriptor(descriptorInfo);
}

void DispatchOutputsDumpLayer::DumpComputeOutput(ID3D12GraphicsCommandList* commandList,
                                                 const DispatchOutput& dispatchOutput,
                                                 unsigned frame,
                                                 unsigned commandListDispatch) {
  D3D12_RESOURCE_DESC desc = dispatchOutput.resource->GetDesc();
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

  std::string formatName = m_ResourceDump.formatToString(format);
  std::wstring formatNameW(formatName.begin(), formatName.end());

  Microsoft::WRL::ComPtr<ID3D12Device> device;
  HRESULT hr = dispatchOutput.resource->GetDevice(IID_PPV_ARGS(&device));
  GITS_ASSERT(hr == S_OK);
  unsigned planeCount = D3D12GetFormatPlaneCount(device.Get(), format);

  for (unsigned arrayIndex = minArrayIndex; arrayIndex <= maxArrayIndex; ++arrayIndex) {
    for (unsigned planeSlice = 0; planeSlice < planeCount; ++planeSlice) {

      std::wstring dumpName = m_DumpPath + L"/dispatch_e_" +
                              m_ResourceDump.m_DumpNameExecutionMarker + L"_f_" +
                              std::to_wstring(frame) + L"_d_" + std::to_wstring(m_DispatchCount) +
                              L"_s_" + std::to_wstring(dispatchOutput.slot) + L"_O" +
                              std::to_wstring(dispatchOutput.ResourceKey) + L"_" + formatNameW;
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
      m_ResourceDump.dumpResource(commandList, dispatchOutput.resource, subresource,
                                  D3D12_RESOURCE_STATE_UNORDERED_ACCESS, dumpName, mipLevel, format,
                                  commandListDispatch);
    }
  }
}

} // namespace DirectX
} // namespace gits
