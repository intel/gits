// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
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
  FrameEndCommand() : Command(CommandId::ID_FRAME_END) {}
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
    GPU_EXECUTION_BEGIN,
    GPU_EXECUTION_END
  };
  MarkerUInt64Command(uint64_t value) : Command(CommandId::ID_MARKER_UINT64), m_Value(value) {}
  MarkerUInt64Command() : Command(CommandId::ID_MARKER_UINT64) {}

public:
  Argument<uint64_t> m_Value{};
};

class IUnknownQueryInterfaceCommand : public Command {
public:
  IUnknownQueryInterfaceCommand(unsigned threadId, IUnknown* object, REFIID riid, void** ppvObject)
      : Command{CommandId::ID_IUNKNOWN_QUERYINTERFACE, threadId},
        m_riid{riid},
        m_ppvObject{ppvObject} {}
  IUnknownQueryInterfaceCommand() : Command(CommandId::ID_IUNKNOWN_QUERYINTERFACE) {}

public:
  InterfaceArgument<IUnknown> m_Object{};
  Argument<HRESULT> m_Result{};
  Argument<IID> m_riid{};
  InterfaceOutputArgument<void> m_ppvObject{};
};

class IUnknownAddRefCommand : public Command {
public:
  IUnknownAddRefCommand(unsigned threadId, IUnknown* object)
      : Command{CommandId::ID_IUNKNOWN_ADDREF, threadId} {}
  IUnknownAddRefCommand() : Command(CommandId::ID_IUNKNOWN_ADDREF) {}

public:
  InterfaceArgument<IUnknown> m_Object{};
  Argument<ULONG> m_Result{};
};

class IUnknownReleaseCommand : public Command {
public:
  IUnknownReleaseCommand(unsigned threadId, IUnknown* object)
      : Command{CommandId::ID_IUNKNOWN_RELEASE, threadId} {}
  IUnknownReleaseCommand() : Command(CommandId::ID_IUNKNOWN_RELEASE) {}

public:
  InterfaceArgument<IUnknown> m_Object{};
  Argument<ULONG> m_Result{};
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
        m_NumRenderTargetDescriptors{NumRenderTargetDescriptors},
        m_pRenderTargetDescriptors{pRenderTargetDescriptors, RTsSingleHandleToDescriptorRange
                                                                 ? 1
                                                                 : NumRenderTargetDescriptors},
        m_RTsSingleHandleToDescriptorRange{RTsSingleHandleToDescriptorRange},
        m_pDepthStencilDescriptor{pDepthStencilDescriptor, 1} {}
  ID3D12GraphicsCommandListOMSetRenderTargetsCommand()
      : Command(CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_OMSETRENDERTARGETS) {}

public:
  InterfaceArgument<ID3D12GraphicsCommandList> m_Object{};
  Argument<UINT> m_NumRenderTargetDescriptors{};
  DescriptorHandleArrayArgument<D3D12_CPU_DESCRIPTOR_HANDLE> m_pRenderTargetDescriptors{};
  Argument<BOOL> m_RTsSingleHandleToDescriptorRange{};
  DescriptorHandleArrayArgument<D3D12_CPU_DESCRIPTOR_HANDLE> m_pDepthStencilDescriptor{};
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
        m_Object{object},
        m_pTiledResource{pTiledResource},
        m_pNumTilesForEntireResource{pNumTilesForEntireResource},
        m_pPackedMipDesc{pPackedMipDesc},
        m_pStandardTileShapeForNonPackedMips{pStandardTileShapeForNonPackedMips},
        m_pNumSubresourceTilings{pNumSubresourceTilings},
        m_FirstSubresourceTilingToGet{FirstSubresourceTilingToGet},
        m_pSubresourceTilingsForNonPackedMips{} {
    if (pNumSubresourceTilings) {
      m_pSubresourceTilingsForNonPackedMips.Value = pSubresourceTilingsForNonPackedMips;
      m_pSubresourceTilingsForNonPackedMips.Size = *pNumSubresourceTilings;
    }
  }
  ID3D12DeviceGetResourceTilingCommand() : Command(CommandId::ID_ID3D12DEVICE_GETRESOURCETILING) {}

public:
  InterfaceArgument<ID3D12Device> m_Object{};
  InterfaceArgument<ID3D12Resource> m_pTiledResource{};
  PointerArgument<UINT> m_pNumTilesForEntireResource{};
  PointerArgument<D3D12_PACKED_MIP_INFO> m_pPackedMipDesc{};
  PointerArgument<D3D12_TILE_SHAPE> m_pStandardTileShapeForNonPackedMips{};
  PointerArgument<UINT> m_pNumSubresourceTilings{};
  Argument<UINT> m_FirstSubresourceTilingToGet{};
  ArrayArgument<D3D12_SUBRESOURCE_TILING> m_pSubresourceTilingsForNonPackedMips{};
};

