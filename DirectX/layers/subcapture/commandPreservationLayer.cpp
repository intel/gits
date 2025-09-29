// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "commandPreservationLayer.h"
#include "intelExtensions.h"

namespace gits {
namespace DirectX {

void CommandPreservationLayer::pre(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  captureGpuAddresses_.push_back(c.pDesc_.value->DestAccelerationStructureData);
  if (c.pDesc_.value->SourceAccelerationStructureData) {
    captureGpuAddresses_.push_back(c.pDesc_.value->SourceAccelerationStructureData);
  }
  captureGpuAddresses_.push_back(c.pDesc_.value->ScratchAccelerationStructureData);

  if (c.pDesc_.value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    captureGpuAddresses_.push_back(c.pDesc_.value->Inputs.InstanceDescs);
  } else if (c.pDesc_.value->Inputs.Type ==
             D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
    for (unsigned i = 0; i < c.pDesc_.value->Inputs.NumDescs; ++i) {
      D3D12_RAYTRACING_GEOMETRY_DESC& desc = const_cast<D3D12_RAYTRACING_GEOMETRY_DESC&>(
          c.pDesc_.value->Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY
              ? c.pDesc_.value->Inputs.pGeometryDescs[i]
              : *c.pDesc_.value->Inputs.ppGeometryDescs[i]);
      if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES) {
        captureGpuAddresses_.push_back(desc.Triangles.Transform3x4);
        captureGpuAddresses_.push_back(desc.Triangles.IndexBuffer);
        captureGpuAddresses_.push_back(desc.Triangles.VertexBuffer.StartAddress);
      } else if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS) {
        captureGpuAddresses_.push_back(desc.AABBs.AABBs.StartAddress);
      }
    }
  }

  for (unsigned i = 0; i < c.NumPostbuildInfoDescs_.value; ++i) {
    captureGpuAddresses_.push_back(c.pPostbuildInfoDescs_.value[i].DestBuffer);
  }
}

void CommandPreservationLayer::post(
    ID3D12GraphicsCommandList4BuildRaytracingAccelerationStructureCommand& c) {
  unsigned index = 0;
  c.pDesc_.value->DestAccelerationStructureData = captureGpuAddresses_[index++];
  if (c.pDesc_.value->SourceAccelerationStructureData) {
    c.pDesc_.value->SourceAccelerationStructureData = captureGpuAddresses_[index++];
  }
  c.pDesc_.value->ScratchAccelerationStructureData = captureGpuAddresses_[index++];

  if (c.pDesc_.value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    c.pDesc_.value->Inputs.InstanceDescs = captureGpuAddresses_[index++];
  } else if (c.pDesc_.value->Inputs.Type ==
             D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
    for (unsigned i = 0; i < c.pDesc_.value->Inputs.NumDescs; ++i) {
      D3D12_RAYTRACING_GEOMETRY_DESC& desc = const_cast<D3D12_RAYTRACING_GEOMETRY_DESC&>(
          c.pDesc_.value->Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY
              ? c.pDesc_.value->Inputs.pGeometryDescs[i]
              : *c.pDesc_.value->Inputs.ppGeometryDescs[i]);
      if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES) {
        desc.Triangles.Transform3x4 = captureGpuAddresses_[index++];
        desc.Triangles.IndexBuffer = captureGpuAddresses_[index++];
        desc.Triangles.VertexBuffer.StartAddress = captureGpuAddresses_[index++];
      } else if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS) {
        desc.AABBs.AABBs.StartAddress = captureGpuAddresses_[index++];
      }
    }
  }

  for (unsigned i = 0; i < c.NumPostbuildInfoDescs_.value; ++i) {
    c.pPostbuildInfoDescs_.value[i].DestBuffer = captureGpuAddresses_[index++];
  }
  captureGpuAddresses_.clear();
}

