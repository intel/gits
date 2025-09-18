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
struct INTC_D3D12_RESOURCE_DESC;
struct INTC_D3D12_COMMAND_QUEUE_DESC_0001;
struct INTC_D3D12_HEAP_DESC;

typedef enum _NvAPI_Status NvAPI_Status;
typedef unsigned long NvU32;
typedef struct _NVAPI_D3D12_SET_CREATE_PIPELINE_STATE_OPTIONS_PARAMS_V1
    NVAPI_D3D12_SET_CREATE_PIPELINE_STATE_OPTIONS_PARAMS_V1;
typedef NVAPI_D3D12_SET_CREATE_PIPELINE_STATE_OPTIONS_PARAMS_V1
    NVAPI_D3D12_SET_CREATE_PIPELINE_STATE_OPTIONS_PARAMS;
typedef struct _NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS_V1
    NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS_V1;
typedef NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS_V1
    NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS;
typedef struct _NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS_V1
    NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS_V1;
typedef NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS_V1
    NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS;
typedef struct _NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS_V1
    NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS_V1;
typedef NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS_V1
    NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS;

namespace gits {
namespace DirectX {

class StateRestoreBeginCommand : public Command {
public:
  StateRestoreBeginCommand() : Command(CommandId::ID_INIT_START) {}
};

class StateRestoreEndCommand : public Command {
public:
  StateRestoreEndCommand() : Command(CommandId::ID_INIT_END) {}
};

class FrameEndCommand : public Command {
public:
  FrameEndCommand(unsigned frameNumber)
      : Command(CommandId::ID_FRAME_END), frameNumber_(frameNumber) {}

public:
  unsigned frameNumber_{};
};

class MarkerUInt64Command : public Command {
public:
  enum Value : uint64_t {
    NONE = 0x10000 + 1, // CTokenMarkerUInt64::COMMON_RESERVED + 1
    STATE_RESTORE_OBJECTS_BEGIN,
    STATE_RESTORE_OBJECTS_END,
    STATE_RESTORE_RTAS_BEGIN,
    STATE_RESTORE_RTAS_END,
    STATE_RESTORE_RESOURCES_BEGIN,
    STATE_RESTORE_RESOURCES_END,
  };
  MarkerUInt64Command(uint64_t value) : Command(CommandId::ID_MARKER_UINT64), value_(value) {}

public:
  uint64_t value_{};
};

class IUnknownQueryInterfaceCommand : public Command {
public:
  IUnknownQueryInterfaceCommand(unsigned threadId, IUnknown* object, REFIID riid, void** ppvObject)
      : Command{CommandId::ID_IUNKNOWN_QUERYINTERFACE, threadId},
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
  IUnknownAddRefCommand(unsigned threadId, IUnknown* object)
      : Command{CommandId::ID_IUNKNOWN_ADDREF, threadId} {}
  IUnknownAddRefCommand() : Command(CommandId::ID_IUNKNOWN_ADDREF) {}

public:
  InterfaceArgument<IUnknown> object_{};
  Argument<ULONG> result_{};
};

class IUnknownReleaseCommand : public Command {
public:
  IUnknownReleaseCommand(unsigned threadId, IUnknown* object)
      : Command{CommandId::ID_IUNKNOWN_RELEASE, threadId} {}
  IUnknownReleaseCommand() : Command(CommandId::ID_IUNKNOWN_RELEASE) {}

public:
  InterfaceArgument<IUnknown> object_{};
  Argument<ULONG> result_{};
};

class ID3D12GraphicsCommandListOMSetRenderTargetsCommand : public Command {
public:
  ID3D12GraphicsCommandListOMSetRenderTargetsCommand(
      unsigned threadId,
      IUnknown* object,
      UINT NumRenderTargetDescriptors,
      const D3D12_CPU_DESCRIPTOR_HANDLE* pRenderTargetDescriptors,
      BOOL RTsSingleHandleToDescriptorRange,
      const D3D12_CPU_DESCRIPTOR_HANDLE* pDepthStencilDescriptor)
      : Command{CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_OMSETRENDERTARGETS, threadId},
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
      unsigned threadId,
      ID3D12Device* object,
      ID3D12Resource* pTiledResource,
      UINT* pNumTilesForEntireResource,
      D3D12_PACKED_MIP_INFO* pPackedMipDesc,
      D3D12_TILE_SHAPE* pStandardTileShapeForNonPackedMips,
      UINT* pNumSubresourceTilings,
      UINT FirstSubresourceTilingToGet,
      D3D12_SUBRESOURCE_TILING* pSubresourceTilingsForNonPackedMips)
      : Command{CommandId::ID_ID3D12DEVICE_GETRESOURCETILING, threadId},
        object_{object},
        pTiledResource_{pTiledResource},
        pNumTilesForEntireResource_{pNumTilesForEntireResource},
        pPackedMipDesc_{pPackedMipDesc},
        pStandardTileShapeForNonPackedMips_{pStandardTileShapeForNonPackedMips},
        pNumSubresourceTilings_{pNumSubresourceTilings},
        FirstSubresourceTilingToGet_{FirstSubresourceTilingToGet},
        pSubresourceTilingsForNonPackedMips_{} {
    if (pNumSubresourceTilings) {
      pSubresourceTilingsForNonPackedMips_.value = pSubresourceTilingsForNonPackedMips;
      pSubresourceTilingsForNonPackedMips_.size = *pNumSubresourceTilings;
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
  ID3D12StateObjectPropertiesGetShaderIdentifierCommand(unsigned threadId,
                                                        ID3D12StateObjectProperties* object,
                                                        LPCWSTR pExportName)
      : Command{CommandId::ID_ID3D12STATEOBJECTPROPERTIES_GETSHADERIDENTIFIER, threadId},
        object_{object},
        pExportName_{pExportName} {}
  ID3D12StateObjectPropertiesGetShaderIdentifierCommand()
      : Command(CommandId::ID_ID3D12STATEOBJECTPROPERTIES_GETSHADERIDENTIFIER) {}

public:
  InterfaceArgument<ID3D12StateObjectProperties> object_{};
  ShaderIdentifierArgument result_{};
  LPCWSTR_Argument pExportName_{};
};

class ID3D12ResourceWriteToSubresourceCommand : public Command {
public:
  ID3D12ResourceWriteToSubresourceCommand(unsigned threadId,
                                          ID3D12Resource* object,
                                          UINT DstSubresource,
                                          const D3D12_BOX* pDstBox,
                                          const void* pSrcData,
                                          UINT SrcRowPitch,
                                          UINT SrcDepthPitch)
      : Command{CommandId::ID_ID3D12RESOURCE_WRITETOSUBRESOURCE, threadId},
        object_{object},
        DstSubresource_{DstSubresource},
        pDstBox_{pDstBox},
        SrcRowPitch_{SrcRowPitch},
        SrcDepthPitch_{SrcDepthPitch} {
    UINT depth = 1;
    if (pDstBox) {
      depth = (pDstBox->back - pDstBox->front);
    } else {
      D3D12_RESOURCE_DESC desc = object->GetDesc();
      if (desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D) {
        depth = desc.DepthOrArraySize;
      }
    }
    size_t pSrcDataSize = SrcDepthPitch * depth;
    pSrcData_.value = const_cast<void*>(pSrcData);
    pSrcData_.size = pSrcDataSize;
  }
  ID3D12ResourceWriteToSubresourceCommand()
      : Command(CommandId::ID_ID3D12RESOURCE_WRITETOSUBRESOURCE) {}

public:
  InterfaceArgument<ID3D12Resource> object_{};
  Argument<HRESULT> result_{};
  Argument<UINT> DstSubresource_{};
  PointerArgument<D3D12_BOX> pDstBox_{};
  BufferArgument pSrcData_{};
  Argument<UINT> SrcRowPitch_{};
  Argument<UINT> SrcDepthPitch_{};
};

class ID3D12ResourceReadFromSubresourceCommand : public Command {
public:
  ID3D12ResourceReadFromSubresourceCommand(unsigned threadId,
                                           ID3D12Resource* object,
                                           void* pDstData,
                                           UINT DstRowPitch,
                                           UINT DstDepthPitch,
                                           UINT SrcSubresource,
                                           const D3D12_BOX* pSrcBox)
      : Command{CommandId::ID_ID3D12RESOURCE_READFROMSUBRESOURCE, threadId},
        object_{object},
        DstRowPitch_{DstRowPitch},
        DstDepthPitch_{DstDepthPitch},
        SrcSubresource_{SrcSubresource},
        pSrcBox_{pSrcBox} {
    UINT depth = 1;
    if (pSrcBox) {
      depth = (pSrcBox->back - pSrcBox->front);
    } else {
      D3D12_RESOURCE_DESC desc = object->GetDesc();
      if (desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D) {
        depth = desc.DepthOrArraySize;
      }
    }
    size_t pDstDataSize = DstDepthPitch * depth;
    pDstData_.value = const_cast<void*>(pDstData);
    pDstData_.size = pDstDataSize;
  }
  ID3D12ResourceReadFromSubresourceCommand()
      : Command(CommandId::ID_ID3D12RESOURCE_READFROMSUBRESOURCE) {}

public:
  InterfaceArgument<ID3D12Resource> object_{};
  Argument<HRESULT> result_{};
  BufferArgument pDstData_{};
  Argument<UINT> DstRowPitch_{};
  Argument<UINT> DstDepthPitch_{};
  Argument<UINT> SrcSubresource_{};
  PointerArgument<D3D12_BOX> pSrcBox_{};
};

class INTC_D3D12_GetSupportedVersionsCommand : public Command {
public:
  INTC_D3D12_GetSupportedVersionsCommand(unsigned threadId,
                                         const ID3D12Device* pDevice,
                                         INTCExtensionVersion* pSupportedExtVersions,
                                         uint32_t* pSupportedExtVersionsCount)
      : Command{CommandId::INTC_D3D12_GETSUPPORTEDVERSIONS, threadId},
        pDevice_{const_cast<ID3D12Device*>(pDevice)},
        pSupportedExtVersionsCount_{pSupportedExtVersionsCount},
        pSupportedExtVersions_{} {
    if (pSupportedExtVersionsCount) {
      pSupportedExtVersions_.value = pSupportedExtVersions;
      pSupportedExtVersions_.size = *pSupportedExtVersionsCount;
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
  INTC_D3D12_CreateDeviceExtensionContextCommand(unsigned threadId,
                                                 const ID3D12Device* pDevice,
                                                 INTCExtensionContext** ppExtensionContext,
                                                 INTCExtensionInfo* pExtensionInfo,
                                                 INTCExtensionAppInfo* pExtensionAppInfo)
      : Command{CommandId::INTC_D3D12_CREATEDEVICEEXTENSIONCONTEXT, threadId},
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
  INTC_D3D12_CreateDeviceExtensionContext1Command(unsigned threadId,
                                                  const ID3D12Device* pDevice,
                                                  INTCExtensionContext** ppExtensionContext,
                                                  INTCExtensionInfo* pExtensionInfo,
                                                  INTCExtensionAppInfo1* pExtensionAppInfo)
      : Command{CommandId::INTC_D3D12_CREATEDEVICEEXTENSIONCONTEXT1, threadId},
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

class INTC_D3D12_SetApplicationInfoCommand : public Command {
public:
  INTC_D3D12_SetApplicationInfoCommand(unsigned threadId, INTCExtensionAppInfo1* pExtensionAppInfo)
      : Command{CommandId::INTC_D3D12_SETAPPLICATIONINFO, threadId},
        pExtensionAppInfo_{pExtensionAppInfo} {}
  INTC_D3D12_SetApplicationInfoCommand() : Command(CommandId::INTC_D3D12_SETAPPLICATIONINFO) {}

public:
  PointerArgument<INTCExtensionAppInfo1> pExtensionAppInfo_;
  Argument<HRESULT> result_{};
};

class INTC_DestroyDeviceExtensionContextCommand : public Command {
public:
  INTC_DestroyDeviceExtensionContextCommand(unsigned threadId,
                                            INTCExtensionContext** ppExtensionContext)
      : Command{CommandId::INTC_DESTROYDEVICEEXTENSIONCONTEXT, threadId},
        ppExtensionContext_{ppExtensionContext} {}
  INTC_DestroyDeviceExtensionContextCommand()
      : Command(CommandId::INTC_DESTROYDEVICEEXTENSIONCONTEXT) {}

public:
  INTCExtensionContextOutputArgument ppExtensionContext_;
  Argument<HRESULT> result_{};
};

class INTC_D3D12_CheckFeatureSupportCommand : public Command {
public:
  INTC_D3D12_CheckFeatureSupportCommand(unsigned threadId,
                                        INTCExtensionContext* pExtensionContext,
                                        INTC_D3D12_FEATURES Feature,
                                        void* pFeatureSupportData,
                                        UINT FeatureSupportDataSize)
      : Command{CommandId::INTC_D3D12_CHECKFEATURESUPPORT, threadId},
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

class INTC_D3D12_CreateCommandQueueCommand : public Command {
public:
  INTC_D3D12_CreateCommandQueueCommand(unsigned threadId,
                                       INTCExtensionContext* pExtensionContext,
                                       const INTC_D3D12_COMMAND_QUEUE_DESC_0001* pDesc,
                                       REFIID riid,
                                       void** ppCommandQueue)
      : Command{CommandId::INTC_D3D12_CREATECOMMANDQUEUE, threadId},
        pExtensionContext_{pExtensionContext},
        pDesc_{pDesc},
        riid_{riid},
        ppCommandQueue_{ppCommandQueue} {}
  INTC_D3D12_CreateCommandQueueCommand() : Command(CommandId::INTC_D3D12_CREATECOMMANDQUEUE) {}

public:
  INTCExtensionContextArgument pExtensionContext_;
  PointerArgument<INTC_D3D12_COMMAND_QUEUE_DESC_0001> pDesc_{};
  Argument<IID> riid_{};
  InterfaceOutputArgument<void> ppCommandQueue_{};
  Argument<HRESULT> result_{};
};

class INTC_D3D12_CreateReservedResourceCommand : public Command {
public:
  INTC_D3D12_CreateReservedResourceCommand(unsigned threadId,
                                           INTCExtensionContext* pExtensionContext,
                                           const INTC_D3D12_RESOURCE_DESC* pDesc,
                                           D3D12_RESOURCE_STATES InitialState,
                                           const D3D12_CLEAR_VALUE* pOptimizedClearValue,
                                           REFIID riid,
                                           void** ppvResource)
      : Command{CommandId::INTC_D3D12_CREATERESERVEDRESOURCE, threadId},
        pExtensionContext_{pExtensionContext},
        pDesc_{pDesc},
        InitialState_{InitialState},
        pOptimizedClearValue_{pOptimizedClearValue},
        riid_{riid},
        ppvResource_{ppvResource} {}
  INTC_D3D12_CreateReservedResourceCommand()
      : Command(CommandId::INTC_D3D12_CREATERESERVEDRESOURCE) {}

public:
  INTCExtensionContextArgument pExtensionContext_;
  PointerArgument<INTC_D3D12_RESOURCE_DESC> pDesc_{};
  Argument<D3D12_RESOURCE_STATES> InitialState_{};
  PointerArgument<D3D12_CLEAR_VALUE> pOptimizedClearValue_{};
  Argument<IID> riid_{};
  InterfaceOutputArgument<void> ppvResource_{};
  Argument<HRESULT> result_{};
};

class INTC_D3D12_SetFeatureSupportCommand : public Command {
public:
  INTC_D3D12_SetFeatureSupportCommand(unsigned threadId,
                                      INTCExtensionContext* pExtensionContext,
                                      INTC_D3D12_FEATURE* pFeature)
      : Command{CommandId::INTC_D3D12_SETFEATURESUPPORT, threadId},
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
  INTC_D3D12_GetResourceAllocationInfoCommand(unsigned threadId,
                                              INTCExtensionContext* pExtensionContext,
                                              UINT visibleMask,
                                              UINT numResourceDescs,
                                              const INTC_D3D12_RESOURCE_DESC_0001* pResourceDescs)
      : Command{CommandId::INTC_D3D12_GETRESOURCEALLOCATIONINFO, threadId},
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
  INTC_D3D12_CreateComputePipelineStateCommand(unsigned threadId,
                                               INTCExtensionContext* pExtensionContext,
                                               const INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC* pDesc,
                                               REFIID riid,
                                               void** ppPipelineState)
      : Command{CommandId::INTC_D3D12_CREATECOMPUTEPIPELINESTATE, threadId},
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
  INTC_D3D12_CreatePlacedResourceCommand(unsigned threadId,
                                         INTCExtensionContext* pExtensionContext,
                                         ID3D12Heap* pHeap,
                                         UINT64 HeapOffset,
                                         const INTC_D3D12_RESOURCE_DESC_0001* pDesc,
                                         D3D12_RESOURCE_STATES InitialState,
                                         const D3D12_CLEAR_VALUE* pOptimizedClearValue,
                                         REFIID riid,
                                         void** ppvResource)
      : Command{CommandId::INTC_D3D12_CREATEPLACEDRESOURCE, threadId},
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
  INTC_D3D12_CreateCommittedResourceCommand(unsigned threadId,
                                            INTCExtensionContext* pExtensionContext,
                                            const D3D12_HEAP_PROPERTIES* pHeapProperties,
                                            D3D12_HEAP_FLAGS HeapFlags,
                                            const INTC_D3D12_RESOURCE_DESC_0001* pDesc,
                                            D3D12_RESOURCE_STATES InitialResourceState,
                                            const D3D12_CLEAR_VALUE* pOptimizedClearValue,
                                            REFIID riidResource,
                                            void** ppvResource)
      : Command{CommandId::INTC_D3D12_CREATECOMMITTEDRESOURCE, threadId},
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

class INTC_D3D12_CreateHeapCommand : public Command {
public:
  INTC_D3D12_CreateHeapCommand(unsigned threadId,
                               INTCExtensionContext* pExtensionContext,
                               const INTC_D3D12_HEAP_DESC* pDesc,
                               REFIID riid,
                               void** ppvHeap)
      : Command{CommandId::INTC_D3D12_CREATEHEAP, threadId},
        pExtensionContext_{pExtensionContext},
        pDesc_{pDesc},
        riid_{riid},
        ppvHeap_{ppvHeap} {}
  INTC_D3D12_CreateHeapCommand() : Command(CommandId::INTC_D3D12_CREATEHEAP) {}

public:
  INTCExtensionContextArgument pExtensionContext_;
  PointerArgument<INTC_D3D12_HEAP_DESC> pDesc_{};
  Argument<IID> riid_{};
  InterfaceOutputArgument<void> ppvHeap_{};
  Argument<HRESULT> result_{};
};

class CreateWindowMetaCommand : public Command {
public:
  CreateWindowMetaCommand(unsigned threadId)
      : Command{CommandId::ID_META_CREATE_WINDOW, threadId} {}
  CreateWindowMetaCommand() : Command(CommandId::ID_META_CREATE_WINDOW) {}

public:
  Argument<HWND> hWnd_{};
  Argument<int> width_{};
  Argument<int> height_{};
};

class MappedDataMetaCommand : public Command {
public:
  MappedDataMetaCommand(unsigned threadId) : Command{CommandId::ID_MAPPED_DATA, threadId} {}
  MappedDataMetaCommand() : Command(CommandId::ID_MAPPED_DATA) {}

public:
  InterfaceArgument<ID3D12Resource> resource_{};
  Argument<void*> mappedAddress_{};
  Argument<unsigned> offset_{};
  BufferArgument data_{};
};

class CreateHeapAllocationMetaCommand : public Command {
public:
  CreateHeapAllocationMetaCommand(unsigned threadId)
      : Command{CommandId::ID_CREATE_HEAP_ALLOCATION, threadId} {}
  CreateHeapAllocationMetaCommand() : Command(CommandId::ID_CREATE_HEAP_ALLOCATION) {}

public:
  InterfaceArgument<ID3D12Heap> heap_{};
  Argument<void*> address_{};
  BufferArgument data_{};
};

class WaitForFenceSignaledCommand : public Command {
public:
  WaitForFenceSignaledCommand(unsigned threadId)
      : Command{CommandId::ID_WAIT_FOR_FENCE_SIGNALED, threadId} {}
  WaitForFenceSignaledCommand() : Command(CommandId::ID_WAIT_FOR_FENCE_SIGNALED) {}

public:
  Argument<HANDLE> event_{};
  InterfaceArgument<ID3D12Fence> fence_{};
  Argument<unsigned> value_{};
};

class DllContainerMetaCommand : public Command {
public:
  DllContainerMetaCommand(unsigned threadId)
      : Command{CommandId::ID_META_DLL_CONTAINER, threadId} {}
  DllContainerMetaCommand() : Command(CommandId::ID_META_DLL_CONTAINER) {}

public:
  LPCWSTR_Argument dllName_{};
  BufferArgument dllData_{};
};

#pragma region DML

class IDMLDeviceCheckFeatureSupportCommand : public Command {
public:
  IDMLDeviceCheckFeatureSupportCommand(unsigned threadId,
                                       IDMLDevice* object,
                                       DML_FEATURE feature,
                                       UINT featureQueryDataSize,
                                       const void* featureQueryData,
                                       UINT featureSupportDataSize,
                                       void* featureSupportData)
      : Command{CommandId::ID_IDMLDEVICE_CHECKFEATURESUPPORT, threadId},
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

#pragma region NVAPI

class NvAPI_InitializeCommand : public Command {
public:
  NvAPI_InitializeCommand(unsigned threadId) : Command{CommandId::ID_NVAPI_INITIALIZE, threadId} {}
  NvAPI_InitializeCommand() : Command(CommandId::ID_NVAPI_INITIALIZE) {}

public:
  Argument<NvAPI_Status> result_{};
};

class NvAPI_UnloadCommand : public Command {
public:
  NvAPI_UnloadCommand(unsigned threadId) : Command{CommandId::ID_NVAPI_UNLOAD, threadId} {}
  NvAPI_UnloadCommand() : Command(CommandId::ID_NVAPI_UNLOAD) {}

public:
  Argument<NvAPI_Status> result_{};
};

class NvAPI_D3D12_SetCreatePipelineStateOptionsCommand : public Command {
public:
  NvAPI_D3D12_SetCreatePipelineStateOptionsCommand(
      unsigned threadId,
      ID3D12Device5* pDevice,
      const NVAPI_D3D12_SET_CREATE_PIPELINE_STATE_OPTIONS_PARAMS* pState)
      : Command{CommandId::ID_NVAPI_D3D12_SETCREATEPIPELINESTATEOPTIONS, threadId},
        pDevice_{pDevice},
        pState_{pState} {}
  NvAPI_D3D12_SetCreatePipelineStateOptionsCommand()
      : Command(CommandId::ID_NVAPI_D3D12_SETCREATEPIPELINESTATEOPTIONS) {}

public:
  InterfaceArgument<ID3D12Device5> pDevice_{};
  PointerArgument<NVAPI_D3D12_SET_CREATE_PIPELINE_STATE_OPTIONS_PARAMS> pState_{};
  Argument<NvAPI_Status> result_{};
};

class NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand : public Command {
public:
  NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand(unsigned threadId,
                                              IUnknown* pDev,
                                              NvU32 uavSlot,
                                              NvU32 uavSpace)
      : Command{CommandId::ID_NVAPI_D3D12_SETNVSHADEREXTNSLOTSPACE, threadId},
        pDev_{pDev},
        uavSlot_{uavSlot},
        uavSpace_{uavSpace} {}
  NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand()
      : Command(CommandId::ID_NVAPI_D3D12_SETNVSHADEREXTNSLOTSPACE) {}

public:
  InterfaceArgument<IUnknown> pDev_{};
  Argument<NvU32> uavSlot_{};
  Argument<NvU32> uavSpace_{};
  Argument<NvAPI_Status> result_{};
};

class NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand : public Command {
public:
  NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand(unsigned threadId,
                                                         IUnknown* pDev,
                                                         NvU32 uavSlot,
                                                         NvU32 uavSpace)
      : Command{CommandId::ID_NVAPI_D3D12_SETNVSHADEREXTNSLOTSPACELOCALTHREAD, threadId},
        pDev_{pDev},
        uavSlot_{uavSlot},
        uavSpace_{uavSpace} {}
  NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand()
      : Command(CommandId::ID_NVAPI_D3D12_SETNVSHADEREXTNSLOTSPACELOCALTHREAD) {}

public:
  InterfaceArgument<IUnknown> pDev_{};
  Argument<NvU32> uavSlot_{};
  Argument<NvU32> uavSpace_{};
  Argument<NvAPI_Status> result_{};
};

class NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand : public Command {
public:
  NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand(
      unsigned threadId,
      ID3D12GraphicsCommandList4* pCommandList,
      const NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS* pParams)
      : Command{CommandId::ID_NVAPI_D3D12_BUILDRAYTRACINGACCELERATIONSTRUCTUREEX, threadId},
        pCommandList_{pCommandList},
        pParams{pParams} {}
  NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand()
      : Command(CommandId::ID_NVAPI_D3D12_BUILDRAYTRACINGACCELERATIONSTRUCTUREEX) {}

public:
  InterfaceArgument<ID3D12GraphicsCommandList4> pCommandList_{};
  PointerArgument<NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS> pParams{};
  Argument<NvAPI_Status> result_{};
};

class NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand : public Command {
public:
  NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand(
      unsigned threadId,
      ID3D12GraphicsCommandList4* pCommandList,
      const NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS* pParams)
      : Command{CommandId::ID_NVAPI_D3D12_BUILDRAYTRACINGOPACITYMICROMAPARRAY, threadId},
        pCommandList_{pCommandList},
        pParams{pParams} {}
  NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand()
      : Command(CommandId::ID_NVAPI_D3D12_BUILDRAYTRACINGOPACITYMICROMAPARRAY) {}

public:
  InterfaceArgument<ID3D12GraphicsCommandList4> pCommandList_{};
  PointerArgument<NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS> pParams{};
  Argument<NvAPI_Status> result_{};
};

class NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand : public Command {
public:
  NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand(
      unsigned threadId,
      ID3D12GraphicsCommandList4* pCommandList,
      const NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS* pParams)
      : Command{CommandId::ID_NVAPI_D3D12_RAYTRACINGEXECUTEMULTIINDIRECTCLUSTEROPERATION, threadId},
        pCommandList_{pCommandList},
        pParams{pParams} {}
  NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand()
      : Command(CommandId::ID_NVAPI_D3D12_RAYTRACINGEXECUTEMULTIINDIRECTCLUSTEROPERATION) {}

public:
  InterfaceArgument<ID3D12GraphicsCommandList4> pCommandList_{};
  PointerArgument<NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS> pParams{};
  Argument<NvAPI_Status> result_{};
};

#pragma endregion

} // namespace DirectX
} // namespace gits
