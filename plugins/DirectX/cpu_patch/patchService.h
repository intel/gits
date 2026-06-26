// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "configurationLib.h"
#include "commandsAuto.h"
#include "commandsCustom.h"
#include "capturePlayerGpuAddressService.h"
#include "shaderIdentifierService.h"
#include "descriptorHandleService.h"

#include <d3d12.h>
#include <vector>
#include <filesystem>

namespace gits {
namespace DirectX {

class PatchService {
public:
  PatchService(const Configuration& gitsConfig, CapturePlayerGpuAddressService& addressService);

  PatchService(const PatchService&) = delete;
  PatchService& operator=(const PatchService&) = delete;

  void preBuildRTAS(ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& command);
  void preDispatchRays(ID3D12GraphicsCommandList4DispatchRaysCommand& command);
  void preExecuteIndirect(ID3D12GraphicsCommandListExecuteIndirectCommand& command);
  void postExecuteIndirect(ID3D12GraphicsCommandListExecuteIndirectCommand& command);
  void preExecute(ID3D12CommandQueueExecuteCommandListsCommand& command);
  void postExecute(ID3D12CommandQueueExecuteCommandListsCommand& command);

  void releaseObject(IUnknownReleaseCommand& command);
  void preGetShaderId(ID3D12StateObjectPropertiesGetShaderIdentifierCommand& command);
  void postGetShaderId(ID3D12StateObjectPropertiesGetShaderIdentifierCommand& command);
  void preGetDescriptorHandle(
      ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& command);
  void postGetDescriptorHandle(
      ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& command);
  void createCommandSignature(unsigned commandSignatureKey,
                              const PointerArgument<D3D12_COMMAND_SIGNATURE_DESC>& desc);

private:
  struct FenceInfo {
    ID3D12Fence* fence{};
    UINT64 fenceValue{};
    bool waitingForExecute{};
  };

  struct PatchInfo {
    enum Type {
      Build,
      DispatchRays,
      ExecuteIndirect,
    };
    Type type{};
    unsigned patchBufferIndex{};
    std::unique_ptr<Command> command;
  };

  void initialize(ID3D12GraphicsCommandList* commandList);
  size_t align(size_t value, size_t alignment);
  void addPatchBuffer(ID3D12GraphicsCommandList* commandList);
  unsigned getPatchBufferIndex(unsigned commandListKey, ID3D12GraphicsCommandList* commandList);
  void patchBuild(
      const PatchInfo& patchInfo,
      const std::vector<CapturePlayerGpuAddressService::GpuAddressMapping>& gpuAddressMappings);
  void patchDispatchRays(
      const PatchInfo& patchInfo,
      const std::vector<CapturePlayerGpuAddressService::GpuAddressMapping>& gpuAddressMappings);
  void patchExecuteIndirect(
      const PatchInfo& patchInfo,
      const std::vector<CapturePlayerGpuAddressService::GpuAddressMapping>& gpuAddressMappings);
  std::vector<char> readFile(std::filesystem::path path);
  UINT64 getPlayerAddress(
      UINT64 captureAddress,
      const std::vector<CapturePlayerGpuAddressService::GpuAddressMapping>& gpuAddressMappings);
  void loadExecuteIndirectDispatchRays();

  const Configuration& gitsConfig_;
  std::filesystem::path path_;
  bool initialized_{};

  std::vector<FenceInfo> patchBufferFences_{};
  std::unordered_map<unsigned, std::vector<unsigned>> currentPatchBuffersByCommandList_;

  static const unsigned patchBufferInitialPoolSize_{16};
  unsigned patchBufferPoolSize_{};
  std::vector<ID3D12Resource*> patchBuffers_{};
  unsigned patchBufferSize_{};

  std::unordered_map<unsigned, std::vector<std::unique_ptr<PatchInfo>>> patchInfoByCommandList_;

  std::unordered_map<unsigned, std::unique_ptr<PointerArgument<D3D12_COMMAND_SIGNATURE_DESC>>>
      commandSignatures_;
  std::unordered_map<unsigned, std::vector<D3D12_DISPATCH_RAYS_DESC>> executeIndirectDispatchRays_;
  UINT64 executeIndirectLastArgumentBufferOffset_{};

  CapturePlayerGpuAddressService& addressService_;
  ShaderIdentifierService shaderIdentifierService_;
  DescriptorHandleService descriptorHandleService_;
};

} // namespace DirectX
} // namespace gits
