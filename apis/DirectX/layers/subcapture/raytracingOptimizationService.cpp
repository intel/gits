// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "raytracingOptimizationService.h"
#include "arguments.h"
#include "log.h"

#include <tuple>

namespace gits {
namespace DirectX {

void RaytracingOptimizationService::BuildAccelerationStructure(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  if (c.m_pDesc.Value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    return;
  }
  RaytracingCommand* command = new RaytracingCommand{};
  command->Key = c.Key;
  command->DestResourceKey = c.m_pDesc.DestAccelerationStructureKey;
  CapturePlayerGpuAddressService::ResourceInfo* info =
      m_GpuAddressService.GetResourceInfoByCaptureAddress(
          c.m_pDesc.Value->DestAccelerationStructureData, true, false);
  if (info) {
    command->AlternateDestResourceKey = info->Key;
  }
  std::tie(command->DestKey, command->DestOffset) = m_RaytracingService.GetAccelerationStructureId(
      c.m_pDesc.DestAccelerationStructureKey, c.m_pDesc.DestAccelerationStructureOffset);
  if (c.m_pDesc.SourceAccelerationStructureKey) {
    std::tie(command->SourceKey, command->SourceOffset) =
        m_RaytracingService.GetAccelerationStructureId(c.m_pDesc.SourceAccelerationStructureKey,
                                                       c.m_pDesc.SourceAccelerationStructureOffset);
  }
  command->UpdateBuild = c.m_pDesc.Value->Inputs.Flags &
                         D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
  command->Buffers.insert(c.m_pDesc.DestAccelerationStructureKey);
  if (c.m_pDesc.SourceAccelerationStructureKey) {
    command->Buffers.insert(c.m_pDesc.SourceAccelerationStructureKey);
  }
  for (ObjectKey key : c.m_pDesc.InputKeys) {
    command->Buffers.insert(key);
  }
  if (c.m_pDesc.Value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
    unsigned inputIndex = 0;
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs = c.m_pDesc.Value->Inputs;
    for (unsigned i = 0; i < inputs.NumDescs; ++i) {
      D3D12_RAYTRACING_GEOMETRY_DESC& desc = const_cast<D3D12_RAYTRACING_GEOMETRY_DESC&>(
          inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY ? inputs.pGeometryDescs[i]
                                                            : *inputs.ppGeometryDescs[i]);
      if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES) {
        inputIndex += 3;
      } else if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS) {
        ++inputIndex;
      } else if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES) {
        if (desc.OmmTriangles.pTriangles) {
          inputIndex += 3;
        }
        if (desc.OmmTriangles.pOmmLinkage) {
          ++inputIndex;
          command->OmmLinkages.insert(m_RaytracingService.GetAccelerationStructureId(
              c.m_pDesc.InputKeys[inputIndex], c.m_pDesc.InputOffsets[inputIndex]));
          ++inputIndex;
        }
      }
    }
  }
  m_CommandsByCommandList[c.m_Object.Key].emplace_back(command);
}

void RaytracingOptimizationService::CopyAccelerationStructure(
    ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c) {
  RaytracingCommand* command = new RaytracingCommand{};
  command->Key = c.Key;
  command->DestResourceKey = c.m_DestAccelerationStructureData.InterfaceKey;
  CapturePlayerGpuAddressService::ResourceInfo* info =
      m_GpuAddressService.GetResourceInfoByCaptureAddress(c.m_DestAccelerationStructureData.Value,
                                                          true, false);
  if (info) {
    command->AlternateDestResourceKey = info->Key;
  }
  std::tie(command->DestKey, command->DestOffset) = m_RaytracingService.GetAccelerationStructureId(
      c.m_DestAccelerationStructureData.InterfaceKey, c.m_DestAccelerationStructureData.Offset);
  std::tie(command->SourceKey, command->SourceOffset) =
      m_RaytracingService.GetAccelerationStructureId(
          c.m_SourceAccelerationStructureData.InterfaceKey,
          c.m_SourceAccelerationStructureData.Offset);
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
  command->Key = c.Key;
  command->DestResourceKey = c.m_pParams.DestAccelerationStructureKey;
  CapturePlayerGpuAddressService::ResourceInfo* info =
      m_GpuAddressService.GetResourceInfoByCaptureAddress(
          c.m_pParams.Value->pDesc->destAccelerationStructureData, true, false);
  if (info) {
    command->AlternateDestResourceKey = info->Key;
  }
  std::tie(command->DestKey, command->DestOffset) = m_RaytracingService.GetAccelerationStructureId(
      c.m_pParams.DestAccelerationStructureKey, c.m_pParams.DestAccelerationStructureOffset);
  if (c.m_pParams.SourceAccelerationStructureKey) {
    std::tie(command->SourceKey, command->SourceOffset) =
        m_RaytracingService.GetAccelerationStructureId(
            c.m_pParams.SourceAccelerationStructureKey,
            c.m_pParams.SourceAccelerationStructureOffset);
  }
  command->UpdateBuild = c.m_pParams.Value->pDesc->inputs.flags &
                         NVAPI_D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE_EX;
  command->Buffers.insert(c.m_pParams.DestAccelerationStructureKey);
  if (c.m_pParams.SourceAccelerationStructureKey) {
    command->Buffers.insert(c.m_pParams.SourceAccelerationStructureKey);
  }
  for (ObjectKey key : c.m_pParams.InputKeys) {
    command->Buffers.insert(key);
  }
  if (c.m_pParams.Value->pDesc->inputs.type ==
      D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
    unsigned inputIndex = 0;
    const NVAPI_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS_EX& inputs =
        c.m_pParams.Value->pDesc->inputs;
    for (unsigned i = 0; i < inputs.numDescs; ++i) {
      const NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX& desc =
          inputs.descsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY
              ? *reinterpret_cast<const NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX*>(
                    reinterpret_cast<const char*>(inputs.pGeometryDescs) +
                    inputs.geometryDescStrideInBytes * i)
              : *inputs.ppGeometryDescs[i];
      if (desc.type == D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES) {
        inputIndex += 3;
      } else if (desc.type == D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS) {
        ++inputIndex;
      } else if (desc.type == NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES_EX) {
        inputIndex += 4;
        if (desc.ommTriangles.ommAttachment.opacityMicromapArray) {
          command->OmmLinkages.insert(m_RaytracingService.GetAccelerationStructureId(
              c.m_pParams.InputKeys[inputIndex], c.m_pParams.InputOffsets[inputIndex]));
        }
        ++inputIndex;
      } else if (desc.type == NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_DMM_TRIANGLES_EX) {
        inputIndex += 8;
      } else if (desc.type == NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_SPHERES_EX) {
        inputIndex += 3;
      } else if (desc.type == NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_LSS_EX) {
        inputIndex += 3;
      }
    }
  }
  m_CommandsByCommandList[c.m_pCommandList.Key].emplace_back(command);
}