class ID3D12StateObjectPropertiesGetShaderIdentifierCommand : public Command {
public:
  ID3D12StateObjectPropertiesGetShaderIdentifierCommand(unsigned threadId,
                                                        ID3D12StateObjectProperties* object,
                                                        LPCWSTR pExportName)
      : Command{CommandId::ID_ID3D12STATEOBJECTPROPERTIES_GETSHADERIDENTIFIER, threadId},
        m_Object{object},
        m_pExportName{pExportName} {}
  ID3D12StateObjectPropertiesGetShaderIdentifierCommand()
      : Command(CommandId::ID_ID3D12STATEOBJECTPROPERTIES_GETSHADERIDENTIFIER) {}

public:
  InterfaceArgument<ID3D12StateObjectProperties> m_Object{};
  ShaderIdentifierArgument m_Result{};
  LPCWSTR_Argument m_pExportName{};
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
        m_Object{object},
        m_DstSubresource{DstSubresource},
        m_pDstBox{pDstBox},
        m_SrcRowPitch{SrcRowPitch},
        m_SrcDepthPitch{SrcDepthPitch} {
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
    m_pSrcData.Value = const_cast<void*>(pSrcData);
    m_pSrcData.Size = pSrcDataSize;
  }
  ID3D12ResourceWriteToSubresourceCommand()
      : Command(CommandId::ID_ID3D12RESOURCE_WRITETOSUBRESOURCE) {}

public:
  InterfaceArgument<ID3D12Resource> m_Object{};
  Argument<HRESULT> m_Result{};
  Argument<UINT> m_DstSubresource{};
  PointerArgument<D3D12_BOX> m_pDstBox{};
  BufferArgument m_pSrcData{};
  Argument<UINT> m_SrcRowPitch{};
  Argument<UINT> m_SrcDepthPitch{};
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
        m_Object{object},
        m_DstRowPitch{DstRowPitch},
        m_DstDepthPitch{DstDepthPitch},
        m_SrcSubresource{SrcSubresource},
        m_pSrcBox{pSrcBox} {
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
    m_pDstData.Value = const_cast<void*>(pDstData);
    m_pDstData.Size = pDstDataSize;
  }
  ID3D12ResourceReadFromSubresourceCommand()
      : Command(CommandId::ID_ID3D12RESOURCE_READFROMSUBRESOURCE) {}

public:
  InterfaceArgument<ID3D12Resource> m_Object{};
  Argument<HRESULT> m_Result{};
  BufferArgument m_pDstData{};
  Argument<UINT> m_DstRowPitch{};
  Argument<UINT> m_DstDepthPitch{};
  Argument<UINT> m_SrcSubresource{};
  PointerArgument<D3D12_BOX> m_pSrcBox{};
};

