// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandSerializer.h"
#include "commandsCustom.h"
#include "commandIdsAuto.h"
#include "commandEncodersCustom.h"

namespace gits {
namespace DirectX {

class StateRestoreBeginSerializer : public stream::CommandSerializer {
public:
  StateRestoreBeginSerializer(const StateRestoreBeginCommand& command) {}
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_INIT_START);
  }
};

class StateRestoreEndSerializer : public stream::CommandSerializer {
public:
  StateRestoreEndSerializer(const StateRestoreEndCommand& command) {}
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_INIT_END);
  }
};

class FrameEndSerializer : public stream::CommandSerializer {
public:
  FrameEndSerializer(const FrameEndCommand& command) {}
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_FRAME_END);
  }
};

class MarkerUInt64Serializer : public stream::CommandSerializer {
public:
  MarkerUInt64Serializer(const MarkerUInt64Command& command) {
    m_DataSize = getSize(command);
    m_Data.reset(new char[m_DataSize]);
    encode(command, m_Data.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_MARKER_UINT64);
  }
};

class IUnknownQueryInterfaceSerializer : public stream::CommandSerializer {
public:
  IUnknownQueryInterfaceSerializer(const IUnknownQueryInterfaceCommand& command) {
    m_DataSize = getSize(command);
    m_Data.reset(new char[m_DataSize]);
    encode(command, m_Data.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_IUNKNOWN_QUERYINTERFACE);
  }
};

class IUnknownAddRefSerializer : public stream::CommandSerializer {
public:
  IUnknownAddRefSerializer(const IUnknownAddRefCommand& command) {
    m_DataSize = getSize(command);
    m_Data.reset(new char[m_DataSize]);
    encode(command, m_Data.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_IUNKNOWN_ADDREF);
  }
};

class IUnknownReleaseSerializer : public stream::CommandSerializer {
public:
  IUnknownReleaseSerializer(const IUnknownReleaseCommand& command) {
    m_DataSize = getSize(command);
    m_Data.reset(new char[m_DataSize]);
    encode(command, m_Data.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_IUNKNOWN_RELEASE);
  }
};

class CreateWindowMetaSerializer : public stream::CommandSerializer {
public:
  CreateWindowMetaSerializer(const CreateWindowMetaCommand& command) {
    m_DataSize = getSize(command);
    m_Data.reset(new char[m_DataSize]);
    encode(command, m_Data.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_META_CREATE_WINDOW);
  }
};

class MappedDataMetaSerializer : public stream::CommandSerializer {
public:
  MappedDataMetaSerializer(const MappedDataMetaCommand& command) {
    m_DataSize = getSize(command);
    m_Data.reset(new char[m_DataSize]);
    encode(command, m_Data.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_MAPPED_DATA);
  }
};

class CreateHeapAllocationMetaSerializer : public stream::CommandSerializer {
public:
  CreateHeapAllocationMetaSerializer(const CreateHeapAllocationMetaCommand& command) {
    m_DataSize = getSize(command);
    m_Data.reset(new char[m_DataSize]);
    encode(command, m_Data.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_CREATE_HEAP_ALLOCATION);
  }
};

class WaitForFenceSignaledSerializer : public stream::CommandSerializer {
public:
  WaitForFenceSignaledSerializer(const WaitForFenceSignaledCommand& command) {
    m_DataSize = getSize(command);
    m_Data.reset(new char[m_DataSize]);
    encode(command, m_Data.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_WAIT_FOR_FENCE_SIGNALED);
  }
};

class DllContainerMetaSerializer : public stream::CommandSerializer {
public:
  DllContainerMetaSerializer(const DllContainerMetaCommand& command) {
    m_DataSize = getSize(command);
    m_Data.reset(new char[m_DataSize]);
    encode(command, m_Data.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_META_DLL_CONTAINER);
  }
};

class INTC_D3D12_GetSupportedVersionsSerializer : public stream::CommandSerializer {
public:
  INTC_D3D12_GetSupportedVersionsSerializer(const INTC_D3D12_GetSupportedVersionsCommand& command) {
    m_DataSize = getSize(command);
    m_Data.reset(new char[m_DataSize]);
    encode(command, m_Data.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::INTC_D3D12_GETSUPPORTEDVERSIONS);
  }
};