void CommandPreservationLayer::pre(ID3D12GraphicsCommandList4DispatchRaysCommand& c) {
  captureGpuAddresses_.push_back(c.pDesc_.value->RayGenerationShaderRecord.StartAddress);
  captureGpuAddresses_.push_back(c.pDesc_.value->MissShaderTable.StartAddress);
  captureGpuAddresses_.push_back(c.pDesc_.value->HitGroupTable.StartAddress);
  captureGpuAddresses_.push_back(c.pDesc_.value->CallableShaderTable.StartAddress);
}

void CommandPreservationLayer::post(ID3D12GraphicsCommandList4DispatchRaysCommand& c) {
  c.pDesc_.value->RayGenerationShaderRecord.StartAddress = captureGpuAddresses_[0];
  c.pDesc_.value->MissShaderTable.StartAddress = captureGpuAddresses_[1];
  c.pDesc_.value->HitGroupTable.StartAddress = captureGpuAddresses_[2];
  c.pDesc_.value->CallableShaderTable.StartAddress = captureGpuAddresses_[3];
  captureGpuAddresses_.clear();
}

void CommandPreservationLayer::pre(ID3D12ResourceGetGPUVirtualAddressCommand& c) {
  captureGpuAddresses_.push_back(c.result_.value);
}

void CommandPreservationLayer::post(ID3D12ResourceGetGPUVirtualAddressCommand& c) {
  c.result_.value = captureGpuAddresses_[0];
  captureGpuAddresses_.clear();
}

void CommandPreservationLayer::pre(ID3D12StateObjectPropertiesGetShaderIdentifierCommand& c) {
  for (unsigned i = 0; i < D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES; ++i) {
    captureShaderIdentifier_[i] = static_cast<uint8_t*>(c.result_.value)[i];
  }
}

void CommandPreservationLayer::post(ID3D12StateObjectPropertiesGetShaderIdentifierCommand& c) {
  for (unsigned i = 0; i < D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES; ++i) {
    static_cast<uint8_t*>(c.result_.value)[i] = captureShaderIdentifier_[i];
  }
}

void CommandPreservationLayer::pre(
    ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& c) {
  captureGpuDescriptorHandle_ = c.result_.value;
}

void CommandPreservationLayer::post(
    ID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStartCommand& c) {
  c.result_.value = captureGpuDescriptorHandle_;
}

void CommandPreservationLayer::pre(INTC_D3D12_CreateComputePipelineStateCommand& c) {
  c.pDesc_.cs = c.pDesc_.value->CS.pShaderBytecode;
  c.pDesc_.compileOptions = c.pDesc_.value->CompileOptions;
  c.pDesc_.internalOptions = c.pDesc_.value->InternalOptions;
}

void CommandPreservationLayer::pre(ID3D12DeviceCreateConstantBufferViewCommand& c) {
  if (c.pDesc_.value && c.pDesc_.value->BufferLocation) {
    captureGpuAddresses_.push_back(c.pDesc_.value->BufferLocation);
  }
}

void CommandPreservationLayer::post(ID3D12DeviceCreateConstantBufferViewCommand& c) {
  if (c.pDesc_.value && c.pDesc_.value->BufferLocation) {
    c.pDesc_.value->BufferLocation = captureGpuAddresses_[0];
    captureGpuAddresses_.clear();
  }
}

void CommandPreservationLayer::pre(ID3D12GraphicsCommandListIASetIndexBufferCommand& c) {
  if (c.pView_.value && c.pView_.value->BufferLocation) {
    captureGpuAddresses_.push_back(c.pView_.value->BufferLocation);
  }
}

void CommandPreservationLayer::post(ID3D12GraphicsCommandListIASetIndexBufferCommand& c) {
  if (c.pView_.value && c.pView_.value->BufferLocation) {
    c.pView_.value->BufferLocation = captureGpuAddresses_[0];
    captureGpuAddresses_.clear();
  }
}