class ID3D12GraphicsCommandListPreviewConvertLinearAlgebraMatrixCommand : public Command {
public:
  ID3D12GraphicsCommandListPreviewConvertLinearAlgebraMatrixCommand(
      unsigned threadId,
      ID3D12GraphicsCommandListPreview* object,
      const D3D12_LINEAR_ALGEBRA_MATRIX_CONVERSION_INFO* pDesc,
      UINT DescCount)
      : Command{CommandId::ID_ID3D12GRAPHICSCOMMANDLISTPREVIEW_CONVERTLINEARALGEBRAMATRIX,
                threadId},
        m_Object{object},
        m_pDesc{pDesc, DescCount},
        m_DescCount{DescCount} {}
  ID3D12GraphicsCommandListPreviewConvertLinearAlgebraMatrixCommand()
      : Command(CommandId::ID_ID3D12GRAPHICSCOMMANDLISTPREVIEW_CONVERTLINEARALGEBRAMATRIX) {}

public:
  InterfaceArgument<ID3D12GraphicsCommandListPreview> m_Object{};
  ArrayArgument<D3D12_LINEAR_ALGEBRA_MATRIX_CONVERSION_INFO> m_pDesc{};
  Argument<UINT> m_DescCount{};
};

class INTC_D3D12_GetSupportedVersionsCommand : public Command {
public:
  INTC_D3D12_GetSupportedVersionsCommand(unsigned threadId,
                                         const ID3D12Device* pDevice,
                                         INTCExtensionVersion* pSupportedExtVersions,
                                         uint32_t* pSupportedExtVersionsCount)
      : Command{CommandId::INTC_D3D12_GETSUPPORTEDVERSIONS, threadId},
        m_pDevice{const_cast<ID3D12Device*>(pDevice)},
        m_pSupportedExtVersionsCount{pSupportedExtVersionsCount},
        m_pSupportedExtVersions{} {
    if (pSupportedExtVersionsCount) {
      m_pSupportedExtVersions.Value = pSupportedExtVersions;
      m_pSupportedExtVersions.Size = *pSupportedExtVersionsCount;
    }
  }
  INTC_D3D12_GetSupportedVersionsCommand() : Command(CommandId::INTC_D3D12_GETSUPPORTEDVERSIONS) {}

public:
  InterfaceArgument<ID3D12Device> m_pDevice{};
  ArrayArgument<INTCExtensionVersion> m_pSupportedExtVersions{};
  PointerArgument<uint32_t> m_pSupportedExtVersionsCount{};
  Argument<HRESULT> m_Result{};
};

class INTC_D3D12_CreateDeviceExtensionContextCommand : public Command {
public:
  INTC_D3D12_CreateDeviceExtensionContextCommand(unsigned threadId,
                                                 const ID3D12Device* pDevice,
                                                 INTCExtensionContext** ppExtensionContext,
                                                 INTCExtensionInfo* pExtensionInfo,
                                                 INTCExtensionAppInfo* pExtensionAppInfo)
      : Command{CommandId::INTC_D3D12_CREATEDEVICEEXTENSIONCONTEXT, threadId},
        m_pDevice{const_cast<ID3D12Device*>(pDevice)},
        m_ppExtensionContext{ppExtensionContext},
        m_pExtensionInfo{pExtensionInfo},
        m_pExtensionAppInfo{pExtensionAppInfo} {}
  INTC_D3D12_CreateDeviceExtensionContextCommand()
      : Command(CommandId::INTC_D3D12_CREATEDEVICEEXTENSIONCONTEXT) {}

public:
  InterfaceArgument<ID3D12Device> m_pDevice;
  INTCExtensionContextOutputArgument m_ppExtensionContext;
  PointerArgument<INTCExtensionInfo> m_pExtensionInfo;
  PointerArgument<INTCExtensionAppInfo> m_pExtensionAppInfo;
  Argument<HRESULT> m_Result{};
};

class INTC_D3D12_CreateDeviceExtensionContext1Command : public Command {
public:
  INTC_D3D12_CreateDeviceExtensionContext1Command(unsigned threadId,
                                                  const ID3D12Device* pDevice,
                                                  INTCExtensionContext** ppExtensionContext,
                                                  INTCExtensionInfo* pExtensionInfo,
                                                  INTCExtensionAppInfo1* pExtensionAppInfo)
      : Command{CommandId::INTC_D3D12_CREATEDEVICEEXTENSIONCONTEXT1, threadId},
        m_pDevice{const_cast<ID3D12Device*>(pDevice)},
        m_ppExtensionContext{ppExtensionContext},
        m_pExtensionInfo{pExtensionInfo},
        m_pExtensionAppInfo{pExtensionAppInfo} {}
  INTC_D3D12_CreateDeviceExtensionContext1Command()
      : Command(CommandId::INTC_D3D12_CREATEDEVICEEXTENSIONCONTEXT1) {}

public:
  InterfaceArgument<ID3D12Device> m_pDevice;
  INTCExtensionContextOutputArgument m_ppExtensionContext;
  PointerArgument<INTCExtensionInfo> m_pExtensionInfo;
  PointerArgument<INTCExtensionAppInfo1> m_pExtensionAppInfo;
  Argument<HRESULT> m_Result{};
};

