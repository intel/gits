// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "printArguments.h"
#include "printStructuresAuto.h"
#include "printCustom.h"
#include "printEnumsAuto.h"
#include "log.h"
#include "nvapi.h"

#include <iomanip>
#include <d3dx12/d3dx12_pipeline_state_stream.h>

namespace gits {
namespace DirectX {

FastOStream& operator<<(FastOStream& stream, BufferArgument& arg) {
  return stream << "Buffer{" << arg.Size << "}";
}

FastOStream& operator<<(FastOStream& stream, OutputBufferArgument& arg) {

  stream << "OutputBuffer{";
  if (arg.CaptureValue) {
    stream << arg.CaptureValue;
  } else if (arg.Value) {
    stream << *arg.Value;
  } else {
    stream << "nullptr";
  }
  stream << "}";
  return stream;
}

FastOStream& operator<<(FastOStream& stream, ShaderIdentifierArgument& arg) {
  stream << "ShaderIdentifier{";
  if (arg.Value) {
    for (unsigned i = 0; i < D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES; ++i) {
      PrintHexFull(stream, *(static_cast<uint8_t*>(arg.Value) + i));
    }
  } else {
    stream << "nullptr";
  }
  stream << "}";
  return stream;
}

FastOStream& operator<<(FastOStream& stream,
                        DescriptorHandleArrayArgument<D3D12_CPU_DESCRIPTOR_HANDLE>& arg) {

  if (!arg.Value) {
    return stream << "nullptr";
  }

  stream << "D3D12_CPU_DESCRIPTOR_HANDLE[";
  for (unsigned i = 0; i < arg.Size; ++i) {
    if (i > 0) {
      stream << ", ";
    }
    stream << "{";
    printObjectKey(stream, arg.InterfaceKeys[i]);
    stream << ", " << arg.Indexes[i] << "}";
  }
  stream << "]";
  return stream;
}

FastOStream& operator<<(FastOStream& stream,
                        DescriptorHandleArgument<D3D12_CPU_DESCRIPTOR_HANDLE>& arg) {
  stream << "D3D12_CPU_DESCRIPTOR_HANDLE{";
  printObjectKey(stream, arg.InterfaceKey);
  stream << ", " << arg.Index << "}";
  return stream;
}

FastOStream& operator<<(FastOStream& stream,
                        DescriptorHandleArgument<D3D12_GPU_DESCRIPTOR_HANDLE>& arg) {
  stream << "D3D12_GPU_DESCRIPTOR_HANDLE{";
  printObjectKey(stream, arg.InterfaceKey);
  stream << ", " << arg.Index << "}";
  return stream;
}

FastOStream& operator<<(FastOStream& stream, LPCWSTR_Argument& arg) {
  printString(stream, arg.Value);
  return stream;
}

FastOStream& operator<<(FastOStream& stream, LPCSTR_Argument& arg) {
  printString(stream, arg.Value);
  return stream;
}

FastOStream& operator<<(FastOStream& stream, D3D12_GPU_VIRTUAL_ADDRESS_Argument& arg) {
  stream << "D3D12_GPU_VIRTUAL_ADDRESS{";
  printObjectKey(stream, arg.InterfaceKey);
  stream << ", " << arg.Offset << "}";
  return stream;
}

FastOStream& operator<<(FastOStream& stream, D3D12_GPU_VIRTUAL_ADDRESSs_Argument& arg) {
  if (!arg.Value) {
    return stream << "nullptr";
  }
  stream << "D3D12_GPU_VIRTUAL_ADDRESS[";
  for (unsigned i = 0; i < arg.Size; ++i) {
    if (i > 0) {
      stream << ", ";
    }
    stream << "{";
    printObjectKey(stream, arg.InterfaceKeys[i]);
    stream << ", " << arg.Offsets[i] << "}";
  }
  return stream << "]";
}

FastOStream& operator<<(FastOStream& stream, D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument& arg) {
  stream << "D3D12_GRAPHICS_PIPELINE_STATE_DESC{";
  printObjectKey(stream, arg.RootSignatureKey);
  stream << ", ";
  stream << arg.Value->VS << ", ";
  stream << arg.Value->PS << ", ";
  stream << arg.Value->DS << ", ";
  stream << arg.Value->HS << ", ";
  stream << arg.Value->GS << ", ";
  stream << arg.Value->StreamOutput << ", ";
  stream << arg.Value->BlendState << ", ";
  stream << arg.Value->SampleMask << ", ";
  stream << arg.Value->RasterizerState << ", ";
  stream << arg.Value->DepthStencilState << ", ";
  stream << arg.Value->InputLayout << ", ";
  stream << arg.Value->IBStripCutValue << ", ";
  stream << arg.Value->PrimitiveTopologyType << ", ";
  stream << arg.Value->NumRenderTargets << ", ";
  printStaticArray(stream, arg.Value->RTVFormats) << ", ";
  stream << arg.Value->DSVFormat << ", ";
  stream << arg.Value->SampleDesc << ", ";
  stream << arg.Value->NodeMask << ", ";
  stream << arg.Value->CachedPSO << ", ";
  stream << arg.Value->Flags;
  stream << "}";
  return stream;
}

FastOStream& operator<<(FastOStream& stream, D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument& arg) {
  stream << "D3D12_COMPUTE_PIPELINE_STATE_DESC{";
  printObjectKey(stream, arg.RootSignatureKey);
  stream << ", ";
  stream << arg.Value->CS << ", ";
  stream << arg.Value->NodeMask << ", ";
  stream << arg.Value->CachedPSO << ", ";
  stream << arg.Value->Flags;
  stream << "}";
  return stream;
}

FastOStream& operator<<(FastOStream& stream, D3D12_TEXTURE_COPY_LOCATION_Argument& arg) {
  if (!arg.Value) {
    return stream << "nullptr";
  }
  stream << "D3D12_TEXTURE_COPY_LOCATION{";

  printObjectKey(stream, arg.ResourceKey);
  stream << ", " << arg.Value->Type << ", ";
  if (arg.Value->Type == D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX) {
    stream << arg.Value->SubresourceIndex;
  } else {
    stream << arg.Value->PlacedFootprint;
  }
  stream << "}";
  return stream;
}

FastOStream& operator<<(FastOStream& stream, D3D12_RESOURCE_BARRIERs_Argument& arg) {
  if (!arg.Value) {
    return stream << "nullptr";
  }
  stream << "D3D12_RESOURCE_BARRIER[";
  for (unsigned i = 0; i < arg.Size; ++i) {
    stream << "{" << arg.Value[i].Type << ", " << arg.Value[i].Flags << ", ";
    switch (arg.Value[i].Type) {
    case D3D12_RESOURCE_BARRIER_TYPE_TRANSITION:
      stream << "D3D12_RESOURCE_TRANSITION_BARRIER{";
      printObjectKey(stream, arg.ResourceKeys[i]);
      stream << ", ";
      if (arg.Value[i].Transition.Subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES) {
        stream << "D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES";
      } else {
        stream << arg.Value[i].Transition.Subresource;
      }
      stream << ", " << arg.Value[i].Transition.StateBefore << ", "
             << arg.Value[i].Transition.StateAfter;
      break;
    case D3D12_RESOURCE_BARRIER_TYPE_ALIASING:
      stream << "D3D12_RESOURCE_ALIASING_BARRIER{";
      printObjectKey(stream, arg.ResourceKeys[i]);
      stream << ", ";
      printObjectKey(stream, arg.ResourceAfterKeys[i]);
      break;
    case D3D12_RESOURCE_BARRIER_TYPE_UAV:
      stream << "D3D12_RESOURCE_UAV_BARRIER{";
      printObjectKey(stream, arg.ResourceKeys[i]);
      break;
    }
    stream << "}}";
  }
  return stream << "]";
}

FastOStream& operator<<(FastOStream& stream, D3D12_INDEX_BUFFER_VIEW_Argument& arg) {
  if (!arg.Value) {
    return stream << "nullptr";
  }
  stream << "D3D12_INDEX_BUFFER_VIEW{{";
  printObjectKey(stream, arg.BufferLocationKey);
  stream << ", " << arg.BufferLocationOffset << "}, ";
  stream << arg.Value->SizeInBytes << ", " << arg.Value->Format;
  return stream << "}";
}

FastOStream& operator<<(FastOStream& stream, D3D12_CONSTANT_BUFFER_VIEW_DESC_Argument& arg) {
  if (!arg.Value) {
    return stream << "nullptr";
  }
  stream << "D3D12_CONSTANT_BUFFER_VIEW_DESC{{";
  printObjectKey(stream, arg.BufferLocationKey);
  stream << ", " << arg.BufferLocationOffset << "}, ";
  stream << arg.Value->SizeInBytes;
  return stream << "}";
}

FastOStream& operator<<(FastOStream& stream, D3D12_VERTEX_BUFFER_VIEWs_Argument& arg) {
  if (!arg.Value) {
    return stream << "nullptr";
  }
  stream << "D3D12_VERTEX_BUFFER_VIEW[";
  for (unsigned i = 0; i < arg.Size; ++i) {
    stream << "{{";
    printObjectKey(stream, arg.BufferLocationKeys[i]);
    stream << ", " << arg.BufferLocationOffsets[i] << "}, ";
    stream << arg.Value[i].SizeInBytes << ", " << arg.Value[i].StrideInBytes << "}";
  }
  return stream << "]";
}

FastOStream& operator<<(FastOStream& stream, D3D12_STREAM_OUTPUT_BUFFER_VIEWs_Argument& arg) {
  return stream << const_cast<const D3D12_STREAM_OUTPUT_BUFFER_VIEW*>(arg.Value);
}

FastOStream& operator<<(FastOStream& stream, D3D12_WRITEBUFFERIMMEDIATE_PARAMETERs_Argument& arg) {
  if (!arg.Value) {
    return stream << "nullptr";
  }
  stream << "D3D12_WRITEBUFFERIMMEDIATE_PARAMETER[";
  for (unsigned i = 0; i < arg.Size; ++i) {
    if (i > 0) {
      stream << ", ";
    }
    stream << "{{";
    printObjectKey(stream, arg.DestKeys[i]);
    stream << ", " << arg.DestOffsets[i] << "}, " << arg.Value[i].Value << "}";
  }
  return stream << "]";
}

FastOStream& operator<<(FastOStream& stream, D3D12_PIPELINE_STATE_STREAM_DESC_Argument& arg) {
  stream << "D3D12_PIPELINE_STATE_STREAM_DESC{";
  size_t offset = 0;
  while (offset < arg.Value->SizeInBytes) {
    auto pStream = static_cast<uint8_t*>(arg.Value->pPipelineStateSubobjectStream) + offset;
    auto subobjectType = *reinterpret_cast<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE*>(pStream);
    if (offset > 0) {
      stream << ", ";
    }
    switch (subobjectType) {
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE:
      printObjectKey(stream, arg.RootSignatureKey);
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM::pRootSignature);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS: {
      const D3D12_SHADER_BYTECODE& subobject =
          *reinterpret_cast<decltype(CD3DX12_PIPELINE_STATE_STREAM::VS)*>(pStream);
      stream << "VS{" << subobject << "}";
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM::VS);
      break;
    }
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS: {
      const D3D12_SHADER_BYTECODE& subobject =
          *reinterpret_cast<decltype(CD3DX12_PIPELINE_STATE_STREAM::PS)*>(pStream);
      stream << "PS{" << subobject << "}";
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM::PS);
      break;
    }
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DS: {
      const D3D12_SHADER_BYTECODE& subobject =
          *reinterpret_cast<decltype(CD3DX12_PIPELINE_STATE_STREAM::DS)*>(pStream);
      stream << "DS{" << subobject << "}";
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM::DS);
      break;
    }
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_HS: {
      const D3D12_SHADER_BYTECODE& subobject =
          *reinterpret_cast<decltype(CD3DX12_PIPELINE_STATE_STREAM::HS)*>(pStream);
      stream << "HS{" << subobject << "}";
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM::HS);
      break;
    }
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_GS: {
      const D3D12_SHADER_BYTECODE& subobject =
          *reinterpret_cast<decltype(CD3DX12_PIPELINE_STATE_STREAM::GS)*>(pStream);
      stream << "GS{" << subobject << "}";
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM::GS);
      break;
    }
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS: {
      const D3D12_SHADER_BYTECODE& subobject =
          *reinterpret_cast<decltype(CD3DX12_PIPELINE_STATE_STREAM::CS)*>(pStream);
      stream << "CS{" << subobject << "}";
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM::CS);
      break;
    }
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS: {
      const D3D12_SHADER_BYTECODE& subobject =
          *reinterpret_cast<decltype(CD3DX12_PIPELINE_STATE_STREAM2::AS)*>(pStream);
      stream << "AS{" << subobject << "}";
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM2::AS);
      break;
    }
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS: {
      const D3D12_SHADER_BYTECODE& subobject =
          *reinterpret_cast<decltype(CD3DX12_PIPELINE_STATE_STREAM2::MS)*>(pStream);
      stream << "MS{" << subobject << "}";
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM2::MS);
      break;
    }
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_STREAM_OUTPUT: {
      const D3D12_STREAM_OUTPUT_DESC& subobject =
          *reinterpret_cast<decltype(CD3DX12_PIPELINE_STATE_STREAM::StreamOutput)*>(pStream);
      stream << subobject;
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM::StreamOutput);
      break;
    }
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND: {
      const D3D12_BLEND_DESC& subobject =
          *reinterpret_cast<decltype(CD3DX12_PIPELINE_STATE_STREAM::BlendState)*>(pStream);
      stream << subobject;
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM::BlendState);
      break;
    }
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_MASK: {
      const UINT& subobject =
          *reinterpret_cast<decltype(CD3DX12_PIPELINE_STATE_STREAM::SampleMask)*>(pStream);
      stream << "SAMPLE_MASK{" << subobject << "}";
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM::SampleMask);
      break;
    }
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER: {
      const D3D12_RASTERIZER_DESC& subobject =
          *reinterpret_cast<decltype(CD3DX12_PIPELINE_STATE_STREAM::RasterizerState)*>(pStream);
      stream << subobject;
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM::RasterizerState);
      break;
    }
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER1: {
      const D3D12_RASTERIZER_DESC1& subobject =
          *reinterpret_cast<decltype(CD3DX12_PIPELINE_STATE_STREAM4::RasterizerState)*>(pStream);
      stream << subobject;
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM4::RasterizerState);
      break;
    }
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER2: {
      const D3D12_RASTERIZER_DESC2& subobject =
          *reinterpret_cast<decltype(CD3DX12_PIPELINE_STATE_STREAM5::RasterizerState)*>(pStream);
      stream << subobject;
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM5::RasterizerState);
      break;
    }
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL: {
      const D3D12_DEPTH_STENCIL_DESC& subobject =
          *reinterpret_cast<CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL*>(pStream);
      stream << subobject;
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL);
      break;
    }
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL1: {
      const D3D12_DEPTH_STENCIL_DESC1& subobject =
          *reinterpret_cast<decltype(CD3DX12_PIPELINE_STATE_STREAM::DepthStencilState)*>(pStream);
      stream << subobject;
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM::DepthStencilState);
      break;
    }
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL2: {
      const D3D12_DEPTH_STENCIL_DESC2& subobject =
          *reinterpret_cast<decltype(CD3DX12_PIPELINE_STATE_STREAM3::DepthStencilState)*>(pStream);
      stream << subobject;
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM3::DepthStencilState);
      break;
    }
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT: {
      const D3D12_INPUT_LAYOUT_DESC& subobject =
          *reinterpret_cast<decltype(CD3DX12_PIPELINE_STATE_STREAM::InputLayout)*>(pStream);
      stream << subobject;
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM::InputLayout);
      break;
    }
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_IB_STRIP_CUT_VALUE: {
      const D3D12_INDEX_BUFFER_STRIP_CUT_VALUE& subobject =
          *reinterpret_cast<decltype(CD3DX12_PIPELINE_STATE_STREAM::IBStripCutValue)*>(pStream);
      stream << subobject;
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM::IBStripCutValue);
      break;
    }
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY: {
      const D3D12_PRIMITIVE_TOPOLOGY_TYPE& subobject =
          *reinterpret_cast<decltype(CD3DX12_PIPELINE_STATE_STREAM::PrimitiveTopologyType)*>(
              pStream);
      stream << subobject;
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM::PrimitiveTopologyType);
      break;
    }
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS: {
      const D3D12_RT_FORMAT_ARRAY& subobject =
          *reinterpret_cast<decltype(CD3DX12_PIPELINE_STATE_STREAM::RTVFormats)*>(pStream);
      stream << subobject;
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM::RTVFormats);
      break;
    }
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT: {
      const DXGI_FORMAT& subobject =
          *reinterpret_cast<decltype(CD3DX12_PIPELINE_STATE_STREAM::DSVFormat)*>(pStream);
      stream << subobject;
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM::DSVFormat);
      break;
    }
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC: {
      const DXGI_SAMPLE_DESC& subobject =
          *reinterpret_cast<decltype(CD3DX12_PIPELINE_STATE_STREAM::SampleDesc)*>(pStream);
      stream << subobject;
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM::SampleDesc);
      break;
    }
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_NODE_MASK: {
      const UINT& subobject =
          *reinterpret_cast<decltype(CD3DX12_PIPELINE_STATE_STREAM::NodeMask)*>(pStream);
      stream << "NODE_MASK{" << subobject << "}";
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM::NodeMask);
      break;
    }
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CACHED_PSO: {
      const D3D12_CACHED_PIPELINE_STATE& subobject =
          *reinterpret_cast<decltype(CD3DX12_PIPELINE_STATE_STREAM::CachedPSO)*>(pStream);
      stream << subobject;
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM::CachedPSO);
      break;
    }
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_FLAGS: {
      const D3D12_PIPELINE_STATE_FLAGS& subobject =
          *reinterpret_cast<decltype(CD3DX12_PIPELINE_STATE_STREAM::Flags)*>(pStream);
      stream << subobject;
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM::Flags);
      break;
    }
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VIEW_INSTANCING: {
      const D3D12_VIEW_INSTANCING_DESC& subobject =
          *reinterpret_cast<decltype(CD3DX12_PIPELINE_STATE_STREAM1::ViewInstancingDesc)*>(pStream);
      stream << subobject;
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM1::ViewInstancingDesc);
      break;
    }
    default: {
      stream << "Unknown subobject type: " << subobjectType;
      break;
    }
    }
  }

  stream << "}";
  return stream;
}

