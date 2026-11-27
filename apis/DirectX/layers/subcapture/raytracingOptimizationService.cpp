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

namespace gits {
namespace DirectX {

void RaytracingOptimizationService::BuildAccelerationStructure(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  if (c.m_pDesc.Value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    return;
  }
  RaytracingCommand* command = new RaytracingCommand{};
  command->Key = c.Key;
  command->DestAddress = c.m_pDesc.Value->DestAccelerationStructureData;
  command->CaptureDestKey = c.m_pDesc.DestAccelerationStructureKey;
  CapturePlayerGpuAddressService::ResourceInfo* info =
      m_GpuAddressService.GetResourceInfoByCaptureAddress(command->DestAddress);
  if (info) {
    command->PlaybackDestKey = info->Key;
  }
  command->SourceAddress = c.m_pDesc.Value->SourceAccelerationStructureData;
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
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs = c.m_pDesc.Value->Inputs;
    for (unsigned i = 0; i < inputs.NumDescs; ++i) {
      D3D12_RAYTRACING_GEOMETRY_DESC& desc = const_cast<D3D12_RAYTRACING_GEOMETRY_DESC&>(
          inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY ? inputs.pGeometryDescs[i]
                                                            : *inputs.ppGeometryDescs[i]);
      if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES) {
        if (desc.OmmTriangles.pOmmLinkage) {
          command->OmmLinkages.insert(desc.OmmTriangles.pOmmLinkage->OpacityMicromapArray);
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
  command->DestAddress = c.m_DestAccelerationStructureData.Value;
  command->CaptureDestKey = c.m_DestAccelerationStructureData.InterfaceKey;
  CapturePlayerGpuAddressService::ResourceInfo* info =
      m_GpuAddressService.GetResourceInfoByCaptureAddress(command->DestAddress);
  if (info) {
    command->PlaybackDestKey = info->Key;
  }
  command->SourceAddress = c.m_SourceAccelerationStructureData.Value;
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
  command->DestAddress = c.m_pParams.Value->pDesc->destAccelerationStructureData;
  command->CaptureDestKey = c.m_pParams.DestAccelerationStructureKey;
  CapturePlayerGpuAddressService::ResourceInfo* info =
      m_GpuAddressService.GetResourceInfoByCaptureAddress(command->DestAddress);
  if (info) {
    command->PlaybackDestKey = info->Key;
  }
  command->SourceAddress = c.m_pParams.Value->pDesc->sourceAccelerationStructureData;
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
    const NVAPI_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS_EX& inputs =
        c.m_pParams.Value->pDesc->inputs;
    for (unsigned i = 0; i < inputs.numDescs; ++i) {
      const NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX& desc =
          inputs.descsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY
              ? *reinterpret_cast<const NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX*>(
                    reinterpret_cast<const char*>(inputs.pGeometryDescs) +
                    inputs.geometryDescStrideInBytes * i)
              : *inputs.ppGeometryDescs[i];
      if (desc.type == NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES_EX) {
        if (desc.ommTriangles.ommAttachment.opacityMicromapArray) {
          command->OmmLinkages.insert(desc.ommTriangles.ommAttachment.opacityMicromapArray);
        }
      }
    }
  }
  m_CommandsByCommandList[c.m_pCommandList.Key].emplace_back(command);
}

void RaytracingOptimizationService::NvapiBuildOpacityMicromapArray(
    NvAPI_D3D12_BuildRaytracingOpacityMicromapArrayCommand& c) {
  RaytracingCommand* command = new RaytracingCommand{};
  command->Key = c.Key;
  command->DestAddress = c.m_pParams.Value->pDesc->destOpacityMicromapArrayData;
  command->CaptureDestKey = c.m_pParams.DestOpacityMicromapArrayDataKey;
  CapturePlayerGpuAddressService::ResourceInfo* info =
      m_GpuAddressService.GetResourceInfoByCaptureAddress(command->DestAddress);
  if (info) {
    command->PlaybackDestKey = info->Key;
  }
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
  if (command->SourceAddress) {
    auto it = m_CommandByAddress.find(command->SourceAddress);
    GITS_ASSERT(it != m_CommandByAddress.end() && it->second);
    source = it->second;
  }

  for (D3D12_GPU_VIRTUAL_ADDRESS address : command->OmmLinkages) {
    auto it = m_CommandByAddress.find(address);
    GITS_ASSERT(it != m_CommandByAddress.end() && it->second);
    command->OpacityMicromapArrays.push_back(it->second);
  }

  // skip intermediate update build command
  if (command->UpdateBuild) {
    GITS_ASSERT(source);
    if (source->Source && source->UpdateBuild) {
      source = source->Source;
      command->SourceAddress = source->DestAddress;
    }
  }

  m_CommandByAddress[command->DestAddress] = command;

  if (source) {
    command->Source = source;
  }
}

void RaytracingOptimizationService::Optimize(std::unordered_set<D3D12_GPU_VIRTUAL_ADDRESS>& ases) {
  for (auto& [keyOffset, command] : m_CommandByAddress) {
    if (ases.contains(keyOffset)) {
      command->Restore = true;
      m_ExistingBuffers.insert(command->CaptureDestKey);
      if (command->PlaybackDestKey && command->PlaybackDestKey != command->CaptureDestKey) {
        m_ExistingBuffers.insert(command->PlaybackDestKey);
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
        m_ExistingBuffers.insert(ommArray->CaptureDestKey);
        if (ommArray->PlaybackDestKey && ommArray->PlaybackDestKey != ommArray->CaptureDestKey) {
          m_ExistingBuffers.insert(ommArray->PlaybackDestKey);
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