class INTC_D3D12_SetApplicationInfoCommand : public Command {
public:
  INTC_D3D12_SetApplicationInfoCommand(unsigned threadId, INTCExtensionAppInfo1* pExtensionAppInfo)
      : Command{CommandId::INTC_D3D12_SETAPPLICATIONINFO, threadId},
        m_pExtensionAppInfo{pExtensionAppInfo} {}
  INTC_D3D12_SetApplicationInfoCommand() : Command(CommandId::INTC_D3D12_SETAPPLICATIONINFO) {}

public:
  PointerArgument<INTCExtensionAppInfo1> m_pExtensionAppInfo;
  Argument<HRESULT> m_Result{};
};

class INTC_DestroyDeviceExtensionContextCommand : public Command {
public:
  INTC_DestroyDeviceExtensionContextCommand(unsigned threadId,
                                            INTCExtensionContext** ppExtensionContext)
      : Command{CommandId::INTC_DESTROYDEVICEEXTENSIONCONTEXT, threadId},
        m_ppExtensionContext{ppExtensionContext} {}
  INTC_DestroyDeviceExtensionContextCommand()
      : Command(CommandId::INTC_DESTROYDEVICEEXTENSIONCONTEXT) {}

public:
  INTCExtensionContextOutputArgument m_ppExtensionContext;
  Argument<HRESULT> m_Result{};
};

class INTC_D3D12_CheckFeatureSupportCommand : public Command {
public:
  INTC_D3D12_CheckFeatureSupportCommand(unsigned threadId,
                                        INTCExtensionContext* pExtensionContext,
                                        INTC_D3D12_FEATURES Feature,
                                        void* pFeatureSupportData,
                                        UINT FeatureSupportDataSize)
      : Command{CommandId::INTC_D3D12_CHECKFEATURESUPPORT, threadId},
        m_pExtensionContext{pExtensionContext},
        m_Feature{Feature},
        m_pFeatureSupportData{pFeatureSupportData, FeatureSupportDataSize},
        m_FeatureSupportDataSize{FeatureSupportDataSize} {}
  INTC_D3D12_CheckFeatureSupportCommand() : Command(CommandId::INTC_D3D12_CHECKFEATURESUPPORT) {}

public:
  INTCExtensionContextArgument m_pExtensionContext;
  Argument<INTC_D3D12_FEATURES> m_Feature{};
  BufferArgument m_pFeatureSupportData{};
  Argument<UINT> m_FeatureSupportDataSize{};
  Argument<HRESULT> m_Result{};
};

class INTC_D3D12_CreateCommandQueueCommand : public Command {
public:
  INTC_D3D12_CreateCommandQueueCommand(unsigned threadId,
                                       INTCExtensionContext* pExtensionContext,
                                       const INTC_D3D12_COMMAND_QUEUE_DESC_0001* pDesc,
                                       REFIID riid,
                                       void** ppCommandQueue)
      : Command{CommandId::INTC_D3D12_CREATECOMMANDQUEUE, threadId},
        m_pExtensionContext{pExtensionContext},
        m_pDesc{pDesc},
        m_riid{riid},
        m_ppCommandQueue{ppCommandQueue} {}
  INTC_D3D12_CreateCommandQueueCommand() : Command(CommandId::INTC_D3D12_CREATECOMMANDQUEUE) {}

public:
  INTCExtensionContextArgument m_pExtensionContext;
  PointerArgument<INTC_D3D12_COMMAND_QUEUE_DESC_0001> m_pDesc{};
  Argument<IID> m_riid{};
  InterfaceOutputArgument<void> m_ppCommandQueue{};
  Argument<HRESULT> m_Result{};
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
        m_pExtensionContext{pExtensionContext},
        m_pDesc{pDesc},
        m_InitialState{InitialState},
        m_pOptimizedClearValue{pOptimizedClearValue},
        m_riid{riid},
        m_ppvResource{ppvResource} {}
  INTC_D3D12_CreateReservedResourceCommand()
      : Command(CommandId::INTC_D3D12_CREATERESERVEDRESOURCE) {}

public:
  INTCExtensionContextArgument m_pExtensionContext;
  PointerArgument<INTC_D3D12_RESOURCE_DESC> m_pDesc{};
  Argument<D3D12_RESOURCE_STATES> m_InitialState{};
  PointerArgument<D3D12_CLEAR_VALUE> m_pOptimizedClearValue{};
  Argument<IID> m_riid{};
  InterfaceOutputArgument<void> m_ppvResource{};
  Argument<HRESULT> m_Result{};
};

