// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "renderTargetsDumpLayer.h"
#include "keyUtils.h"
#include "gits.h"
#include "log.h"
#include "configurationLib.h"
#include "yaml-cpp/yaml.h"

#include <fstream>
#include <d3dx12.h>
#include <wrl/client.h>

namespace gits {
namespace DirectX {

RenderTargetsDumpLayer::RenderTargetsDumpLayer()
    : Layer("RenderTargetsDump"),
      m_ResourceDump(Configurator::Get().directx.features.renderTargetsDump.format),
      m_FrameRange(Configurator::Get().directx.features.renderTargetsDump.frames),
      m_DrawRange(Configurator::Get().directx.features.renderTargetsDump.draws),
      m_DryRun(Configurator::Get().directx.features.renderTargetsDump.dryRun) {

  auto& dumpPath = Configurator::Get().common.player.outputDir.empty()
                       ? Configurator::Get().common.player.streamDir / "render_targets"
                       : Configurator::Get().common.player.outputDir;
  if (!dumpPath.empty() && !std::filesystem::exists(dumpPath)) {
    std::filesystem::create_directory(dumpPath);
  }
  m_DumpPath = dumpPath;
}

RenderTargetsDumpLayer::~RenderTargetsDumpLayer() {
  try {
    if (m_DryRun) {
      YAML::Node output;
      output["DrawsWithTextureByFrame"] = YAML::Node();
      for (const auto& [frame, dispatchNumbers] : m_DryRunInfo.drawsWithTextureByFrame) {
        for (unsigned dispatchNumber : dispatchNumbers) {
          output["DrawsWithTextureByFrame"][frame].push_back(dispatchNumber);
        }
        output["DrawsWithTextureByFrame"][frame].SetStyle(YAML::EmitterStyle::Flow);
      }
      std::ofstream file(std::filesystem::path(m_DumpPath) / "RenderTargetsDumpDryRun.yaml");
      file << output;
    }
  } catch (const std::exception&) {
    topmost_exception_handler("RenderTargetsDumpLayer::~RenderTargetsDumpLayer");
  }
}

void RenderTargetsDumpLayer::Post(StateRestoreBeginCommand& c) {
  m_CurrentFrame = 0;
}
void RenderTargetsDumpLayer::Post(StateRestoreEndCommand& c) {
  m_DrawCount = 0;
  m_ExecuteCount = 0;
  m_CurrentFrame = 1;
}

void RenderTargetsDumpLayer::Post(ID3D12DeviceCreateRenderTargetViewCommand& c) {
  RenderTarget renderTarget{};
  renderTarget.resource = c.m_pResource.Value;
  renderTarget.ResourceKey = c.m_pResource.Key;
  if (c.m_pDesc.Value) {
    renderTarget.isDesc = true;
    renderTarget.desc = *c.m_pDesc.Value;
  }

  m_RenderTargetsByDescriptorHandle[std::make_pair(c.m_DestDescriptor.InterfaceKey,
                                                   c.m_DestDescriptor.Index)] = renderTarget;
}

void RenderTargetsDumpLayer::Post(ID3D12DeviceCreateDepthStencilViewCommand& c) {
  DepthStencil depthStencil;
  depthStencil.resource = c.m_pResource.Value;
  depthStencil.ResourceKey = c.m_pResource.Key;
  if (c.m_pDesc.Value) {
    depthStencil.isDesc = true;
    depthStencil.desc = *c.m_pDesc.Value;
  }

  m_DepthStencilsByDescriptorHandle[std::make_pair(c.m_DestDescriptor.InterfaceKey,
                                                   c.m_DestDescriptor.Index)] = depthStencil;
}

void RenderTargetsDumpLayer::Post(ID3D12DeviceCopyDescriptorsSimpleCommand& c) {
  unsigned srcHeapKey = c.m_SrcDescriptorRangeStart.InterfaceKey;
  unsigned srcHeapIndex = c.m_SrcDescriptorRangeStart.Index;
  unsigned destHeapKey = c.m_DestDescriptorRangeStart.InterfaceKey;
  unsigned destHeapIndex = c.m_DestDescriptorRangeStart.Index;

  for (unsigned i = 0; i < c.m_NumDescriptors.Value; ++i) {
    CopyDescriptors(m_RenderTargetsByDescriptorHandle, srcHeapKey, srcHeapIndex, destHeapKey,
                    destHeapIndex);
    CopyDescriptors(m_DepthStencilsByDescriptorHandle, srcHeapKey, srcHeapIndex, destHeapKey,
                    destHeapIndex);
    ++srcHeapIndex;
    ++destHeapIndex;
  }
}

void RenderTargetsDumpLayer::Post(ID3D12DeviceCopyDescriptorsCommand& c) {
  if (!c.m_NumDestDescriptorRanges.Value || !c.m_NumSrcDescriptorRanges.Value) {
    return;
  }

  unsigned destRangeIndex = 0;
  unsigned destIndex = 0;
  unsigned destRangeSize =
      c.m_pDestDescriptorRangeSizes.Value ? c.m_pDestDescriptorRangeSizes.Value[destRangeIndex] : 1;
  unsigned destHeapKey = c.m_pDestDescriptorRangeStarts.InterfaceKeys[destRangeIndex];

  for (unsigned srcRangeIndex = 0; srcRangeIndex < c.m_NumSrcDescriptorRanges.Value;
       ++srcRangeIndex) {
    unsigned srcRangeSize =
        c.m_pSrcDescriptorRangeSizes.Value ? c.m_pSrcDescriptorRangeSizes.Value[srcRangeIndex] : 1;
    unsigned srcHeapKey = c.m_pSrcDescriptorRangeStarts.InterfaceKeys[srcRangeIndex];
    unsigned srcHeapIndex = c.m_pSrcDescriptorRangeStarts.Indexes[srcRangeIndex];
    for (unsigned srcIndex = 0; srcIndex < srcRangeSize; ++srcIndex, ++destIndex) {
      if (destIndex == destRangeSize) {
        destIndex = 0;
        ++destRangeIndex;
        destRangeSize = c.m_pDestDescriptorRangeSizes.Value
                            ? c.m_pDestDescriptorRangeSizes.Value[destRangeIndex]
                            : 1;
        destHeapKey = c.m_pDestDescriptorRangeStarts.InterfaceKeys[destRangeIndex];
      }
      unsigned destHeapIndex = c.m_pDestDescriptorRangeStarts.Indexes[destRangeIndex] + destIndex;

      CopyDescriptors(m_RenderTargetsByDescriptorHandle, srcHeapKey, srcHeapIndex, destHeapKey,
                      destHeapIndex);
      CopyDescriptors(m_DepthStencilsByDescriptorHandle, srcHeapKey, srcHeapIndex, destHeapKey,
                      destHeapIndex);
    }
  }
}

template <typename Descriptors>
void RenderTargetsDumpLayer::CopyDescriptors(Descriptors& descriptors,
                                             unsigned srcHeapKey,
                                             unsigned srcHeapIndex,
                                             unsigned destHeapKey,
                                             unsigned destHeapIndex) {
  auto itSrc = descriptors.find(std::make_pair(srcHeapKey, srcHeapIndex));
  if (itSrc != descriptors.end()) {
    descriptors[std::make_pair(destHeapKey, destHeapIndex)] = itSrc->second;
  } else {
    auto itDest = descriptors.find(std::make_pair(destHeapKey, destHeapIndex));
    if (itDest != descriptors.end()) {
      descriptors.erase(itDest);
    }
  }
}

void RenderTargetsDumpLayer::Post(ID3D12GraphicsCommandListOMSetRenderTargetsCommand& c) {

  auto& renderTargets = m_RenderTargetsByCommandList[c.m_Object.Key];
  renderTargets.clear();

  {
    unsigned heapKey{};
    unsigned heapIndex{};
    for (unsigned i = 0; i < c.m_NumRenderTargetDescriptors.Value; ++i) {
      if (i == 0 || !c.m_RTsSingleHandleToDescriptorRange.Value) {
        heapKey = c.m_pRenderTargetDescriptors.InterfaceKeys[i];
        heapIndex = c.m_pRenderTargetDescriptors.Indexes[i];
      } else {
        ++heapIndex;
      }
      if (heapKey) {
        auto it = m_RenderTargetsByDescriptorHandle.find(std::make_pair(heapKey, heapIndex));
        if (it == m_RenderTargetsByDescriptorHandle.end()) {
          LOG_ERROR << "RenderTargetsDumpLayer - cannot find rendertarget O" << heapKey << " "
                    << heapIndex;
          continue;
        }
        it->second.slot = i;
        renderTargets.push_back(it->second);
      }
    }
  }

  if (c.m_pDepthStencilDescriptor.Value && c.m_pDepthStencilDescriptor.InterfaceKeys[0]) {
    auto it = m_DepthStencilsByDescriptorHandle.find(std::make_pair(
        c.m_pDepthStencilDescriptor.InterfaceKeys[0], c.m_pDepthStencilDescriptor.Indexes[0]));
    if (it != m_DepthStencilsByDescriptorHandle.end()) {
      m_DepthStencilByCommandList[c.m_Object.Key] = it->second;
    } else {
      LOG_ERROR << "RenderTargetsDumpLayer - cannot find depthstencil O"
                << c.m_pDepthStencilDescriptor.InterfaceKeys[0] << " "
                << c.m_pDepthStencilDescriptor.Indexes[0];
    }
  }
}

void RenderTargetsDumpLayer::Post(ID3D12GraphicsCommandListDrawInstancedCommand& c) {
  OnDraw(c.m_Object.Value, c.m_Object.Key);
}

void RenderTargetsDumpLayer::Post(ID3D12GraphicsCommandListDrawIndexedInstancedCommand& c) {
  OnDraw(c.m_Object.Value, c.m_Object.Key);
}

void RenderTargetsDumpLayer::OnDraw(ID3D12GraphicsCommandList* commandList,
                                    unsigned commandListKey) {
  ++m_DrawCount;
  unsigned commandListDrawCount = ++m_DrawCountByCommandList[commandListKey];
  if (!m_FrameRange[m_CurrentFrame] || !m_DrawRange[m_DrawCount]) {
    return;
  }

  auto itRenderTargets = m_RenderTargetsByCommandList.find(commandListKey);
  if (itRenderTargets != m_RenderTargetsByCommandList.end()) {
    for (RenderTarget& renderTarget : itRenderTargets->second) {
      if (renderTarget.resource) {
        if (m_DryRun) {
          m_DryRunInfo.drawsWithTextureByFrame[m_CurrentFrame].insert(m_DrawCount);
        } else {
          DumpRenderTarget(commandList, renderTarget, m_CurrentFrame, commandListDrawCount,
                           m_DrawCount);
        }
      }
    }
  }

  auto itDepthStencil = m_DepthStencilByCommandList.find(commandListKey);
  if (itDepthStencil != m_DepthStencilByCommandList.end()) {
    if (itDepthStencil->second.resource) {
      if (m_DryRun) {
        m_DryRunInfo.drawsWithTextureByFrame[m_CurrentFrame].insert(m_DrawCount);
      } else {
        DumpDepthStencil(commandList, itDepthStencil->second, m_CurrentFrame, commandListDrawCount,
                         m_DrawCount);
      }
    }
  }
}

void RenderTargetsDumpLayer::DumpRenderTarget(ID3D12GraphicsCommandList* commandList,
                                              RenderTarget& renderTarget,
                                              unsigned frame,
                                              unsigned commandListDraw,
                                              unsigned frameDraw) {
  D3D12_RESOURCE_DESC desc = renderTarget.resource->GetDesc();
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

  if (renderTarget.isDesc) {
    format = renderTarget.desc.Format;
    switch (renderTarget.desc.ViewDimension) {
    case D3D12_RTV_DIMENSION_TEXTURE1D:
      mipLevel = renderTarget.desc.Texture1D.MipSlice;
      break;
    case D3D12_RTV_DIMENSION_TEXTURE1DARRAY:
      mipLevel = renderTarget.desc.Texture1DArray.MipSlice;
      minArrayIndex = renderTarget.desc.Texture1DArray.FirstArraySlice;
      maxArrayIndex = renderTarget.desc.Texture1DArray.FirstArraySlice +
                      std::min(arraySize, renderTarget.desc.Texture1DArray.ArraySize) - 1;
      break;
    case D3D12_RTV_DIMENSION_TEXTURE2D:
      mipLevel = renderTarget.desc.Texture2D.MipSlice;
      break;
    case D3D12_RTV_DIMENSION_TEXTURE2DARRAY:
      mipLevel = renderTarget.desc.Texture2DArray.MipSlice;
      minArrayIndex = renderTarget.desc.Texture2DArray.FirstArraySlice;
      maxArrayIndex = renderTarget.desc.Texture2DArray.FirstArraySlice +
                      std::min(arraySize, renderTarget.desc.Texture2DArray.ArraySize) - 1;
      break;
    case D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY:
      minArrayIndex = renderTarget.desc.Texture2DMSArray.FirstArraySlice;
      maxArrayIndex = renderTarget.desc.Texture2DMSArray.FirstArraySlice +
                      std::min(arraySize, renderTarget.desc.Texture2DMSArray.ArraySize) - 1;
      break;
    case D3D12_RTV_DIMENSION_TEXTURE3D:
      mipLevel = renderTarget.desc.Texture3D.MipSlice;
      minSlice = renderTarget.desc.Texture3D.FirstWSlice;
      if (renderTarget.desc.Texture3D.WSize != static_cast<UINT>(-1)) {
        maxSlice = renderTarget.desc.Texture3D.FirstWSlice + renderTarget.desc.Texture3D.WSize - 1;
      }
      break;
    }
  }

  std::string formatName = m_ResourceDump.formatToString(format);
  std::wstring formatNameW(formatName.begin(), formatName.end());

  Microsoft::WRL::ComPtr<ID3D12Device> device;
  HRESULT hr = renderTarget.resource->GetDevice(IID_PPV_ARGS(&device));
  GITS_ASSERT(hr == S_OK);
  unsigned planeCount = D3D12GetFormatPlaneCount(device.Get(), format);

  for (unsigned arrayIndex = minArrayIndex; arrayIndex <= maxArrayIndex; ++arrayIndex) {
    for (unsigned planeSlice = 0; planeSlice < planeCount; ++planeSlice) {

      std::wstring dumpName = m_DumpPath + L"/draw_e_" + m_ResourceDump.dumpNameExecutionMarker +
                              L"_f_" + std::to_wstring(frame) + L"_d_" +
                              std::to_wstring(m_DrawCount) + L"_rt_" +
                              std::to_wstring(renderTarget.slot) + L"_O" +
                              std::to_wstring(renderTarget.ResourceKey) + L"_" + formatNameW;
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
      m_ResourceDump.dumpResource(commandList, renderTarget.resource, subresource,
                                  D3D12_RESOURCE_STATE_RENDER_TARGET, dumpName, mipLevel, format,
                                  commandListDraw);
    }
  }
}

void RenderTargetsDumpLayer::DumpDepthStencil(ID3D12GraphicsCommandList* commandList,
                                              DepthStencil& depthStencil,
                                              unsigned frame,
                                              unsigned commandListDraw,
                                              unsigned frameDraw) {
  D3D12_RESOURCE_DESC desc = depthStencil.resource->GetDesc();
  DXGI_FORMAT format = desc.Format;

  unsigned arraySize =
      desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D ? 1 : desc.DepthOrArraySize;
  unsigned minArrayIndex = 0;
  unsigned maxArrayIndex =
      desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D ? 0 : desc.DepthOrArraySize - 1;
  unsigned mipLevel = 0;

  if (depthStencil.isDesc) {
    format = depthStencil.desc.Format;
    switch (depthStencil.desc.ViewDimension) {
    case D3D12_DSV_DIMENSION_TEXTURE1D:
      mipLevel = depthStencil.desc.Texture1D.MipSlice;
      break;
    case D3D12_DSV_DIMENSION_TEXTURE1DARRAY:
      mipLevel = depthStencil.desc.Texture1DArray.MipSlice;
      minArrayIndex = depthStencil.desc.Texture1DArray.FirstArraySlice;
      maxArrayIndex = depthStencil.desc.Texture1DArray.FirstArraySlice +
                      std::min(arraySize, depthStencil.desc.Texture1DArray.ArraySize) - 1;
      break;
    case D3D12_DSV_DIMENSION_TEXTURE2D:
      mipLevel = depthStencil.desc.Texture2D.MipSlice;
      break;
    case D3D12_DSV_DIMENSION_TEXTURE2DARRAY:
      mipLevel = depthStencil.desc.Texture2DArray.MipSlice;
      minArrayIndex = depthStencil.desc.Texture2DArray.FirstArraySlice;
      maxArrayIndex = depthStencil.desc.Texture2DArray.FirstArraySlice +
                      std::min(arraySize, depthStencil.desc.Texture2DArray.ArraySize) - 1;
      break;
    case D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY:
      minArrayIndex = depthStencil.desc.Texture2DMSArray.FirstArraySlice;
      maxArrayIndex = depthStencil.desc.Texture2DMSArray.FirstArraySlice +
                      std::min(arraySize, depthStencil.desc.Texture2DMSArray.ArraySize) - 1;
      break;
    }
  }

  std::string formatName = m_ResourceDump.formatToString(format);
  std::wstring formatNameW(formatName.begin(), formatName.end());

  Microsoft::WRL::ComPtr<ID3D12Device> device;
  HRESULT hr = depthStencil.resource->GetDevice(IID_PPV_ARGS(&device));
  GITS_ASSERT(hr == S_OK);
  unsigned planeCount = D3D12GetFormatPlaneCount(device.Get(), format);

  for (unsigned arrayIndex = minArrayIndex; arrayIndex <= maxArrayIndex; ++arrayIndex) {
    for (unsigned planeSlice = 0; planeSlice < planeCount; ++planeSlice) {

      std::wstring dumpName = m_DumpPath + L"/draw_e_" + m_ResourceDump.dumpNameExecutionMarker +
                              L"_f_" + std::to_wstring(frame) + L"_d_" +
                              std::to_wstring(m_DrawCount) + L"_ds_O" +
                              std::to_wstring(depthStencil.ResourceKey) + L"_" + formatNameW;
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
      m_ResourceDump.dumpResource(commandList, depthStencil.resource, subresource,
                                  D3D12_RESOURCE_STATE_DEPTH_WRITE, dumpName, mipLevel, format,
                                  commandListDraw);
    }
  }
}

void RenderTargetsDumpLayer::Post(ID3D12CommandQueueExecuteCommandListsCommand& c) {
  for (unsigned i = 0; i < c.m_NumCommandLists.Value; ++i) {
    unsigned commandListKey = c.m_ppCommandLists.Keys[i];
    m_RenderTargetsByCommandList.erase(commandListKey);
    m_DepthStencilByCommandList.erase(commandListKey);
    m_DrawCountByCommandList.erase(commandListKey);
  }
  ++m_ExecuteCount;
  m_ResourceDump.executeCommandLists(c.Key, c.m_Object.Key, c.m_Object.Value,
                                     c.m_ppCommandLists.Value, c.m_NumCommandLists.Value,
                                     m_CurrentFrame, m_ExecuteCount);
}

void RenderTargetsDumpLayer::Post(ID3D12CommandQueueWaitCommand& c) {
  m_ResourceDump.commandQueueWait(c.Key, c.m_Object.Key, c.m_pFence.Key, c.m_Value.Value);
}

void RenderTargetsDumpLayer::Post(ID3D12CommandQueueSignalCommand& c) {
  m_ResourceDump.commandQueueSignal(c.Key, c.m_Object.Key, c.m_pFence.Key, c.m_Value.Value);
}

void RenderTargetsDumpLayer::Post(ID3D12FenceSignalCommand& c) {
  m_ResourceDump.fenceSignal(c.Key, c.m_Object.Key, c.m_Value.Value);
}

void RenderTargetsDumpLayer::Post(ID3D12DeviceCreateFenceCommand& c) {
  m_ResourceDump.fenceSignal(c.Key, c.m_ppFence.Key, c.m_InitialValue.Value);
}

void RenderTargetsDumpLayer::Post(ID3D12Device3EnqueueMakeResidentCommand& c) {
  m_ResourceDump.fenceSignal(c.Key, c.m_pFenceToSignal.Key, c.m_FenceValueToSignal.Value);
}

void RenderTargetsDumpLayer::Post(IDXGISwapChainPresentCommand& c) {
  if (!(c.m_Flags.Value & DXGI_PRESENT_TEST)) {
    m_DrawCount = 0;
    m_ExecuteCount = 0;
    if (!IsStateRestoreKey(c.Key)) {
      ++m_CurrentFrame;
    }
  }
}

void RenderTargetsDumpLayer::Post(IDXGISwapChain1Present1Command& c) {
  if (!(c.m_PresentFlags.Value & DXGI_PRESENT_TEST)) {
    m_DrawCount = 0;
    m_ExecuteCount = 0;
    if (!IsStateRestoreKey(c.Key)) {
      ++m_CurrentFrame;
    }
  }
}

} // namespace DirectX
} // namespace gits