FastOStream& operator<<(FastOStream& stream, D3D12_STATE_OBJECT_DESC_Argument& arg) {
  if (!arg.Value) {
    return stream << "nullptr";
  }
  stream << "D3D12_STATE_OBJECT_DESC{";
  stream << arg.Value->Type << ", " << arg.Value->NumSubobjects << ", [";
  for (unsigned i = 0; i < arg.Value->NumSubobjects; ++i) {
    const D3D12_STATE_SUBOBJECT& subobject = arg.Value->pSubobjects[i];
    if (i > 0) {
      stream << ", ";
    }
    stream << "{" << subobject.Type << ", {";
    switch (subobject.Type) {
    case D3D12_STATE_SUBOBJECT_TYPE_STATE_OBJECT_CONFIG: {
      auto desc = static_cast<const D3D12_STATE_OBJECT_CONFIG*>(subobject.pDesc);
      stream << desc->Flags;
      break;
    }
    case D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE: {
      auto desc = static_cast<const D3D12_GLOBAL_ROOT_SIGNATURE*>(subobject.pDesc);
      printObjectKey(stream, arg.InterfaceKeysBySubobject[i]);
      break;
    }
    case D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE: {
      auto desc = static_cast<const D3D12_LOCAL_ROOT_SIGNATURE*>(subobject.pDesc);
      printObjectKey(stream, arg.InterfaceKeysBySubobject[i]);
      break;
    }
    case D3D12_STATE_SUBOBJECT_TYPE_NODE_MASK: {
      auto desc = static_cast<const D3D12_NODE_MASK*>(subobject.pDesc);
      stream << desc->NodeMask;
      break;
    }
    case D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY: {
      auto desc = static_cast<const D3D12_DXIL_LIBRARY_DESC*>(subobject.pDesc);
      stream << "{" << desc->DXILLibrary.BytecodeLength << "}, " << desc->NumExports << ", [";
      for (unsigned j = 0; j < desc->NumExports; ++j) {
        if (j > 0) {
          stream << ", ";
        }
        stream << desc->pExports[j];
      }
      stream << "]";
      break;
    }
    case D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION: {
      auto desc = static_cast<const D3D12_EXISTING_COLLECTION_DESC*>(subobject.pDesc);
      printObjectKey(stream, arg.InterfaceKeysBySubobject[i]);
      stream << ", " << desc->NumExports << ", [";
      for (unsigned j = 0; j < desc->NumExports; ++j) {
        if (j > 0) {
          stream << ", ";
        }
        stream << desc->pExports[j];
      }
      stream << "]";
      break;
    }
    case D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION: {
      auto desc = static_cast<const D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(subobject.pDesc);
      for (unsigned j = 0; j < arg.Value->NumSubobjects; ++j) {
        if (desc->pSubobjectToAssociate == &arg.Value->pSubobjects[j]) {
          stream << j;
          break;
        }
      }
      stream << ", " << desc->NumExports << ", [";
      for (unsigned j = 0; j < desc->NumExports; ++j) {
        if (j > 0) {
          stream << ", ";
        }
        printString(stream, desc->pExports[j]);
      }
      stream << "]";
      break;
    }
    case D3D12_STATE_SUBOBJECT_TYPE_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION: {
      auto desc = static_cast<const D3D12_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(subobject.pDesc);
      printString(stream, desc->SubobjectToAssociate);
      stream << ", " << desc->NumExports << ", [";
      for (unsigned j = 0; j < desc->NumExports; ++j) {
        if (j > 0) {
          stream << ", ";
        }
        printString(stream, desc->pExports[j]);
      }
      stream << "]";
      break;
    }
    case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG: {
      auto desc = static_cast<const D3D12_RAYTRACING_SHADER_CONFIG*>(subobject.pDesc);
      stream << desc->MaxPayloadSizeInBytes << ", " << desc->MaxAttributeSizeInBytes;
      break;
    }
    case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG: {
      auto desc = static_cast<const D3D12_RAYTRACING_PIPELINE_CONFIG*>(subobject.pDesc);
      stream << desc->MaxTraceRecursionDepth;
      break;
    }
    case D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP: {
      auto desc = static_cast<const D3D12_HIT_GROUP_DESC*>(subobject.pDesc);
      printString(stream, desc->HitGroupExport);
      stream << ", " << desc->Type << ", ";
      printString(stream, desc->AnyHitShaderImport);
      stream << ", ";
      printString(stream, desc->ClosestHitShaderImport);
      stream << ", ";
      printString(stream, desc->IntersectionShaderImport);
      break;
    }
    case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG1: {
      auto desc = static_cast<const D3D12_RAYTRACING_PIPELINE_CONFIG1*>(subobject.pDesc);
      stream << desc->MaxTraceRecursionDepth << ", " << desc->Flags;
      break;
    }
    case D3D12_STATE_SUBOBJECT_TYPE_WORK_GRAPH: {
      auto desc = static_cast<const D3D12_WORK_GRAPH_DESC*>(subobject.pDesc);
      printString(stream, desc->ProgramName);
      stream << ", " << desc->Flags << ", " << desc->NumEntrypoints << ", [";
      for (unsigned j = 0; j < desc->NumEntrypoints; ++j) {
        if (j > 0) {
          stream << ", ";
        }
        stream << "{";
        printString(stream, desc->pEntrypoints[j].Name);
        stream << ", " << desc->pEntrypoints[j].ArrayIndex;
        stream << "}";
      }
      stream << "]";
      stream << ", " << desc->NumExplicitlyDefinedNodes << ", [";
      for (unsigned j = 0; j < desc->NumExplicitlyDefinedNodes; ++j) {
        if (j > 0) {
          stream << ", ";
        }
        stream << "{";
        stream << desc->pExplicitlyDefinedNodes[j].NodeType << ", {";
        stream << "D3D12_SHADER_NODE";
        stream << "}}";
      }
      stream << "]";
      break;
    }
    case D3D12_STATE_SUBOBJECT_TYPE_STREAM_OUTPUT: {
      auto desc = static_cast<const D3D12_STREAM_OUTPUT_DESC*>(subobject.pDesc);
      break;
    }
    case D3D12_STATE_SUBOBJECT_TYPE_BLEND: {
      auto desc = static_cast<const D3D12_BLEND_DESC*>(subobject.pDesc);
      break;
    }
    case D3D12_STATE_SUBOBJECT_TYPE_SAMPLE_MASK: {
      auto desc = static_cast<const D3D12_SAMPLE_MASK*>(subobject.pDesc);
      break;
    }
    case D3D12_STATE_SUBOBJECT_TYPE_RASTERIZER: {
      auto desc = static_cast<const D3D12_RASTERIZER_DESC2*>(subobject.pDesc);
      break;
    }
    case D3D12_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL: {
      auto desc = static_cast<const D3D12_DEPTH_STENCIL_DESC*>(subobject.pDesc);
      break;
    }
    case D3D12_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT: {
      auto desc = static_cast<const D3D12_INPUT_LAYOUT_DESC*>(subobject.pDesc);
      break;
    }
    case D3D12_STATE_SUBOBJECT_TYPE_IB_STRIP_CUT_VALUE: {
      auto desc = static_cast<const D3D12_IB_STRIP_CUT_VALUE*>(subobject.pDesc);
      break;
    }
    case D3D12_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY: {
      auto desc = static_cast<const D3D12_PRIMITIVE_TOPOLOGY_DESC*>(subobject.pDesc);
      break;
    }
    case D3D12_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS: {
      auto desc = static_cast<const D3D12_RT_FORMAT_ARRAY*>(subobject.pDesc);
      break;
    }
    case D3D12_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT: {
      auto desc = static_cast<const D3D12_DEPTH_STENCIL_FORMAT*>(subobject.pDesc);
      break;
    }
    case D3D12_STATE_SUBOBJECT_TYPE_SAMPLE_DESC: {
      auto desc = static_cast<const DXGI_SAMPLE_DESC*>(subobject.pDesc);
      break;
    }
    case D3D12_STATE_SUBOBJECT_TYPE_FLAGS: {
      auto desc = static_cast<const D3D12_PIPELINE_STATE_FLAGS*>(subobject.pDesc);
      break;
    }
    case D3D12_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL1: {
      auto desc = static_cast<const D3D12_DEPTH_STENCIL_DESC1*>(subobject.pDesc);
      break;
    }
    case D3D12_STATE_SUBOBJECT_TYPE_VIEW_INSTANCING: {
      auto desc = static_cast<const D3D12_VIEW_INSTANCING_DESC*>(subobject.pDesc);
      break;
    }
    case D3D12_STATE_SUBOBJECT_TYPE_GENERIC_PROGRAM: {
      auto desc = static_cast<const D3D12_GENERIC_PROGRAM_DESC*>(subobject.pDesc);
      break;
    }
    case D3D12_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL2: {
      auto desc = static_cast<const D3D12_DEPTH_STENCIL_DESC2*>(subobject.pDesc);
      break;
    }
    }
    stream << "}}";
  }
  return stream << "]}";
}

