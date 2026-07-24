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

namespace gits {
namespace DirectX {

class StateRestoreBeginSerializer : public stream::CommandSerializer {
public:
  explicit StateRestoreBeginSerializer(const StateRestoreBeginCommand& command);
  unsigned Id() const override;
};

class StateRestoreEndSerializer : public stream::CommandSerializer {
public:
  explicit StateRestoreEndSerializer(const StateRestoreEndCommand& command);
  unsigned Id() const override;
};

class FrameEndSerializer : public stream::CommandSerializer {
public:
  explicit FrameEndSerializer(const FrameEndCommand& command);
  unsigned Id() const override;
};

class MarkerUInt64Serializer : public stream::CommandSerializer {
public:
  explicit MarkerUInt64Serializer(const MarkerUInt64Command& command);
  unsigned Id() const override;
};

class IUnknownQueryInterfaceSerializer : public stream::CommandSerializer {
public:
  explicit IUnknownQueryInterfaceSerializer(const IUnknownQueryInterfaceCommand& command);
  unsigned Id() const override;
};

class IUnknownAddRefSerializer : public stream::CommandSerializer {
public:
  explicit IUnknownAddRefSerializer(const IUnknownAddRefCommand& command);
  unsigned Id() const override;
};

class IUnknownReleaseSerializer : public stream::CommandSerializer {
public:
  explicit IUnknownReleaseSerializer(const IUnknownReleaseCommand& command);
  unsigned Id() const override;
};

class CreateWindowMetaSerializer : public stream::CommandSerializer {
public:
  explicit CreateWindowMetaSerializer(const CreateWindowMetaCommand& command);
  unsigned Id() const override;
};

class MappedDataMetaSerializer : public stream::CommandSerializer {
public:
  explicit MappedDataMetaSerializer(const MappedDataMetaCommand& command);
  unsigned Id() const override;
};

class CreateHeapAllocationMetaSerializer : public stream::CommandSerializer {
public:
  explicit CreateHeapAllocationMetaSerializer(const CreateHeapAllocationMetaCommand& command);
  unsigned Id() const override;
};

class WaitForFenceSignaledSerializer : public stream::CommandSerializer {
public:
  explicit WaitForFenceSignaledSerializer(const WaitForFenceSignaledCommand& command);
  unsigned Id() const override;
};

class DllContainerMetaSerializer : public stream::CommandSerializer {
public:
  explicit DllContainerMetaSerializer(const DllContainerMetaCommand& command);
  unsigned Id() const override;
};

class INTC_D3D12_GetSupportedVersionsSerializer : public stream::CommandSerializer {
public:
  explicit INTC_D3D12_GetSupportedVersionsSerializer(
      const INTC_D3D12_GetSupportedVersionsCommand& command);
  unsigned Id() const override;
};

class INTC_D3D12_CreateDeviceExtensionContextSerializer : public stream::CommandSerializer {
public:
  explicit INTC_D3D12_CreateDeviceExtensionContextSerializer(
      const INTC_D3D12_CreateDeviceExtensionContextCommand& command);
  unsigned Id() const override;
};

class INTC_D3D12_CreateDeviceExtensionContext1Serializer : public stream::CommandSerializer {
public:
  explicit INTC_D3D12_CreateDeviceExtensionContext1Serializer(
      const INTC_D3D12_CreateDeviceExtensionContext1Command& command);
  unsigned Id() const override;
};

class INTC_D3D12_SetApplicationInfoSerializer : public stream::CommandSerializer {
public:
  explicit INTC_D3D12_SetApplicationInfoSerializer(
      const INTC_D3D12_SetApplicationInfoCommand& command);
  unsigned Id() const override;
};

class INTC_DestroyDeviceExtensionContextSerializer : public stream::CommandSerializer {
public:
  explicit INTC_DestroyDeviceExtensionContextSerializer(
      const INTC_DestroyDeviceExtensionContextCommand& command);
  unsigned Id() const override;
};

class INTC_D3D12_CheckFeatureSupportSerializer : public stream::CommandSerializer {
public:
  explicit INTC_D3D12_CheckFeatureSupportSerializer(
      const INTC_D3D12_CheckFeatureSupportCommand& command);
  unsigned Id() const override;
};

class INTC_D3D12_CreateCommandQueueSerializer : public stream::CommandSerializer {
public:
  explicit INTC_D3D12_CreateCommandQueueSerializer(
      const INTC_D3D12_CreateCommandQueueCommand& command);
  unsigned Id() const override;
};

class INTC_D3D12_CreateReservedResourceSerializer : public stream::CommandSerializer {
public:
  explicit INTC_D3D12_CreateReservedResourceSerializer(
      const INTC_D3D12_CreateReservedResourceCommand& command);
  unsigned Id() const override;
};

class INTC_D3D12_SetFeatureSupportSerializer : public stream::CommandSerializer {
public:
  explicit INTC_D3D12_SetFeatureSupportSerializer(
      const INTC_D3D12_SetFeatureSupportCommand& command);
  unsigned Id() const override;
};

class INTC_D3D12_GetResourceAllocationInfoSerializer : public stream::CommandSerializer {
public:
  explicit INTC_D3D12_GetResourceAllocationInfoSerializer(
      const INTC_D3D12_GetResourceAllocationInfoCommand& command);
  unsigned Id() const override;
};

class INTC_D3D12_CreateComputePipelineStateSerializer : public stream::CommandSerializer {
public:
  explicit INTC_D3D12_CreateComputePipelineStateSerializer(
      const INTC_D3D12_CreateComputePipelineStateCommand& command);
  unsigned Id() const override;
};

class INTC_D3D12_CreatePlacedResourceSerializer : public stream::CommandSerializer {
public:
  explicit INTC_D3D12_CreatePlacedResourceSerializer(
      const INTC_D3D12_CreatePlacedResourceCommand& command);
  unsigned Id() const override;
};

class INTC_D3D12_CreateCommittedResourceSerializer : public stream::CommandSerializer {
public:
  explicit INTC_D3D12_CreateCommittedResourceSerializer(
      const INTC_D3D12_CreateCommittedResourceCommand& command);
  unsigned Id() const override;
};

class INTC_D3D12_CreateHeapSerializer : public stream::CommandSerializer {
public:
  explicit INTC_D3D12_CreateHeapSerializer(const INTC_D3D12_CreateHeapCommand& command);
  unsigned Id() const override;
};

class NvAPI_InitializeSerializer : public stream::CommandSerializer {
public:
  explicit NvAPI_InitializeSerializer(const NvAPI_InitializeCommand& command);
  unsigned Id() const override;
};

class NvAPI_UnloadSerializer : public stream::CommandSerializer {
public:
  explicit NvAPI_UnloadSerializer(const NvAPI_UnloadCommand& command);
  unsigned Id() const override;
};

class NvAPI_D3D12_SetCreatePipelineStateOptionsSerializer : public stream::CommandSerializer {
public:
  explicit NvAPI_D3D12_SetCreatePipelineStateOptionsSerializer(
      const NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& command);
  unsigned Id() const override;
};

class NvAPI_D3D12_SetNvShaderExtnSlotSpaceSerializer : public stream::CommandSerializer {
public:
  explicit NvAPI_D3D12_SetNvShaderExtnSlotSpaceSerializer(
      const NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& command);
  unsigned Id() const override;
};

class NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadSerializer : public stream::CommandSerializer {
public:
  explicit NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadSerializer(
      const NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& command);
  unsigned Id() const override;
};

class NvAPI_D3D12_BuildRaytracingAccelerationStructureExSerializer
    : public stream::CommandSerializer {
public:
  explicit NvAPI_D3D12_BuildRaytracingAccelerationStructureExSerializer(
      const NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command);
  unsigned Id() const override;
};

class NvAPI_D3D12_BuildRaytracingOpacityMicromapArraySerializer : public stream::CommandSerializer {
public:
  explicit NvAPI_D3D12_BuildRaytracingOpacityMicromapArraySerializer(
      const NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command);
  unsigned Id() const override;
};

class NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationSerializer
    : public stream::CommandSerializer {
public:
  explicit NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationSerializer(
      const NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& command);
  unsigned Id() const override;
};

} // namespace DirectX
} // namespace gits
