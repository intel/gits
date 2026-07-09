// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

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
  void Optimize(std::unordered_set<std::pair<unsigned, unsigned>, UnsignedPairHash>& ases);
  std::vector<std::pair<unsigned, unsigned>>& GetOptimizedCommands() {
    return m_OptimizedCommandsWithSources;
  }

private:
  struct RaytracingCommand {
    unsigned Id{};
    unsigned CommandKey{};
    unsigned DestKey{};
    unsigned DestOffset{};
    unsigned SourceKey{};
    unsigned SourceOffset{};
    bool UpdateBuild{};
    RaytracingCommand* Source{};
    std::unordered_set<RaytracingCommand*> Destinations;
    bool Restore{};
  };

  unsigned m_CommandUniqueId{};
  std::unordered_map<unsigned, std::vector<std::unique_ptr<RaytracingCommand>>>
      m_CommandsByCommandList;
  std::unordered_map<std::pair<unsigned, unsigned>, RaytracingCommand*, UnsignedPairHash>
      m_CommandByKeyOffset;
  std::unordered_map<unsigned, std::unique_ptr<RaytracingCommand>> m_CommandById;

  std::vector<std::pair<unsigned, unsigned>> m_OptimizedCommandsWithSources;

private:
  void StoreCommand(std::unique_ptr<RaytracingCommand>& command);
};

} // namespace DirectX
} // namespace gits