FastOStream& operator<<(FastOStream& stream, D3D12_SHADER_RESOURCE_VIEW_DESC_Argument& arg) {
  if (!arg.Value) {
    return stream << "nullptr";
  }
  stream << "D3D12_SHADER_RESOURCE_VIEW_DESC{";

  stream << arg.Value->Format << ", " << arg.Value->ViewDimension << ", "
         << arg.Value->Shader4ComponentMapping << ", ";
  switch (arg.Value->ViewDimension) {
  case D3D12_SRV_DIMENSION_BUFFER:
    stream << arg.Value->Buffer;
    break;
  case D3D12_SRV_DIMENSION_TEXTURE1D:
    stream << arg.Value->Texture1D;
    break;
  case D3D12_SRV_DIMENSION_TEXTURE1DARRAY:
    stream << arg.Value->Texture1DArray;
    break;
  case D3D12_SRV_DIMENSION_TEXTURE2D:
    stream << arg.Value->Texture2D;
    break;
  case D3D12_SRV_DIMENSION_TEXTURE2DARRAY:
    stream << arg.Value->Texture2DArray;
    break;
  case D3D12_SRV_DIMENSION_TEXTURE2DMS:
    stream << arg.Value->Texture2DMS;
    break;
  case D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY:
    stream << arg.Value->Texture2DMSArray;
    break;
  case D3D12_SRV_DIMENSION_TEXTURE3D:
    stream << arg.Value->Texture3D;
    break;
  case D3D12_SRV_DIMENSION_TEXTURECUBE:
    stream << arg.Value->Buffer;
    break;
  case D3D12_SRV_DIMENSION_TEXTURECUBEARRAY:
    stream << arg.Value->TextureCubeArray;
    break;
  case D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE:
    stream << "{";
    printObjectKey(stream, arg.RaytracingLocationKey);
    stream << ", " << arg.RaytracingLocationOffset << "}";
    break;
  }
  return stream << "}}";
}

FastOStream& operator<<(FastOStream& stream, ArrayArgument<D3D12_RESIDENCY_PRIORITY>& arg) {
  if (!arg.Value) {
    return stream << "nullptr";
  }

  stream << "[";
  for (int i = 0; i < arg.Size; ++i) {
    if (i > 0) {
      stream << ", ";
    }
    switch (arg.Value[i]) {
    case D3D12_RESIDENCY_PRIORITY_MINIMUM:
      stream << "D3D12_RESIDENCY_PRIORITY_MINIMUM";
      break;
    case D3D12_RESIDENCY_PRIORITY_LOW:
      stream << "D3D12_RESIDENCY_PRIORITY_LOW";
      break;
    case D3D12_RESIDENCY_PRIORITY_NORMAL:
      stream << "D3D12_RESIDENCY_PRIORITY_NORMAL";
      break;
    case D3D12_RESIDENCY_PRIORITY_HIGH:
      stream << "D3D12_RESIDENCY_PRIORITY_HIGH";
      break;
    case D3D12_RESIDENCY_PRIORITY_MAXIMUM:
      stream << "D3D12_RESIDENCY_PRIORITY_MAXIMUM";
      break;
    default:
      stream << "0x";
      PrintHex(stream, static_cast<unsigned>(arg.Value[i]));
      break;
    }
  }
  stream << "]";
  return stream;
}