class INTC_D3D12_SetFeatureSupportCommand : public Command {
public:
  INTC_D3D12_SetFeatureSupportCommand(unsigned threadId,
                                      INTCExtensionContext* pExtensionContext,
                                      INTC_D3D12_FEATURE* pFeature)
      : Command{CommandId::INTC_D3D12_SETFEATURESUPPORT, threadId},
        m_pExtensionContext{pExtensionContext},
        m_pFeature{pFeature} {}
  INTC_D3D12_SetFeatureSupportCommand() : Command(CommandId::INTC_D3D12_SETFEATURESUPPORT) {}

public:
  INTCExtensionContextArgument m_pExtensionContext;
  PointerArgument<INTC_D3D12_FEATURE> m_pFeature{};
  Argument<HRESULT> m_Result{};
};

class INTC_D3D12_GetResourceAllocationInfoCommand : public Command {
public:
  INTC_D3D12_GetResourceAllocationInfoCommand(unsigned threadId,
                                              INTCExtensionContext* pExtensionContext,
                                              UINT visibleMask,
                                              UINT numResourceDescs,
                                              const INTC_D3D12_RESOURCE_DESC_0001* pResourceDescs)
      : Command{CommandId::INTC_D3D12_GETRESOURCEALLOCATIONINFO, threadId},
        m_pExtensionContext{pExtensionContext},
        m_visibleMask{visibleMask},
        m_numResourceDescs{numResourceDescs},
        m_pResourceDescs{pResourceDescs} {}
  INTC_D3D12_GetResourceAllocationInfoCommand()
      : Command(CommandId::INTC_D3D12_GETRESOURCEALLOCATIONINFO) {}

public:
  INTCExtensionContextArgument m_pExtensionContext;
  Argument<UINT> m_visibleMask{};
  Argument<UINT> m_numResourceDescs{};
  PointerArgument<INTC_D3D12_RESOURCE_DESC_0001> m_pResourceDescs{};
  Argument<D3D12_RESOURCE_ALLOCATION_INFO> m_Result{};
};