class INTC_D3D12_CreateDeviceExtensionContextSerializer : public stream::CommandSerializer {
public:
  INTC_D3D12_CreateDeviceExtensionContextSerializer(
      const INTC_D3D12_CreateDeviceExtensionContextCommand& command) {
    m_DataSize = getSize(command);
    m_Data.reset(new char[m_DataSize]);
    encode(command, m_Data.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::INTC_D3D12_CREATEDEVICEEXTENSIONCONTEXT);
  }
};

class INTC_D3D12_CreateDeviceExtensionContext1Serializer : public stream::CommandSerializer {
public:
  INTC_D3D12_CreateDeviceExtensionContext1Serializer(
      const INTC_D3D12_CreateDeviceExtensionContext1Command& command) {
    m_DataSize = getSize(command);
    m_Data.reset(new char[m_DataSize]);
    encode(command, m_Data.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::INTC_D3D12_CREATEDEVICEEXTENSIONCONTEXT1);
  }
};

class INTC_D3D12_SetApplicationInfoSerializer : public stream::CommandSerializer {
public:
  INTC_D3D12_SetApplicationInfoSerializer(const INTC_D3D12_SetApplicationInfoCommand& command) {
    m_DataSize = getSize(command);
    m_Data.reset(new char[m_DataSize]);
    encode(command, m_Data.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::INTC_D3D12_SETAPPLICATIONINFO);
  }
};

class INTC_DestroyDeviceExtensionContextSerializer : public stream::CommandSerializer {
public:
  INTC_DestroyDeviceExtensionContextSerializer(
      const INTC_DestroyDeviceExtensionContextCommand& command) {
    m_DataSize = getSize(command);
    m_Data.reset(new char[m_DataSize]);
    encode(command, m_Data.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::INTC_DESTROYDEVICEEXTENSIONCONTEXT);
  }
};

class INTC_D3D12_CheckFeatureSupportSerializer : public stream::CommandSerializer {
public:
  INTC_D3D12_CheckFeatureSupportSerializer(const INTC_D3D12_CheckFeatureSupportCommand& command) {
    m_DataSize = getSize(command);
    m_Data.reset(new char[m_DataSize]);
    encode(command, m_Data.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::INTC_D3D12_CHECKFEATURESUPPORT);
  }
};

class INTC_D3D12_CreateCommandQueueSerializer : public stream::CommandSerializer {
public:
  INTC_D3D12_CreateCommandQueueSerializer(const INTC_D3D12_CreateCommandQueueCommand& command) {
    m_DataSize = getSize(command);
    m_Data.reset(new char[m_DataSize]);
    encode(command, m_Data.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::INTC_D3D12_CREATECOMMANDQUEUE);
  }
};

class INTC_D3D12_CreateReservedResourceSerializer : public stream::CommandSerializer {
public:
  INTC_D3D12_CreateReservedResourceSerializer(
      const INTC_D3D12_CreateReservedResourceCommand& command) {
    m_DataSize = getSize(command);
    m_Data.reset(new char[m_DataSize]);
    encode(command, m_Data.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::INTC_D3D12_CREATERESERVEDRESOURCE);
  }
};

class INTC_D3D12_SetFeatureSupportSerializer : public stream::CommandSerializer {
public:
  INTC_D3D12_SetFeatureSupportSerializer(const INTC_D3D12_SetFeatureSupportCommand& command) {
    m_DataSize = getSize(command);
    m_Data.reset(new char[m_DataSize]);
    encode(command, m_Data.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::INTC_D3D12_SETFEATURESUPPORT);
  }
};

class INTC_D3D12_GetResourceAllocationInfoSerializer : public stream::CommandSerializer {
public:
  INTC_D3D12_GetResourceAllocationInfoSerializer(
      const INTC_D3D12_GetResourceAllocationInfoCommand& command) {
    m_DataSize = getSize(command);
    m_Data.reset(new char[m_DataSize]);
    encode(command, m_Data.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::INTC_D3D12_GETRESOURCEALLOCATIONINFO);
  }
};

class INTC_D3D12_CreateComputePipelineStateSerializer : public stream::CommandSerializer {
public:
  INTC_D3D12_CreateComputePipelineStateSerializer(
      const INTC_D3D12_CreateComputePipelineStateCommand& command) {
    m_DataSize = getSize(command);
    m_Data.reset(new char[m_DataSize]);
    encode(command, m_Data.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::INTC_D3D12_CREATECOMPUTEPIPELINESTATE);
  }
};