FastOStream& operator<<(FastOStream& stream, D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument& arg) {
  if (!arg.Value) {
    return stream << "nullptr";
  }

  stream << "D3D12_RENDER_PASS_RENDER_TARGET_DESC[";
  for (int i = 0, j = 0; i < arg.Size; ++i) {
    if (i > 0) {
      stream << ", ";
    }
    stream << "{{";
    stream << arg.Value[i].cpuDescriptor.ptr << ", ";
    printObjectKey(stream, arg.DescriptorKeys[i]);
    stream << ", " << arg.DescriptorIndexes[i];
    stream << "}, {";
    stream << arg.Value[i].BeginningAccess.Type;
    if (arg.Value[i].BeginningAccess.Type == D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR) {
      stream << ", " << arg.Value[i].BeginningAccess.Clear;
    } else if (arg.Value[i].BeginningAccess.Type ==
                   D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE_LOCAL_RENDER ||
               arg.Value[i].BeginningAccess.Type ==
                   D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE_LOCAL_SRV ||
               arg.Value[i].BeginningAccess.Type ==
                   D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE_LOCAL_UAV) {
      stream << ", " << arg.Value[i].BeginningAccess.PreserveLocal;
    }
    stream << "}, {";
    stream << arg.Value[i].EndingAccess.Type;
    if (arg.Value[i].EndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE) {
      stream << ", {";
      printObjectKey(stream, arg.ResolveSrcResourceKeys[j]);
      stream << ", ";
      printObjectKey(stream, arg.ResolveDstResourceKeys[j]);
      stream << ", " << arg.Value[i].EndingAccess.Resolve.SubresourceCount << ", [";
      for (unsigned k = 0; k < arg.Value[i].EndingAccess.Resolve.SubresourceCount; ++k) {
        if (k > 0) {
          stream << ", ";
        }
        stream << "{" << arg.Value[i].EndingAccess.Resolve.pSubresourceParameters[k] << "}";
      }
      stream << "], " << arg.Value[i].EndingAccess.Resolve.Format << ", "
             << arg.Value[i].EndingAccess.Resolve.ResolveMode << ", "
             << arg.Value[i].EndingAccess.Resolve.PreserveResolveSource << "}";
      ++j;
    } else if (arg.Value[i].EndingAccess.Type ==
                   D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE_LOCAL_RENDER ||
               arg.Value[i].EndingAccess.Type ==
                   D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE_LOCAL_SRV ||
               arg.Value[i].EndingAccess.Type ==
                   D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE_LOCAL_UAV) {
      stream << ", " << arg.Value[i].EndingAccess.PreserveLocal;
    }
    stream << "}}";
  }
  stream << "]";
  return stream;
}

FastOStream& operator<<(FastOStream& stream, D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument& arg) {
  if (!arg.Value) {
    return stream << "nullptr";
  }

  stream << "D3D12_RENDER_PASS_DEPTH_STENCIL_DESC{{";
  stream << arg.Value->cpuDescriptor.ptr << ", ";
  printObjectKey(stream, arg.DescriptorKey);
  stream << ", " << arg.DescriptorIndex;
  stream << "}, {";
  stream << arg.Value->DepthBeginningAccess.Type;
  if (arg.Value->DepthBeginningAccess.Type == D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR) {
    stream << ", " << arg.Value->DepthBeginningAccess.Clear;
  } else if (arg.Value->DepthBeginningAccess.Type ==
                 D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE_LOCAL_RENDER ||
             arg.Value->DepthBeginningAccess.Type ==
                 D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE_LOCAL_SRV ||
             arg.Value->DepthBeginningAccess.Type ==
                 D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE_LOCAL_UAV) {
    stream << ", " << arg.Value->DepthBeginningAccess.PreserveLocal;
  }
  stream << "}, {";
  stream << arg.Value->StencilBeginningAccess.Type;
  if (arg.Value->StencilBeginningAccess.Type == D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR) {
    stream << ", " << arg.Value->StencilBeginningAccess.Clear;
  } else if (arg.Value->StencilBeginningAccess.Type ==
                 D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE_LOCAL_RENDER ||
             arg.Value->StencilBeginningAccess.Type ==
                 D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE_LOCAL_SRV ||
             arg.Value->StencilBeginningAccess.Type ==
                 D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE_LOCAL_UAV) {
    stream << ", " << arg.Value->StencilBeginningAccess.PreserveLocal;
  }
  stream << "}, {";
  stream << arg.Value->DepthEndingAccess.Type;
  if (arg.Value->DepthEndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE) {
    stream << ", {";
    printObjectKey(stream, arg.ResolveSrcDepthKey);
    stream << ", ";
    printObjectKey(stream, arg.ResolveDstDepthKey);
    stream << ", " << arg.Value->DepthEndingAccess.Resolve.SubresourceCount << ", [";
    for (unsigned k = 0; k < arg.Value->DepthEndingAccess.Resolve.SubresourceCount; ++k) {
      if (k > 0) {
        stream << ", ";
      }
      stream << "{" << arg.Value->DepthEndingAccess.Resolve.pSubresourceParameters[k] << "}";
    }
    stream << "], " << arg.Value->DepthEndingAccess.Resolve.Format << ", "
           << arg.Value->DepthEndingAccess.Resolve.ResolveMode << ", "
           << arg.Value->DepthEndingAccess.Resolve.PreserveResolveSource << "}";
  } else if (arg.Value->DepthEndingAccess.Type ==
                 D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE_LOCAL_RENDER ||
             arg.Value->DepthEndingAccess.Type ==
                 D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE_LOCAL_SRV ||
             arg.Value->DepthEndingAccess.Type ==
                 D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE_LOCAL_UAV) {
    stream << ", " << arg.Value->DepthEndingAccess.PreserveLocal;
  }
  stream << "}, {";
  stream << arg.Value->DepthEndingAccess.Type;
  if (arg.Value->StencilEndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE) {
    stream << ", {";
    printObjectKey(stream, arg.ResolveSrcStencilKey);
    stream << ", ";
    printObjectKey(stream, arg.ResolveDstStencilKey);
    stream << ", " << arg.Value->StencilEndingAccess.Resolve.SubresourceCount << ", [";
    for (unsigned k = 0; k < arg.Value->StencilEndingAccess.Resolve.SubresourceCount; ++k) {
      if (k > 0) {
        stream << ", ";
      }
      stream << "{" << arg.Value->StencilEndingAccess.Resolve.pSubresourceParameters[k] << "}";
    }
    stream << "], " << arg.Value->StencilEndingAccess.Resolve.Format << ", "
           << arg.Value->StencilEndingAccess.Resolve.ResolveMode << ", "
           << arg.Value->StencilEndingAccess.Resolve.PreserveResolveSource << "}";
  } else if (arg.Value->StencilEndingAccess.Type ==
                 D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE_LOCAL_RENDER ||
             arg.Value->StencilEndingAccess.Type ==
                 D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE_LOCAL_SRV ||
             arg.Value->StencilEndingAccess.Type ==
                 D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE_LOCAL_UAV) {
    stream << ", " << arg.Value->StencilEndingAccess.PreserveLocal;
  }
  stream << "}";

  return stream;
}

FastOStream& operator<<(FastOStream& stream, D3D12_EXTENSION_ARGUMENTS_Argument& arg) {
  if (!arg.Value) {
    return stream << "nullptr";
  }

  GITS_ASSERT(false, "operator<< not implemented for D3D12_EXTENSION_ARGUMENTS_Argument");
  return stream;
}

FastOStream& operator<<(FastOStream& stream, D3D12_EXTENDED_OPERATION_DATA_Argument& arg) {
  if (!arg.Value) {
    return stream << "nullptr";
  }

  GITS_ASSERT(false, "operator<< not implemented for D3D12_EXTENDED_OPERATION_DATA_Argument");
  return stream;
}