void CommandPreservationLayer::pre(ID3D12GraphicsCommandListIASetVertexBuffersCommand& c) {
  if (c.pViews_.value) {
    for (unsigned i = 0; i < c.NumViews_.value; ++i) {
      if (c.pViews_.value[i].BufferLocation) {
        captureGpuAddresses_.push_back(c.pViews_.value[i].BufferLocation);
      }
    }
  }
}

void CommandPreservationLayer::post(ID3D12GraphicsCommandListIASetVertexBuffersCommand& c) {
  if (c.pViews_.value) {
    unsigned index = 0;
    for (unsigned i = 0; i < c.NumViews_.value; ++i) {
      if (c.pViews_.value[i].BufferLocation) {
        c.pViews_.value[i].BufferLocation = captureGpuAddresses_[index++];
      }
    }
    captureGpuAddresses_.clear();
  }
}

void CommandPreservationLayer::pre(ID3D12GraphicsCommandListSOSetTargetsCommand& c) {
  if (c.pViews_.value) {
    for (unsigned i = 0; i < c.NumViews_.value; ++i) {
      if (c.pViews_.value[i].SizeInBytes) {
        if (c.pViews_.value[i].BufferLocation) {
          captureGpuAddresses_.push_back(c.pViews_.value[i].BufferLocation);
        }
        if (c.pViews_.value[i].BufferFilledSizeLocation) {
          captureGpuAddresses_.push_back(c.pViews_.value[i].BufferFilledSizeLocation);
        }
      }
    }
  }
}

void CommandPreservationLayer::post(ID3D12GraphicsCommandListSOSetTargetsCommand& c) {
  if (c.pViews_.value) {
    unsigned index = 0;
    for (unsigned i = 0; i < c.NumViews_.value; ++i) {
      if (c.pViews_.value[i].SizeInBytes) {
        if (c.pViews_.value[i].BufferLocation) {
          c.pViews_.value[i].BufferLocation = captureGpuAddresses_[index++];
        }
        if (c.pViews_.value[i].BufferFilledSizeLocation) {
          c.pViews_.value[i].BufferFilledSizeLocation = captureGpuAddresses_[index++];
        }
      }
    }
  }
  captureGpuAddresses_.clear();
}

void CommandPreservationLayer::pre(ID3D12GraphicsCommandList2WriteBufferImmediateCommand& c) {
  if (c.pParams_.value) {
    for (unsigned i = 0; i < c.Count_.value; ++i) {
      if (c.pParams_.value[i].Dest) {
        captureGpuAddresses_.push_back(c.pParams_.value[i].Dest);
      }
    }
  }
}

void CommandPreservationLayer::post(ID3D12GraphicsCommandList2WriteBufferImmediateCommand& c) {
  if (c.pParams_.value) {
    unsigned index = 0;
    for (unsigned i = 0; i < c.Count_.value; ++i) {
      if (c.pParams_.value[i].Dest) {
        c.pParams_.value[i].Dest = captureGpuAddresses_[index++];
      }
    }
    captureGpuAddresses_.clear();
  }
}

void CommandPreservationLayer::pre(ID3D12DeviceCreateShaderResourceViewCommand& c) {
  if (c.pDesc_.value &&
      c.pDesc_.value->ViewDimension == D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE) {
    captureGpuAddresses_.push_back(c.pDesc_.value->RaytracingAccelerationStructure.Location);
  }
}

void CommandPreservationLayer::post(ID3D12DeviceCreateShaderResourceViewCommand& c) {
  if (c.pDesc_.value &&
      c.pDesc_.value->ViewDimension == D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE) {
    c.pDesc_.value->RaytracingAccelerationStructure.Location = captureGpuAddresses_[0];
    captureGpuAddresses_.clear();
  }
}

void CommandPreservationLayer::pre(
    ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand& c) {
  captureGpuAddresses_.push_back(c.BufferLocation_.value);
}

void CommandPreservationLayer::post(
    ID3D12GraphicsCommandListSetComputeRootConstantBufferViewCommand& c) {
  c.BufferLocation_.value = captureGpuAddresses_[0];
  captureGpuAddresses_.clear();
}

