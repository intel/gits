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

  void PreBuildRTAS(ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& command);
  void PreDispatchRays(ID3D12GraphicsCommandList4DispatchRaysCommand& command);
  void PreExecuteIndirect(ID3D12GraphicsCommandListExecuteIndirectCommand& command);
  void PostExecuteIndirect(ID3D12GraphicsCommandListExecuteIndirectCommand& command);
  void PreExecute(ID3D12CommandQueueExecuteCommandListsCommand& command);
  void PostExecute(ID3D12CommandQueueExecuteCommandListsCommand& command);

  void ReleaseObject(IUnknownReleaseCommand& command);
  void PreGetShaderId(ID3D12StateObjectPropertiesGetShaderIdentifierCommand& command);
  void PostGetShaderId(ID3D12StateObjectPropertiesGetShaderIdentifierCommand& command);
  void PreGetDescriptorHandle(
      ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& command);
  void PostGetDescriptorHandle(
      ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& command);
  void CreateCommandSignature(unsigned commandSignatureKey,
                              const PointerArgument<D3D12_COMMAND_SIGNATURE_DESC>& desc);

private:
  struct FenceInfo {
    ID3D12Fence* Fence{};
    UINT64 FenceValue{};
    bool WaitingForExecute{};
  };

  struct PatchInfo {
    enum Type {
      Build,
      DispatchRays,
      ExecuteIndirect,
    };
    Type Type{};
    unsigned PatchBufferIndex{};
    std::unique_ptr<Command> Command;
  };

  void Initialize(ID3D12GraphicsCommandList* commandList);
  size_t Align(size_t value, size_t alignment);
  void AddPatchBuffer(ID3D12GraphicsCommandList* commandList);
  unsigned GetPatchBufferIndex(unsigned commandListKey, ID3D12GraphicsCommandList* commandList);
  void PatchBuild(
      const PatchInfo& patchInfo,
      const std::vector<CapturePlayerGpuAddressService::GpuAddressMapping>& gpuAddressMappings);
  void PatchDispatchRays(
      const PatchInfo& patchInfo,
      const std::vector<CapturePlayerGpuAddressService::GpuAddressMapping>& gpuAddressMappings);
  void PatchExecuteIndirect(
      const PatchInfo& patchInfo,
      const std::vector<CapturePlayerGpuAddressService::GpuAddressMapping>& gpuAddressMappings);
  std::vector<char> ReadFile(std::filesystem::path path);
  UINT64 GetPlayerAddress(
      UINT64 captureAddress,
      const std::vector<CapturePlayerGpuAddressService::GpuAddressMapping>& gpuAddressMappings);
  void LoadExecuteIndirectDispatchRays();

  const Configuration& m_GitsConfig;
  std::filesystem::path m_Path;
  bool m_Initialized{};

  std::vector<FenceInfo> m_PatchBufferFences{};
  std::unordered_map<unsigned, std::vector<unsigned>> m_CurrentPatchBuffersByCommandList;

  static const unsigned PatchBufferInitialPoolSize{16};
  unsigned m_PatchBufferPoolSize{};
  std::vector<ID3D12Resource*> m_PatchBuffers{};
  unsigned m_PatchBufferSize{};

  std::unordered_map<unsigned, std::vector<std::unique_ptr<PatchInfo>>> m_PatchInfoByCommandList;

  std::unordered_map<unsigned, std::unique_ptr<PointerArgument<D3D12_COMMAND_SIGNATURE_DESC>>>
      m_CommandSignatures;
  std::unordered_map<unsigned, std::vector<D3D12_DISPATCH_RAYS_DESC>> m_ExecuteIndirectDispatchRays;
  UINT64 m_ExecuteIndirectLastArgumentBufferOffset{};

  CapturePlayerGpuAddressService& m_AddressService;
  ShaderIdentifierService m_ShaderIdentifierService;
  DescriptorHandleService m_DescriptorHandleService;
};

} // namespace DirectX
} // namespace gits