FastOStream& operator<<(FastOStream& stream,
                        PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC>& arg) {
  if (!arg.Value) {
    return stream << "nullptr";
  }
  stream << "D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC{{";
  printObjectKey(stream, arg.DestAccelerationStructureKey);
  stream << ", " << arg.DestAccelerationStructureOffset << "} (0x";
  PrintHex(stream, arg.Value->DestAccelerationStructureData) << "), {";

  stream << arg.Value->Inputs.Type << ", " << arg.Value->Inputs.Flags << ", "
         << arg.Value->Inputs.NumDescs << ", " << arg.Value->Inputs.DescsLayout << ", ";
  if (!arg.InputKeys.empty() &&
      arg.Value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    stream << "{";
    printObjectKey(stream, arg.InputKeys[0]);
    stream << ", " << arg.InputOffsets[0] << "}";
  } else if (arg.Value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
    unsigned inputIndex = 0;
    stream << "[";
    for (unsigned i = 0; i < arg.Value->Inputs.NumDescs; ++i) {
      D3D12_RAYTRACING_GEOMETRY_DESC& desc = const_cast<D3D12_RAYTRACING_GEOMETRY_DESC&>(
          arg.Value->Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY
              ? arg.Value->Inputs.pGeometryDescs[i]
              : *arg.Value->Inputs.ppGeometryDescs[i]);

      if (i > 0) {
        stream << ", ";
      }
      stream << "{" << desc.Type << ", " << desc.Flags << ", {";

      if (!arg.InputKeys.empty() && desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES) {
        stream << "{";
        printObjectKey(stream, arg.InputKeys[inputIndex]);
        stream << ", " << arg.InputOffsets[inputIndex] << "}, ";
        ++inputIndex;
        stream << desc.Triangles.IndexFormat << ", " << desc.Triangles.VertexFormat << ", "
               << desc.Triangles.IndexCount << ", " << desc.Triangles.VertexCount << ", {";
        printObjectKey(stream, arg.InputKeys[inputIndex]);
        stream << ", " << arg.InputOffsets[inputIndex] << "}, {{";
        ++inputIndex;
        printObjectKey(stream, arg.InputKeys[inputIndex]);
        stream << ", " << arg.InputOffsets[inputIndex] << "}, ";
        ++inputIndex;
        stream << desc.Triangles.VertexBuffer.StrideInBytes << "}";
      } else if (!arg.InputKeys.empty() &&
                 desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS) {
        stream << desc.AABBs.AABBCount << ", {{";
        printObjectKey(stream, arg.InputKeys[inputIndex]);
        stream << ", " << arg.InputOffsets[inputIndex] << "}, ";
        ++inputIndex;
        stream << desc.AABBs.AABBs.StrideInBytes << "}";
      } else if (!arg.InputKeys.empty() &&
                 desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES) {
        stream << "{{";
        if (desc.OmmTriangles.pTriangles) {
          auto& triangles = *desc.OmmTriangles.pTriangles;
          stream << "{";
          printObjectKey(stream, arg.InputKeys[inputIndex]);
          stream << ", " << arg.InputOffsets[inputIndex] << "}, ";
          ++inputIndex;
          stream << triangles.IndexFormat << ", " << triangles.VertexFormat << ", "
                 << triangles.IndexCount << ", " << triangles.VertexCount << ", {";
          printObjectKey(stream, arg.InputKeys[inputIndex]);
          stream << ", " << arg.InputOffsets[inputIndex] << "}, {{";
          ++inputIndex;
          printObjectKey(stream, arg.InputKeys[inputIndex]);
          stream << ", " << arg.InputOffsets[inputIndex] << "}, ";
          ++inputIndex;
          stream << triangles.VertexBuffer.StrideInBytes << "}";
        } else {
          stream << "nullptr";
        }
        stream << "}, {";
        if (desc.OmmTriangles.pOmmLinkage) {
          auto& ommLinkage = *desc.OmmTriangles.pOmmLinkage;
          stream << "{{";
          printObjectKey(stream, arg.InputKeys[inputIndex]);
          stream << ", " << arg.InputOffsets[inputIndex] << "}, ";
          ++inputIndex;
          stream << ommLinkage.OpacityMicromapIndexBuffer.StrideInBytes << "}, ";
          stream << ommLinkage.OpacityMicromapIndexFormat << ", ";
          stream << ommLinkage.OpacityMicromapBaseLocation << ", {";
          printObjectKey(stream, arg.InputKeys[inputIndex]);
          stream << ", " << arg.InputOffsets[inputIndex] << "}";

        } else {
          stream << "nullptr";
        }
        stream << "}}";
      }
      stream << "}}";
    }
    stream << "]";
  } else if (!arg.InputKeys.empty() &&
             arg.Value->Inputs.Type ==
                 D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_OPACITY_MICROMAP_ARRAY) {
    unsigned inputIndex = 0;
    stream << "[{" << arg.Value->Inputs.pOpacityMicromapArrayDesc->NumOmmHistogramEntries << ", [";
    for (unsigned i = 0; i < arg.Value->Inputs.pOpacityMicromapArrayDesc->NumOmmHistogramEntries;
         ++i) {
      if (i > 0) {
        stream << ", ";
      }
      const auto& entry = arg.Value->Inputs.pOpacityMicromapArrayDesc->pOmmHistogram[i];
      stream << "{" << entry.Count << ", " << entry.SubdivisionLevel << ", " << entry.Format << "}";
    }
    stream << "], {";
    printObjectKey(stream, arg.InputKeys[inputIndex]);
    stream << ", " << arg.InputOffsets[inputIndex] << "}, {{";
    ++inputIndex;
    printObjectKey(stream, arg.InputKeys[inputIndex]);
    stream << ", " << arg.InputOffsets[inputIndex] << "}, ";
    ++inputIndex;
    stream << arg.Value->Inputs.pOpacityMicromapArrayDesc->PerOmmDescs.StrideInBytes << "}";
    stream << "}]";
  }

  stream << "}, {";
  printObjectKey(stream, arg.SourceAccelerationStructureKey);
  stream << ", " << arg.SourceAccelerationStructureOffset << "}, {";
  printObjectKey(stream, arg.ScratchAccelerationStructureKey);
  stream << ", " << arg.ScratchAccelerationStructureOffset << "}";
  stream << "}";
  return stream;
}

FastOStream& operator<<(
    FastOStream& stream,
    ArrayArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>& arg) {
  if (!arg.Value) {
    return stream << "nullptr";
  }

  stream << "[";
  for (unsigned i = 0; i < arg.DestBufferKeys.size(); ++i) {
    if (i > 0) {
      stream << ", ";
    }
    stream << "D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC{{";
    printObjectKey(stream, arg.DestBufferKeys[i]);
    stream << ", " << arg.DestBufferOffsets[i] << "}, ";
    stream << arg.Value->InfoType;
    stream << "}";
  }
  stream << "]";
  return stream;
}

FastOStream& operator<<(
    FastOStream& stream,
    PointerArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>& arg) {
  if (!arg.Value) {
    return stream << "nullptr";
  }

  stream << "D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC{{";
  printObjectKey(stream, arg.destBufferKey);
  stream << ", " << arg.destBufferOffset << "}, ";
  stream << arg.Value->InfoType;
  stream << "}";
  return stream;
}

FastOStream& operator<<(FastOStream& stream,
                        PointerArgument<D3D12_UNORDERED_ACCESS_VIEW_DESC>& arg) {
  if (!arg.Value) {
    return stream << "nullptr";
  }

  stream << "D3D12_UNORDERED_ACCESS_VIEW_DESC{";
  stream << arg.Value->Format << ", ";
  stream << arg.Value->ViewDimension << ", ";
  stream << toStr(arg.Value->ViewDimension) << "{";
  switch (arg.Value->ViewDimension) {
  case D3D12_UAV_DIMENSION_BUFFER:
    stream << arg.Value->Buffer;
    break;
  case D3D12_UAV_DIMENSION_TEXTURE1D:
    stream << arg.Value->Texture1D;
    break;
  case D3D12_UAV_DIMENSION_TEXTURE1DARRAY:
    stream << arg.Value->Texture1DArray;
    break;
  case D3D12_UAV_DIMENSION_TEXTURE2D:
    stream << arg.Value->Texture2D;
    break;
  case D3D12_UAV_DIMENSION_TEXTURE2DARRAY:
    stream << arg.Value->Texture2DArray;
    break;
  case D3D12_UAV_DIMENSION_TEXTURE2DMS:
    stream << arg.Value->Texture2DMS;
    break;
  case D3D12_UAV_DIMENSION_TEXTURE2DMSARRAY:
    stream << arg.Value->Texture2DMSArray;
    break;
  case D3D12_UAV_DIMENSION_TEXTURE3D:
    stream << arg.Value->Texture3D;
    break;
  case D3D12_UAV_DIMENSION_BUFFER_BYTE_OFFSET:
    stream << arg.Value->BufferByteOffset;
    break;
  }
  stream << "}}";
  return stream;
}

FastOStream& operator<<(FastOStream& stream, D3D12_BARRIER_GROUPs_Argument& arg) {
  if (!arg.Value) {
    return stream << "nullptr";
  }
  unsigned resourceKeyIndex{};
  stream << "D3D12_BARRIER_GROUP[";
  for (unsigned i = 0; i < arg.Size; ++i) {
    if (i > 0) {
      stream << ", ";
    }
    stream << "{" << arg.Value[i].Type << ", " << arg.Value[i].NumBarriers << ", ";
    if (arg.Value[i].Type == D3D12_BARRIER_TYPE_GLOBAL) {
      stream << "D3D12_GLOBAL_BARRIER[";
      for (unsigned j = 0; j < arg.Value[i].NumBarriers; ++j) {
        if (j > 0) {
          stream << ", ";
        }
        const D3D12_GLOBAL_BARRIER& barrier = arg.Value[i].pGlobalBarriers[j];
        stream << "{" << barrier.SyncBefore << ", " << barrier.SyncAfter << ", "
               << barrier.AccessBefore << ", " << barrier.AccessAfter << "}";
      }
      stream << "]";
    } else if (arg.Value[i].Type == D3D12_BARRIER_TYPE_TEXTURE) {
      stream << "D3D12_TEXTURE_BARRIER[";
      for (unsigned j = 0; j < arg.Value[i].NumBarriers; ++j) {
        if (j > 0) {
          stream << ", ";
        }
        const D3D12_TEXTURE_BARRIER& barrier = arg.Value[i].pTextureBarriers[j];
        stream << "{" << barrier.SyncBefore << ", " << barrier.SyncAfter << ", "
               << barrier.AccessBefore << ", " << barrier.AccessAfter << ", "
               << barrier.LayoutBefore << ", " << barrier.LayoutAfter << ", ";
        printObjectKey(stream, arg.ResourceKeys[resourceKeyIndex++]);
        stream << ", " << barrier.Subresources << ", " << barrier.Flags << "}";
      }
      stream << "]";
    } else if (arg.Value[i].Type == D3D12_BARRIER_TYPE_BUFFER) {
      stream << "D3D12_BUFFER_BARRIER[";
      for (unsigned j = 0; j < arg.Value[i].NumBarriers; ++j) {
        if (j > 0) {
          stream << ", ";
        }
        const D3D12_BUFFER_BARRIER& barrier = arg.Value[i].pBufferBarriers[j];
        stream << "{" << barrier.SyncBefore << ", " << barrier.SyncAfter << ", "
               << barrier.AccessBefore << ", " << barrier.AccessAfter << ", ";
        printObjectKey(stream, arg.ResourceKeys[resourceKeyIndex++]);
        stream << ", " << barrier.Offset << ", " << barrier.Size << "}";
      }
      stream << "]";
    }
    stream << "}";
  }
  return stream << "]";
}

FastOStream& operator<<(FastOStream& stream, PointerArgument<D3D12_DISPATCH_RAYS_DESC>& arg) {
  if (!arg.Value) {
    return stream << "nullptr";
  }
  stream << "D3D12_DISPATCH_RAYS_DESC{";

  stream << "{{";
  printObjectKey(stream, arg.RayGenerationShaderRecordKey);
  stream << ", " << arg.RayGenerationShaderRecordOffset << "}, ";
  stream << arg.Value->RayGenerationShaderRecord.SizeInBytes << "}, ";

  stream << "{{";
  printObjectKey(stream, arg.MissShaderTableKey);
  stream << ", " << arg.MissShaderTableOffset << "}, ";
  stream << arg.Value->MissShaderTable.SizeInBytes << ", "
         << arg.Value->MissShaderTable.StrideInBytes << "}, ";

  stream << "{{";
  printObjectKey(stream, arg.HitGroupTableKey);
  stream << ", " << arg.HitGroupTableOffset << "}, ";
  stream << arg.Value->HitGroupTable.SizeInBytes << ", " << arg.Value->HitGroupTable.StrideInBytes
         << "}, ";

  stream << "{{";
  printObjectKey(stream, arg.CallableShaderTableKey);
  stream << ", " << arg.CallableShaderTableOffset << "}, ";
  stream << arg.Value->CallableShaderTable.SizeInBytes << ", "
         << arg.Value->CallableShaderTable.StrideInBytes << "}, ";

  stream << arg.Value->Width << ", " << arg.Value->Height << ", " << arg.Value->Depth;

  stream << "}";
  return stream;
}

