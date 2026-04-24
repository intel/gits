// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "commandPreservationLayer.h"
#include "intelExtensions.h"

namespace gits {
namespace DirectX {

void CommandPreservationLayer::Pre(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  m_CaptureGpuAddresses.push_back(c.m_pDesc.Value->DestAccelerationStructureData);
  if (c.m_pDesc.Value->SourceAccelerationStructureData) {
    m_CaptureGpuAddresses.push_back(c.m_pDesc.Value->SourceAccelerationStructureData);
  }
  m_CaptureGpuAddresses.push_back(c.m_pDesc.Value->ScratchAccelerationStructureData);

  if (c.m_pDesc.Value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    m_CaptureGpuAddresses.push_back(c.m_pDesc.Value->Inputs.InstanceDescs);
  } else if (c.m_pDesc.Value->Inputs.Type ==
             D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
    for (unsigned i = 0; i < c.m_pDesc.Value->Inputs.NumDescs; ++i) {
      D3D12_RAYTRACING_GEOMETRY_DESC& desc = const_cast<D3D12_RAYTRACING_GEOMETRY_DESC&>(
          c.m_pDesc.Value->Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY
              ? c.m_pDesc.Value->Inputs.pGeometryDescs[i]
              : *c.m_pDesc.Value->Inputs.ppGeometryDescs[i]);
      if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES) {
        m_CaptureGpuAddresses.push_back(desc.Triangles.Transform3x4);
        m_CaptureGpuAddresses.push_back(desc.Triangles.IndexBuffer);
        m_CaptureGpuAddresses.push_back(desc.Triangles.VertexBuffer.StartAddress);
      } else if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS) {
        m_CaptureGpuAddresses.push_back(desc.AABBs.AABBs.StartAddress);
      } else if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES) {
        if (desc.OmmTriangles.pTriangles) {
          m_CaptureGpuAddresses.push_back(desc.OmmTriangles.pTriangles->Transform3x4);
          m_CaptureGpuAddresses.push_back(desc.OmmTriangles.pTriangles->IndexBuffer);
          m_CaptureGpuAddresses.push_back(desc.OmmTriangles.pTriangles->VertexBuffer.StartAddress);
        }
        if (desc.OmmTriangles.pOmmLinkage) {
          m_CaptureGpuAddresses.push_back(
              desc.OmmTriangles.pOmmLinkage->OpacityMicromapIndexBuffer.StartAddress);
          m_CaptureGpuAddresses.push_back(desc.OmmTriangles.pOmmLinkage->OpacityMicromapArray);
        }
      }
    }
  } else if (c.m_pDesc.Value->Inputs.Type ==
             D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_OPACITY_MICROMAP_ARRAY) {
    if (c.m_pDesc.Value->Inputs.pOpacityMicromapArrayDesc) {
      auto& ommDesc = *const_cast<D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC*>(
          c.m_pDesc.Value->Inputs.pOpacityMicromapArrayDesc);
      m_CaptureGpuAddresses.push_back(ommDesc.InputBuffer);
      m_CaptureGpuAddresses.push_back(ommDesc.PerOmmDescs.StartAddress);
    }
  }

  for (unsigned i = 0; i < c.m_NumPostbuildInfoDescs.Value; ++i) {
    m_CaptureGpuAddresses.push_back(c.m_pPostbuildInfoDescs.Value[i].DestBuffer);
  }
}

