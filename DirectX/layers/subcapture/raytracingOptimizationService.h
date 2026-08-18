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

#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <vector>

namespace gits {
namespace DirectX {

class RaytracingOptimizationService {
public:
  void BuildAccelerationStructure(
      ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c);
  void CopyAccelerationStructure(
      ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c);
  void NvapiBuildAccelerationStructureEx(
      NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& c);
  void NvapiBuildOpacityMicromapArray(NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& c);
  void ExecuteCommandLists(ID3D12CommandQueueExecuteCommandListsCommand& c);
  void Optimize(std::unordered_set<std::pair<GITSKey, unsigned>, UnsignedPairHash>& ases);
  std::vector<std::pair<GITSKey, GITSKey>>& GetOptimizedCommands() {
    return m_OptimizedCommandsWithSources;
  }
  std::unordered_set<GITSKey>& GetExistingBuffers() {
    return m_ExistingBuffers;
  }

private:
  struct RaytracingCommand {
    unsigned Id{};
    GITSKey CommandKey{};
    GITSKey DestKey{};
    unsigned DestOffset{};
    GITSKey SourceKey{};
    unsigned SourceOffset{};
    bool UpdateBuild{};
    RaytracingCommand* Source{};
    bool Restore{};
    std::unordered_set<GITSKey> Buffers;
    std::unordered_set<std::pair<GITSKey, unsigned>, UnsignedPairHash> OmmLinkages;
    std::vector<RaytracingCommand*> OpacityMicromapArrays;
  };

  unsigned m_CommandUniqueId{};
  std::unordered_map<GITSKey, std::vector<std::unique_ptr<RaytracingCommand>>>
      m_CommandsByCommandList;
  std::unordered_map<std::pair<GITSKey, unsigned>, RaytracingCommand*, UnsignedPairHash>
      m_CommandByKeyOffset;
  std::unordered_map<GITSKey, std::unique_ptr<RaytracingCommand>> m_CommandById;

  std::vector<std::pair<GITSKey, GITSKey>> m_OptimizedCommandsWithSources;
  std::unordered_set<GITSKey> m_ExistingBuffers;

private:
  void StoreCommand(std::unique_ptr<RaytracingCommand>& command);
};

} // namespace DirectX
} // namespace gits