FastOStream& operator<<(FastOStream& stream,
                        PointerArgument<D3D12_VERSIONED_ROOT_SIGNATURE_DESC>& arg) {
  if (!arg.Value) {
    return stream << "nullptr";
  }
  stream << "D3D12_VERSIONED_ROOT_SIGNATURE_DESC{";
  stream << arg.Value->Version << ", ";
  switch (arg.Value->Version) {
  case D3D_ROOT_SIGNATURE_VERSION_1_0:
    break;
  case D3D_ROOT_SIGNATURE_VERSION_1_1:
    break;
  case D3D_ROOT_SIGNATURE_VERSION_1_2:
    break;
  }
  stream << "}";
  return stream;
}

FastOStream& operator<<(FastOStream& stream, DML_BINDING_TABLE_DESC_Argument& arg) {
  if (!arg.Value) {
    return stream << "nullptr";
  }
  stream << "DML_BINDING_TABLE_DESC{";
  printObjectKey(stream, arg.TableFields.DispatchableKey);
  stream << ", ";
  stream << arg.Value->CPUDescriptorHandle << ", ";
  stream << arg.Value->GPUDescriptorHandle << ", ";
  stream << arg.Value->SizeInDescriptors;
  stream << "}";
  return stream;
}

FastOStream& operator<<(FastOStream& stream, DML_GRAPH_DESC_Argument& arg) {
  return stream << const_cast<const DML_GRAPH_DESC*>(arg.Value);
}

FastOStream& operator<<(FastOStream& stream, DML_BINDING_DESC_Argument& arg) {
  if (!arg.Value) {
    return stream << "nullptr";
  }
  stream << "DML_BINDING_DESC{";
  stream << arg.Value->Type << ", ";
  stream << "{";
  for (unsigned i = 0; i < arg.ResourceKeysSize; ++i) {
    if (i > 0) {
      stream << ", ";
    }
    printObjectKey(stream, arg.ResourceKeys[i]);
  }
  stream << "}}";
  return stream;
}

FastOStream& operator<<(FastOStream& stream, DML_BINDING_DESCs_Argument& arg) {
  if (!arg.Value) {
    return stream << "nullptr";
  }
  unsigned currentKey = 0;
  stream << "DML_BINDING_DESC[";
  for (unsigned i = 0; i < arg.Size; ++i) {
    if (i > 0) {
      stream << ", ";
    }
    stream << "{";
    stream << arg.Value[i].Type << ", ";
    stream << "{";
    switch (arg.Value[i].Type) {
    case DML_BINDING_TYPE_NONE:
      stream << "DML_BINDING_TYPE_NONE";
      break;
    case DML_BINDING_TYPE_BUFFER:
      printObjectKey(stream, arg.ResourceKeys[currentKey++]);
      break;
    case DML_BINDING_TYPE_BUFFER_ARRAY: {
      const auto* bufferArray =
          reinterpret_cast<const DML_BUFFER_ARRAY_BINDING*>(arg.Value[i].Desc);
      for (unsigned j = 0; j < bufferArray->BindingCount; ++j) {
        if (j > 0) {
          stream << ", ";
        }
        printObjectKey(stream, arg.ResourceKeys[currentKey++]);
      }
      break;
    }
    }
    stream << "}}";
  }
  stream << "]";
  return stream;
}

FastOStream& operator<<(FastOStream& stream, DML_OPERATOR_DESC_Argument& arg) {
  return stream << const_cast<const DML_OPERATOR_DESC*>(arg.Value);
}

FastOStream& operator<<(FastOStream& stream, xess_d3d12_init_params_t_Argument& arg) {
  if (!arg.Value) {
    return stream << "nullptr";
  }
  stream << "xess_d3d12_init_params_t{";
  stream << "{" << arg.Value->outputResolution.x << ", " << arg.Value->outputResolution.y << "}, ";
  stream << toStr(arg.Value->qualitySetting);
  stream << ", ";
  stream << arg.Value->initFlags << ", ";
  stream << arg.Value->creationNodeMask << ", ";
  stream << arg.Value->visibleNodeMask << ", ";
  printObjectKey(stream, arg.TempBufferHeapKey);
  stream << ", " << arg.Value->bufferHeapOffset << ", ";
  printObjectKey(stream, arg.TempTextureHeapKey);
  stream << ", " << arg.Value->textureHeapOffset << ", ";
  printObjectKey(stream, arg.PipelineLibraryKey);
  stream << "}";
  return stream;
}

FastOStream& operator<<(FastOStream& stream, xess_d3d12_execute_params_t_Argument& arg) {
  if (!arg.Value) {
    return stream << "nullptr";
  }
  stream << "xess_d3d12_execute_params_t{";
  printObjectKey(stream, arg.ColorTextureKey);
  stream << ", ";
  printObjectKey(stream, arg.VelocityTextureKey);
  stream << ", ";
  printObjectKey(stream, arg.DepthTextureKey);
  stream << ", ";
  printObjectKey(stream, arg.ExposureScaleTextureKey);
  stream << ", ";
  printObjectKey(stream, arg.ResponsivePixelMaskTextureKey);
  stream << ", ";
  printObjectKey(stream, arg.OutputTextureKey);
  stream << ", ";
  stream << arg.Value->jitterOffsetX << ", ";
  stream << arg.Value->jitterOffsetY << ", ";
  stream << arg.Value->exposureScale << ", ";
  stream << arg.Value->resetHistory << ", ";
  stream << arg.Value->inputWidth << ", ";
  stream << arg.Value->inputHeight << ", ";
  stream << arg.Value->inputColorBase << ", ";
  stream << arg.Value->inputMotionVectorBase << ", ";
  stream << arg.Value->inputDepthBase << ", ";
  stream << arg.Value->inputResponsiveMaskBase << ", ";
  stream << arg.Value->reserved0 << ", ";
  stream << arg.Value->outputColorBase << ", ";
  printObjectKey(stream, arg.DescriptorHeapKey);
  stream << ", ";
  stream << arg.Value->descriptorHeapOffset;
  stream << "}";
  return stream;
}

FastOStream& operator<<(FastOStream& stream, DML_CheckFeatureSupport_BufferArgument& arg) {
  return stream << "FeatureQueryDataBuffer{" << arg.Size << "}";
}

FastOStream& operator<<(FastOStream& stream, DSTORAGE_QUEUE_DESC_Argument& arg) {
  if (!arg.Value) {
    return stream << "nullptr";
  }
  stream << "DSTORAGE_QUEUE_DESC{";
  stream << arg.Value->SourceType << ", ";
  stream << arg.Value->Capacity << ", ";
  stream << arg.Value->Priority << ", ";
  if (arg.Value->Name) {
    stream << "\"" << arg.Value->Name << "\", ";
  } else {
    stream << "nullptr, ";
  }
  printObjectKey(stream, arg.DeviceKey);
  stream << "}";
  return stream;
}

FastOStream& operator<<(FastOStream& stream, DSTORAGE_REQUEST_Argument& arg) {
  stream << "DSTORAGE_REQUEST{";
  stream << arg.Value->Options << ", ";
  switch (arg.Value->Options.SourceType) {
  case DSTORAGE_REQUEST_SOURCE_MEMORY:
    stream << arg.Value->Source.Memory << ", ";
    break;
  case DSTORAGE_REQUEST_SOURCE_FILE:
    stream << "DSTORAGE_SOURCE_FILE{";
    printObjectKey(stream, arg.FileKey) << ", ";
    stream << arg.Value->Source.File.Offset << ", ";
    stream << arg.Value->Source.File.Size;
    stream << "}, ";
    break;
  default:
    stream << "unknown, ";
  }
  switch (arg.Value->Options.DestinationType) {
  case DSTORAGE_REQUEST_DESTINATION_MEMORY:
    stream << arg.Value->Destination.Memory << ", ";
    break;
  case DSTORAGE_REQUEST_DESTINATION_BUFFER:
    stream << "DSTORAGE_DESTINATION_BUFFER{";
    printObjectKey(stream, arg.ResourceKey) << ", ";
    stream << arg.Value->Destination.Buffer.Offset << ", ";
    stream << arg.Value->Destination.Buffer.Size;
    stream << "}, ";
    break;
  case DSTORAGE_REQUEST_DESTINATION_TEXTURE_REGION:
    stream << "DSTORAGE_DESTINATION_TEXTURE_REGION{";
    printObjectKey(stream, arg.ResourceKey) << ", ";
    stream << arg.Value->Destination.Texture.SubresourceIndex << ", ";
    stream << arg.Value->Destination.Texture.Region;
    stream << "}, ";
    break;
  case DSTORAGE_REQUEST_DESTINATION_MULTIPLE_SUBRESOURCES:
    stream << "DSTORAGE_DESTINATION_MULTIPLE_SUBRESOURCES{";
    printObjectKey(stream, arg.ResourceKey) << ", ";
    stream << arg.Value->Destination.MultipleSubresources.FirstSubresource;
    stream << "}, ";
    break;
  case DSTORAGE_REQUEST_DESTINATION_TILES:
    stream << "DSTORAGE_DESTINATION_TEXTURE_TILES{";
    printObjectKey(stream, arg.ResourceKey) << ", ";
    stream << arg.Value->Destination.Tiles.TiledRegionStartCoordinate << ", ";
    stream << arg.Value->Destination.Tiles.TileRegionSize;
    stream << "}, ";
    break;
  default:
    stream << "unknown, ";
  }
  stream << arg.Value->UncompressedSize << ", ";
  stream << arg.Value->CancellationTag << ", ";
  if (arg.Value->Name) {
    stream << arg.Value->Name;
  } else {
    stream << "nullptr";
  }
  stream << "}";
  return stream;
}