void CommandPreservationLayer::Post(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  unsigned index = 0;
  c.m_pDesc.Value->DestAccelerationStructureData = m_CaptureGpuAddresses[index++];
  if (c.m_pDesc.Value->SourceAccelerationStructureData) {
    c.m_pDesc.Value->SourceAccelerationStructureData = m_CaptureGpuAddresses[index++];
  }
  c.m_pDesc.Value->ScratchAccelerationStructureData = m_CaptureGpuAddresses[index++];

  if (c.m_pDesc.Value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    c.m_pDesc.Value->Inputs.InstanceDescs = m_CaptureGpuAddresses[index++];
  } else if (c.m_pDesc.Value->Inputs.Type ==
             D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
    for (unsigned i = 0; i < c.m_pDesc.Value->Inputs.NumDescs; ++i) {
      D3D12_RAYTRACING_GEOMETRY_DESC& desc = const_cast<D3D12_RAYTRACING_GEOMETRY_DESC&>(
          c.m_pDesc.Value->Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY
              ? c.m_pDesc.Value->Inputs.pGeometryDescs[i]
              : *c.m_pDesc.Value->Inputs.ppGeometryDescs[i]);
      if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES) {
        desc.Triangles.Transform3x4 = m_CaptureGpuAddresses[index++];
        desc.Triangles.IndexBuffer = m_CaptureGpuAddresses[index++];
        desc.Triangles.VertexBuffer.StartAddress = m_CaptureGpuAddresses[index++];
      } else if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS) {
        desc.AABBs.AABBs.StartAddress = m_CaptureGpuAddresses[index++];
      } else if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES) {
        if (desc.OmmTriangles.pTriangles) {
          auto& triangles =
              *const_cast<D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC*>(desc.OmmTriangles.pTriangles);
          triangles.Transform3x4 = m_CaptureGpuAddresses[index++];
          triangles.IndexBuffer = m_CaptureGpuAddresses[index++];
          triangles.VertexBuffer.StartAddress = m_CaptureGpuAddresses[index++];
        }
        if (desc.OmmTriangles.pOmmLinkage) {
          auto& ommLinkage = *const_cast<D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC*>(
              desc.OmmTriangles.pOmmLinkage);
          ommLinkage.OpacityMicromapIndexBuffer.StartAddress = m_CaptureGpuAddresses[index++];
          ommLinkage.OpacityMicromapArray = m_CaptureGpuAddresses[index++];
        }
      }
    }
  } else if (c.m_pDesc.Value->Inputs.Type ==
             D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_OPACITY_MICROMAP_ARRAY) {
    if (c.m_pDesc.Value->Inputs.pOpacityMicromapArrayDesc) {
      auto& ommDesc = *const_cast<D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC*>(
          c.m_pDesc.Value->Inputs.pOpacityMicromapArrayDesc);
      ommDesc.InputBuffer = m_CaptureGpuAddresses[index++];
      ommDesc.PerOmmDescs.StartAddress = m_CaptureGpuAddresses[index++];
    }
  }

  for (unsigned i = 0; i < c.m_NumPostbuildInfoDescs.Value; ++i) {
    c.m_pPostbuildInfoDescs.Value[i].DestBuffer = m_CaptureGpuAddresses[index++];
  }
  m_CaptureGpuAddresses.clear();
}

void CommandPreservationLayer::Pre(ID3D12GraphicsCommandList4DispatchRaysCommand& c) {
  m_CaptureGpuAddresses.push_back(c.m_pDesc.Value->RayGenerationShaderRecord.StartAddress);
  m_CaptureGpuAddresses.push_back(c.m_pDesc.Value->MissShaderTable.StartAddress);
  m_CaptureGpuAddresses.push_back(c.m_pDesc.Value->HitGroupTable.StartAddress);
  m_CaptureGpuAddresses.push_back(c.m_pDesc.Value->CallableShaderTable.StartAddress);
}

void CommandPreservationLayer::Post(ID3D12GraphicsCommandList4DispatchRaysCommand& c) {
  c.m_pDesc.Value->RayGenerationShaderRecord.StartAddress = m_CaptureGpuAddresses[0];
  c.m_pDesc.Value->MissShaderTable.StartAddress = m_CaptureGpuAddresses[1];
  c.m_pDesc.Value->HitGroupTable.StartAddress = m_CaptureGpuAddresses[2];
  c.m_pDesc.Value->CallableShaderTable.StartAddress = m_CaptureGpuAddresses[3];
  m_CaptureGpuAddresses.clear();
}

void CommandPreservationLayer::Pre(ID3D12ResourceGetGPUVirtualAddressCommand& c) {
  m_CaptureGpuAddresses.push_back(c.m_Result.Value);
}

void CommandPreservationLayer::Post(ID3D12ResourceGetGPUVirtualAddressCommand& c) {
  c.m_Result.Value = m_CaptureGpuAddresses[0];
  m_CaptureGpuAddresses.clear();
}

void CommandPreservationLayer::Pre(ID3D12StateObjectPropertiesGetShaderIdentifierCommand& c) {
  for (unsigned i = 0; i < D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES; ++i) {
    m_CaptureShaderIdentifier[i] = static_cast<uint8_t*>(c.m_Result.Value)[i];
  }
}

void CommandPreservationLayer::Post(ID3D12StateObjectPropertiesGetShaderIdentifierCommand& c) {
  for (unsigned i = 0; i < D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES; ++i) {
    static_cast<uint8_t*>(c.m_Result.Value)[i] = m_CaptureShaderIdentifier[i];
  }
}

void CommandPreservationLayer::Pre(
    ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& c) {
  m_CaptureGpuDescriptorHandle = c.m_Result.Value;
}

void CommandPreservationLayer::Post(
    ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& c) {
  c.m_Result.Value = m_CaptureGpuDescriptorHandle;
}

void CommandPreservationLayer::Pre(INTC_D3D12_CreateComputePipelineStateCommand& c) {
  c.m_pDesc.Cs = c.m_pDesc.Value->CS.pShaderBytecode;
  c.m_pDesc.CompileOptions = c.m_pDesc.Value->CompileOptions;
  c.m_pDesc.InternalOptions = c.m_pDesc.Value->InternalOptions;
}