void CommandPreservationLayer::pre(
    ID3D12GraphicsCommandListSetGraphicsRootConstantBufferViewCommand& c) {
  captureGpuAddresses_.push_back(c.BufferLocation_.value);
}

void CommandPreservationLayer::post(
    ID3D12GraphicsCommandListSetGraphicsRootConstantBufferViewCommand& c) {
  c.BufferLocation_.value = captureGpuAddresses_[0];
  captureGpuAddresses_.clear();
}

void CommandPreservationLayer::pre(
    ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand& c) {
  captureGpuAddresses_.push_back(c.BufferLocation_.value);
}

void CommandPreservationLayer::post(
    ID3D12GraphicsCommandListSetComputeRootShaderResourceViewCommand& c) {
  c.BufferLocation_.value = captureGpuAddresses_[0];
  captureGpuAddresses_.clear();
}

void CommandPreservationLayer::pre(
    ID3D12GraphicsCommandListSetGraphicsRootShaderResourceViewCommand& c) {
  captureGpuAddresses_.push_back(c.BufferLocation_.value);
}

void CommandPreservationLayer::post(
    ID3D12GraphicsCommandListSetGraphicsRootShaderResourceViewCommand& c) {
  c.BufferLocation_.value = captureGpuAddresses_[0];
  captureGpuAddresses_.clear();
}

void CommandPreservationLayer::pre(
    ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& c) {
  captureGpuAddresses_.push_back(c.BufferLocation_.value);
}

void CommandPreservationLayer::post(
    ID3D12GraphicsCommandListSetComputeRootUnorderedAccessViewCommand& c) {
  c.BufferLocation_.value = captureGpuAddresses_[0];
  captureGpuAddresses_.clear();
}

void CommandPreservationLayer::pre(
    ID3D12GraphicsCommandListSetGraphicsRootUnorderedAccessViewCommand& c) {
  captureGpuAddresses_.push_back(c.BufferLocation_.value);
}

void CommandPreservationLayer::post(
    ID3D12GraphicsCommandListSetGraphicsRootUnorderedAccessViewCommand& c) {
  c.BufferLocation_.value = captureGpuAddresses_[0];
  captureGpuAddresses_.clear();
}

void CommandPreservationLayer::pre(
    ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c) {
  captureGpuAddresses_.push_back(c.DestAccelerationStructureData_.value);
  captureGpuAddresses_.push_back(c.SourceAccelerationStructureData_.value);
}

void CommandPreservationLayer::post(
    ID3D12GraphicsCommandList4CopyRaytracingAccelerationStructureCommand& c) {
  c.DestAccelerationStructureData_.value = captureGpuAddresses_[0];
  c.SourceAccelerationStructureData_.value = captureGpuAddresses_[1];
  captureGpuAddresses_.clear();
}

void CommandPreservationLayer::pre(
    ID3D12GraphicsCommandList4EmitRaytracingAccelerationStructurePostbuildInfoCommand& c) {
  captureGpuAddresses_.push_back(c.pDesc_.value->DestBuffer);
  for (unsigned i = 0; i < c.NumSourceAccelerationStructures_.value; ++i) {
    captureGpuAddresses_.push_back(c.pSourceAccelerationStructureData_.value[i]);
  }
}

void CommandPreservationLayer::post(
    ID3D12GraphicsCommandList4EmitRaytracingAccelerationStructurePostbuildInfoCommand& c) {
  unsigned index = 0;
  c.pDesc_.value->DestBuffer = captureGpuAddresses_[index++];
  for (unsigned i = 0; i < c.NumSourceAccelerationStructures_.value; ++i) {
    c.pSourceAccelerationStructureData_.value[i] = captureGpuAddresses_[index++];
  }
  captureGpuAddresses_.clear();
}

} // namespace DirectX
} // namespace gits