FastOStream& operator<<(
    FastOStream& stream,
    PointerArgument<NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS>& arg) {
  if (!arg.Value) {
    return stream << "nullptr";
  }
  stream << "NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS{";
  stream << arg.Value->version << ", {";
  printObjectKey(stream, arg.DestAccelerationStructureKey);
  stream << ", " << arg.DestAccelerationStructureOffset << "} (0x";
  PrintHex(stream, arg.Value->pDesc->destAccelerationStructureData) << "), {";

  stream << arg.Value->pDesc->inputs.type << ", " << arg.Value->pDesc->inputs.flags << ", "
         << arg.Value->pDesc->inputs.numDescs << ", " << arg.Value->pDesc->inputs.descsLayout
         << ", " << arg.Value->pDesc->inputs.geometryDescStrideInBytes << ", ";
  if (!arg.InputKeys.empty() &&
      arg.Value->pDesc->inputs.type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    stream << "{";
    printObjectKey(stream, arg.InputKeys[0]);
    stream << ", " << arg.InputOffsets[0] << "}";
  } else if (arg.Value->pDesc->inputs.type ==
             D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
    unsigned inputIndex = 0;
    stream << "[";
    for (unsigned i = 0; i < arg.Value->pDesc->inputs.numDescs; ++i) {
      const NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX* desc =
          arg.Value->pDesc->inputs.descsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY
              ? (NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX*)((char*)(arg.Value->pDesc->inputs
                                                                       .pGeometryDescs) +
                                                           arg.Value->pDesc->inputs
                                                                   .geometryDescStrideInBytes *
                                                               i)
              : arg.Value->pDesc->inputs.ppGeometryDescs[i];
      if (i > 0) {
        stream << ", ";
      }
      stream << "{" << desc->type << ", " << desc->flags << ", {";

      if (!arg.InputKeys.empty() && desc->type == D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES) {
        stream << "{";
        printObjectKey(stream, arg.InputKeys[inputIndex]);
        stream << ", " << arg.InputOffsets[inputIndex] << "}, ";
        ++inputIndex;
        stream << desc->triangles.IndexFormat << ", " << desc->triangles.VertexFormat << ", "
               << desc->triangles.IndexCount << ", " << desc->triangles.VertexCount << ", {";
        printObjectKey(stream, arg.InputKeys[inputIndex]);
        stream << ", " << arg.InputOffsets[inputIndex] << "}, {{";
        ++inputIndex;
        printObjectKey(stream, arg.InputKeys[inputIndex]);
        stream << ", " << arg.InputOffsets[inputIndex] << "}, ";
        ++inputIndex;
        stream << desc->triangles.VertexBuffer.StrideInBytes << "}";
      } else if (desc->type == D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS) {
        stream << desc->aabbs.AABBCount << ", {{";
        printObjectKey(stream, arg.InputKeys[inputIndex]);
        stream << ", " << arg.InputOffsets[inputIndex] << "}, ";
        ++inputIndex;
        stream << desc->aabbs.AABBs.StrideInBytes << "}";
      } else if (desc->type == NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES_EX) {
        stream << "{";
        printObjectKey(stream, arg.InputKeys[inputIndex]);
        stream << ", " << arg.InputOffsets[inputIndex] << "}, ";
        ++inputIndex;
        stream << desc->ommTriangles.triangles.IndexFormat << ", "
               << desc->ommTriangles.triangles.VertexFormat << ", "
               << desc->ommTriangles.triangles.IndexCount << ", "
               << desc->ommTriangles.triangles.VertexCount << ", {";
        printObjectKey(stream, arg.InputKeys[inputIndex]);
        stream << ", " << arg.InputOffsets[inputIndex] << "}, {{";
        ++inputIndex;
        printObjectKey(stream, arg.InputKeys[inputIndex]);
        stream << ", " << arg.InputOffsets[inputIndex] << "}, ";
        ++inputIndex;
        stream << desc->ommTriangles.triangles.VertexBuffer.StrideInBytes << "}, {{";
        printObjectKey(stream, arg.InputKeys[inputIndex]);
        stream << ", " << arg.InputOffsets[inputIndex] << "}, ";
        ++inputIndex;
        stream << desc->ommTriangles.ommAttachment.opacityMicromapIndexBuffer.StrideInBytes
               << "}, ";
        stream << desc->ommTriangles.ommAttachment.opacityMicromapIndexFormat << ", "
               << desc->ommTriangles.ommAttachment.opacityMicromapBaseLocation << ", {";
        printObjectKey(stream, arg.InputKeys[inputIndex]);
        stream << ", " << arg.InputOffsets[inputIndex] << "}, ";
        ++inputIndex;
        stream << desc->ommTriangles.ommAttachment.numOMMUsageCounts << ", ";
        if (!desc->ommTriangles.ommAttachment.pOMMUsageCounts) {
          stream << "nullptr";
        } else {
          stream << "[";
          for (unsigned j = 0; j < desc->ommTriangles.ommAttachment.numOMMUsageCounts; ++j) {
            if (j > 0) {
              stream << ", ";
            }
            stream << "NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_USAGE_COUNT{";
            stream << desc->ommTriangles.ommAttachment.pOMMUsageCounts[j].count << ", "
                   << desc->ommTriangles.ommAttachment.pOMMUsageCounts[j].subdivisionLevel << ", "
                   << desc->ommTriangles.ommAttachment.pOMMUsageCounts[j].format << "}";
          }
          stream << "]";
        }
        stream << "}";
      } else if (desc->type == NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_DMM_TRIANGLES_EX) {
        stream << "{";
        printObjectKey(stream, arg.InputKeys[inputIndex]);
        stream << ", " << arg.InputOffsets[inputIndex] << "}, ";
        ++inputIndex;
        stream << desc->dmmTriangles.triangles.IndexFormat << ", "
               << desc->dmmTriangles.triangles.VertexFormat << ", "
               << desc->dmmTriangles.triangles.IndexCount << ", "
               << desc->dmmTriangles.triangles.VertexCount << ", {";
        printObjectKey(stream, arg.InputKeys[inputIndex]);
        stream << ", " << arg.InputOffsets[inputIndex] << "}, {{";
        ++inputIndex;
        printObjectKey(stream, arg.InputKeys[inputIndex]);
        stream << ", " << arg.InputOffsets[inputIndex] << "}, ";
        ++inputIndex;
        stream << desc->dmmTriangles.triangles.VertexBuffer.StrideInBytes << "}, {{";
        printObjectKey(stream, arg.InputKeys[inputIndex]);
        stream << ", " << arg.InputOffsets[inputIndex] << "}, ";
        ++inputIndex;
        stream << desc->dmmTriangles.dmmAttachment.triangleMicromapIndexBuffer.StrideInBytes
               << "}, ";
        stream << desc->dmmTriangles.dmmAttachment.triangleMicromapIndexFormat << ", "
               << desc->dmmTriangles.dmmAttachment.triangleMicromapBaseLocation << ", {{";
        printObjectKey(stream, arg.InputKeys[inputIndex]);
        stream << ", " << arg.InputOffsets[inputIndex] << "}, ";
        ++inputIndex;
        stream << desc->dmmTriangles.dmmAttachment.trianglePrimitiveFlagsBuffer.StrideInBytes
               << "}, {{";
        printObjectKey(stream, arg.InputKeys[inputIndex]);
        stream << ", " << arg.InputOffsets[inputIndex] << "}, ";
        ++inputIndex;
        stream << desc->dmmTriangles.dmmAttachment.vertexBiasAndScaleBuffer.StrideInBytes << "}, ";
        stream << desc->dmmTriangles.dmmAttachment.vertexBiasAndScaleFormat << ", {{";
        printObjectKey(stream, arg.InputKeys[inputIndex]);
        stream << ", " << arg.InputOffsets[inputIndex] << "}, ";
        ++inputIndex;
        stream << desc->dmmTriangles.dmmAttachment.vertexDisplacementVectorBuffer.StrideInBytes
               << "}, ";
        stream << desc->dmmTriangles.dmmAttachment.vertexDisplacementVectorFormat << ", {";
        printObjectKey(stream, arg.InputKeys[inputIndex]);
        stream << ", " << arg.InputOffsets[inputIndex] << "}, ";
        ++inputIndex;
        stream << ", " << desc->dmmTriangles.dmmAttachment.numDMMUsageCounts << ", ";
        if (!desc->dmmTriangles.dmmAttachment.pDMMUsageCounts) {
          stream << "nullptr";
        } else {
          stream << "[";
          for (unsigned j = 0; j < desc->dmmTriangles.dmmAttachment.numDMMUsageCounts; ++j) {
            if (j > 0) {
              stream << ", ";
            }
            stream << "NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_USAGE_COUNT{";
            stream << desc->dmmTriangles.dmmAttachment.pDMMUsageCounts[j].count << ", "
                   << desc->dmmTriangles.dmmAttachment.pDMMUsageCounts[j].subdivisionLevel << ", "
                   << desc->dmmTriangles.dmmAttachment.pDMMUsageCounts[j].format << "}";
          }
          stream << "]";
        }
        stream << "}";
      } else if (desc->type == NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_SPHERES_EX) {
        stream << "{" << desc->spheres.vertexCount << ", " << desc->spheres.indexCount << ", {{";
        printObjectKey(stream, arg.InputKeys[inputIndex]);
        stream << ", " << arg.InputOffsets[inputIndex] << "}, ";
        ++inputIndex;
        stream << desc->spheres.vertexPositionBuffer.StrideInBytes << "}, ";
        stream << desc->spheres.vertexPositionFormat << ", {{";
        printObjectKey(stream, arg.InputKeys[inputIndex]);
        stream << ", " << arg.InputOffsets[inputIndex] << "}, ";
        ++inputIndex;
        stream << desc->spheres.vertexRadiusBuffer.StrideInBytes << "}, ";
        stream << desc->spheres.vertexRadiusFormat << ", {{";
        printObjectKey(stream, arg.InputKeys[inputIndex]);
        stream << ", " << arg.InputOffsets[inputIndex] << "}, ";
        ++inputIndex;
        stream << desc->spheres.indexBuffer.StrideInBytes << "}, ";
        stream << desc->spheres.indexFormat << "}";
      } else if (desc->type == NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_LSS_EX) {
        stream << "{" << desc->lss.vertexCount << ", " << desc->lss.indexCount << ", "
               << desc->lss.primitiveCount << ", {{";
        printObjectKey(stream, arg.InputKeys[inputIndex]);
        stream << ", " << arg.InputOffsets[inputIndex] << "}, ";
        ++inputIndex;
        stream << desc->lss.vertexPositionBuffer.StrideInBytes << "}, ";
        stream << desc->lss.vertexPositionFormat << ", {{";
        printObjectKey(stream, arg.InputKeys[inputIndex]);
        stream << ", " << arg.InputOffsets[inputIndex] << "}, ";
        ++inputIndex;
        stream << desc->lss.vertexRadiusBuffer.StrideInBytes << "}, ";
        stream << desc->lss.vertexRadiusFormat << ", {{";
        printObjectKey(stream, arg.InputKeys[inputIndex]);
        stream << ", " << arg.InputOffsets[inputIndex] << "}, ";
        ++inputIndex;
        stream << desc->lss.indexBuffer.StrideInBytes << "}, ";
        stream << desc->lss.indexFormat << ", " << desc->lss.endcapMode << ", "
               << desc->lss.primitiveFormat << "}";
      }
      stream << "}}";
    }
    stream << "]";
  }

  stream << "}, {";
  printObjectKey(stream, arg.SourceAccelerationStructureKey);
  stream << ", " << arg.SourceAccelerationStructureOffset << "}, {";
  printObjectKey(stream, arg.ScratchAccelerationStructureKey);
  stream << ", " << arg.ScratchAccelerationStructureOffset << "}, ";

  stream << arg.Value->numPostbuildInfoDescs << ", ";
  if (!arg.Value->pPostbuildInfoDescs) {
    stream << "nullptr";
  } else {
    stream << "[";
    for (unsigned i = 0; i < arg.DestPostBuildBufferKeys.size(); ++i) {
      if (i > 0) {
        stream << ", ";
      }
      stream << "D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC{{";
      printObjectKey(stream, arg.DestPostBuildBufferKeys[i]);
      stream << ", " << arg.DestPostBuildBufferOffsets[i] << "}, ";
      stream << arg.Value->pPostbuildInfoDescs[i].InfoType;
      stream << "}";
    }
    stream << "]";
  }

  stream << "}";

  return stream;
}

