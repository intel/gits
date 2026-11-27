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
#include "analyzerRaytracingService.h"

#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <vector>

namespace gits {
namespace DirectX {

class RaytracingOptimizationService {
public:
  RaytracingOptimizationService(AnalyzerRaytracingService& raytracingService,
                                CapturePlayerGpuAddressService& gpuAddressService)
      : m_RaytracingService(raytracingService), m_GpuAddressService(gpuAddressService) {}
  void BuildAccelerationStructure(
      ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c);
  void CopyAccelerationStructure(
      ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c);
  void NvapiBuildAccelerationStructureEx(
      NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& c);
  void NvapiBuildOpacityMicromapArray(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& c);
  void ExecuteCommandLists(ID3D12CommandQueueExecuteCommandListsCommand& c);
  void Optimize(std::unordered_set<AnalyzerRaytracingService::KeyOffset, KeyOffsetHash>& ases);
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
    // for placed resources heap key and heap offset, otherwise resource key and resource offset
    ObjectKey DestKey{};
    UINT64 DestOffset{};
    ObjectKey SourceKey{};
    UINT64 SourceOffset{};
    ObjectKey DestResourceKey{};
    ObjectKey AlternateDestResourceKey{};
    bool UpdateBuild{};
    RaytracingCommand* Source{};
    bool Restore{};
    std::unordered_set<ObjectKey> Buffers;
    std::unordered_set<AnalyzerRaytracingService::KeyOffset, KeyOffsetHash> OmmLinkages;
    std::vector<RaytracingCommand*> OpacityMicromapArrays;
  };

  AnalyzerRaytracingService& m_RaytracingService;
  CapturePlayerGpuAddressService& m_GpuAddressService;
  unsigned m_CommandUniqueId{};
  std::unordered_map<ObjectKey, std::vector<std::unique_ptr<RaytracingCommand>>>
      m_CommandsByCommandList;
  std::unordered_map<AnalyzerRaytracingService::KeyOffset, RaytracingCommand*, KeyOffsetHash>
      m_CommandByKeyOffset;
  std::unordered_map<unsigned, std::unique_ptr<RaytracingCommand>> m_CommandById;

  std::vector<std::pair<CommandKey, CommandKey>> m_OptimizedCommandsWithSources;
  std::unordered_set<ObjectKey> m_ExistingBuffers;

private:
  void StoreCommand(std::unique_ptr<RaytracingCommand>& command);
};

} // namespace DirectX
} // namespace gits
