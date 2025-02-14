// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "command.h"
#include "commandIdsAuto.h"
#include "arguments.h"

#include <unknwn.h>

struct INTCExtensionVersion;
struct INTCExtensionContext;
enum INTC_D3D12_FEATURES;
struct INTC_D3D12_FEATURE;
struct INTCExtensionInfo;
struct INTCExtensionAppInfo;
struct INTCExtensionAppInfo1;
struct INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC;
struct INTC_D3D12_RESOURCE_DESC_0001;

namespace gits {
namespace DirectX {

class IUnknownQueryInterfaceCommand : public Command {
public:
  IUnknownQueryInterfaceCommand(
      unsigned key, unsigned threadId, IUnknown* object, REFIID riid, void** ppvObject)
      : Command{CommandId::ID_IUNKNOWN_QUERYINTERFACE, key, threadId},
        riid_{riid},
        ppvObject_{ppvObject} {}
  IUnknownQueryInterfaceCommand() : Command(CommandId::ID_IUNKNOWN_QUERYINTERFACE) {}

public:
  InterfaceArgument<IUnknown> object_{};
  Argument<HRESULT> result_{};
  Argument<IID> riid_{};
  InterfaceOutputArgument<void> ppvObject_{};
};

class IUnknownAddRefCommand : public Command {
public:
  IUnknownAddRefCommand(unsigned key, unsigned threadId, IUnknown* object)
      : Command{CommandId::ID_IUNKNOWN_ADDREF, key, threadId} {}
  IUnknownAddRefCommand() : Command(CommandId::ID_IUNKNOWN_ADDREF) {}

public:
  InterfaceArgument<IUnknown> object_{};
  Argument<ULONG> result_{};
};

class IUnknownReleaseCommand : public Command {
public:
  IUnknownReleaseCommand(unsigned key, unsigned threadId, IUnknown* object)
      : Command{CommandId::ID_IUNKNOWN_RELEASE, key, threadId} {}
  IUnknownReleaseCommand() : Command(CommandId::ID_IUNKNOWN_RELEASE) {}

public:
  InterfaceArgument<IUnknown> object_{};
  Argument<ULONG> result_{};
};

class ID3D12GraphicsCommandListOMSetRenderTargetsCommand : public Command {
public:
  ID3D12GraphicsCommandListOMSetRenderTargetsCommand(
      unsigned key,
      unsigned threadId,
      IUnknown* object,
      UINT NumRenderTargetDescriptors,
      const D3D12_CPU_DESCRIPTOR_HANDLE* pRenderTargetDescriptors,
      BOOL RTsSingleHandleToDescriptorRange,
      const D3D12_CPU_DESCRIPTOR_HANDLE* pDepthStencilDescriptor)
      : Command{CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_OMSETRENDERTARGETS, key, threadId},
        NumRenderTargetDescriptors_{NumRenderTargetDescriptors},
        pRenderTargetDescriptors_{pRenderTargetDescriptors, NumRenderTargetDescriptors},
        RTsSingleHandleToDescriptorRange_{RTsSingleHandleToDescriptorRange},
        pDepthStencilDescriptor_{pDepthStencilDescriptor, 1} {}
  ID3D12GraphicsCommandListOMSetRenderTargetsCommand()
      : Command(CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_OMSETRENDERTARGETS) {}

public:
  InterfaceArgument<ID3D12GraphicsCommandList> object_{};
  Argument<UINT> NumRenderTargetDescriptors_{};
  DescriptorHandleArrayArgument<D3D12_CPU_DESCRIPTOR_HANDLE> pRenderTargetDescriptors_{};
  Argument<BOOL> RTsSingleHandleToDescriptorRange_{};
  DescriptorHandleArrayArgument<D3D12_CPU_DESCRIPTOR_HANDLE> pDepthStencilDescriptor_{};
};

// Note: Not auto generated due to pSubresourceTilingsForNonPackedMips_
//   pNumSubresourceTilings is optional and can't be used as part of the initializer list
class ID3D12DeviceGetResourceTilingCommand : public Command {
public:
  ID3D12DeviceGetResourceTilingCommand(
      unsigned key,
      unsigned threadId,
      ID3D12Device* object,
      ID3D12Resource* pTiledResource,
      UINT* pNumTilesForEntireResource,
      D3D12_PACKED_MIP_INFO* pPackedMipDesc,
      D3D12_TILE_SHAPE* pStandardTileShapeForNonPackedMips,
      UINT* pNumSubresourceTilings,
      UINT FirstSubresourceTilingToGet,
      D3D12_SUBRESOURCE_TILING* pSubresourceTilingsForNonPackedMips)
      : Command{CommandId::ID_ID3D12DEVICE_GETRESOURCETILING, key, threadId},
        object_{object},
        pTiledResource_{pTiledResource},
        pNumTilesForEntireResource_{pNumTilesForEntireResource},
        pPackedMipDesc_{pPackedMipDesc},
        pStandardTileShapeForNonPackedMips_{pStandardTileShapeForNonPackedMips},
        pNumSubresourceTilings_{pNumSubresourceTilings},
        FirstSubresourceTilingToGet_{FirstSubresourceTilingToGet},
        pSubresourceTilingsForNonPackedMips_{} {
    if (pNumSubresourceTilings) {
      pSubresourceTilingsForNonPackedMips_ = {pSubresourceTilingsForNonPackedMips,
                                              *pNumSubresourceTilings};
    }
  }
  ID3D12DeviceGetResourceTilingCommand() : Command(CommandId::ID_ID3D12DEVICE_GETRESOURCETILING) {}

public:
  InterfaceArgument<ID3D12Device> object_{};
  InterfaceArgument<ID3D12Resource> pTiledResource_{};
  PointerArgument<UINT> pNumTilesForEntireResource_{};
  PointerArgument<D3D12_PACKED_MIP_INFO> pPackedMipDesc_{};
  PointerArgument<D3D12_TILE_SHAPE> pStandardTileShapeForNonPackedMips_{};
  PointerArgument<UINT> pNumSubresourceTilings_{};
  Argument<UINT> FirstSubresourceTilingToGet_{};
  ArrayArgument<D3D12_SUBRESOURCE_TILING> pSubresourceTilingsForNonPackedMips_{};
};

class ID3D12StateObjectPropertiesGetShaderIdentifierCommand : public Command {
public:
  ID3D12StateObjectPropertiesGetShaderIdentifierCommand(unsigned key,
                                                        unsigned threadId,
                                                        ID3D12StateObjectProperties* object,
                                                        LPCWSTR pExportName)
      : Command{CommandId::ID_ID3D12STATEOBJECTPROPERTIES_GETSHADERIDENTIFIER, key, threadId},
        object_{object},
        pExportName_{pExportName} {}
  ID3D12StateObjectPropertiesGetShaderIdentifierCommand()
      : Command(CommandId::ID_ID3D12STATEOBJECTPROPERTIES_GETSHADERIDENTIFIER) {}

public:
  InterfaceArgument<ID3D12StateObjectProperties> object_{};
  ShaderIdentifierArgument result_{};
  LPCWSTR_Argument pExportName_{};
};

class INTC_D3D12_GetSupportedVersionsCommand : public Command {
public:
  INTC_D3D12_GetSupportedVersionsCommand(unsigned key,
                                         unsigned threadId,
                                         const ID3D12Device* pDevice,
                                         INTCExtensionVersion* pSupportedExtVersions,
                                         uint32_t* pSupportedExtVersionsCount)
      : Command{CommandId::INTC_D3D12_GETSUPPORTEDVERSIONS, key, threadId},
        pDevice_{const_cast<ID3D12Device*>(pDevice)},
        pSupportedExtVersionsCount_{pSupportedExtVersionsCount},
        pSupportedExtVersions_{} {
    if (pSupportedExtVersionsCount) {
      pSupportedExtVersions_ = {pSupportedExtVersions, *pSupportedExtVersionsCount};
    }
  }
  INTC_D3D12_GetSupportedVersionsCommand() : Command(CommandId::INTC_D3D12_GETSUPPORTEDVERSIONS) {}

public:
  InterfaceArgument<ID3D12Device> pDevice_{};
  ArrayArgument<INTCExtensionVersion> pSupportedExtVersions_{};
  PointerArgument<uint32_t> pSupportedExtVersionsCount_{};
  Argument<HRESULT> result_{};
};

class INTC_D3D12_CreateDeviceExtensionContextCommand : public Command {
public:
  INTC_D3D12_CreateDeviceExtensionContextCommand(unsigned key,
                                                 unsigned threadId,
                                                 const ID3D12Device* pDevice,
                                                 INTCExtensionContext** ppExtensionContext,
                                                 INTCExtensionInfo* pExtensionInfo,
                                                 INTCExtensionAppInfo* pExtensionAppInfo)
      : Command{CommandId::INTC_D3D12_CREATEDEVICEEXTENSIONCONTEXT, key, threadId},
        pDevice_{const_cast<ID3D12Device*>(pDevice)},
        ppExtensionContext_{ppExtensionContext},
        pExtensionInfo_{pExtensionInfo},
        pExtensionAppInfo_{pExtensionAppInfo} {}
  INTC_D3D12_CreateDeviceExtensionContextCommand()
      : Command(CommandId::INTC_D3D12_CREATEDEVICEEXTENSIONCONTEXT) {}

public:
  InterfaceArgument<ID3D12Device> pDevice_;
  INTCExtensionContextOutputArgument ppExtensionContext_;
  PointerArgument<INTCExtensionInfo> pExtensionInfo_;
  PointerArgument<INTCExtensionAppInfo> pExtensionAppInfo_;
  Argument<HRESULT> result_{};
};

class INTC_D3D12_CreateDeviceExtensionContext1Command : public Command {
public:
  INTC_D3D12_CreateDeviceExtensionContext1Command(unsigned key,
                                                  unsigned threadId,
                                                  const ID3D12Device* pDevice,
                                                  INTCExtensionContext** ppExtensionContext,
                                                  INTCExtensionInfo* pExtensionInfo,
                                                  INTCExtensionAppInfo1* pExtensionAppInfo)
      : Command{CommandId::INTC_D3D12_CREATEDEVICEEXTENSIONCONTEXT1, key, threadId},
        pDevice_{const_cast<ID3D12Device*>(pDevice)},
        ppExtensionContext_{ppExtensionContext},
        pExtensionInfo_{pExtensionInfo},
        pExtensionAppInfo_{pExtensionAppInfo} {}
  INTC_D3D12_CreateDeviceExtensionContext1Command()
      : Command(CommandId::INTC_D3D12_CREATEDEVICEEXTENSIONCONTEXT1) {}

public:
  InterfaceArgument<ID3D12Device> pDevice_;
  INTCExtensionContextOutputArgument ppExtensionContext_;
  PointerArgument<INTCExtensionInfo> pExtensionInfo_;
  PointerArgument<INTCExtensionAppInfo1> pExtensionAppInfo_;
  Argument<HRESULT> result_{};
};

class INTC_DestroyDeviceExtensionContextCommand : public Command {
public:
  INTC_DestroyDeviceExtensionContextCommand(unsigned key,
                                            unsigned threadId,
                                            INTCExtensionContext** ppExtensionContext)
      : Command{CommandId::INTC_DESTROYDEVICEEXTENSIONCONTEXT, key, threadId},
        ppExtensionContext_{ppExtensionContext} {}
  INTC_DestroyDeviceExtensionContextCommand()
      : Command(CommandId::INTC_DESTROYDEVICEEXTENSIONCONTEXT) {}

public:
  INTCExtensionContextOutputArgument ppExtensionContext_;
  Argument<HRESULT> result_{};
};

class INTC_D3D12_CheckFeatureSupportCommand : public Command {
public:
  INTC_D3D12_CheckFeatureSupportCommand(unsigned key,
                                        unsigned threadId,
                                        INTCExtensionContext* pExtensionContext,
                                        INTC_D3D12_FEATURES Feature,
                                        void* pFeatureSupportData,
                                        UINT FeatureSupportDataSize)
      : Command{CommandId::INTC_D3D12_CHECKFEATURESUPPORT, key, threadId},
        pExtensionContext_{pExtensionContext},
        Feature_{Feature},
        pFeatureSupportData_{pFeatureSupportData, FeatureSupportDataSize},
        FeatureSupportDataSize_{FeatureSupportDataSize} {}
  INTC_D3D12_CheckFeatureSupportCommand() : Command(CommandId::INTC_D3D12_CHECKFEATURESUPPORT) {}

public:
  INTCExtensionContextArgument pExtensionContext_;
  Argument<INTC_D3D12_FEATURES> Feature_{};
  BufferArgument pFeatureSupportData_{};
  Argument<UINT> FeatureSupportDataSize_{};
  Argument<HRESULT> result_{};
};

class INTC_D3D12_SetFeatureSupportCommand : public Command {
public:
  INTC_D3D12_SetFeatureSupportCommand(unsigned key,
                                      unsigned threadId,
                                      INTCExtensionContext* pExtensionContext,
                                      INTC_D3D12_FEATURE* pFeature)
      : Command{CommandId::INTC_D3D12_SETFEATURESUPPORT, key, threadId},
        pExtensionContext_{pExtensionContext},
        pFeature_{pFeature} {}
  INTC_D3D12_SetFeatureSupportCommand() : Command(CommandId::INTC_D3D12_SETFEATURESUPPORT) {}

public:
  INTCExtensionContextArgument pExtensionContext_;
  PointerArgument<INTC_D3D12_FEATURE> pFeature_{};
  Argument<HRESULT> result_{};
};

class INTC_D3D12_GetResourceAllocationInfoCommand : public Command {
public:
  INTC_D3D12_GetResourceAllocationInfoCommand(unsigned key,
                                              unsigned threadId,
                                              INTCExtensionContext* pExtensionContext,
                                              UINT visibleMask,
                                              UINT numResourceDescs,
                                              const INTC_D3D12_RESOURCE_DESC_0001* pResourceDescs)
      : Command{CommandId::INTC_D3D12_GETRESOURCEALLOCATIONINFO, key, threadId},
        pExtensionContext_{pExtensionContext},
        visibleMask_{visibleMask},
        numResourceDescs_{numResourceDescs},
        pResourceDescs_{pResourceDescs} {}
  INTC_D3D12_GetResourceAllocationInfoCommand()
      : Command(CommandId::INTC_D3D12_GETRESOURCEALLOCATIONINFO) {}

public:
  INTCExtensionContextArgument pExtensionContext_;
  Argument<UINT> visibleMask_{};
  Argument<UINT> numResourceDescs_{};
  PointerArgument<INTC_D3D12_RESOURCE_DESC_0001> pResourceDescs_{};
  Argument<D3D12_RESOURCE_ALLOCATION_INFO> result_{};
};

class INTC_D3D12_CreateComputePipelineStateCommand : public Command {
public:
  INTC_D3D12_CreateComputePipelineStateCommand(unsigned key,
                                               unsigned threadId,
                                               INTCExtensionContext* pExtensionContext,
                                               const INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC* pDesc,
                                               REFIID riid,
                                               void** ppPipelineState)
      : Command{CommandId::INTC_D3D12_CREATECOMPUTEPIPELINESTATE, key, threadId},
        pExtensionContext_{pExtensionContext},
        pDesc_{pDesc},
        riid_{riid},
        ppPipelineState_{ppPipelineState} {}
  INTC_D3D12_CreateComputePipelineStateCommand()
      : Command(CommandId::INTC_D3D12_CREATECOMPUTEPIPELINESTATE) {}

public:
  INTCExtensionContextArgument pExtensionContext_;
  PointerArgument<INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC> pDesc_{};
  Argument<IID> riid_{};
  InterfaceOutputArgument<void> ppPipelineState_{};
  Argument<HRESULT> result_{};
};

class INTC_D3D12_CreatePlacedResourceCommand : public Command {
public:
  INTC_D3D12_CreatePlacedResourceCommand(unsigned key,
                                         unsigned threadId,
                                         INTCExtensionContext* pExtensionContext,
                                         ID3D12Heap* pHeap,
                                         UINT64 HeapOffset,
                                         const INTC_D3D12_RESOURCE_DESC_0001* pDesc,
                                         D3D12_RESOURCE_STATES InitialState,
                                         const D3D12_CLEAR_VALUE* pOptimizedClearValue,
                                         REFIID riid,
                                         void** ppvResource)
      : Command{CommandId::INTC_D3D12_CREATEPLACEDRESOURCE, key, threadId},
        pExtensionContext_{pExtensionContext},
        pHeap_{pHeap},
        HeapOffset_{HeapOffset},
        pDesc_{pDesc},
        InitialState_{InitialState},
        pOptimizedClearValue_{pOptimizedClearValue},
        riid_{riid},
        ppvResource_{ppvResource} {}
  INTC_D3D12_CreatePlacedResourceCommand() : Command(CommandId::INTC_D3D12_CREATEPLACEDRESOURCE) {}

public:
  INTCExtensionContextArgument pExtensionContext_;
  InterfaceArgument<ID3D12Heap> pHeap_{};
  Argument<UINT64> HeapOffset_{};
  PointerArgument<INTC_D3D12_RESOURCE_DESC_0001> pDesc_{};
  Argument<D3D12_RESOURCE_STATES> InitialState_{};
  PointerArgument<D3D12_CLEAR_VALUE> pOptimizedClearValue_{};
  Argument<IID> riid_{};
  InterfaceOutputArgument<void> ppvResource_{};
  Argument<HRESULT> result_{};
};

class INTC_D3D12_CreateCommittedResourceCommand : public Command {
public:
  INTC_D3D12_CreateCommittedResourceCommand(unsigned key,
                                            unsigned threadId,
                                            INTCExtensionContext* pExtensionContext,
                                            const D3D12_HEAP_PROPERTIES* pHeapProperties,
                                            D3D12_HEAP_FLAGS HeapFlags,
                                            const INTC_D3D12_RESOURCE_DESC_0001* pDesc,
                                            D3D12_RESOURCE_STATES InitialResourceState,
                                            const D3D12_CLEAR_VALUE* pOptimizedClearValue,
                                            REFIID riidResource,
                                            void** ppvResource)
      : Command{CommandId::INTC_D3D12_CREATECOMMITTEDRESOURCE, key, threadId},
        pExtensionContext_{pExtensionContext},
        pHeapProperties_{pHeapProperties},
        HeapFlags_{HeapFlags},
        pDesc_{pDesc},
        InitialResourceState_{InitialResourceState},
        pOptimizedClearValue_{pOptimizedClearValue},
        riidResource_{riidResource},
        ppvResource_{ppvResource} {}
  INTC_D3D12_CreateCommittedResourceCommand()
      : Command(CommandId::INTC_D3D12_CREATECOMMITTEDRESOURCE) {}

public:
  INTCExtensionContextArgument pExtensionContext_;
  PointerArgument<D3D12_HEAP_PROPERTIES> pHeapProperties_{};
  Argument<D3D12_HEAP_FLAGS> HeapFlags_{};
  PointerArgument<INTC_D3D12_RESOURCE_DESC_0001> pDesc_{};
  Argument<D3D12_RESOURCE_STATES> InitialResourceState_{};
  PointerArgument<D3D12_CLEAR_VALUE> pOptimizedClearValue_{};
  Argument<IID> riidResource_{};
  InterfaceOutputArgument<void> ppvResource_{};
  Argument<HRESULT> result_{};
};

class CreateWindowMetaCommand : public Command {
public:
  CreateWindowMetaCommand(unsigned key, unsigned threadId)
      : Command{CommandId::ID_META_CREATE_WINDOW, key, threadId} {}
  CreateWindowMetaCommand() : Command(CommandId::ID_META_CREATE_WINDOW) {}

public:
  Argument<HWND> hWnd_{};
  Argument<int> width_{};
  Argument<int> height_{};
};

class MappedDataMetaCommand : public Command {
public:
  MappedDataMetaCommand(unsigned key, unsigned threadId)
      : Command{CommandId::ID_MAPPED_DATA, key, threadId} {}
  MappedDataMetaCommand() : Command(CommandId::ID_MAPPED_DATA) {}

public:
  InterfaceArgument<ID3D12Resource> resource_{};
  Argument<void*> mappedAddress_{};
  Argument<unsigned> offset_{};
  BufferArgument data_{};
};

class CreateHeapAllocationMetaCommand : public Command {
public:
  CreateHeapAllocationMetaCommand(unsigned key, unsigned threadId)
      : Command{CommandId::ID_CREATE_HEAP_ALLOCATION, key, threadId} {}
  CreateHeapAllocationMetaCommand() : Command(CommandId::ID_CREATE_HEAP_ALLOCATION) {}

public:
  InterfaceArgument<ID3D12Heap> heap_{};
  Argument<void*> address_{};
  BufferArgument data_{};
};

class WaitForFenceSignaledCommand : public Command {
public:
  WaitForFenceSignaledCommand(unsigned key, unsigned threadId)
      : Command{CommandId::ID_WAIT_FOR_FENCE_SIGNALED, key, threadId} {}
  WaitForFenceSignaledCommand() : Command(CommandId::ID_WAIT_FOR_FENCE_SIGNALED) {}

public:
  Argument<HANDLE> event_{};
  InterfaceArgument<ID3D12Fence> fence_{};
  Argument<unsigned> value_{};
};

#pragma region DML

class IDMLDeviceCheckFeatureSupportCommand : public Command {
public:
  IDMLDeviceCheckFeatureSupportCommand(unsigned key,
                                       unsigned threadId,
                                       IDMLDevice* object,
                                       DML_FEATURE feature,
                                       UINT featureQueryDataSize,
                                       const void* featureQueryData,
                                       UINT featureSupportDataSize,
                                       void* featureSupportData)
      : Command{CommandId::ID_IDMLDEVICE_CHECKFEATURESUPPORT, key, threadId},
        object_{object},
        feature_{feature},
        featureQueryDataSize_{featureQueryDataSize},
        featureQueryData_{featureQueryData, featureQueryDataSize, feature},
        featureSupportDataSize_{featureSupportDataSize},
        featureSupportData_{featureSupportData, featureSupportDataSize} {}
  IDMLDeviceCheckFeatureSupportCommand() : Command(CommandId::ID_IDMLDEVICE_CHECKFEATURESUPPORT) {}

public:
  InterfaceArgument<IDMLDevice> object_{};
  Argument<HRESULT> result_{};
  Argument<DML_FEATURE> feature_{};
  Argument<UINT> featureQueryDataSize_{};
  DML_CheckFeatureSupport_BufferArgument featureQueryData_{};
  Argument<UINT> featureSupportDataSize_{};
  BufferArgument featureSupportData_{};
};

#pragma endregion

} // namespace DirectX
} // namespace gits