class INTC_D3D12_CreatePlacedResourceSerializer : public stream::CommandSerializer {
public:
  INTC_D3D12_CreatePlacedResourceSerializer(const INTC_D3D12_CreatePlacedResourceCommand& command) {
    m_DataSize = getSize(command);
    m_Data.reset(new char[m_DataSize]);
    encode(command, m_Data.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::INTC_D3D12_CREATEPLACEDRESOURCE);
  }
};

class INTC_D3D12_CreateCommittedResourceSerializer : public stream::CommandSerializer {
public:
  INTC_D3D12_CreateCommittedResourceSerializer(
      const INTC_D3D12_CreateCommittedResourceCommand& command) {
    m_DataSize = getSize(command);
    m_Data.reset(new char[m_DataSize]);
    encode(command, m_Data.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::INTC_D3D12_CREATECOMMITTEDRESOURCE);
  }
};

class INTC_D3D12_CreateHeapSerializer : public stream::CommandSerializer {
public:
  INTC_D3D12_CreateHeapSerializer(const INTC_D3D12_CreateHeapCommand& command) {
    m_DataSize = getSize(command);
    m_Data.reset(new char[m_DataSize]);
    encode(command, m_Data.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::INTC_D3D12_CREATEHEAP);
  }
};

class NvAPI_InitializeSerializer : public stream::CommandSerializer {
public:
  NvAPI_InitializeSerializer(const NvAPI_InitializeCommand& command) {
    m_DataSize = getSize(command);
    m_Data.reset(new char[m_DataSize]);
    encode(command, m_Data.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_NVAPI_INITIALIZE);
  }
};

class NvAPI_UnloadSerializer : public stream::CommandSerializer {
public:
  NvAPI_UnloadSerializer(const NvAPI_UnloadCommand& command) {
    m_DataSize = getSize(command);
    m_Data.reset(new char[m_DataSize]);
    encode(command, m_Data.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_NVAPI_UNLOAD);
  }
};

class NvAPI_D3D12_SetCreatePipelineStateOptionsSerializer : public stream::CommandSerializer {
public:
  NvAPI_D3D12_SetCreatePipelineStateOptionsSerializer(
      const NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& command) {
    m_DataSize = getSize(command);
    m_Data.reset(new char[m_DataSize]);
    encode(command, m_Data.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_NVAPI_D3D12_SETCREATEPIPELINESTATEOPTIONS);
  }
};

class NvAPI_D3D12_SetNvShaderExtnSlotSpaceSerializer : public stream::CommandSerializer {
public:
  NvAPI_D3D12_SetNvShaderExtnSlotSpaceSerializer(
      const NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& command) {
    m_DataSize = getSize(command);
    m_Data.reset(new char[m_DataSize]);
    encode(command, m_Data.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_NVAPI_D3D12_SETNVSHADEREXTNSLOTSPACE);
  }
};

class NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadSerializer : public stream::CommandSerializer {
public:
  NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadSerializer(
      const NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& command) {
    m_DataSize = getSize(command);
    m_Data.reset(new char[m_DataSize]);
    encode(command, m_Data.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_NVAPI_D3D12_SETNVSHADEREXTNSLOTSPACELOCALTHREAD);
  }
};

class NvAPI_D3D12_BuildRaytracingAccelerationStructureExSerializer
    : public stream::CommandSerializer {
public:
  NvAPI_D3D12_BuildRaytracingAccelerationStructureExSerializer(
      const NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command) {
    m_DataSize = getSize(command);
    m_Data.reset(new char[m_DataSize]);
    encode(command, m_Data.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_NVAPI_D3D12_BUILDRAYTRACINGACCELERATIONSTRUCTUREEX);
  }
};

class NvAPI_D3D12_BuildRaytracingOpacityMicromapArraySerializer : public stream::CommandSerializer {
public:
  NvAPI_D3D12_BuildRaytracingOpacityMicromapArraySerializer(
      const NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command) {
    m_DataSize = getSize(command);
    m_Data.reset(new char[m_DataSize]);
    encode(command, m_Data.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_NVAPI_D3D12_BUILDRAYTRACINGOPACITYMICROMAPARRAY);
  }
};

class NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationSerializer
    : public stream::CommandSerializer {
public:
  NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationSerializer(
      const NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& command) {
    m_DataSize = getSize(command);
    m_Data.reset(new char[m_DataSize]);
    encode(command, m_Data.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(
        CommandId::ID_NVAPI_D3D12_RAYTRACINGEXECUTEMULTIINDIRECTCLUSTEROPERATION);
  }
};

} // namespace DirectX
} // namespace gits