void RaytracingOptimizationService::NvapiBuildOpacityMicromapArray(
    NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& c) {
  RaytracingCommand* command = new RaytracingCommand{};
  command->Key = c.Key;
  command->DestResourceKey = c.m_pParams.DestOpacityMicromapArrayDataKey;
  CapturePlayerGpuAddressService::ResourceInfo* info =
      m_GpuAddressService.GetResourceInfoByCaptureAddress(
          c.m_pParams.Value->pDesc->destOpacityMicromapArrayData, true, false);
  if (info) {
    command->AlternateDestResourceKey = info->Key;
  }
  std::tie(command->DestKey, command->DestOffset) = m_RaytracingService.GetAccelerationStructureId(
      c.m_pParams.DestOpacityMicromapArrayDataKey, c.m_pParams.DestOpacityMicromapArrayDataOffset);
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
  for (ObjectKey commandListKey : c.m_ppCommandLists.Keys) {
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
    auto it = m_CommandByKeyOffset.find(
        AnalyzerRaytracingService::KeyOffset{command->SourceKey, command->SourceOffset});
    GITS_ASSERT(it != m_CommandByKeyOffset.end() && it->second);
    source = it->second;
  }

  for (auto& keyOffset : command->OmmLinkages) {
    auto it = m_CommandByKeyOffset.find(keyOffset);
    GITS_ASSERT(it != m_CommandByKeyOffset.end() && it->second);
    command->OpacityMicromapArrays.push_back(it->second);
  }

  // skip intermediate update build command
  if (command->UpdateBuild) {
    GITS_ASSERT(source);
    if (source->Source && source->UpdateBuild) {
      source = source->Source;
      command->SourceKey = source->DestKey;
      command->SourceOffset = source->DestOffset;
    }
  }

  m_CommandByKeyOffset[AnalyzerRaytracingService::KeyOffset{command->DestKey,
                                                            command->DestOffset}] = command;

  if (source) {
    command->Source = source;
  }
}

void RaytracingOptimizationService::Optimize(
    std::unordered_set<AnalyzerRaytracingService::KeyOffset, KeyOffsetHash>& ases) {
  for (auto& [keyOffset, command] : m_CommandByKeyOffset) {
    if (ases.contains(keyOffset)) {
      command->Restore = true;
      m_ExistingBuffers.insert(command->DestResourceKey);
      if (command->AlternateDestResourceKey &&
          command->AlternateDestResourceKey != command->DestResourceKey) {
        m_ExistingBuffers.insert(command->AlternateDestResourceKey);
      }
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
      for (RaytracingCommand* ommArray : command->OpacityMicromapArrays) {
        m_ExistingBuffers.insert(ommArray->DestResourceKey);
        if (ommArray->AlternateDestResourceKey &&
            ommArray->AlternateDestResourceKey != ommArray->DestResourceKey) {
          m_ExistingBuffers.insert(ommArray->AlternateDestResourceKey);
        }
        ommArray->Restore = true;
        while (ommArray->Source) {
          ommArray->Source->Restore = true;
          ommArray = ommArray->Source;
        }
      }
    }
  }

  // prepare commands
  for (auto& it : m_CommandById) {
    RaytracingCommand* command = it.second.get();
    if (command->Restore) {
      CommandKey commandKey = command->Key;
      CommandKey sourceKey{};
      if (command->Source) {
        sourceKey = command->Source->Key;
      }
      m_OptimizedCommandsWithSources.emplace_back(commandKey, sourceKey);
    }
  }
}

} // namespace DirectX
} // namespace gits
