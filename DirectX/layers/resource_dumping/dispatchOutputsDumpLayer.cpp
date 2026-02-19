// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "dispatchOutputsDumpLayer.h"
#include "gits.h"
#include "log.h"
#include "configurationLib.h"
#include "yaml-cpp/yaml.h"

#include <fstream>
#include <d3dx12.h>
#include <wrl/client.h>

namespace gits {
namespace DirectX {

DispatchOutputsDumpLayer::DispatchOutputsDumpLayer()
    : Layer("DispatchOutputsDump"),
      resourceDump_(Configurator::Get().directx.features.dispatchOutputsDump.format),
      frameRange_(Configurator::Get().directx.features.dispatchOutputsDump.frames),
      dispatchRange_(Configurator::Get().directx.features.dispatchOutputsDump.dispatches),
      dryRun_(Configurator::Get().directx.features.dispatchOutputsDump.dryRun) {
  const auto& config = Configurator::Get();
  auto& dumpPath = config.common.player.outputDir.empty()
                       ? config.common.player.streamDir / "dispatch_outputs"
                       : config.common.player.outputDir;
  if (!dumpPath.empty() && !std::filesystem::exists(dumpPath)) {
    std::filesystem::create_directories(dumpPath);
  }
  dumpPath_ = dumpPath;

  analysisFileName_ = config.common.player.streamDir.filename().string() + "_frames-" +
                      config.directx.features.dispatchOutputsDump.frames +
                      "_DispatchOutputsDumpAnalysis.txt";
  inAnalysis_ = !std::filesystem::exists(analysisFileName_);
  if (inAnalysis_) {
    LOG_INFO << "DISPATCH OUTPUTS DUMP IN ANALYSIS. RUN AGAIN TO DUMP RESOURCES";
    dryRun_ = false;
  } else {
    std::ifstream analysisFile(analysisFileName_);
    std::string line;
    while (std::getline(analysisFile, line)) {
      std::istringstream iss(line);
      unsigned dispatchKey{};
      unsigned slot{};
      unsigned resourceKey{};
      iss >> dispatchKey;
      iss >> slot;
      auto& resourceKeys = resourceKeysBySlotByDispatch_[dispatchKey][slot];
      while (iss >> resourceKey) {
        resourceKeys.insert(resourceKey);
      }
    }
  }
}

DispatchOutputsDumpLayer::~DispatchOutputsDumpLayer() {
  try {
    if (inAnalysis_) {
      std::ofstream analysisFile(analysisFileName_);
      for (const auto& [dispatchKey, resourceKeysBySlot] : resourceKeysBySlotByDispatch_) {
        for (const auto& [slot, resourceKeys] : resourceKeysBySlot) {
          analysisFile << dispatchKey << " " << slot;
          for (unsigned resourceKey : resourceKeys) {
            analysisFile << " " << resourceKey;
          }
          analysisFile << "\n";
        }
      }
    }

    if (dryRun_) {
      YAML::Node output;
      output["DispatchesWithTextureByFrame"] = YAML::Node();
      for (const auto& [frame, dispatchNumbers] : dryRunInfo_.dispatchesWithTextureByFrame) {
        for (unsigned dispatchNumber : dispatchNumbers) {
          output["DispatchesWithTextureByFrame"][frame].push_back(dispatchNumber);
        }
        output["DispatchesWithTextureByFrame"][frame].SetStyle(YAML::EmitterStyle::Flow);
      }
      std::ofstream file(std::filesystem::path(dumpPath_) / "DispatchOutputsDumpDryRun.yaml");
      file << output;
    }

  } catch (const std::exception&) {
    topmost_exception_handler("DispatchOutputsDumpLayer::~DispatchOutputsDumpLayer");
  }
}

void DispatchOutputsDumpLayer::post(ID3D12DeviceCreateCommittedResourceCommand& c) {
  resourceByKey_[c.ppvResource_.key] = static_cast<ID3D12Resource*>(*c.ppvResource_.value);
}

void DispatchOutputsDumpLayer::post(ID3D12Device4CreateCommittedResource1Command& c) {
  resourceByKey_[c.ppvResource_.key] = static_cast<ID3D12Resource*>(*c.ppvResource_.value);
}

void DispatchOutputsDumpLayer::post(ID3D12Device8CreateCommittedResource2Command& c) {
  resourceByKey_[c.ppvResource_.key] = static_cast<ID3D12Resource*>(*c.ppvResource_.value);
}

void DispatchOutputsDumpLayer::post(ID3D12Device10CreateCommittedResource3Command& c) {
  resourceByKey_[c.ppvResource_.key] = static_cast<ID3D12Resource*>(*c.ppvResource_.value);
}

void DispatchOutputsDumpLayer::post(INTC_D3D12_CreateCommittedResourceCommand& c) {
  resourceByKey_[c.ppvResource_.key] = static_cast<ID3D12Resource*>(*c.ppvResource_.value);
}

void DispatchOutputsDumpLayer::post(ID3D12DeviceCreatePlacedResourceCommand& c) {
  resourceByKey_[c.ppvResource_.key] = static_cast<ID3D12Resource*>(*c.ppvResource_.value);
}

void DispatchOutputsDumpLayer::post(ID3D12Device8CreatePlacedResource1Command& c) {
  resourceByKey_[c.ppvResource_.key] = static_cast<ID3D12Resource*>(*c.ppvResource_.value);
}

void DispatchOutputsDumpLayer::post(ID3D12Device10CreatePlacedResource2Command& c) {
  resourceByKey_[c.ppvResource_.key] = static_cast<ID3D12Resource*>(*c.ppvResource_.value);
}

void DispatchOutputsDumpLayer::post(INTC_D3D12_CreatePlacedResourceCommand& c) {
  resourceByKey_[c.ppvResource_.key] = static_cast<ID3D12Resource*>(*c.ppvResource_.value);
}

void DispatchOutputsDumpLayer::post(ID3D12DeviceCreateReservedResourceCommand& c) {
  resourceByKey_[c.ppvResource_.key] = static_cast<ID3D12Resource*>(*c.ppvResource_.value);
}

void DispatchOutputsDumpLayer::post(ID3D12Device4CreateReservedResource1Command& c) {
  resourceByKey_[c.ppvResource_.key] = static_cast<ID3D12Resource*>(*c.ppvResource_.value);
}

void DispatchOutputsDumpLayer::post(ID3D12Device10CreateReservedResource2Command& c) {
  resourceByKey_[c.ppvResource_.key] = static_cast<ID3D12Resource*>(*c.ppvResource_.value);
}

void DispatchOutputsDumpLayer::post(INTC_D3D12_CreateReservedResourceCommand& c) {
  resourceByKey_[c.ppvResource_.key] = static_cast<ID3D12Resource*>(*c.ppvResource_.value);
}

void DispatchOutputsDumpLayer::post(IUnknownReleaseCommand& c) {
  if (c.result_.value == 0) {
    descriptorService_.destroyObject(c.object_.key);
    resourceByKey_.erase(c.object_.key);
  }
}

void DispatchOutputsDumpLayer::post(ID3D12DeviceCreateDescriptorHeapCommand& c) {
  descriptorHeapInfos_[c.ppvHeap_.key].type = c.pDescriptorHeapDesc_.value->Type;
  descriptorHeapInfos_[c.ppvHeap_.key].numDescriptors =
      c.pDescriptorHeapDesc_.value->NumDescriptors;
}

void DispatchOutputsDumpLayer::post(ID3D12DeviceCreateRenderTargetViewCommand& c) {
  createDescriptor(c.DestDescriptor_.interfaceKey, c.DestDescriptor_.index, c.pResource_.key,
                   DescriptorHeapTracker::DescriptorInfo::RTV);
}

void DispatchOutputsDumpLayer::post(ID3D12DeviceCreateDepthStencilViewCommand& c) {
  createDescriptor(c.DestDescriptor_.interfaceKey, c.DestDescriptor_.index, c.pResource_.key,
                   DescriptorHeapTracker::DescriptorInfo::DSV);
}

void DispatchOutputsDumpLayer::post(ID3D12DeviceCreateShaderResourceViewCommand& c) {
  createDescriptor(c.DestDescriptor_.interfaceKey, c.DestDescriptor_.index, c.pResource_.key,
                   DescriptorHeapTracker::DescriptorInfo::SRV);
}

void DispatchOutputsDumpLayer::post(ID3D12DeviceCreateUnorderedAccessViewCommand& c) {
  createDescriptor(c.DestDescriptor_.interfaceKey, c.DestDescriptor_.index, c.pResource_.key,
                   DescriptorHeapTracker::DescriptorInfo::UAV);
}

void DispatchOutputsDumpLayer::post(ID3D12DeviceCreateConstantBufferViewCommand& c) {
  createDescriptor(c.DestDescriptor_.interfaceKey, c.DestDescriptor_.index,
                   c.pDesc_.bufferLocationKey, DescriptorHeapTracker::DescriptorInfo::CBV);
}

void DispatchOutputsDumpLayer::post(ID3D12DeviceCreateSamplerCommand& c) {
  createDescriptor(c.DestDescriptor_.interfaceKey, c.DestDescriptor_.index, 0,
                   DescriptorHeapTracker::DescriptorInfo::Sampler);
}

void DispatchOutputsDumpLayer::post(ID3D12DeviceCopyDescriptorsSimpleCommand& c) {
  descriptorService_.copyDescriptors(c);
}

void DispatchOutputsDumpLayer::post(ID3D12DeviceCopyDescriptorsCommand& c) {
  descriptorService_.copyDescriptors(c);
}

void DispatchOutputsDumpLayer::post(ID3D12DeviceCreateRootSignatureCommand& c) {
  rootSignatureService_.createRootSignature(c);
}

void DispatchOutputsDumpLayer::post(StateRestoreBeginCommand& c) {
  stateRestorePhase_ = true;
}

void DispatchOutputsDumpLayer::post(StateRestoreEndCommand& c) {
  stateRestorePhase_ = false;
  dispatchCount_ = 0;
  executeCount_ = 0;
}

void DispatchOutputsDumpLayer::post(IDXGISwapChainPresentCommand& c) {
  if (!(c.Flags_.value & DXGI_PRESENT_TEST)) {
    dispatchCount_ = 0;
    executeCount_ = 0;
  }
}

void DispatchOutputsDumpLayer::post(IDXGISwapChain1Present1Command& c) {
  if (!(c.PresentFlags_.value & DXGI_PRESENT_TEST)) {
    dispatchCount_ = 0;
    executeCount_ = 0;
  }
}

void DispatchOutputsDumpLayer::post(ID3D12CommandQueueExecuteCommandListsCommand& c) {
  ++executeCount_;
  unsigned frame = stateRestorePhase_ ? 0 : CGits::Instance().CurrentFrame();

  for (unsigned i = 0; i < c.NumCommandLists_.value; ++i) {
    dispatchCountByCommandList_.erase(c.ppCommandLists_.keys[i]);

    if (inAnalysis_) {
      const auto indicesBySlotByDispatchIt =
          indicesBySlotByDispatchByCommandList_.find(c.ppCommandLists_.keys[i]);
      if (indicesBySlotByDispatchIt != indicesBySlotByDispatchByCommandList_.end()) {
        for (const auto& [dispatchKey, indicesBySlot] : indicesBySlotByDispatchIt->second) {
          for (const auto& [slot, indicesInfo] : indicesBySlot) {
            for (unsigned index : indicesInfo.indices) {
              const auto* descriptorInfo =
                  descriptorService_.getDescriptorInfo(indicesInfo.descriptorHeapKey, index);
              if (descriptorInfo &&
                  descriptorInfo->descriptorType == DescriptorHeapTracker::DescriptorInfo::UAV &&
                  descriptorInfo->resourceKey) {
                ID3D12Resource* resource = resourceByKey_[descriptorInfo->resourceKey];
                if (resource && resource->GetDesc().Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
                  resourceKeysBySlotByDispatch_[dispatchKey][slot].insert(
                      descriptorInfo->resourceKey);
                }
              }
            }
          }
        }
      }
    }
  }

  resourceDump_.executeCommandLists(c.key, c.object_.key, c.object_.value, c.ppCommandLists_.value,
                                    c.NumCommandLists_.value, frame, executeCount_);
}

void DispatchOutputsDumpLayer::post(ID3D12CommandQueueWaitCommand& c) {
  resourceDump_.commandQueueWait(c.key, c.object_.key, c.pFence_.key, c.Value_.value);
}

void DispatchOutputsDumpLayer::post(ID3D12CommandQueueSignalCommand& c) {
  resourceDump_.commandQueueSignal(c.key, c.object_.key, c.pFence_.key, c.Value_.value);
}

void DispatchOutputsDumpLayer::post(ID3D12FenceSignalCommand& c) {
  resourceDump_.fenceSignal(c.key, c.object_.key, c.Value_.value);
}

void DispatchOutputsDumpLayer::post(ID3D12DeviceCreateFenceCommand& c) {
  resourceDump_.fenceSignal(c.key, c.ppFence_.key, c.InitialValue_.value);
}

void DispatchOutputsDumpLayer::post(ID3D12Device3EnqueueMakeResidentCommand& c) {
  resourceDump_.fenceSignal(c.key, c.pFenceToSignal_.key, c.FenceValueToSignal_.value);
}

void DispatchOutputsDumpLayer::post(ID3D12GraphicsCommandListResetCommand& c) {
  if (!inAnalysis_) {
    return;
  }

  indicesBySlotByCommandList_[c.object_.key].clear();
  indicesBySlotByDispatchByCommandList_[c.object_.key].clear();
  resourceKeyFromSetViewBySlotByCommandList[c.object_.key].clear();
}

void DispatchOutputsDumpLayer::post(ID3D12GraphicsCommandListSetComputeRootSignatureCommand& c) {
  commandListInfos_[c.object_.key].computeRootSignature = c.pRootSignature_.key;
}

void DispatchOutputsDumpLayer::post(
    ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& c) {
  if (!inAnalysis_) {
    return;
  }

  ID3D12Resource* resource = resourceByKey_[c.BufferLocation_.interfaceKey];
  if (!resource || resource->GetDesc().Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
    return;
  }

  indicesBySlotByCommandList_[c.object_.key][c.RootParameterIndex_.value] = {};
  resourceKeyFromSetViewBySlotByCommandList[c.object_.key][c.RootParameterIndex_.value] =
      c.BufferLocation_.interfaceKey;
}

void DispatchOutputsDumpLayer::post(
    ID3D12GraphicsCommandListSetComputeRootDescriptorTableCommand& c) {
  if (!inAnalysis_ || !c.BaseDescriptor_.value.ptr) {
    return;
  }
  unsigned rootSignatureKey = commandListInfos_[c.object_.key].computeRootSignature;
  GITS_ASSERT(rootSignatureKey);
  unsigned numDescriptors = descriptorHeapInfos_[c.BaseDescriptor_.interfaceKey].numDescriptors;
  GITS_ASSERT(numDescriptors);

  std::vector<unsigned> indices = rootSignatureService_.getDescriptorTableIndexes(
      rootSignatureKey, c.BaseDescriptor_.interfaceKey, c.RootParameterIndex_.value,
      c.BaseDescriptor_.index, numDescriptors, false);

  indicesBySlotByCommandList_[c.object_.key][c.RootParameterIndex_.value].descriptorHeapKey =
      c.BaseDescriptor_.interfaceKey;
  indicesBySlotByCommandList_[c.object_.key][c.RootParameterIndex_.value].indices = indices;
}

void DispatchOutputsDumpLayer::post(ID3D12GraphicsCommandListDispatchCommand& c) {
  unsigned commandListKey = c.object_.key;
  ID3D12GraphicsCommandList* commandList = c.object_.value;

  ++dispatchCount_;
  unsigned commandListDispatchCount = ++dispatchCountByCommandList_[commandListKey];
  unsigned frame = stateRestorePhase_ ? 0 : CGits::Instance().CurrentFrame();

  if (!frameRange_[frame]) {
    return;
  }
  if (!dispatchRange_[dispatchCount_] && !inAnalysis_) {
    return;
  }

  if (inAnalysis_) {
    indicesBySlotByDispatchByCommandList_[commandListKey][c.key] =
        indicesBySlotByCommandList_[commandListKey];
    auto resourceKeyBySlotIt = resourceKeyFromSetViewBySlotByCommandList.find(commandListKey);
    if (resourceKeyBySlotIt != resourceKeyFromSetViewBySlotByCommandList.end()) {
      for (const auto& [slot, resourceKey] : resourceKeyBySlotIt->second) {
        resourceKeysBySlotByDispatch_[c.key][slot].insert(resourceKey);
      }
    }
  } else {
    auto resourceKeysBySlotIt = resourceKeysBySlotByDispatch_.find(c.key);
    if (resourceKeysBySlotIt != resourceKeysBySlotByDispatch_.end()) {
      for (const auto& [slot, resourceKeys] : resourceKeysBySlotIt->second) {
        for (unsigned resourceKey : resourceKeys) {
          if (dryRun_) {
            dryRunInfo_.dispatchesWithTextureByFrame[frame].insert(dispatchCount_);
          } else {
            ID3D12Resource* resource = resourceByKey_[resourceKey];
            GITS_ASSERT(resource);
            DispatchOutput dispatchOutput{resourceKey, resource, slot};
            dumpComputeOutput(commandList, dispatchOutput, frame, commandListDispatchCount);
          }
        }
      }
    }
  }
}

void DispatchOutputsDumpLayer::createDescriptor(
    unsigned heapKey,
    unsigned descriptorIndex,
    unsigned resourceKey,
    DescriptorHeapTracker::DescriptorInfo::DescriptorType descriptorType) {
  auto* descriptorInfo = new DescriptorHeapTracker::DescriptorInfo;
  descriptorInfo->heapKey = heapKey;
  descriptorInfo->descriptorIndex = descriptorIndex;
  descriptorInfo->resourceKey = resourceKey;
  descriptorInfo->descriptorType = descriptorType;
  descriptorService_.createDescriptor(descriptorInfo);
}

void DispatchOutputsDumpLayer::dumpComputeOutput(ID3D12GraphicsCommandList* commandList,
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

  std::string formatName = resourceDump_.formatToString(format);
  std::wstring formatNameW(formatName.begin(), formatName.end());

  Microsoft::WRL::ComPtr<ID3D12Device> device;
  HRESULT hr = dispatchOutput.resource->GetDevice(IID_PPV_ARGS(&device));
  GITS_ASSERT(hr == S_OK);
  unsigned planeCount = D3D12GetFormatPlaneCount(device.Get(), format);

  for (unsigned arrayIndex = minArrayIndex; arrayIndex <= maxArrayIndex; ++arrayIndex) {
    for (unsigned planeSlice = 0; planeSlice < planeCount; ++planeSlice) {

      std::wstring dumpName = dumpPath_ + L"/dispatch_e_" + resourceDump_.dumpNameExecutionMarker +
                              L"_f_" + std::to_wstring(frame) + L"_d_" +
                              std::to_wstring(dispatchCount_) + L"_s_" +
                              std::to_wstring(dispatchOutput.slot) + L"_O" +
                              std::to_wstring(dispatchOutput.resourceKey) + L"_" + formatNameW;
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
      resourceDump_.dumpResource(commandList, dispatchOutput.resource, subresource,
                                 D3D12_RESOURCE_STATE_UNORDERED_ACCESS, dumpName, mipLevel, format,
                                 commandListDispatch);
    }
  }
}

} // namespace DirectX
} // namespace gits