void CommandPreservationLayer::Pre(ID3D12DeviceCreateConstantBufferViewCommand& c) {
  if (c.m_pDesc.Value && c.m_pDesc.Value->BufferLocation) {
    m_CaptureGpuAddresses.push_back(c.m_pDesc.Value->BufferLocation);
  }
}

void CommandPreservationLayer::Post(ID3D12DeviceCreateConstantBufferViewCommand& c) {
  if (c.m_pDesc.Value && c.m_pDesc.Value->BufferLocation) {
    c.m_pDesc.Value->BufferLocation = m_CaptureGpuAddresses[0];
    m_CaptureGpuAddresses.clear();
  }
}

void CommandPreservationLayer::Pre(ID3D12GraphicsCommandListIASetIndexBufferCommand& c) {
  if (c.m_pView.Value && c.m_pView.Value->BufferLocation) {
    m_CaptureGpuAddresses.push_back(c.m_pView.Value->BufferLocation);
  }
}

void CommandPreservationLayer::Post(ID3D12GraphicsCommandListIASetIndexBufferCommand& c) {
  if (c.m_pView.Value && c.m_pView.Value->BufferLocation) {
    c.m_pView.Value->BufferLocation = m_CaptureGpuAddresses[0];
    m_CaptureGpuAddresses.clear();
  }
}

void CommandPreservationLayer::Pre(ID3D12GraphicsCommandListIASetVertexBuffersCommand& c) {
  if (c.m_pViews.Value) {
    for (unsigned i = 0; i < c.m_NumViews.Value; ++i) {
      if (c.m_pViews.Value[i].BufferLocation) {
        m_CaptureGpuAddresses.push_back(c.m_pViews.Value[i].BufferLocation);
      }
    }
  }
}

void CommandPreservationLayer::Post(ID3D12GraphicsCommandListIASetVertexBuffersCommand& c) {
  if (c.m_pViews.Value) {
    unsigned index = 0;
    for (unsigned i = 0; i < c.m_NumViews.Value; ++i) {
      if (c.m_pViews.Value[i].BufferLocation) {
        c.m_pViews.Value[i].BufferLocation = m_CaptureGpuAddresses[index++];
      }
    }
    m_CaptureGpuAddresses.clear();
  }
}

void CommandPreservationLayer::Pre(ID3D12GraphicsCommandListSOSetTargetsCommand& c) {
  if (c.m_pViews.Value) {
    for (unsigned i = 0; i < c.m_NumViews.Value; ++i) {
      if (c.m_pViews.Value[i].SizeInBytes) {
        if (c.m_pViews.Value[i].BufferLocation) {
          m_CaptureGpuAddresses.push_back(c.m_pViews.Value[i].BufferLocation);
        }
        if (c.m_pViews.Value[i].BufferFilledSizeLocation) {
          m_CaptureGpuAddresses.push_back(c.m_pViews.Value[i].BufferFilledSizeLocation);
        }
      }
    }
  }
}

void CommandPreservationLayer::Post(ID3D12GraphicsCommandListSOSetTargetsCommand& c) {
  if (c.m_pViews.Value) {
    unsigned index = 0;
    for (unsigned i = 0; i < c.m_NumViews.Value; ++i) {
      if (c.m_pViews.Value[i].SizeInBytes) {
        if (c.m_pViews.Value[i].BufferLocation) {
          c.m_pViews.Value[i].BufferLocation = m_CaptureGpuAddresses[index++];
        }
        if (c.m_pViews.Value[i].BufferFilledSizeLocation) {
          c.m_pViews.Value[i].BufferFilledSizeLocation = m_CaptureGpuAddresses[index++];
        }
      }
    }
  }
  m_CaptureGpuAddresses.clear();
}

void CommandPreservationLayer::Pre(ID3D12GraphicsCommandList2WriteBufferImmediateCommand& c) {
  if (c.m_pParams.Value) {
    for (unsigned i = 0; i < c.m_Count.Value; ++i) {
      if (c.m_pParams.Value[i].Dest) {
        m_CaptureGpuAddresses.push_back(c.m_pParams.Value[i].Dest);
      }
    }
  }
}

void CommandPreservationLayer::Post(ID3D12GraphicsCommandList2WriteBufferImmediateCommand& c) {
  if (c.m_pParams.Value) {
    unsigned index = 0;
    for (unsigned i = 0; i < c.m_Count.Value; ++i) {
      if (c.m_pParams.Value[i].Dest) {
        c.m_pParams.Value[i].Dest = m_CaptureGpuAddresses[index++];
      }
    }
    m_CaptureGpuAddresses.clear();
  }
}