class INTC_D3D12_CreateComputePipelineStateCommand : public Command {
public:
  INTC_D3D12_CreateComputePipelineStateCommand(unsigned threadId,
                                               INTCExtensionContext* pExtensionContext,
                                               const INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC* pDesc,
                                               REFIID riid,
                                               void** ppPipelineState)
      : Command{CommandId::INTC_D3D12_CREATECOMPUTEPIPELINESTATE, threadId},
        m_pExtensionContext{pExtensionContext},
        m_pDesc{pDesc},
        m_riid{riid},
        m_ppPipelineState{ppPipelineState} {}
  INTC_D3D12_CreateComputePipelineStateCommand()
      : Command(CommandId::INTC_D3D12_CREATECOMPUTEPIPELINESTATE) {}

public:
  INTCExtensionContextArgument m_pExtensionContext;
  PointerArgument<INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC> m_pDesc{};
  Argument<IID> m_riid{};
  InterfaceOutputArgument<void> m_ppPipelineState{};
  Argument<HRESULT> m_Result{};
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
        m_pExtensionContext{pExtensionContext},
        m_pHeap{pHeap},
        m_HeapOffset{HeapOffset},
        m_pDesc{pDesc},
        m_InitialState{InitialState},
        m_pOptimizedClearValue{pOptimizedClearValue},
        m_riid{riid},
        m_ppvResource{ppvResource} {}
  INTC_D3D12_CreatePlacedResourceCommand() : Command(CommandId::INTC_D3D12_CREATEPLACEDRESOURCE) {}

public:
  INTCExtensionContextArgument m_pExtensionContext;
  InterfaceArgument<ID3D12Heap> m_pHeap{};
  Argument<UINT64> m_HeapOffset{};
  PointerArgument<INTC_D3D12_RESOURCE_DESC_0001> m_pDesc{};
  Argument<D3D12_RESOURCE_STATES> m_InitialState{};
  PointerArgument<D3D12_CLEAR_VALUE> m_pOptimizedClearValue{};
  Argument<IID> m_riid{};
  InterfaceOutputArgument<void> m_ppvResource{};
  Argument<HRESULT> m_Result{};
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
        m_pExtensionContext{pExtensionContext},
        m_pHeapProperties{pHeapProperties},
        m_HeapFlags{HeapFlags},
        m_pDesc{pDesc},
        m_InitialResourceState{InitialResourceState},
        m_pOptimizedClearValue{pOptimizedClearValue},
        m_riidResource{riidResource},
        m_ppvResource{ppvResource} {}
  INTC_D3D12_CreateCommittedResourceCommand()
      : Command(CommandId::INTC_D3D12_CREATECOMMITTEDRESOURCE) {}

public:
  INTCExtensionContextArgument m_pExtensionContext;
  PointerArgument<D3D12_HEAP_PROPERTIES> m_pHeapProperties{};
  Argument<D3D12_HEAP_FLAGS> m_HeapFlags{};
  PointerArgument<INTC_D3D12_RESOURCE_DESC_0001> m_pDesc{};
  Argument<D3D12_RESOURCE_STATES> m_InitialResourceState{};
  PointerArgument<D3D12_CLEAR_VALUE> m_pOptimizedClearValue{};
  Argument<IID> m_riidResource{};
  InterfaceOutputArgument<void> m_ppvResource{};
  Argument<HRESULT> m_Result{};
};

class INTC_D3D12_CreateHeapCommand : public Command {
public:
  INTC_D3D12_CreateHeapCommand(unsigned threadId,
                               INTCExtensionContext* pExtensionContext,
                               const INTC_D3D12_HEAP_DESC* pDesc,
                               REFIID riid,
                               void** ppvHeap)
      : Command{CommandId::INTC_D3D12_CREATEHEAP, threadId},
        m_pExtensionContext{pExtensionContext},
        m_pDesc{pDesc},
        m_riid{riid},
        m_ppvHeap{ppvHeap} {}
  INTC_D3D12_CreateHeapCommand() : Command(CommandId::INTC_D3D12_CREATEHEAP) {}

public:
  INTCExtensionContextArgument m_pExtensionContext;
  PointerArgument<INTC_D3D12_HEAP_DESC> m_pDesc{};
  Argument<IID> m_riid{};
  InterfaceOutputArgument<void> m_ppvHeap{};
  Argument<HRESULT> m_Result{};
};

class CreateWindowMetaCommand : public Command {
public:
  CreateWindowMetaCommand(unsigned threadId)
      : Command{CommandId::ID_META_CREATE_WINDOW, threadId} {}
  CreateWindowMetaCommand() : Command(CommandId::ID_META_CREATE_WINDOW) {}

public:
  Argument<HWND> m_hWnd{};
  Argument<int> m_width{};
  Argument<int> m_height{};
};

class MappedDataMetaCommand : public Command {
public:
  MappedDataMetaCommand(unsigned threadId) : Command{CommandId::ID_MAPPED_DATA, threadId} {}
  MappedDataMetaCommand() : Command(CommandId::ID_MAPPED_DATA) {}

public:
  InterfaceArgument<ID3D12Resource> m_resource{};
  Argument<void*> m_mappedAddress{};
  Argument<unsigned> m_offset{};
  BufferArgument m_data{};
};

