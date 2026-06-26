// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "capturePlayerGpuAddressService.h"
#include "capturePlayerShaderIdentifierService.h"
#include "capturePlayerDescriptorHandleService.h"

#include <d3d12.h>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace directx {

struct CpuPatchServices {
  CapturePlayerGpuAddressService& GpuAddress;
  CapturePlayerShaderIdentifierService& ShaderIdentifiers;
  CapturePlayerDescriptorHandleService& DescriptorHandles;
};

class PatchService {
public:
  PatchService(std::filesystem::path dataDir, CpuPatchServices services);
  static PatchService& Get();

  PatchService(const PatchService&) = delete;
  PatchService& operator=(const PatchService&) = delete;

  void PreBuildRaytracingAccelerationStructure(
      unsigned commandListKey,
      unsigned commandKey,
      ID3D12GraphicsCommandList* commandList,
      D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* desc);
  void PreDispatchRays(unsigned commandListKey,
                       unsigned commandKey,
                       ID3D12GraphicsCommandList* commandList,
                       D3D12_DISPATCH_RAYS_DESC* desc);
  void PreExecuteIndirect(unsigned commandListKey,
                          unsigned commandKey,
                          ID3D12GraphicsCommandList* commandList,
                          unsigned commandSignatureKey,
                          ID3D12Resource** argumentBuffer,
                          UINT64* argumentBufferOffset,
                          UINT maxCommandCount);
  void PostExecuteIndirect(UINT64* argumentBufferOffset, UINT64 restoreOffset);
  void PreExecuteCommandLists(const unsigned* commandListKeys, size_t commandListCount);
  void PostExecuteCommandLists(ID3D12CommandQueue* queue,
                               const unsigned* commandListKeys,
                               size_t commandListCount);
  void RegisterCommandSignature(unsigned commandSignatureKey,
                                const D3D12_COMMAND_SIGNATURE_DESC& desc);

private:
  struct FenceInfo {
    ID3D12Fence* fence{};
    UINT64 fenceValue{};
    bool waitingForExecute{};
  };

  struct PendingPatch {
    enum class Type {
      Build,
      DispatchRays,
      ExecuteIndirect,
    };
    Type type{};
    unsigned commandKey{};
    unsigned commandListKey{};
    unsigned patchBufferIndex{};
    unsigned numInstanceDescs{};
    D3D12_DISPATCH_RAYS_DESC dispatchRaysDesc{};
    unsigned commandSignatureKey{};
    UINT64 argumentBufferOffset{};
    UINT maxCommandCount{};
    ID3D12GraphicsCommandList* commandList{};
  };

  struct OwnedCommandSignatureDesc {
    D3D12_COMMAND_SIGNATURE_DESC desc{};
    std::vector<D3D12_INDIRECT_ARGUMENT_DESC> argumentDescs;
  };

  void Initialize(ID3D12GraphicsCommandList* commandList);
  static size_t Align(size_t value, size_t alignment);
  void AddPatchBuffer(ID3D12GraphicsCommandList* commandList);
  unsigned GetPatchBufferIndex(unsigned commandListKey, ID3D12GraphicsCommandList* commandList);
  void PatchBuild(
      const PendingPatch& patchInfo,
      const std::vector<CapturePlayerGpuAddressService::GpuAddressMapping>& gpuAddressMappings);
  void PatchDispatchRays(
      const PendingPatch& patchInfo,
      const std::vector<CapturePlayerGpuAddressService::GpuAddressMapping>& gpuAddressMappings);
  void PatchExecuteIndirect(
      const PendingPatch& patchInfo,
      const std::vector<CapturePlayerGpuAddressService::GpuAddressMapping>& gpuAddressMappings);
  void PatchBindingTableRecords(
      std::vector<char>& patchedData,
      unsigned stride,
      const std::vector<CapturePlayerGpuAddressService::GpuAddressMapping>& gpuAddressMappings);
  std::vector<char> ReadPatchBinaryFile(const std::filesystem::path& path);
  static UINT64 GetPlayerAddress(
      UINT64 captureAddress,
      const std::vector<CapturePlayerGpuAddressService::GpuAddressMapping>& gpuAddressMappings);
  void LoadExecuteIndirectDispatchRays();

  std::filesystem::path m_DataDir;
  std::filesystem::path m_CpuPatchDir;
  CpuPatchServices m_Services;
  bool m_Initialized{};

  std::vector<FenceInfo> m_PatchBufferFences;
  std::unordered_map<unsigned, std::vector<unsigned>> m_CurrentPatchBuffersByCommandList;

  static constexpr unsigned m_PatchBufferInitialPoolSize = 16;
  unsigned m_PatchBufferPoolSize{};
  std::vector<ID3D12Resource*> m_PatchBuffers;
  unsigned m_PatchBufferSize{};

  std::unordered_map<unsigned, std::vector<PendingPatch>> m_PendingPatchesByCommandList;

  std::unordered_map<unsigned, OwnedCommandSignatureDesc> m_CommandSignatures;
  std::unordered_map<unsigned, std::vector<D3D12_DISPATCH_RAYS_DESC>> m_ExecuteIndirectDispatchRays;
  UINT64 m_ExecuteIndirectLastArgumentBufferOffset{};
};

} // namespace directx
