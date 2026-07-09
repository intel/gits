// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "raytracingOptimizationService.h"
#include "log.h"

namespace gits {
namespace DirectX {

void RaytracingOptimizationService::BuildAccelerationStructure(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  if (c.m_pDesc.Value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    return;
  }
  RaytracingCommand* command = new RaytracingCommand{};
  command->CommandKey = c.Key;
  command->DestKey = c.m_pDesc.DestAccelerationStructureKey;
  command->DestOffset = c.m_pDesc.DestAccelerationStructureOffset;
  command->SourceKey = c.m_pDesc.SourceAccelerationStructureKey;
  command->SourceOffset = c.m_pDesc.SourceAccelerationStructureOffset;
  command->UpdateBuild = c.m_pDesc.Value->Inputs.Flags &
                         D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
  command->Buffers.insert(c.m_pDesc.DestAccelerationStructureKey);
  if (c.m_pDesc.SourceAccelerationStructureKey) {
    command->Buffers.insert(c.m_pDesc.SourceAccelerationStructureKey);
  }
  for (unsigned key : c.m_pDesc.InputKeys) {
    command->Buffers.insert(key);
  }
  m_CommandsByCommandList[c.m_Object.Key].emplace_back(command);
}

void RaytracingOptimizationService::CopyAccelerationStructure(
    ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c) {
  RaytracingCommand* command = new RaytracingCommand{};
  command->CommandKey = c.Key;
  command->DestKey = c.m_DestAccelerationStructureData.InterfaceKey;
  command->DestOffset = c.m_DestAccelerationStructureData.Offset;
  command->SourceKey = c.m_SourceAccelerationStructureData.InterfaceKey;
  command->SourceOffset = c.m_SourceAccelerationStructureData.Offset;
  command->Buffers.insert(c.m_DestAccelerationStructureData.InterfaceKey);
  command->Buffers.insert(c.m_SourceAccelerationStructureData.InterfaceKey);
  m_CommandsByCommandList[c.m_Object.Key].emplace_back(command);
}

void RaytracingOptimizationService::NvapiBuildAccelerationStructureEx(
    NvAPI_D3D12_BuildRaytracingAccelerationStructureExCommand& c) {
  if (c.m_pParams.Value->pDesc->inputs.type ==
      D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    return;
  }
  RaytracingCommand* command = new RaytracingCommand{};
  command->CommandKey = c.Key;
  command->DestKey = c.m_pParams.DestAccelerationStructureKey;
  command->DestOffset = c.m_pParams.DestAccelerationStructureOffset;
  command->SourceKey = c.m_pParams.SourceAccelerationStructureKey;
  command->SourceOffset = c.m_pParams.SourceAccelerationStructureOffset;
  command->UpdateBuild = c.m_pParams.Value->pDesc->inputs.flags &
                         NVAPI_D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE_EX;
  command->Buffers.insert(c.m_pParams.DestAccelerationStructureKey);
  if (c.m_pParams.SourceAccelerationStructureKey) {
    command->Buffers.insert(c.m_pParams.SourceAccelerationStructureKey);
  }
  for (unsigned key : c.m_pParams.InputKeys) {
    command->Buffers.insert(key);
  }
  m_CommandsByCommandList[c.m_pCommandList.Key].emplace_back(command);
}

void RaytracingOptimizationService::NvapiBuildOpacityMicromapArray(
    NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& c) {
  RaytracingCommand* command = new RaytracingCommand{};
  command->CommandKey = c.Key;
  command->DestKey = c.m_pParams.DestOpacityMicromapArrayDataKey;
  command->DestOffset = c.m_pParams.DestOpacityMicromapArrayDataOffset;
  command->Restore = true;
  command->Buffers.insert(c.m_pParams.DestOpacityMicromapArrayDataKey);
  if (c.m_pParams.InputBufferKey) {
    command->Buffers.insert(c.m_pParams.InputBufferKey);
  }
  if (c.m_pParams.PerOMMDescsKey) {
    command->Buffers.insert(c.m_pParams.PerOMMDescsKey);
  }
  m_CommandsByCommandList[c.m_pCommandList.Key].emplace_back(command);
}

void RaytracingOptimizationService::ExecuteCommandLists(
    ID3D12CommandQueueExecuteCommandListsCommand& c) {
  for (unsigned commandListKey : c.m_ppCommandLists.Keys) {
    auto it = m_CommandsByCommandList.find(commandListKey);
    if (it != m_CommandsByCommandList.end()) {
      for (auto& command : it->second) {
        StoreCommand(command);
      }
      m_CommandsByCommandList.erase(it);
    }
  }
}

void RaytracingOptimizationService::StoreCommand(std::unique_ptr<RaytracingCommand>& commandPtr) {
  commandPtr->Id = ++m_CommandUniqueId;
  auto& ptr = m_CommandById[commandPtr->Id];
  ptr.swap(commandPtr);
  RaytracingCommand* command = ptr.get();

  RaytracingCommand* source{};
  if (command->SourceKey) {
    auto it = m_CommandByKeyOffset.find({command->SourceKey, command->SourceOffset});
    GITS_ASSERT(it != m_CommandByKeyOffset.end() && it->second);
    source = it->second;
  }

  // skip intermediate update build command
  if (command->UpdateBuild) {
    GITS_ASSERT(source);
    if (source->Source) {
      source = source->Source;
      command->SourceKey = source->DestKey;
      command->SourceOffset = source->DestOffset;
    }
  }

  m_CommandByKeyOffset[{command->DestKey, command->DestOffset}] = command;

  if (source) {
    command->Source = source;
  }
}

void RaytracingOptimizationService::Optimize(
    std::unordered_set<std::pair<unsigned, unsigned>, UnsignedPairHash>& ases) {
  for (auto& [keyOffset, command] : m_CommandByKeyOffset) {
    if (ases.contains(keyOffset)) {
      command->Restore = true;
    }
  }

  // mark source nodes for restoration
  for (auto& it : m_CommandById) {
    RaytracingCommand* command = it.second.get();
    if (command->Restore) {
      while (command->Source) {
        command->Source->Restore = true;
        command = command->Source;
      }
    }
  }

  // prepare sorted commands
  for (auto& it : m_CommandById) {
    RaytracingCommand* command = it.second.get();
    if (command->Restore) {
      unsigned commandKey = command->CommandKey;
      unsigned sourceKey = 0;
      if (command->Source) {
        sourceKey = command->Source->CommandKey;
      }
      m_OptimizedCommandsWithSources.emplace_back(commandKey, sourceKey);
      for (unsigned key : command->Buffers) {
        m_OptimizedBuffers.insert(key);
      }
    }
  }
  std::sort(m_OptimizedCommandsWithSources.begin(), m_OptimizedCommandsWithSources.end());
}

} // namespace DirectX
} // namespace gits
