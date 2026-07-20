// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "commandSerializersCustom.h"
#include "commandIdsAuto.h"
#include "commandEncodersCustom.h"

namespace gits {
namespace DirectX {

StateRestoreBeginSerializer::StateRestoreBeginSerializer(const StateRestoreBeginCommand& command) {}

unsigned StateRestoreBeginSerializer::Id() const {
  return static_cast<unsigned>(CommandId::ID_INIT_START);
}

StateRestoreEndSerializer::StateRestoreEndSerializer(const StateRestoreEndCommand& command) {}

unsigned StateRestoreEndSerializer::Id() const {
  return static_cast<unsigned>(CommandId::ID_INIT_END);
}

FrameEndSerializer::FrameEndSerializer(const FrameEndCommand& command) {}

unsigned FrameEndSerializer::Id() const {
  return static_cast<unsigned>(CommandId::ID_FRAME_END);
}

MarkerUInt64Serializer::MarkerUInt64Serializer(const MarkerUInt64Command& command) {
  m_DataSize = GetSize(command);
  m_Data.reset(new char[m_DataSize]);
  Encode(command, m_Data.get());
}

unsigned MarkerUInt64Serializer::Id() const {
  return static_cast<unsigned>(CommandId::ID_MARKER_UINT64);
}

IUnknownQueryInterfaceSerializer::IUnknownQueryInterfaceSerializer(
    const IUnknownQueryInterfaceCommand& command) {
  m_DataSize = GetSize(command);
  m_Data.reset(new char[m_DataSize]);
  Encode(command, m_Data.get());
}

unsigned IUnknownQueryInterfaceSerializer::Id() const {
  return static_cast<unsigned>(CommandId::ID_IUNKNOWN_QUERYINTERFACE);
}

IUnknownAddRefSerializer::IUnknownAddRefSerializer(const IUnknownAddRefCommand& command) {
  m_DataSize = GetSize(command);
  m_Data.reset(new char[m_DataSize]);
  Encode(command, m_Data.get());
}

unsigned IUnknownAddRefSerializer::Id() const {
  return static_cast<unsigned>(CommandId::ID_IUNKNOWN_ADDREF);
}

IUnknownReleaseSerializer::IUnknownReleaseSerializer(const IUnknownReleaseCommand& command) {
  m_DataSize = GetSize(command);
  m_Data.reset(new char[m_DataSize]);
  Encode(command, m_Data.get());
}

unsigned IUnknownReleaseSerializer::Id() const {
  return static_cast<unsigned>(CommandId::ID_IUNKNOWN_RELEASE);
}

CreateWindowMetaSerializer::CreateWindowMetaSerializer(const CreateWindowMetaCommand& command) {
  m_DataSize = GetSize(command);
  m_Data.reset(new char[m_DataSize]);
  Encode(command, m_Data.get());
}

unsigned CreateWindowMetaSerializer::Id() const {
  return static_cast<unsigned>(CommandId::ID_META_CREATE_WINDOW);
}

MappedDataMetaSerializer::MappedDataMetaSerializer(const MappedDataMetaCommand& command) {
  m_DataSize = GetSize(command);
  m_Data.reset(new char[m_DataSize]);
  Encode(command, m_Data.get());
}

unsigned MappedDataMetaSerializer::Id() const {
  return static_cast<unsigned>(CommandId::ID_MAPPED_DATA);
}

CreateHeapAllocationMetaSerializer::CreateHeapAllocationMetaSerializer(
    const CreateHeapAllocationMetaCommand& command) {
  m_DataSize = GetSize(command);
  m_Data.reset(new char[m_DataSize]);
  Encode(command, m_Data.get());
}

unsigned CreateHeapAllocationMetaSerializer::Id() const {
  return static_cast<unsigned>(CommandId::ID_CREATE_HEAP_ALLOCATION);
}

WaitForFenceSignaledDeprecatedSerializer::WaitForFenceSignaledDeprecatedSerializer(
    const WaitForFenceSignaledDeprecatedCommand& command) {
  m_DataSize = GetSize(command);
  m_Data.reset(new char[m_DataSize]);
  Encode(command, m_Data.get());
}

unsigned WaitForFenceSignaledDeprecatedSerializer::Id() const {
  return static_cast<unsigned>(CommandId::ID_WAIT_FOR_FENCE_SIGNALED_DEPRECATED);
}

WaitForFenceSignaledSerializer::WaitForFenceSignaledSerializer(
    const WaitForFenceSignaledCommand& command) {
  m_DataSize = GetSize(command);
  m_Data.reset(new char[m_DataSize]);
  Encode(command, m_Data.get());
}

unsigned WaitForFenceSignaledSerializer::Id() const {
  return static_cast<unsigned>(CommandId::ID_WAIT_FOR_FENCE_SIGNALED);
}

DllContainerMetaSerializer::DllContainerMetaSerializer(const DllContainerMetaCommand& command) {
  m_DataSize = GetSize(command);
  m_Data.reset(new char[m_DataSize]);
  Encode(command, m_Data.get());
}

unsigned DllContainerMetaSerializer::Id() const {
  return static_cast<unsigned>(CommandId::ID_META_DLL_CONTAINER);
}

INTC_D3D12_GetSupportedVersionsSerializer::INTC_D3D12_GetSupportedVersionsSerializer(
    const INTC_D3D12_GetSupportedVersionsCommand& command) {
  m_DataSize = GetSize(command);
  m_Data.reset(new char[m_DataSize]);
  Encode(command, m_Data.get());
}

unsigned INTC_D3D12_GetSupportedVersionsSerializer::Id() const {
  return static_cast<unsigned>(CommandId::INTC_D3D12_GETSUPPORTEDVERSIONS);
}

INTC_D3D12_CreateDeviceExtensionContextSerializer::
    INTC_D3D12_CreateDeviceExtensionContextSerializer(
        const INTC_D3D12_CreateDeviceExtensionContextCommand& command) {
  m_DataSize = GetSize(command);
  m_Data.reset(new char[m_DataSize]);
  Encode(command, m_Data.get());
}

unsigned INTC_D3D12_CreateDeviceExtensionContextSerializer::Id() const {
  return static_cast<unsigned>(CommandId::INTC_D3D12_CREATEDEVICEEXTENSIONCONTEXT);
}

INTC_D3D12_CreateDeviceExtensionContext1Serializer::
    INTC_D3D12_CreateDeviceExtensionContext1Serializer(
        const INTC_D3D12_CreateDeviceExtensionContext1Command& command) {
  m_DataSize = GetSize(command);
  m_Data.reset(new char[m_DataSize]);
  Encode(command, m_Data.get());
}

unsigned INTC_D3D12_CreateDeviceExtensionContext1Serializer::Id() const {
  return static_cast<unsigned>(CommandId::INTC_D3D12_CREATEDEVICEEXTENSIONCONTEXT1);
}

INTC_D3D12_SetApplicationInfoSerializer::INTC_D3D12_SetApplicationInfoSerializer(
    const INTC_D3D12_SetApplicationInfoCommand& command) {
  m_DataSize = GetSize(command);
  m_Data.reset(new char[m_DataSize]);
  Encode(command, m_Data.get());
}

unsigned INTC_D3D12_SetApplicationInfoSerializer::Id() const {
  return static_cast<unsigned>(CommandId::INTC_D3D12_SETAPPLICATIONINFO);
}

INTC_DestroyDeviceExtensionContextSerializer::INTC_DestroyDeviceExtensionContextSerializer(
    const INTC_DestroyDeviceExtensionContextCommand& command) {
  m_DataSize = GetSize(command);
  m_Data.reset(new char[m_DataSize]);
  Encode(command, m_Data.get());
}

unsigned INTC_DestroyDeviceExtensionContextSerializer::Id() const {
  return static_cast<unsigned>(CommandId::INTC_DESTROYDEVICEEXTENSIONCONTEXT);
}

INTC_D3D12_CheckFeatureSupportSerializer::INTC_D3D12_CheckFeatureSupportSerializer(
    const INTC_D3D12_CheckFeatureSupportCommand& command) {
  m_DataSize = GetSize(command);
  m_Data.reset(new char[m_DataSize]);
  Encode(command, m_Data.get());
}

unsigned INTC_D3D12_CheckFeatureSupportSerializer::Id() const {
  return static_cast<unsigned>(CommandId::INTC_D3D12_CHECKFEATURESUPPORT);
}

INTC_D3D12_CreateCommandQueueSerializer::INTC_D3D12_CreateCommandQueueSerializer(
    const INTC_D3D12_CreateCommandQueueCommand& command) {
  m_DataSize = GetSize(command);
  m_Data.reset(new char[m_DataSize]);
  Encode(command, m_Data.get());
}

unsigned INTC_D3D12_CreateCommandQueueSerializer::Id() const {
  return static_cast<unsigned>(CommandId::INTC_D3D12_CREATECOMMANDQUEUE);
}

INTC_D3D12_CreateReservedResourceSerializer::INTC_D3D12_CreateReservedResourceSerializer(
    const INTC_D3D12_CreateReservedResourceCommand& command) {
  m_DataSize = GetSize(command);
  m_Data.reset(new char[m_DataSize]);
  Encode(command, m_Data.get());
}

unsigned INTC_D3D12_CreateReservedResourceSerializer::Id() const {
  return static_cast<unsigned>(CommandId::INTC_D3D12_CREATERESERVEDRESOURCE);
}

INTC_D3D12_SetFeatureSupportSerializer::INTC_D3D12_SetFeatureSupportSerializer(
    const INTC_D3D12_SetFeatureSupportCommand& command) {
  m_DataSize = GetSize(command);
  m_Data.reset(new char[m_DataSize]);
  Encode(command, m_Data.get());
}

unsigned INTC_D3D12_SetFeatureSupportSerializer::Id() const {
  return static_cast<unsigned>(CommandId::INTC_D3D12_SETFEATURESUPPORT);
}

INTC_D3D12_GetResourceAllocationInfoSerializer::INTC_D3D12_GetResourceAllocationInfoSerializer(
    const INTC_D3D12_GetResourceAllocationInfoCommand& command) {
  m_DataSize = GetSize(command);
  m_Data.reset(new char[m_DataSize]);
  Encode(command, m_Data.get());
}

unsigned INTC_D3D12_GetResourceAllocationInfoSerializer::Id() const {
  return static_cast<unsigned>(CommandId::INTC_D3D12_GETRESOURCEALLOCATIONINFO);
}

INTC_D3D12_CreateComputePipelineStateSerializer::INTC_D3D12_CreateComputePipelineStateSerializer(
    const INTC_D3D12_CreateComputePipelineStateCommand& command) {
  m_DataSize = GetSize(command);
  m_Data.reset(new char[m_DataSize]);
  Encode(command, m_Data.get());
}

unsigned INTC_D3D12_CreateComputePipelineStateSerializer::Id() const {
  return static_cast<unsigned>(CommandId::INTC_D3D12_CREATECOMPUTEPIPELINESTATE);
}

INTC_D3D12_CreatePlacedResourceSerializer::INTC_D3D12_CreatePlacedResourceSerializer(
    const INTC_D3D12_CreatePlacedResourceCommand& command) {
  m_DataSize = GetSize(command);
  m_Data.reset(new char[m_DataSize]);
  Encode(command, m_Data.get());
}

unsigned INTC_D3D12_CreatePlacedResourceSerializer::Id() const {
  return static_cast<unsigned>(CommandId::INTC_D3D12_CREATEPLACEDRESOURCE);
}

INTC_D3D12_CreateCommittedResourceSerializer::INTC_D3D12_CreateCommittedResourceSerializer(
    const INTC_D3D12_CreateCommittedResourceCommand& command) {
  m_DataSize = GetSize(command);
  m_Data.reset(new char[m_DataSize]);
  Encode(command, m_Data.get());
}

unsigned INTC_D3D12_CreateCommittedResourceSerializer::Id() const {
  return static_cast<unsigned>(CommandId::INTC_D3D12_CREATECOMMITTEDRESOURCE);
}

INTC_D3D12_CreateHeapSerializer::INTC_D3D12_CreateHeapSerializer(
    const INTC_D3D12_CreateHeapCommand& command) {
  m_DataSize = GetSize(command);
  m_Data.reset(new char[m_DataSize]);
  Encode(command, m_Data.get());
}

unsigned INTC_D3D12_CreateHeapSerializer::Id() const {
  return static_cast<unsigned>(CommandId::INTC_D3D12_CREATEHEAP);
}

NvAPI_InitializeSerializer::NvAPI_InitializeSerializer(const NvAPI_InitializeCommand& command) {
  m_DataSize = GetSize(command);
  m_Data.reset(new char[m_DataSize]);
  Encode(command, m_Data.get());
}

unsigned NvAPI_InitializeSerializer::Id() const {
  return static_cast<unsigned>(CommandId::ID_NVAPI_INITIALIZE);
}

NvAPI_UnloadSerializer::NvAPI_UnloadSerializer(const NvAPI_UnloadCommand& command) {
  m_DataSize = GetSize(command);
  m_Data.reset(new char[m_DataSize]);
  Encode(command, m_Data.get());
}

unsigned NvAPI_UnloadSerializer::Id() const {
  return static_cast<unsigned>(CommandId::ID_NVAPI_UNLOAD);
}

NvAPI_D3D12_SetCreatePipelineStateOptionsSerializer::
    NvAPI_D3D12_SetCreatePipelineStateOptionsSerializer(
        const NvAPI_D3D12_SetCreatePipelineStateOptionsCommand& command) {
  m_DataSize = GetSize(command);
  m_Data.reset(new char[m_DataSize]);
  Encode(command, m_Data.get());
}

unsigned NvAPI_D3D12_SetCreatePipelineStateOptionsSerializer::Id() const {
  return static_cast<unsigned>(CommandId::ID_NVAPI_D3D12_SETCREATEPIPELINESTATEOPTIONS);
}

NvAPI_D3D12_SetNvShaderExtnSlotSpaceSerializer::NvAPI_D3D12_SetNvShaderExtnSlotSpaceSerializer(
    const NvAPI_D3D12_SetNvShaderExtnSlotSpaceCommand& command) {
  m_DataSize = GetSize(command);
  m_Data.reset(new char[m_DataSize]);
  Encode(command, m_Data.get());
}

unsigned NvAPI_D3D12_SetNvShaderExtnSlotSpaceSerializer::Id() const {
  return static_cast<unsigned>(CommandId::ID_NVAPI_D3D12_SETNVSHADEREXTNSLOTSPACE);
}

NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadSerializer::
    NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadSerializer(
        const NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadCommand& command) {
  m_DataSize = GetSize(command);
  m_Data.reset(new char[m_DataSize]);
  Encode(command, m_Data.get());
}

unsigned NvAPI_D3D12_SetNvShaderExtnSlotSpaceLocalThreadSerializer::Id() const {
  return static_cast<unsigned>(CommandId::ID_NVAPI_D3D12_SETNVSHADEREXTNSLOTSPACELOCALTHREAD);
}

NvAPI_D3D12_BuildRaytracingAccelerationStructureExSerializer::
    NvAPI_D3D12_BuildRaytracingAccelerationStructureExSerializer(
        const NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& command) {
  m_DataSize = GetSize(command);
  m_Data.reset(new char[m_DataSize]);
  Encode(command, m_Data.get());
}

unsigned NvAPI_D3D12_BuildRaytracingAccelerationStructureExSerializer::Id() const {
  return static_cast<unsigned>(CommandId::ID_NVAPI_D3D12_BUILDRAYTRACINGACCELERATIONSTRUCTUREEX);
}

NvAPI_D3D12_BuildRaytracingOpacityMicromapArraySerializer::
    NvAPI_D3D12_BuildRaytracingOpacityMicromapArraySerializer(
        const NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& command) {
  m_DataSize = GetSize(command);
  m_Data.reset(new char[m_DataSize]);
  Encode(command, m_Data.get());
}

unsigned NvAPI_D3D12_BuildRaytracingOpacityMicromapArraySerializer::Id() const {
  return static_cast<unsigned>(CommandId::ID_NVAPI_D3D12_BUILDRAYTRACINGOPACITYMICROMAPARRAY);
}

NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationSerializer::
    NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationSerializer(
        const NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationCommand& command) {
  m_DataSize = GetSize(command);
  m_Data.reset(new char[m_DataSize]);
  Encode(command, m_Data.get());
}

unsigned NvAPI_D3D12_RaytracingExecuteMultiIndirectClusterOperationSerializer::Id() const {
  return static_cast<unsigned>(
      CommandId::ID_NVAPI_D3D12_RAYTRACINGEXECUTEMULTIINDIRECTCLUSTEROPERATION);
}

} // namespace DirectX
} // namespace gits