FastOStream& operator<<(
    FastOStream& stream,
    PointerArgument<NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS>& arg) {
  if (!arg.Value) {
    return stream << "nullptr";
  }
  stream << "NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS{";

  stream << arg.Value->version << ", {";

  printObjectKey(stream, arg.DestOpacityMicromapArrayDataKey);
  stream << ", " << arg.DestOpacityMicromapArrayDataOffset << "} (0x";
  PrintHex(stream, arg.Value->pDesc->destOpacityMicromapArrayData) << "), {";

  stream << arg.Value->pDesc->inputs.flags << ", " << arg.Value->pDesc->inputs.numOMMUsageCounts
         << ", ";
  if (!arg.Value->pDesc->inputs.pOMMUsageCounts) {
    stream << "nullptr";
  } else {
    stream << "[";
    for (unsigned i = 0; i < arg.Value->pDesc->inputs.numOMMUsageCounts; ++i) {
      if (i > 0) {
        stream << ", ";
      }
      stream << "NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_USAGE_COUNT{";
      stream << arg.Value->pDesc->inputs.pOMMUsageCounts[i].count << ", "
             << arg.Value->pDesc->inputs.pOMMUsageCounts[i].subdivisionLevel << ", "
             << arg.Value->pDesc->inputs.pOMMUsageCounts[i].format << "}";
    }
    stream << "]";
  }
  stream << ", {";
  printObjectKey(stream, arg.InputBufferKey);
  stream << ", " << arg.InputBufferOffset << "}, {{";
  printObjectKey(stream, arg.PerOMMDescsKey);
  stream << ", " << arg.PerOMMDescsOffset << "}, "
         << arg.Value->pDesc->inputs.perOMMDescs.StrideInBytes << "}";

  stream << "}, {";
  printObjectKey(stream, arg.ScratchOpacityMicromapArrayDataKey);
  stream << ", " << arg.ScratchOpacityMicromapArrayDataOffset << "}, ";

  stream << arg.Value->numPostbuildInfoDescs << ", ";
  if (!arg.Value->pPostbuildInfoDescs) {
    stream << "nullptr";
  } else {
    stream << "[";
    for (unsigned i = 0; i < arg.DestPostBuildBufferKeys.size(); ++i) {
      if (i > 0) {
        stream << ", ";
      }
      stream << "D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC{{";
      printObjectKey(stream, arg.DestPostBuildBufferKeys[i]);
      stream << ", " << arg.DestPostBuildBufferOffsets[i] << "}, ";
      stream << arg.Value->pPostbuildInfoDescs[i].infoType;
      stream << "}";
    }
    stream << "]";
  }

  return stream;
}

FastOStream& operator<<(
    FastOStream& stream,
    PointerArgument<NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS>& arg) {
  if (!arg.Value) {
    return stream << "nullptr";
  }
  stream << "NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS{";

  stream << arg.Value->version << ", ";

  stream << arg.Value->pDesc->inputs.maxArgCount << ", " << arg.Value->pDesc->inputs.flags << ", "
         << arg.Value->pDesc->inputs.type << ", " << arg.Value->pDesc->inputs.mode << ", {";

  if (arg.Value->pDesc->inputs.type ==
      NVAPI_D3D12_RAYTRACING_MULTI_INDIRECT_CLUSTER_OPERATION_TYPE_BUILD_BLAS_FROM_CLAS) {
    stream << arg.Value->pDesc->inputs.clasDesc.maxTotalClasCount << ", "
           << arg.Value->pDesc->inputs.clasDesc.maxClasCountPerArg;
  } else if (arg.Value->pDesc->inputs.type ==
             NVAPI_D3D12_RAYTRACING_MULTI_INDIRECT_CLUSTER_OPERATION_TYPE_MOVE_CLUSTER_OBJECT) {
    stream << arg.Value->pDesc->inputs.movesDesc.type << ", "
           << arg.Value->pDesc->inputs.movesDesc.maxBytesMoved;
  } else {
    stream << static_cast<DXGI_FORMAT>(arg.Value->pDesc->inputs.trianglesDesc.vertexFormat) << ", "
           << arg.Value->pDesc->inputs.trianglesDesc.maxGeometryIndexValue << ", "
           << arg.Value->pDesc->inputs.trianglesDesc.maxUniqueGeometryCountPerArg << ", "
           << arg.Value->pDesc->inputs.trianglesDesc.maxTriangleCountPerArg << ", "
           << arg.Value->pDesc->inputs.trianglesDesc.maxVertexCountPerArg << ", "
           << arg.Value->pDesc->inputs.trianglesDesc.maxTotalTriangleCount << ", "
           << arg.Value->pDesc->inputs.trianglesDesc.maxTotalVertexCount << ", "
           << arg.Value->pDesc->inputs.trianglesDesc.minPositionTruncateBitCount;
  }

  stream << "}}, ";
  stream << static_cast<
                NVAPI_D3D12_RAYTRACING_MULTI_INDIRECT_CLUSTER_OPERATION_ADDRESS_RESOLUTION_FLAGS>(
                arg.Value->pDesc->addressResolutionFlags)
         << ", {";

  printObjectKey(stream, arg.BatchResultDataKey);
  stream << ", " << arg.BatchResultDataOffset << "}, {";

  printObjectKey(stream, arg.BatchScratchDataKey);
  stream << ", " << arg.BatchScratchDataOffset << "}, {{";

  printObjectKey(stream, arg.DestinationAddressArrayKey);
  stream << ", " << arg.DestinationAddressArrayOffset << "}, "
         << arg.Value->pDesc->destinationAddressArray.StrideInBytes << "}, {{";

  printObjectKey(stream, arg.ResultSizeArrayKey);
  stream << ", " << arg.ResultSizeArrayOffset << "}, "
         << arg.Value->pDesc->resultSizeArray.StrideInBytes << "}, {{";

  printObjectKey(stream, arg.IndirectArgArrayKey);
  stream << ", " << arg.IndirectArgArrayOffset << "}, "
         << arg.Value->pDesc->indirectArgArray.StrideInBytes << "}, {";

  printObjectKey(stream, arg.IndirectArgCountKey);
  stream << ", " << arg.IndirectArgCountOffset << "}";

  return stream;
}

FastOStream& operator<<(FastOStream& stream, xell_frame_report_t_Argument& arg) {
  if (!arg.Value) {
    return stream << "nullptr";
  }
  stream << "xell_frame_report_t[";
  for (unsigned i = 0; i < arg.FRAME_REPORTS_COUNT; ++i) {
    if (i > 0) {
      stream << ", ";
    }
    stream << "{";
    stream << arg.Value[i].m_frame_id << ", ";
    stream << arg.Value[i].m_sim_start_ts << ", ";
    stream << arg.Value[i].m_sim_end_ts << ", ";
    stream << arg.Value[i].m_render_submit_start_ts << ", ";
    stream << arg.Value[i].m_render_submit_end_ts << ", ";
    stream << arg.Value[i].m_present_start_ts << ", ";
    stream << arg.Value[i].m_present_end_ts;
    stream << "}";
  }
  stream << "]";
  return stream;
}

FastOStream& operator<<(FastOStream& stream, xefg_swapchain_d3d12_init_params_t_Argument& arg) {
  if (!arg.Value) {
    return stream << "nullptr";
  }
  stream << "xefg_swapchain_d3d12_init_params_t{";
  printObjectKey(stream, arg.ApplicationSwapChainKey);
  stream << ", " << arg.Value->initFlags << ", ";
  stream << arg.Value->maxInterpolatedFrames << ", ";
  stream << arg.Value->creationNodeMask << ", ";
  stream << arg.Value->visibleNodeMask << ", ";
  printObjectKey(stream, arg.TempBufferHeapKey);
  stream << ", " << arg.Value->bufferHeapOffset << ", ";
  printObjectKey(stream, arg.TempTextureHeapKey);
  stream << ", " << arg.Value->textureHeapOffset << ", ";
  printObjectKey(stream, arg.PipelineLibraryKey);
  stream << ", " << toStr(arg.Value->uiMode);
  stream << "}";
  return stream;
}

FastOStream& operator<<(FastOStream& stream, xefg_swapchain_d3d12_resource_data_t_Argument& arg) {
  if (!arg.Value) {
    return stream << "nullptr";
  }
  stream << "xefg_swapchain_d3d12_resource_data_t{";
  stream << toStr(arg.Value->type) << ", ";
  stream << toStr(arg.Value->validity) << ", ";
  stream << arg.Value->resourceBase << ", ";
  stream << arg.Value->resourceSize << ", ";
  printObjectKey(stream, arg.ResourceKey);
  stream << ", " << toStr(arg.Value->incomingState);
  stream << "}";
  return stream;
}

} // namespace DirectX
} // namespace gits