void CommandPreservationLayer::Pre(ID3D12DeviceCreateShaderResourceViewCommand& c) {
  if (c.m_pDesc.Value &&
      c.m_pDesc.Value->ViewDimension == D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE) {
    m_CaptureGpuAddresses.push_back(c.m_pDesc.Value->RaytracingAccelerationStructure.Location);
  }
}

void CommandPreservationLayer::Post(ID3D12DeviceCreateShaderResourceViewCommand& c) {
  if (c.m_pDesc.Value &&
      c.m_pDesc.Value->ViewDimension == D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE) {
    c.m_pDesc.Value->RaytracingAccelerationStructure.Location = m_CaptureGpuAddresses[0];
    m_CaptureGpuAddresses.clear();
  }
}

void CommandPreservationLayer::Pre(
    ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand& c) {
  m_CaptureGpuAddresses.push_back(c.m_BufferLocation.Value);
}

void CommandPreservationLayer::Post(
    ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand& c) {
  c.m_BufferLocation.Value = m_CaptureGpuAddresses[0];
  m_CaptureGpuAddresses.clear();
}

void CommandPreservationLayer::Pre(
    ID3D12GraphicsCommandListSetGraphicsRootConstantBufferViewCommand& c) {
  m_CaptureGpuAddresses.push_back(c.m_BufferLocation.Value);
}

void CommandPreservationLayer::Post(
    ID3D12GraphicsCommandListSetGraphicsRootConstantBufferViewCommand& c) {
  c.m_BufferLocation.Value = m_CaptureGpuAddresses[0];
  m_CaptureGpuAddresses.clear();
}

void CommandPreservationLayer::Pre(
    ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand& c) {
  m_CaptureGpuAddresses.push_back(c.m_BufferLocation.Value);
}

void CommandPreservationLayer::Post(
    ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand& c) {
  c.m_BufferLocation.Value = m_CaptureGpuAddresses[0];
  m_CaptureGpuAddresses.clear();
}

void CommandPreservationLayer::Pre(
    ID3D12GraphicsCommandListSetGraphicsRootShaderResourceViewCommand& c) {
  m_CaptureGpuAddresses.push_back(c.m_BufferLocation.Value);
}

void CommandPreservationLayer::Post(
    ID3D12GraphicsCommandListSetGraphicsRootShaderResourceViewCommand& c) {
  c.m_BufferLocation.Value = m_CaptureGpuAddresses[0];
  m_CaptureGpuAddresses.clear();
}

void CommandPreservationLayer::Pre(
    ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& c) {
  m_CaptureGpuAddresses.push_back(c.m_BufferLocation.Value);
}

void CommandPreservationLayer::Post(
    ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& c) {
  c.m_BufferLocation.Value = m_CaptureGpuAddresses[0];
  m_CaptureGpuAddresses.clear();
}

void CommandPreservationLayer::Pre(
    ID3D12GraphicsCommandListSetGraphicsRootUnorderedAccessViewCommand& c) {
  m_CaptureGpuAddresses.push_back(c.m_BufferLocation.Value);
}

void CommandPreservationLayer::Post(
    ID3D12GraphicsCommandListSetGraphicsRootUnorderedAccessViewCommand& c) {
  c.m_BufferLocation.Value = m_CaptureGpuAddresses[0];
  m_CaptureGpuAddresses.clear();
}

void CommandPreservationLayer::Pre(
    ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c) {
  m_CaptureGpuAddresses.push_back(c.m_DestAccelerationStructureData.Value);
  m_CaptureGpuAddresses.push_back(c.m_SourceAccelerationStructureData.Value);
}

void CommandPreservationLayer::Post(
    ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c) {
  c.m_DestAccelerationStructureData.Value = m_CaptureGpuAddresses[0];
  c.m_SourceAccelerationStructureData.Value = m_CaptureGpuAddresses[1];
  m_CaptureGpuAddresses.clear();
}

void CommandPreservationLayer::Pre(
    ID3D12GraphicsCommandList4EmitRaytracingAccelerationStructurePostbuildInfoCommand& c) {
  m_CaptureGpuAddresses.push_back(c.m_pDesc.Value->DestBuffer);
  for (unsigned i = 0; i < c.m_NumSourceAccelerationStructures.Value; ++i) {
    m_CaptureGpuAddresses.push_back(c.m_pSourceAccelerationStructureData.Value[i]);
  }
}

void CommandPreservationLayer::Post(
    ID3D12GraphicsCommandList4EmitRaytracingAccelerationStructurePostbuildInfoCommand& c) {
  unsigned index = 0;
  c.m_pDesc.Value->DestBuffer = m_CaptureGpuAddresses[index++];
  for (unsigned i = 0; i < c.m_NumSourceAccelerationStructures.Value; ++i) {
    c.m_pSourceAccelerationStructureData.Value[i] = m_CaptureGpuAddresses[index++];
  }
  m_CaptureGpuAddresses.clear();
}

} // namespace DirectX
} // namespace gits