class CreateHeapAllocationMetaCommand : public Command {
public:
  CreateHeapAllocationMetaCommand(unsigned threadId)
      : Command{CommandId::ID_CREATE_HEAP_ALLOCATION, threadId} {}
  CreateHeapAllocationMetaCommand() : Command(CommandId::ID_CREATE_HEAP_ALLOCATION) {}

public:
  InterfaceArgument<ID3D12Heap> m_heap{};
  Argument<void*> m_address{};
  BufferArgument m_data{};
};

class WaitForFenceSignaledCommand : public Command {
public:
  WaitForFenceSignaledCommand(unsigned threadId)
      : Command{CommandId::ID_WAIT_FOR_FENCE_SIGNALED, threadId} {}
  WaitForFenceSignaledCommand() : Command(CommandId::ID_WAIT_FOR_FENCE_SIGNALED) {}

public:
  Argument<HANDLE> m_event{};
  InterfaceArgument<ID3D12Fence> m_fence{};
  Argument<unsigned> m_Value{};
};

class DllContainerMetaCommand : public Command {
public:
  DllContainerMetaCommand(unsigned threadId)
      : Command{CommandId::ID_META_DLL_CONTAINER, threadId} {}
  DllContainerMetaCommand() : Command(CommandId::ID_META_DLL_CONTAINER) {}

public:
  LPCWSTR_Argument m_dllName{};
  BufferArgument m_dllData{};
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
        m_Object{object},
        m_feature{feature},
        m_featureQueryDataSize{featureQueryDataSize},
        m_featureQueryData{featureQueryData, featureQueryDataSize, feature},
        m_featureSupportDataSize{featureSupportDataSize},
        m_featureSupportData{featureSupportData, featureSupportDataSize} {}
  IDMLDeviceCheckFeatureSupportCommand() : Command(CommandId::ID_IDMLDEVICE_CHECKFEATURESUPPORT) {}

public:
  InterfaceArgument<IDMLDevice> m_Object{};
  Argument<HRESULT> m_Result{};
  Argument<DML_FEATURE> m_feature{};
  Argument<UINT> m_featureQueryDataSize{};
  DML_CheckFeatureSupport_BufferArgument m_featureQueryData{};
  Argument<UINT> m_featureSupportDataSize{};
  BufferArgument m_featureSupportData{};
};

#pragma region NVAPI

class NvAPI_InitializeCommand : public Command {
public:
  NvAPI_InitializeCommand(unsigned threadId) : Command{CommandId::ID_NVAPI_INITIALIZE, threadId} {}
  NvAPI_InitializeCommand() : Command(CommandId::ID_NVAPI_INITIALIZE) {}

public:
  Argument<NvAPI_Status> m_Result{};
};

class NvAPI_UnloadCommand : public Command {
public:
  NvAPI_UnloadCommand(unsigned threadId) : Command{CommandId::ID_NVAPI_UNLOAD, threadId} {}
  NvAPI_UnloadCommand() : Command(CommandId::ID_NVAPI_UNLOAD) {}

public:
  Argument<NvAPI_Status> m_Result{};
};

class NvAPI_D3D12_SetCreatePipelineStateOptionsCommand : public Command {
public:
  NvAPI_D3D12_SetCreatePipelineStateOptionsCommand(
      unsigned threadId,
      ID3D12Device5* pDevice,
      const NVAPI_D3D12_SET_CREATE_PIPELINE_STATE_OPTIONS_PARAMS* pState)
      : Command{CommandId::ID_NVAPI_D3D12_SETCREATEPIPELINESTATEOPTIONS, threadId},
        m_pDevice{pDevice},
        m_pState{pState} {}
  NvAPI_D3D12_SetCreatePipelineStateOptionsCommand()
      : Command(CommandId::ID_NVAPI_D3D12_SETCREATEPIPELINESTATEOPTIONS) {}

public:
  InterfaceArgument<ID3D12Device5> m_pDevice{};
  PointerArgument<NVAPI_D3D12_SET_CREATE_PIPELINE_STATE_OPTIONS_PARAMS> m_pState{};
  Argument<NvAPI_Status> m_Result{};
};

class NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand : public Command {
public:
  NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand(unsigned threadId,
                                              IUnknown* pDev,
                                              NvU32 uavSlot,
                                              NvU32 uavSpace)
      : Command{CommandId::ID_NVAPI_D3D12_SETNVSHADEREXTNSLOTSPACE, threadId},
        m_pDev{pDev},
        m_uavSlot{uavSlot},
        m_uavSpace{uavSpace} {}
  NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand()
      : Command(CommandId::ID_NVAPI_D3D12_SETNVSHADEREXTNSLOTSPACE) {}

public:
  InterfaceArgument<IUnknown> m_pDev{};
  Argument<NvU32> m_uavSlot{};
  Argument<NvU32> m_uavSpace{};
  Argument<NvAPI_Status> m_Result{};
};

class NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand : public Command {
public:
  NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand(unsigned threadId,
                                                         IUnknown* pDev,
                                                         NvU32 uavSlot,
                                                         NvU32 uavSpace)
      : Command{CommandId::ID_NVAPI_D3D12_SETNVSHADEREXTNSLOTSPACELOCALTHREAD, threadId},
        m_pDev{pDev},
        m_uavSlot{uavSlot},
        m_uavSpace{uavSpace} {}
  NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand()
      : Command(CommandId::ID_NVAPI_D3D12_SETNVSHADEREXTNSLOTSPACELOCALTHREAD) {}

public:
  InterfaceArgument<IUnknown> m_pDev{};
  Argument<NvU32> m_uavSlot{};
  Argument<NvU32> m_uavSpace{};
  Argument<NvAPI_Status> m_Result{};
};

class NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand : public Command {
public:
  NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand(
      unsigned threadId,
      ID3D12GraphicsCommandList4* pCommandList,
      const NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS* pParams)
      : Command{CommandId::ID_NVAPI_D3D12_BUILDRAYTRACINGACCELERATIONSTRUCTUREEX, threadId},
        m_pCommandList{pCommandList},
        m_pParams{pParams} {}
  NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand()
      : Command(CommandId::ID_NVAPI_D3D12_BUILDRAYTRACINGACCELERATIONSTRUCTUREEX) {}

public:
  InterfaceArgument<ID3D12GraphicsCommandList4> m_pCommandList{};
  PointerArgument<NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS> m_pParams{};
  Argument<NvAPI_Status> m_Result{};
};

class NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand : public Command {
public:
  NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand(
      unsigned threadId,
      ID3D12GraphicsCommandList4* pCommandList,
      const NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS* pParams)
      : Command{CommandId::ID_NVAPI_D3D12_BUILDRAYTRACINGOPACITYMICROMAPARRAY, threadId},
        m_pCommandList{pCommandList},
        m_pParams{pParams} {}
  NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand()
      : Command(CommandId::ID_NVAPI_D3D12_BUILDRAYTRACINGOPACITYMICROMAPARRAY) {}

public:
  InterfaceArgument<ID3D12GraphicsCommandList4> m_pCommandList{};
  PointerArgument<NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS> m_pParams{};
  Argument<NvAPI_Status> m_Result{};
};

class NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand : public Command {
public:
  NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand(
      unsigned threadId,
      ID3D12GraphicsCommandList4* pCommandList,
      const NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS* pParams)
      : Command{CommandId::ID_NVAPI_D3D12_RAYTRACINGEXECUTEMULTIINDIRECTCLUSTEROPERATION, threadId},
        m_pCommandList{pCommandList},
        m_pParams{pParams} {}
  NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand()
      : Command(CommandId::ID_NVAPI_D3D12_RAYTRACINGEXECUTEMULTIINDIRECTCLUSTEROPERATION) {}

public:
  InterfaceArgument<ID3D12GraphicsCommandList4> m_pCommandList{};
  PointerArgument<NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS> m_pParams{};
  Argument<NvAPI_Status> m_Result{};
};

#pragma endregion

} // namespace DirectX
} // namespace gits
