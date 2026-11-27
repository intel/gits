// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once
#include "arguments.h"
#include "commandsAuto.h"
#include "commandsCustom.h"
#include "hashUtils.h"
#include "capturePlayerGpuAddressService.h"

#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <vector>

namespace gits {
namespace DirectX {

class RaytracingOptimizationService {
public:
  RaytracingOptimizationService(CapturePlayerGpuAddressService& gpuAddressService)
      : m_GpuAddressService(gpuAddressService) {}
  void BuildAccelerationStructure(
      ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c);
  void CopyAccelerationStructure(
      ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c);
  void NvapiBuildAccelerationStructureEx(
      NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& c);
  void NvapiBuildOpacityMicromapArray(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& c);
  void ExecuteCommandLists(ID3D12CommandQueueExecuteCommandListsCommand& c);
  void Optimize(std::unordered_set<D3D12_GPU_VIRTUAL_ADDRESS>& ases);
  std::vector<std::pair<CommandKey, CommandKey>>& GetOptimizedCommands() {
    return m_OptimizedCommandsWithSources;
  }
  std::unordered_set<ObjectKey>& GetExistingBuffers() {
    return m_ExistingBuffers;
  }

private:
  struct RaytracingCommand {
    unsigned Id{};
    CommandKey Key{};
    D3D12_GPU_VIRTUAL_ADDRESS DestAddress{};
    ObjectKey CaptureDestKey{};
    ObjectKey PlaybackDestKey{};
    D3D12_GPU_VIRTUAL_ADDRESS SourceAddress{};
    bool UpdateBuild{};
    RaytracingCommand* Source{};
    bool Restore{};
    std::unordered_set<ObjectKey> Buffers;
    std::unordered_set<D3D12_GPU_VIRTUAL_ADDRESS> OmmLinkages;
    std::vector<RaytracingCommand*> OpacityMicromapArrays;
  };

  CapturePlayerGpuAddressService& m_GpuAddressService;
  unsigned m_CommandUniqueId{};
  std::unordered_map<ObjectKey, std::vector<std::unique_ptr<RaytracingCommand>>>
      m_CommandsByCommandList;
  std::unordered_map<D3D12_GPU_VIRTUAL_ADDRESS, RaytracingCommand*> m_CommandByAddress;
  std::unordered_map<unsigned, std::unique_ptr<RaytracingCommand>> m_CommandById;

  std::vector<std::pair<CommandKey, CommandKey>> m_OptimizedCommandsWithSources;
  std::unordered_set<ObjectKey> m_ExistingBuffers;

private:
  void StoreCommand(std::unique_ptr<RaytracingCommand>& command);
};

} // namespace DirectX
} // namespace gits
