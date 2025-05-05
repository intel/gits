// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "renderTargetsDumpLayer.h"
#include "gits.h"
#include "configurationLib.h"

#include <d3dx12.h>
#include <wrl/client.h>

namespace gits {
namespace DirectX {

RenderTargetsDumpLayer::RenderTargetsDumpLayer()
    : Layer("RenderTargetsDump"),
      resourceDump_(Configurator::Get().directx.features.renderTargetsDump.format == "jpg" ? true
                                                                                           : false),
      frameRange_(Configurator::Get().directx.features.renderTargetsDump.frames),
      drawRange_(Configurator::Get().directx.features.renderTargetsDump.draws) {

  auto& dumpPath = Configurator::Get().common.player.outputDir.empty()
                       ? Configurator::Get().common.player.streamDir / "render_targets"
                       : Configurator::Get().common.player.outputDir;
  if (!dumpPath.empty() && !std::filesystem::exists(dumpPath)) {
    std::filesystem::create_directory(dumpPath);
  }
  dumpPath_ = dumpPath;
};

void RenderTargetsDumpLayer::post(ID3D12DeviceCreateRenderTargetViewCommand& c) {
  if (c.key & Command::stateRestoreKeyMask) {
    stateRestorePhase_ = true;
  }
  RenderTarget renderTarget{};
  renderTarget.resource = c.pResource_.value;
  renderTarget.resourceKey = c.pResource_.key;
  if (c.pDesc_.value) {
    renderTarget.isDesc = true;
    renderTarget.desc = *c.pDesc_.value;
  }

  renderTargetsByDescriptorHandle_[std::make_pair(c.DestDescriptor_.interfaceKey,
                                                  c.DestDescriptor_.index)] = renderTarget;
}

void RenderTargetsDumpLayer::post(ID3D12DeviceCreateDepthStencilViewCommand& c) {
  if (c.key & Command::stateRestoreKeyMask) {
    stateRestorePhase_ = true;
  }
  DepthStencil depthStencil;
  depthStencil.resource = c.pResource_.value;
  depthStencil.resourceKey = c.pResource_.key;
  if (c.pDesc_.value) {
    depthStencil.isDesc = true;
    depthStencil.desc = *c.pDesc_.value;
  }

  depthStencilsByDescriptorHandle_[std::make_pair(c.DestDescriptor_.interfaceKey,
                                                  c.DestDescriptor_.index)] = depthStencil;
}

void RenderTargetsDumpLayer::post(ID3D12DeviceCopyDescriptorsSimpleCommand& c) {
  unsigned srcHeapKey = c.SrcDescriptorRangeStart_.interfaceKey;
  unsigned srcHeapIndex = c.SrcDescriptorRangeStart_.index;
  unsigned destHeapKey = c.DestDescriptorRangeStart_.interfaceKey;
  unsigned destHeapIndex = c.DestDescriptorRangeStart_.index;

  for (unsigned i = 0; i < c.NumDescriptors_.value; ++i) {
    copyDescriptors(renderTargetsByDescriptorHandle_, srcHeapKey, srcHeapIndex, destHeapKey,
                    destHeapIndex);
    copyDescriptors(depthStencilsByDescriptorHandle_, srcHeapKey, srcHeapIndex, destHeapKey,
                    destHeapIndex);
    ++srcHeapIndex;
    ++destHeapIndex;
  }
}

void RenderTargetsDumpLayer::post(ID3D12DeviceCopyDescriptorsCommand& c) {
  unsigned destRangeIndex = 0;
  unsigned destIndex = 0;
  unsigned destRangeSize =
      c.pDestDescriptorRangeSizes_.value ? c.pDestDescriptorRangeSizes_.value[destRangeIndex] : 1;
  unsigned destHeapKey = c.pDestDescriptorRangeStarts_.interfaceKeys[destRangeIndex];

  for (unsigned srcRangeIndex = 0; srcRangeIndex < c.NumSrcDescriptorRanges_.value;
       ++srcRangeIndex) {
    unsigned srcRangeSize =
        c.pSrcDescriptorRangeSizes_.value ? c.pSrcDescriptorRangeSizes_.value[srcRangeIndex] : 1;
    unsigned srcHeapKey = c.pSrcDescriptorRangeStarts_.interfaceKeys[srcRangeIndex];
    unsigned srcHeapIndex = c.pSrcDescriptorRangeStarts_.indexes[srcRangeIndex];
    for (unsigned srcIndex = 0; srcIndex < srcRangeSize; ++srcIndex, ++destIndex) {
      if (destIndex == destRangeSize) {
        destIndex = 0;
        ++destRangeIndex;
        destRangeSize = c.pDestDescriptorRangeSizes_.value
                            ? c.pDestDescriptorRangeSizes_.value[destRangeIndex]
                            : 1;
        destHeapKey = c.pDestDescriptorRangeStarts_.interfaceKeys[destRangeIndex];
      }
      unsigned destHeapIndex = c.pDestDescriptorRangeStarts_.indexes[destRangeIndex] + destIndex;

      copyDescriptors(renderTargetsByDescriptorHandle_, srcHeapKey, srcHeapIndex, destHeapKey,
                      destHeapIndex);
      copyDescriptors(depthStencilsByDescriptorHandle_, srcHeapKey, srcHeapIndex, destHeapKey,
                      destHeapIndex);
    }
  }
}

template <typename Descriptors>
void RenderTargetsDumpLayer::copyDescriptors(Descriptors& descriptors,
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

void RenderTargetsDumpLayer::post(ID3D12GraphicsCommandListOMSetRenderTargetsCommand& c) {

  auto& renderTargets = renderTargetsByCommandList_[c.object_.key];
  renderTargets.clear();

  for (unsigned i = 0; i < c.NumRenderTargetDescriptors_.value; ++i) {
    if (c.pRenderTargetDescriptors_.interfaceKeys[i]) {
      auto it = renderTargetsByDescriptorHandle_.find(std::make_pair(
          c.pRenderTargetDescriptors_.interfaceKeys[i], c.pRenderTargetDescriptors_.indexes[i]));
      if (it == renderTargetsByDescriptorHandle_.end()) {
        Log(ERR) << "RenderTargetsDumpLayer - cannot find rendertarget O"
                 << c.pRenderTargetDescriptors_.interfaceKeys[i] << " "
                 << c.pRenderTargetDescriptors_.indexes[i];
        continue;
      }
      it->second.slot = i;
      renderTargets.push_back(it->second);
    }
  }

  if (c.pDepthStencilDescriptor_.value && c.pDepthStencilDescriptor_.interfaceKeys[0]) {
    auto it = depthStencilsByDescriptorHandle_.find(std::make_pair(
        c.pDepthStencilDescriptor_.interfaceKeys[0], c.pDepthStencilDescriptor_.indexes[0]));
    if (it != depthStencilsByDescriptorHandle_.end()) {
      depthStencilByCommandList_[c.object_.key] = it->second;
    } else {
      Log(ERR) << "RenderTargetsDumpLayer - cannot find depthstencil O"
               << c.pDepthStencilDescriptor_.interfaceKeys[0] << " "
               << c.pDepthStencilDescriptor_.indexes[0];
    }
  }
}

void RenderTargetsDumpLayer::post(ID3D12GraphicsCommandListDrawInstancedCommand& c) {
  onDraw(c.object_.value, c.object_.key);
}

void RenderTargetsDumpLayer::post(ID3D12GraphicsCommandListDrawIndexedInstancedCommand& c) {
  onDraw(c.object_.value, c.object_.key);
}

void RenderTargetsDumpLayer::onDraw(ID3D12GraphicsCommandList* commandList,
                                    unsigned commandListKey) {
  ++drawCount_;
  unsigned commandListDrawCount = ++drawCountByCommandList_[commandListKey];
  unsigned frame = stateRestorePhase_ ? 0 : CGits::Instance().CurrentFrame();
  if (!frameRange_[frame] || !drawRange_[drawCount_]) {
    return;
  }

  auto itRenderTargets = renderTargetsByCommandList_.find(commandListKey);
  if (itRenderTargets != renderTargetsByCommandList_.end()) {
    for (RenderTarget& renderTarget : itRenderTargets->second) {
      dumpRenderTarget(commandList, renderTarget, frame, commandListDrawCount, drawCount_);
    }
  }

  auto itDepthStencil = depthStencilByCommandList_.find(commandListKey);
  if (itDepthStencil != depthStencilByCommandList_.end()) {
    dumpDepthStencil(commandList, itDepthStencil->second, frame, commandListDrawCount, drawCount_);
  }
}

void RenderTargetsDumpLayer::dumpRenderTarget(ID3D12GraphicsCommandList* commandList,
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

  std::string formatName = resourceDump_.formatToString(format);
  std::wstring formatNameW(formatName.begin(), formatName.end());

  Microsoft::WRL::ComPtr<ID3D12Device> device;
  HRESULT hr = renderTarget.resource->GetDevice(IID_PPV_ARGS(&device));
  GITS_ASSERT(hr == S_OK);
  unsigned planeCount = D3D12GetFormatPlaneCount(device.Get(), format);

  for (unsigned arrayIndex = minArrayIndex; arrayIndex <= maxArrayIndex; ++arrayIndex) {
    for (unsigned planeSlice = 0; planeSlice < planeCount; ++planeSlice) {

      std::wstring dumpName = dumpPath_ + L"/draw_e_" + resourceDump_.dumpNameExecutionMarker +
                              L"_f_" + std::to_wstring(frame) + L"_d_" +
                              std::to_wstring(drawCount_) + L"_rt_" +
                              std::to_wstring(renderTarget.slot) + L"_O" +
                              std::to_wstring(renderTarget.resourceKey) + L"_" + formatNameW;
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
      resourceDump_.dumpResource(commandList, renderTarget.resource, subresource,
                                 D3D12_RESOURCE_STATE_RENDER_TARGET, dumpName, mipLevel, format,
                                 commandListDraw);
    }
  }
}

void RenderTargetsDumpLayer::dumpDepthStencil(ID3D12GraphicsCommandList* commandList,
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

  std::string formatName = resourceDump_.formatToString(format);
  std::wstring formatNameW(formatName.begin(), formatName.end());

  Microsoft::WRL::ComPtr<ID3D12Device> device;
  HRESULT hr = depthStencil.resource->GetDevice(IID_PPV_ARGS(&device));
  GITS_ASSERT(hr == S_OK);
  unsigned planeCount = D3D12GetFormatPlaneCount(device.Get(), format);

  for (unsigned arrayIndex = minArrayIndex; arrayIndex <= maxArrayIndex; ++arrayIndex) {
    for (unsigned planeSlice = 0; planeSlice < planeCount; ++planeSlice) {

      std::wstring dumpName = dumpPath_ + L"/draw_e_" + resourceDump_.dumpNameExecutionMarker +
                              L"_f_" + std::to_wstring(frame) + L"_d_" +
                              std::to_wstring(drawCount_) + L"_ds_O" +
                              std::to_wstring(depthStencil.resourceKey) + L"_" + formatNameW;
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
      resourceDump_.dumpResource(commandList, depthStencil.resource, subresource,
                                 D3D12_RESOURCE_STATE_DEPTH_WRITE, dumpName, mipLevel, format,
                                 commandListDraw);
    }
  }
}

void RenderTargetsDumpLayer::post(ID3D12CommandQueueExecuteCommandListsCommand& c) {

  ++executeCount_;
  unsigned frame = stateRestorePhase_ ? 0 : CGits::Instance().CurrentFrame();

  for (unsigned i = 0; i < c.NumCommandLists_.value; ++i) {
    unsigned commandListKey = c.ppCommandLists_.keys[i];
    renderTargetsByCommandList_.erase(commandListKey);
    depthStencilByCommandList_.erase(commandListKey);
    drawCountByCommandList_.erase(commandListKey);
  }

  resourceDump_.executeCommandLists(c.key, c.object_.key, c.object_.value, c.ppCommandLists_.value,
                                    c.NumCommandLists_.value, frame, executeCount_);
}

void RenderTargetsDumpLayer::post(ID3D12CommandQueueWaitCommand& c) {
  resourceDump_.commandQueueWait(c.key, c.object_.key, c.pFence_.key, c.Value_.value);
}

void RenderTargetsDumpLayer::post(ID3D12CommandQueueSignalCommand& c) {
  resourceDump_.commandQueueSignal(c.key, c.object_.key, c.pFence_.key, c.Value_.value);
}

void RenderTargetsDumpLayer::post(ID3D12FenceSignalCommand& c) {
  resourceDump_.fenceSignal(c.key, c.object_.key, c.Value_.value);
}

void RenderTargetsDumpLayer::post(ID3D12DeviceCreateFenceCommand& c) {
  resourceDump_.fenceSignal(c.key, c.ppFence_.key, c.InitialValue_.value);
}

void RenderTargetsDumpLayer::post(ID3D12Device3EnqueueMakeResidentCommand& c) {
  resourceDump_.fenceSignal(c.key, c.pFenceToSignal_.key, c.FenceValueToSignal_.value);
}

void RenderTargetsDumpLayer::post(IDXGISwapChainPresentCommand& c) {
  if (!(c.Flags_.value & DXGI_PRESENT_TEST)) {
    drawCount_ = 0;
    executeCount_ = 0;
    stateRestorePhase_ = false;
  }
}

void RenderTargetsDumpLayer::post(IDXGISwapChain1Present1Command& c) {
  if (!(c.PresentFlags_.value & DXGI_PRESENT_TEST)) {
    drawCount_ = 0;
    executeCount_ = 0;
    stateRestorePhase_ = false;
  }
}

} // namespace DirectX
} // namespace gits
