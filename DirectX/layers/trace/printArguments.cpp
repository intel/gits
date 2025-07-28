// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "printArguments.h"
#include "printStructuresAuto.h"
#include "printCustom.h"
#include "printEnumsAuto.h"
#include "gits.h"
#include "nvapi.h"

#include <iomanip>
#include <d3dx12/d3dx12_pipeline_state_stream.h>

namespace gits {
namespace DirectX {

FastOStream& operator<<(FastOStream& stream, BufferArgument& arg) {
  return stream << "Buffer{" << arg.size << "}";
}

FastOStream& operator<<(FastOStream& stream, OutputBufferArgument& arg) {

  stream << "OutputBuffer{";
  if (arg.captureValue) {
    stream << arg.captureValue;
  } else if (arg.value) {
    stream << *arg.value;
  } else {
    stream << "nullptr";
  }
  stream << "}";
  return stream;
}

FastOStream& operator<<(FastOStream& stream, ShaderIdentifierArgument& arg) {
  stream << "ShaderIdentifier{";
  if (arg.value) {
    for (unsigned i = 0; i < D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES; ++i) {
      printHexFull(stream, *(static_cast<uint8_t*>(arg.value) + i));
    }
  } else {
    stream << "nullptr";
  }
  stream << "}";
  return stream;
}

FastOStream& operator<<(FastOStream& stream,
                        DescriptorHandleArrayArgument<D3D12_CPU_DESCRIPTOR_HANDLE>& arg) {

  if (!arg.value) {
    return stream << "nullptr";
  }

  stream << "D3D12_CPU_DESCRIPTOR_HANDLE[";
  for (unsigned i = 0; i < arg.size; ++i) {
    if (i > 0) {
      stream << ", ";
    }
    stream << "{";
    printObjectKey(stream, arg.interfaceKeys[i]);
    stream << ", " << arg.indexes[i] << "}";
  }
  stream << "]";
  return stream;
}

FastOStream& operator<<(FastOStream& stream,
                        DescriptorHandleArgument<D3D12_CPU_DESCRIPTOR_HANDLE>& arg) {
  stream << "D3D12_CPU_DESCRIPTOR_HANDLE{";
  printObjectKey(stream, arg.interfaceKey);
  stream << ", " << arg.index << "}";
  return stream;
}

FastOStream& operator<<(FastOStream& stream,
                        DescriptorHandleArgument<D3D12_GPU_DESCRIPTOR_HANDLE>& arg) {
  stream << "D3D12_GPU_DESCRIPTOR_HANDLE{";
  printObjectKey(stream, arg.interfaceKey);
  stream << ", " << arg.index << "}";
  return stream;
}

FastOStream& operator<<(FastOStream& stream, LPCWSTR_Argument& arg) {
  printString(stream, arg.value);
  return stream;
}

FastOStream& operator<<(FastOStream& stream, LPCSTR_Argument& arg) {
  printString(stream, arg.value);
  return stream;
}

FastOStream& operator<<(FastOStream& stream, D3D12_GPU_VIRTUAL_ADDRESS_Argument& arg) {
  stream << "D3D12_GPU_VIRTUAL_ADDRESS{";
  printObjectKey(stream, arg.interfaceKey);
  stream << ", " << arg.offset << "}";
  return stream;
}

FastOStream& operator<<(FastOStream& stream, D3D12_GPU_VIRTUAL_ADDRESSs_Argument& arg) {
  if (!arg.value) {
    return stream << "nullptr";
  }
  stream << "D3D12_GPU_VIRTUAL_ADDRESS[";
  for (unsigned i = 0; i < arg.size; ++i) {
    if (i > 0) {
      stream << ", ";
    }
    stream << "{";
    printObjectKey(stream, arg.interfaceKeys[i]);
    stream << ", " << arg.offsets[i] << "}";
  }
  return stream << "]";
}

FastOStream& operator<<(FastOStream& stream, D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument& arg) {
  stream << "D3D12_GRAPHICS_PIPELINE_STATE_DESC{";
  printObjectKey(stream, arg.rootSignatureKey);
  stream << ", ";
  stream << arg.value->VS << ", ";
  stream << arg.value->PS << ", ";
  stream << arg.value->DS << ", ";
  stream << arg.value->HS << ", ";
  stream << arg.value->GS << ", ";
  stream << arg.value->StreamOutput << ", ";
  stream << arg.value->BlendState << ", ";
  stream << arg.value->SampleMask << ", ";
  stream << arg.value->RasterizerState << ", ";
  stream << arg.value->DepthStencilState << ", ";
  stream << arg.value->InputLayout << ", ";
  stream << arg.value->IBStripCutValue << ", ";
  stream << arg.value->PrimitiveTopologyType << ", ";
  stream << arg.value->NumRenderTargets << ", ";
  printStaticArray(stream, arg.value->RTVFormats) << ", ";
  stream << arg.value->DSVFormat << ", ";
  stream << arg.value->SampleDesc << ", ";
  stream << arg.value->NodeMask << ", ";
  stream << arg.value->CachedPSO << ", ";
  stream << arg.value->Flags;
  stream << "}";
  return stream;
}

FastOStream& operator<<(FastOStream& stream, D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument& arg) {
  stream << "D3D12_COMPUTE_PIPELINE_STATE_DESC{";
  printObjectKey(stream, arg.rootSignatureKey);
  stream << ", ";
  stream << arg.value->CS << ", ";
  stream << arg.value->NodeMask << ", ";
  stream << arg.value->CachedPSO << ", ";
  stream << arg.value->Flags;
  stream << "}";
  return stream;
}

FastOStream& operator<<(FastOStream& stream, D3D12_TEXTURE_COPY_LOCATION_Argument& arg) {
  if (!arg.value) {
    return stream << "nullptr";
  }
  stream << "D3D12_TEXTURE_COPY_LOCATION{";

  printObjectKey(stream, arg.resourceKey);
  stream << ", " << arg.value->Type << ", ";
  if (arg.value->Type == D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX) {
    stream << arg.value->SubresourceIndex;
  } else {
    stream << arg.value->PlacedFootprint;
  }
  stream << "}";
  return stream;
}

FastOStream& operator<<(FastOStream& stream, D3D12_RESOURCE_BARRIERs_Argument& arg) {
  if (!arg.value) {
    return stream << "nullptr";
  }
  stream << "D3D12_RESOURCE_BARRIER[";
  for (unsigned i = 0; i < arg.size; ++i) {
    stream << "{" << arg.value[i].Type << ", " << arg.value[i].Flags << ", ";
    switch (arg.value[i].Type) {
    case D3D12_RESOURCE_BARRIER_TYPE_TRANSITION:
      stream << "D3D12_RESOURCE_TRANSITION_BARRIER{";
      printObjectKey(stream, arg.resourceKeys[i]);
      stream << ", ";
      if (arg.value[i].Transition.Subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES) {
        stream << "D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES";
      } else {
        stream << arg.value[i].Transition.Subresource;
      }
      stream << ", " << arg.value[i].Transition.StateBefore << ", "
             << arg.value[i].Transition.StateAfter;
      break;
    case D3D12_RESOURCE_BARRIER_TYPE_ALIASING:
      stream << "D3D12_RESOURCE_ALIASING_BARRIER{";
      printObjectKey(stream, arg.resourceKeys[i]);
      stream << ", ";
      printObjectKey(stream, arg.resourceAfterKeys[i]);
      break;
    case D3D12_RESOURCE_BARRIER_TYPE_UAV:
      stream << "D3D12_RESOURCE_UAV_BARRIER{";
      printObjectKey(stream, arg.resourceKeys[i]);
      break;
    }
    stream << "}}";
  }
  return stream << "]";
}

FastOStream& operator<<(FastOStream& stream, D3D12_INDEX_BUFFER_VIEW_Argument& arg) {
  if (!arg.value) {
    return stream << "nullptr";
  }
  stream << "D3D12_INDEX_BUFFER_VIEW{{";
  printObjectKey(stream, arg.bufferLocationKey);
  stream << ", " << arg.bufferLocationOffset << "}, ";
  stream << arg.value->SizeInBytes << ", " << arg.value->Format;
  return stream << "}";
}

FastOStream& operator<<(FastOStream& stream, D3D12_CONSTANT_BUFFER_VIEW_DESC_Argument& arg) {
  if (!arg.value) {
    return stream << "nullptr";
  }
  stream << "D3D12_CONSTANT_BUFFER_VIEW_DESC{{";
  printObjectKey(stream, arg.bufferLocationKey);
  stream << ", " << arg.bufferLocationOffset << "}, ";
  stream << arg.value->SizeInBytes;
  return stream << "}";
}

FastOStream& operator<<(FastOStream& stream, D3D12_VERTEX_BUFFER_VIEWs_Argument& arg) {
  if (!arg.value) {
    return stream << "nullptr";
  }
  stream << "D3D12_VERTEX_BUFFER_VIEW[";
  for (unsigned i = 0; i < arg.size; ++i) {
    stream << "{{";
    printObjectKey(stream, arg.bufferLocationKeys[i]);
    stream << ", " << arg.bufferLocationOffsets[i] << "}, ";
    stream << arg.value[i].SizeInBytes << ", " << arg.value[i].StrideInBytes << "}";
  }
  return stream << "]";
}

FastOStream& operator<<(FastOStream& stream, D3D12_STREAM_OUTPUT_BUFFER_VIEWs_Argument& arg) {
  return stream << const_cast<const D3D12_STREAM_OUTPUT_BUFFER_VIEW*>(arg.value);
}

FastOStream& operator<<(FastOStream& stream, D3D12_WRITEBUFFERIMMEDIATE_PARAMETERs_Argument& arg) {
  if (!arg.value) {
    return stream << "nullptr";
  }
  stream << "D3D12_WRITEBUFFERIMMEDIATE_PARAMETER[";
  for (unsigned i = 0; i < arg.size; ++i) {
    if (i > 0) {
      stream << ", ";
    }
    stream << "{{";
    printObjectKey(stream, arg.destKeys[i]);
    stream << ", " << arg.destOffsets[i] << "}, " << arg.value[i].Value << "}";
  }
  return stream << "]";
}

FastOStream& operator<<(FastOStream& stream, D3D12_PIPELINE_STATE_STREAM_DESC_Argument& arg) {
  stream << "D3D12_PIPELINE_STATE_STREAM_DESC{";
  size_t offset = 0;
  while (offset < arg.value->SizeInBytes) {
    auto pStream = static_cast<uint8_t*>(arg.value->pPipelineStateSubobjectStream) + offset;
    auto subobjectType = *reinterpret_cast<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE*>(pStream);
    if (offset > 0) {
      stream << ", ";
    }
    switch (subobjectType) {
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE:
      printObjectKey(stream, arg.rootSignatureKey);
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
  if (!arg.value) {
    return stream << "nullptr";
  }
  stream << "D3D12_STATE_OBJECT_DESC{";
  stream << arg.value->Type << ", " << arg.value->NumSubobjects << ", [";
  for (unsigned i = 0; i < arg.value->NumSubobjects; ++i) {
    const D3D12_STATE_SUBOBJECT& subobject = arg.value->pSubobjects[i];
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
      printObjectKey(stream, arg.interfaceKeysBySubobject[i]);
      break;
    }
    case D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE: {
      auto desc = static_cast<const D3D12_LOCAL_ROOT_SIGNATURE*>(subobject.pDesc);
      printObjectKey(stream, arg.interfaceKeysBySubobject[i]);
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
      printObjectKey(stream, arg.interfaceKeysBySubobject[i]);
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
      for (unsigned j = 0; j < arg.value->NumSubobjects; ++j) {
        if (desc->pSubobjectToAssociate == &arg.value->pSubobjects[j]) {
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
  if (!arg.value) {
    return stream << "nullptr";
  }
  stream << "D3D12_SHADER_RESOURCE_VIEW_DESC{";

  stream << arg.value->Format << ", " << arg.value->ViewDimension << ", "
         << arg.value->Shader4ComponentMapping << ", ";
  switch (arg.value->ViewDimension) {
  case D3D12_SRV_DIMENSION_BUFFER:
    stream << arg.value->Buffer;
    break;
  case D3D12_SRV_DIMENSION_TEXTURE1D:
    stream << arg.value->Texture1D;
    break;
  case D3D12_SRV_DIMENSION_TEXTURE1DARRAY:
    stream << arg.value->Texture1DArray;
    break;
  case D3D12_SRV_DIMENSION_TEXTURE2D:
    stream << arg.value->Texture2D;
    break;
  case D3D12_SRV_DIMENSION_TEXTURE2DARRAY:
    stream << arg.value->Texture2DArray;
    break;
  case D3D12_SRV_DIMENSION_TEXTURE2DMS:
    stream << arg.value->Texture2DMS;
    break;
  case D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY:
    stream << arg.value->Texture2DMSArray;
    break;
  case D3D12_SRV_DIMENSION_TEXTURE3D:
    stream << arg.value->Texture3D;
    break;
  case D3D12_SRV_DIMENSION_TEXTURECUBE:
    stream << arg.value->Buffer;
    break;
  case D3D12_SRV_DIMENSION_TEXTURECUBEARRAY:
    stream << arg.value->TextureCubeArray;
    break;
  case D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE:
    stream << "{";
    printObjectKey(stream, arg.raytracingLocationKey);
    stream << ", " << arg.raytracingLocationOffset << "}";
    break;
  }
  return stream << "}}";
}

FastOStream& operator<<(FastOStream& stream, ArrayArgument<D3D12_RESIDENCY_PRIORITY>& arg) {
  if (!arg.value) {
    return stream << "nullptr";
  }

  stream << "[";
  for (int i = 0; i < arg.size; ++i) {
    if (i > 0) {
      stream << ", ";
    }
    switch (arg.value[i]) {
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
      printHex(stream, static_cast<unsigned>(arg.value[i]));
      break;
    }
  }
  stream << "]";
  return stream;
}

FastOStream& operator<<(FastOStream& stream, D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument& arg) {
  if (!arg.value) {
    return stream << "nullptr";
  }

  stream << "D3D12_RENDER_PASS_RENDER_TARGET_DESC[";
  for (int i = 0, j = 0; i < arg.size; ++i) {
    if (i > 0) {
      stream << ", ";
    }
    stream << "{{";
    stream << arg.value[i].cpuDescriptor.ptr << ", ";
    printObjectKey(stream, arg.descriptorKeys[i]);
    stream << ", " << arg.descriptorIndexes[i];
    stream << "}, {";
    stream << arg.value[i].BeginningAccess.Type;
    if (arg.value[i].BeginningAccess.Type == D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR) {
      stream << ", " << arg.value[i].BeginningAccess.Clear;
    } else if (arg.value[i].BeginningAccess.Type ==
                   D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE_LOCAL_RENDER ||
               arg.value[i].BeginningAccess.Type ==
                   D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE_LOCAL_SRV ||
               arg.value[i].BeginningAccess.Type ==
                   D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE_LOCAL_UAV) {
      stream << ", " << arg.value[i].BeginningAccess.PreserveLocal;
    }
    stream << "}, {";
    stream << arg.value[i].EndingAccess.Type;
    if (arg.value[i].EndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE) {
      stream << ", {";
      printObjectKey(stream, arg.resolveSrcResourceKeys[j]);
      stream << ", ";
      printObjectKey(stream, arg.resolveDstResourceKeys[j]);
      stream << ", " << arg.value[i].EndingAccess.Resolve.SubresourceCount << ", [";
      for (unsigned k = 0; k < arg.value[i].EndingAccess.Resolve.SubresourceCount; ++k) {
        if (k > 0) {
          stream << ", ";
        }
        stream << "{" << arg.value[i].EndingAccess.Resolve.pSubresourceParameters[k] << "}";
      }
      stream << "], " << arg.value[i].EndingAccess.Resolve.Format << ", "
             << arg.value[i].EndingAccess.Resolve.ResolveMode << ", "
             << arg.value[i].EndingAccess.Resolve.PreserveResolveSource << "}";
      ++j;
    } else if (arg.value[i].EndingAccess.Type ==
                   D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE_LOCAL_RENDER ||
               arg.value[i].EndingAccess.Type ==
                   D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE_LOCAL_SRV ||
               arg.value[i].EndingAccess.Type ==
                   D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE_LOCAL_UAV) {
      stream << ", " << arg.value[i].EndingAccess.PreserveLocal;
    }
    stream << "}}";
  }
  stream << "]";
  return stream;
}

FastOStream& operator<<(FastOStream& stream, D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument& arg) {
  if (!arg.value) {
    return stream << "nullptr";
  }

  stream << "D3D12_RENDER_PASS_DEPTH_STENCIL_DESC{{";
  stream << arg.value->cpuDescriptor.ptr << ", ";
  printObjectKey(stream, arg.descriptorKey);
  stream << ", " << arg.descriptorIndex;
  stream << "}, {";
  stream << arg.value->DepthBeginningAccess.Type;
  if (arg.value->DepthBeginningAccess.Type == D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR) {
    stream << ", " << arg.value->DepthBeginningAccess.Clear;
  } else if (arg.value->DepthBeginningAccess.Type ==
                 D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE_LOCAL_RENDER ||
             arg.value->DepthBeginningAccess.Type ==
                 D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE_LOCAL_SRV ||
             arg.value->DepthBeginningAccess.Type ==
                 D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE_LOCAL_UAV) {
    stream << ", " << arg.value->DepthBeginningAccess.PreserveLocal;
  }
  stream << "}, {";
  stream << arg.value->StencilBeginningAccess.Type;
  if (arg.value->StencilBeginningAccess.Type == D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR) {
    stream << ", " << arg.value->StencilBeginningAccess.Clear;
  } else if (arg.value->StencilBeginningAccess.Type ==
                 D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE_LOCAL_RENDER ||
             arg.value->StencilBeginningAccess.Type ==
                 D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE_LOCAL_SRV ||
             arg.value->StencilBeginningAccess.Type ==
                 D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE_LOCAL_UAV) {
    stream << ", " << arg.value->StencilBeginningAccess.PreserveLocal;
  }
  stream << "}, {";
  stream << arg.value->DepthEndingAccess.Type;
  if (arg.value->DepthEndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE) {
    stream << ", {";
    printObjectKey(stream, arg.resolveSrcDepthKey);
    stream << ", ";
    printObjectKey(stream, arg.resolveDstDepthKey);
    stream << ", " << arg.value->DepthEndingAccess.Resolve.SubresourceCount << ", [";
    for (unsigned k = 0; k < arg.value->DepthEndingAccess.Resolve.SubresourceCount; ++k) {
      if (k > 0) {
        stream << ", ";
      }
      stream << "{" << arg.value->DepthEndingAccess.Resolve.pSubresourceParameters[k] << "}";
    }
    stream << "], " << arg.value->DepthEndingAccess.Resolve.Format << ", "
           << arg.value->DepthEndingAccess.Resolve.ResolveMode << ", "
           << arg.value->DepthEndingAccess.Resolve.PreserveResolveSource << "}";
  } else if (arg.value->DepthEndingAccess.Type ==
                 D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE_LOCAL_RENDER ||
             arg.value->DepthEndingAccess.Type ==
                 D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE_LOCAL_SRV ||
             arg.value->DepthEndingAccess.Type ==
                 D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE_LOCAL_UAV) {
    stream << ", " << arg.value->DepthEndingAccess.PreserveLocal;
  }
  stream << "}, {";
  stream << arg.value->DepthEndingAccess.Type;
  if (arg.value->StencilEndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE) {
    stream << ", {";
    printObjectKey(stream, arg.resolveSrcStencilKey);
    stream << ", ";
    printObjectKey(stream, arg.resolveDstStencilKey);
    stream << ", " << arg.value->StencilEndingAccess.Resolve.SubresourceCount << ", [";
    for (unsigned k = 0; k < arg.value->StencilEndingAccess.Resolve.SubresourceCount; ++k) {
      if (k > 0) {
        stream << ", ";
      }
      stream << "{" << arg.value->StencilEndingAccess.Resolve.pSubresourceParameters[k] << "}";
    }
    stream << "], " << arg.value->StencilEndingAccess.Resolve.Format << ", "
           << arg.value->StencilEndingAccess.Resolve.ResolveMode << ", "
           << arg.value->StencilEndingAccess.Resolve.PreserveResolveSource << "}";
  } else if (arg.value->StencilEndingAccess.Type ==
                 D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE_LOCAL_RENDER ||
             arg.value->StencilEndingAccess.Type ==
                 D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE_LOCAL_SRV ||
             arg.value->StencilEndingAccess.Type ==
                 D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE_LOCAL_UAV) {
    stream << ", " << arg.value->StencilEndingAccess.PreserveLocal;
  }
  stream << "}";

  return stream;
}

FastOStream& operator<<(FastOStream& stream,
                        PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC>& arg) {
  if (!arg.value) {
    return stream << "nullptr";
  }
  stream << "D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC{{";
  printObjectKey(stream, arg.destAccelerationStructureKey);
  stream << ", " << arg.destAccelerationStructureOffset << "}, {";

  stream << arg.value->Inputs.Type << ", " << arg.value->Inputs.Flags << ", "
         << arg.value->Inputs.NumDescs << ", " << arg.value->Inputs.DescsLayout << ", ";
  if (!arg.inputKeys.empty() &&
      arg.value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    stream << "{";
    printObjectKey(stream, arg.inputKeys[0]);
    stream << ", " << arg.inputOffsets[0] << "}";
  } else if (arg.value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
    unsigned inputIndex = 0;
    stream << "[";
    for (unsigned i = 0; i < arg.value->Inputs.NumDescs; ++i) {
      D3D12_RAYTRACING_GEOMETRY_DESC& desc = const_cast<D3D12_RAYTRACING_GEOMETRY_DESC&>(
          arg.value->Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY
              ? arg.value->Inputs.pGeometryDescs[i]
              : *arg.value->Inputs.ppGeometryDescs[i]);

      if (i > 0) {
        stream << ", ";
      }
      stream << "{" << desc.Type << ", " << desc.Flags << ", {";

      if (!arg.inputKeys.empty() && desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES) {
        stream << "{";
        printObjectKey(stream, arg.inputKeys[inputIndex]);
        stream << ", " << arg.inputOffsets[inputIndex] << "}, ";
        ++inputIndex;
        stream << desc.Triangles.IndexFormat << ", " << desc.Triangles.VertexFormat << ", "
               << desc.Triangles.IndexCount << ", " << desc.Triangles.VertexCount << ", {";
        printObjectKey(stream, arg.inputKeys[inputIndex]);
        stream << ", " << arg.inputOffsets[inputIndex] << "}, {{";
        ++inputIndex;
        printObjectKey(stream, arg.inputKeys[inputIndex]);
        stream << ", " << arg.inputOffsets[inputIndex] << "}, ";
        ++inputIndex;
        stream << desc.Triangles.VertexBuffer.StrideInBytes << "}";
      } else if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS) {
        stream << desc.AABBs.AABBCount << ", {{";
        printObjectKey(stream, arg.inputKeys[inputIndex]);
        stream << ", " << arg.inputOffsets[inputIndex] << "}, ";
        ++inputIndex;
        stream << desc.AABBs.AABBs.StrideInBytes << "}";
      }
      stream << "}}";
    }
    stream << "]";
  }

  stream << "}, {";
  printObjectKey(stream, arg.sourceAccelerationStructureKey);
  stream << ", " << arg.sourceAccelerationStructureOffset << "}, {";
  printObjectKey(stream, arg.scratchAccelerationStructureKey);
  stream << ", " << arg.scratchAccelerationStructureOffset << "}";
  stream << "}";
  return stream;
}

FastOStream& operator<<(
    FastOStream& stream,
    ArrayArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>& arg) {
  if (!arg.value) {
    return stream << "nullptr";
  }

  stream << "[";
  for (unsigned i = 0; i < arg.destBufferKeys.size(); ++i) {
    if (i > 0) {
      stream << ", ";
    }
    stream << "D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC{{";
    printObjectKey(stream, arg.destBufferKeys[i]);
    stream << ", " << arg.destBufferOffsets[i] << "}, ";
    stream << arg.value->InfoType;
    stream << "}";
  }
  stream << "]";
  return stream;
}

FastOStream& operator<<(
    FastOStream& stream,
    PointerArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>& arg) {
  if (!arg.value) {
    return stream << "nullptr";
  }

  stream << "D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC{{";
  printObjectKey(stream, arg.destBufferKey);
  stream << ", " << arg.destBufferOffset << "}, ";
  stream << arg.value->InfoType;
  stream << "}";
  return stream;
}

FastOStream& operator<<(FastOStream& stream, PointerArgument<D3D12_DISPATCH_RAYS_DESC>& arg) {
  if (!arg.value) {
    return stream << "nullptr";
  }
  stream << "D3D12_DISPATCH_RAYS_DESC{";

  stream << "{{";
  printObjectKey(stream, arg.rayGenerationShaderRecordKey);
  stream << ", " << arg.rayGenerationShaderRecordOffset << "}, ";
  stream << arg.value->RayGenerationShaderRecord.SizeInBytes << "}, ";

  stream << "{{";
  printObjectKey(stream, arg.missShaderTableKey);
  stream << ", " << arg.missShaderTableOffset << "}, ";
  stream << arg.value->MissShaderTable.SizeInBytes << ", "
         << arg.value->MissShaderTable.StrideInBytes << "}, ";

  stream << "{{";
  printObjectKey(stream, arg.hitGroupTableKey);
  stream << ", " << arg.hitGroupTableOffset << "}, ";
  stream << arg.value->HitGroupTable.SizeInBytes << ", " << arg.value->HitGroupTable.StrideInBytes
         << "}, ";

  stream << "{{";
  printObjectKey(stream, arg.callableShaderTableKey);
  stream << ", " << arg.callableShaderTableOffset << "}, ";
  stream << arg.value->CallableShaderTable.SizeInBytes << ", "
         << arg.value->CallableShaderTable.StrideInBytes << "}, ";

  stream << arg.value->Width << ", " << arg.value->Height << ", " << arg.value->Depth;

  stream << "}";
  return stream;
}

FastOStream& operator<<(FastOStream& stream, PointerArgument<D3D12_ROOT_SIGNATURE_DESC>& arg) {
  if (!arg.value) {
    return stream << "nullptr";
  }
  stream << "D3D12_ROOT_SIGNATURE_DESC{";
  stream << arg.value->NumParameters << ", ";
  stream << "array, ";
  stream << arg.value->NumStaticSamplers << ", ";
  stream << "array, ";
  stream << arg.value->Flags;
  stream << "}";
  return stream;
}

FastOStream& operator<<(FastOStream& stream,
                        PointerArgument<D3D12_VERSIONED_ROOT_SIGNATURE_DESC>& arg) {
  if (!arg.value) {
    return stream << "nullptr";
  }
  stream << "D3D12_VERSIONED_ROOT_SIGNATURE_DESC{";
  stream << arg.value->Version << ", ";
  switch (arg.value->Version) {
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
  if (!arg.value) {
    return stream << "nullptr";
  }
  stream << "DML_BINDING_TABLE_DESC{";
  printObjectKey(stream, arg.data.dispatchableKey);
  stream << ", ";
  stream << arg.value->CPUDescriptorHandle << ", ";
  stream << arg.value->GPUDescriptorHandle << ", ";
  stream << arg.value->SizeInDescriptors;
  stream << "}";
  return stream;
}

FastOStream& operator<<(FastOStream& stream, DML_GRAPH_DESC_Argument& arg) {
  return stream << const_cast<const DML_GRAPH_DESC*>(arg.value);
}

FastOStream& operator<<(FastOStream& stream, DML_BINDING_DESC_Argument& arg) {
  if (!arg.value) {
    return stream << "nullptr";
  }
  stream << "DML_BINDING_DESC{";
  stream << arg.value->Type << ", ";
  stream << "{";
  for (unsigned i = 0; i < arg.resourceKeysSize; ++i) {
    if (i > 0) {
      stream << ", ";
    }
    printObjectKey(stream, arg.resourceKeys[i]);
  }
  stream << "}}";
  return stream;
}

FastOStream& operator<<(FastOStream& stream, DML_BINDING_DESCs_Argument& arg) {
  if (!arg.value) {
    return stream << "nullptr";
  }
  unsigned currentKey = 0;
  stream << "DML_BINDING_DESC[";
  for (unsigned i = 0; i < arg.size; ++i) {
    if (i > 0) {
      stream << ", ";
    }
    stream << "{";
    stream << arg.value[i].Type << ", ";
    stream << "{";
    switch (arg.value[i].Type) {
    case DML_BINDING_TYPE_NONE:
      stream << "DML_BINDING_TYPE_NONE";
      break;
    case DML_BINDING_TYPE_BUFFER:
      printObjectKey(stream, arg.resourceKeys[currentKey++]);
      break;
    case DML_BINDING_TYPE_BUFFER_ARRAY: {
      const auto* bufferArray =
          reinterpret_cast<const DML_BUFFER_ARRAY_BINDING*>(arg.value[i].Desc);
      for (unsigned j = 0; j < bufferArray->BindingCount; ++j) {
        if (j > 0) {
          stream << ", ";
        }
        printObjectKey(stream, arg.resourceKeys[currentKey++]);
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
  return stream << const_cast<const DML_OPERATOR_DESC*>(arg.value);
}

FastOStream& operator<<(FastOStream& stream, xess_d3d12_init_params_t_Argument& arg) {
  if (!arg.value) {
    return stream << "nullptr";
  }
  stream << "xess_d3d12_init_params_t{";
  stream << "{" << arg.value->outputResolution.x << ", " << arg.value->outputResolution.y << "}, ";
  stream << toStr(arg.value->qualitySetting);
  stream << ", ";
  stream << arg.value->initFlags << ", ";
  stream << arg.value->creationNodeMask << ", ";
  stream << arg.value->visibleNodeMask << ", ";
  printObjectKey(stream, arg.tempBufferHeapKey);
  stream << ", " << arg.value->bufferHeapOffset << ", ";
  printObjectKey(stream, arg.tempTextureHeapKey);
  stream << ", " << arg.value->textureHeapOffset << ", ";
  printObjectKey(stream, arg.pipelineLibraryKey);
  stream << "}";
  return stream;
}

FastOStream& operator<<(FastOStream& stream, xess_d3d12_execute_params_t_Argument& arg) {
  if (!arg.value) {
    return stream << "nullptr";
  }
  stream << "xess_d3d12_execute_params_t{";
  printObjectKey(stream, arg.colorTextureKey);
  stream << ", ";
  printObjectKey(stream, arg.velocityTextureKey);
  stream << ", ";
  printObjectKey(stream, arg.depthTextureKey);
  stream << ", ";
  printObjectKey(stream, arg.exposureScaleTextureKey);
  stream << ", ";
  printObjectKey(stream, arg.responsivePixelMaskTextureKey);
  stream << ", ";
  printObjectKey(stream, arg.outputTextureKey);
  stream << ", ";
  stream << arg.value->jitterOffsetX << ", ";
  stream << arg.value->jitterOffsetY << ", ";
  stream << arg.value->exposureScale << ", ";
  stream << arg.value->resetHistory << ", ";
  stream << arg.value->inputWidth << ", ";
  stream << arg.value->inputHeight << ", ";
  stream << arg.value->inputColorBase << ", ";
  stream << arg.value->inputMotionVectorBase << ", ";
  stream << arg.value->inputDepthBase << ", ";
  stream << arg.value->inputResponsiveMaskBase << ", ";
  stream << arg.value->reserved0 << ", ";
  stream << arg.value->outputColorBase << ", ";
  printObjectKey(stream, arg.descriptorHeapKey);
  stream << ", ";
  stream << arg.value->descriptorHeapOffset;
  stream << "}";
  return stream;
}

FastOStream& operator<<(FastOStream& stream, DML_CheckFeatureSupport_BufferArgument& arg) {
  return stream << "FeatureQueryDataBuffer{" << arg.size << "}";
}

FastOStream& operator<<(FastOStream& stream, DSTORAGE_QUEUE_DESC_Argument& arg) {
  if (!arg.value) {
    return stream << "nullptr";
  }
  stream << "DSTORAGE_QUEUE_DESC{";
  stream << arg.value->SourceType << ", ";
  stream << arg.value->Capacity << ", ";
  stream << arg.value->Priority << ", ";
  if (arg.value->Name) {
    stream << "\"" << arg.value->Name << "\", ";
  } else {
    stream << "nullptr, ";
  }
  printObjectKey(stream, arg.deviceKey);
  stream << "}";
  return stream;
}

FastOStream& operator<<(FastOStream& stream, DSTORAGE_REQUEST_Argument& arg) {
  stream << "DSTORAGE_REQUEST{";
  stream << arg.value->Options << ", ";
  switch (arg.value->Options.SourceType) {
  case DSTORAGE_REQUEST_SOURCE_MEMORY:
    stream << arg.value->Source.Memory << ", ";
    break;
  case DSTORAGE_REQUEST_SOURCE_FILE:
    stream << "DSTORAGE_SOURCE_FILE{";
    printObjectKey(stream, arg.fileKey) << ", ";
    stream << arg.value->Source.File.Offset << ", ";
    stream << arg.value->Source.File.Size;
    stream << "}, ";
    break;
  default:
    stream << "unknown, ";
  }
  switch (arg.value->Options.DestinationType) {
  case DSTORAGE_REQUEST_DESTINATION_MEMORY:
    stream << arg.value->Destination.Memory << ", ";
    break;
  case DSTORAGE_REQUEST_DESTINATION_BUFFER:
    stream << "DSTORAGE_DESTINATION_BUFFER{";
    printObjectKey(stream, arg.resourceKey) << ", ";
    stream << arg.value->Destination.Buffer.Offset << ", ";
    stream << arg.value->Destination.Buffer.Size;
    stream << "}, ";
    break;
  case DSTORAGE_REQUEST_DESTINATION_TEXTURE_REGION:
    stream << "DSTORAGE_DESTINATION_TEXTURE_REGION{";
    printObjectKey(stream, arg.resourceKey) << ", ";
    stream << arg.value->Destination.Texture.SubresourceIndex << ", ";
    stream << arg.value->Destination.Texture.Region;
    stream << "}, ";
    break;
  case DSTORAGE_REQUEST_DESTINATION_MULTIPLE_SUBRESOURCES:
    stream << "DSTORAGE_DESTINATION_MULTIPLE_SUBRESOURCES{";
    printObjectKey(stream, arg.resourceKey) << ", ";
    stream << arg.value->Destination.MultipleSubresources.FirstSubresource;
    stream << "}, ";
    break;
  case DSTORAGE_REQUEST_DESTINATION_TILES:
    stream << "DSTORAGE_DESTINATION_TEXTURE_TILES{";
    printObjectKey(stream, arg.resourceKey) << ", ";
    stream << arg.value->Destination.Tiles.TiledRegionStartCoordinate << ", ";
    stream << arg.value->Destination.Tiles.TileRegionSize;
    stream << "}, ";
    break;
  default:
    stream << "unknown, ";
  }
  stream << arg.value->UncompressedSize << ", ";
  stream << arg.value->CancellationTag << ", ";
  if (arg.value->Name) {
    stream << arg.value->Name;
  } else {
    stream << "nullptr";
  }
  stream << "}";
  return stream;
}

FastOStream& operator<<(
    FastOStream& stream,
    PointerArgument<NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS>& arg) {
  if (!arg.value) {
    return stream << "nullptr";
  }
  stream << "NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS{";
  stream << arg.value->version << ", {";
  printObjectKey(stream, arg.destAccelerationStructureKey);
  stream << ", " << arg.destAccelerationStructureOffset << "} (0x";
  printHex(stream, arg.value->pDesc->destAccelerationStructureData) << "), {";

  stream << arg.value->pDesc->inputs.type << ", " << arg.value->pDesc->inputs.flags << ", "
         << arg.value->pDesc->inputs.numDescs << ", " << arg.value->pDesc->inputs.descsLayout
         << ", " << arg.value->pDesc->inputs.geometryDescStrideInBytes << ", ";
  if (!arg.inputKeys.empty() &&
      arg.value->pDesc->inputs.type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    stream << "{";
    printObjectKey(stream, arg.inputKeys[0]);
    stream << ", " << arg.inputOffsets[0] << "}";
  } else if (arg.value->pDesc->inputs.type ==
             D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
    unsigned inputIndex = 0;
    stream << "[";
    for (unsigned i = 0; i < arg.value->pDesc->inputs.numDescs; ++i) {
      const NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX* desc =
          arg.value->pDesc->inputs.descsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY
              ? (NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX*)((char*)(arg.value->pDesc->inputs
                                                                       .pGeometryDescs) +
                                                           arg.value->pDesc->inputs
                                                                   .geometryDescStrideInBytes *
                                                               i)
              : arg.value->pDesc->inputs.ppGeometryDescs[i];
      if (i > 0) {
        stream << ", ";
      }
      stream << "{" << desc->type << ", " << desc->flags << ", {";

      if (!arg.inputKeys.empty() && desc->type == D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES) {
        stream << "{";
        printObjectKey(stream, arg.inputKeys[inputIndex]);
        stream << ", " << arg.inputOffsets[inputIndex] << "}, ";
        ++inputIndex;
        stream << desc->triangles.IndexFormat << ", " << desc->triangles.VertexFormat << ", "
               << desc->triangles.IndexCount << ", " << desc->triangles.VertexCount << ", {";
        printObjectKey(stream, arg.inputKeys[inputIndex]);
        stream << ", " << arg.inputOffsets[inputIndex] << "}, {{";
        ++inputIndex;
        printObjectKey(stream, arg.inputKeys[inputIndex]);
        stream << ", " << arg.inputOffsets[inputIndex] << "}, ";
        ++inputIndex;
        stream << desc->triangles.VertexBuffer.StrideInBytes << "}";
      } else if (desc->type == D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS) {
        stream << desc->aabbs.AABBCount << ", {{";
        printObjectKey(stream, arg.inputKeys[inputIndex]);
        stream << ", " << arg.inputOffsets[inputIndex] << "}, ";
        ++inputIndex;
        stream << desc->aabbs.AABBs.StrideInBytes << "}";
      } else if (desc->type == NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES_EX) {
        stream << "{";
        printObjectKey(stream, arg.inputKeys[inputIndex]);
        stream << ", " << arg.inputOffsets[inputIndex] << "}, ";
        ++inputIndex;
        stream << desc->ommTriangles.triangles.IndexFormat << ", "
               << desc->ommTriangles.triangles.VertexFormat << ", "
               << desc->ommTriangles.triangles.IndexCount << ", "
               << desc->ommTriangles.triangles.VertexCount << ", {";
        printObjectKey(stream, arg.inputKeys[inputIndex]);
        stream << ", " << arg.inputOffsets[inputIndex] << "}, {{";
        ++inputIndex;
        printObjectKey(stream, arg.inputKeys[inputIndex]);
        stream << ", " << arg.inputOffsets[inputIndex] << "}, ";
        ++inputIndex;
        stream << desc->ommTriangles.triangles.VertexBuffer.StrideInBytes << "}, {{";
        printObjectKey(stream, arg.inputKeys[inputIndex]);
        stream << ", " << arg.inputOffsets[inputIndex] << "}, ";
        ++inputIndex;
        stream << desc->ommTriangles.ommAttachment.opacityMicromapIndexBuffer.StrideInBytes
               << "}, ";
        stream << desc->ommTriangles.ommAttachment.opacityMicromapIndexFormat << ", "
               << desc->ommTriangles.ommAttachment.opacityMicromapBaseLocation << ", {";
        printObjectKey(stream, arg.inputKeys[inputIndex]);
        stream << ", " << arg.inputOffsets[inputIndex] << "}, ";
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
        printObjectKey(stream, arg.inputKeys[inputIndex]);
        stream << ", " << arg.inputOffsets[inputIndex] << "}, ";
        ++inputIndex;
        stream << desc->dmmTriangles.triangles.IndexFormat << ", "
               << desc->dmmTriangles.triangles.VertexFormat << ", "
               << desc->dmmTriangles.triangles.IndexCount << ", "
               << desc->dmmTriangles.triangles.VertexCount << ", {";
        printObjectKey(stream, arg.inputKeys[inputIndex]);
        stream << ", " << arg.inputOffsets[inputIndex] << "}, {{";
        ++inputIndex;
        printObjectKey(stream, arg.inputKeys[inputIndex]);
        stream << ", " << arg.inputOffsets[inputIndex] << "}, ";
        ++inputIndex;
        stream << desc->dmmTriangles.triangles.VertexBuffer.StrideInBytes << "}, {{";
        printObjectKey(stream, arg.inputKeys[inputIndex]);
        stream << ", " << arg.inputOffsets[inputIndex] << "}, ";
        ++inputIndex;
        stream << desc->dmmTriangles.dmmAttachment.triangleMicromapIndexBuffer.StrideInBytes
               << "}, ";
        stream << desc->dmmTriangles.dmmAttachment.triangleMicromapIndexFormat << ", "
               << desc->dmmTriangles.dmmAttachment.triangleMicromapBaseLocation << ", {{";
        printObjectKey(stream, arg.inputKeys[inputIndex]);
        stream << ", " << arg.inputOffsets[inputIndex] << "}, ";
        ++inputIndex;
        stream << desc->dmmTriangles.dmmAttachment.trianglePrimitiveFlagsBuffer.StrideInBytes
               << "}, {{";
        printObjectKey(stream, arg.inputKeys[inputIndex]);
        stream << ", " << arg.inputOffsets[inputIndex] << "}, ";
        ++inputIndex;
        stream << desc->dmmTriangles.dmmAttachment.vertexBiasAndScaleBuffer.StrideInBytes << "}, ";
        stream << desc->dmmTriangles.dmmAttachment.vertexBiasAndScaleFormat << ", {{";
        printObjectKey(stream, arg.inputKeys[inputIndex]);
        stream << ", " << arg.inputOffsets[inputIndex] << "}, ";
        ++inputIndex;
        stream << desc->dmmTriangles.dmmAttachment.vertexDisplacementVectorBuffer.StrideInBytes
               << "}, ";
        stream << desc->dmmTriangles.dmmAttachment.vertexDisplacementVectorFormat << ", {";
        printObjectKey(stream, arg.inputKeys[inputIndex]);
        stream << ", " << arg.inputOffsets[inputIndex] << "}, ";
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
        printObjectKey(stream, arg.inputKeys[inputIndex]);
        stream << ", " << arg.inputOffsets[inputIndex] << "}, ";
        ++inputIndex;
        stream << desc->spheres.vertexPositionBuffer.StrideInBytes << "}, ";
        stream << desc->spheres.vertexPositionFormat << ", {{";
        printObjectKey(stream, arg.inputKeys[inputIndex]);
        stream << ", " << arg.inputOffsets[inputIndex] << "}, ";
        ++inputIndex;
        stream << desc->spheres.vertexRadiusBuffer.StrideInBytes << "}, ";
        stream << desc->spheres.vertexRadiusFormat << ", {{";
        printObjectKey(stream, arg.inputKeys[inputIndex]);
        stream << ", " << arg.inputOffsets[inputIndex] << "}, ";
        ++inputIndex;
        stream << desc->spheres.indexBuffer.StrideInBytes << "}, ";
        stream << desc->spheres.indexFormat << "}";
      } else if (desc->type == NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_LSS_EX) {
        stream << "{" << desc->lss.vertexCount << ", " << desc->lss.indexCount << ", "
               << desc->lss.primitiveCount << ", {{";
        printObjectKey(stream, arg.inputKeys[inputIndex]);
        stream << ", " << arg.inputOffsets[inputIndex] << "}, ";
        ++inputIndex;
        stream << desc->lss.vertexPositionBuffer.StrideInBytes << "}, ";
        stream << desc->lss.vertexPositionFormat << ", {{";
        printObjectKey(stream, arg.inputKeys[inputIndex]);
        stream << ", " << arg.inputOffsets[inputIndex] << "}, ";
        ++inputIndex;
        stream << desc->lss.vertexRadiusBuffer.StrideInBytes << "}, ";
        stream << desc->lss.vertexRadiusFormat << ", {{";
        printObjectKey(stream, arg.inputKeys[inputIndex]);
        stream << ", " << arg.inputOffsets[inputIndex] << "}, ";
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
  printObjectKey(stream, arg.sourceAccelerationStructureKey);
  stream << ", " << arg.sourceAccelerationStructureOffset << "}, {";
  printObjectKey(stream, arg.scratchAccelerationStructureKey);
  stream << ", " << arg.scratchAccelerationStructureOffset << "}, ";

  stream << arg.value->numPostbuildInfoDescs << ", ";
  if (!arg.value->pPostbuildInfoDescs) {
    stream << "nullptr";
  } else {
    stream << "[";
    for (unsigned i = 0; i < arg.destPostBuildBufferKeys.size(); ++i) {
      if (i > 0) {
        stream << ", ";
      }
      stream << "D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC{{";
      printObjectKey(stream, arg.destPostBuildBufferKeys[i]);
      stream << ", " << arg.destPostBuildBufferOffsets[i] << "}, ";
      stream << arg.value->pPostbuildInfoDescs[i].InfoType;
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
  if (!arg.value) {
    return stream << "nullptr";
  }
  stream << "NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS{";

  stream << arg.value->version << ", {";

  printObjectKey(stream, arg.destOpacityMicromapArrayDataKey);
  stream << ", " << arg.destOpacityMicromapArrayDataOffset << "} (0x";
  printHex(stream, arg.value->pDesc->destOpacityMicromapArrayData) << "), {";

  stream << arg.value->pDesc->inputs.flags << ", " << arg.value->pDesc->inputs.numOMMUsageCounts
         << ", ";
  if (!arg.value->pDesc->inputs.pOMMUsageCounts) {
    stream << "nullptr";
  } else {
    stream << "[";
    for (unsigned i = 0; i < arg.value->pDesc->inputs.numOMMUsageCounts; ++i) {
      if (i > 0) {
        stream << ", ";
      }
      stream << "NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_USAGE_COUNT{";
      stream << arg.value->pDesc->inputs.pOMMUsageCounts[i].count << ", "
             << arg.value->pDesc->inputs.pOMMUsageCounts[i].subdivisionLevel << ", "
             << arg.value->pDesc->inputs.pOMMUsageCounts[i].format << "}";
    }
    stream << "]";
  }
  stream << ", {";
  printObjectKey(stream, arg.inputBufferKey);
  stream << ", " << arg.inputBufferOffset << "}, {{";
  printObjectKey(stream, arg.perOMMDescsKey);
  stream << ", " << arg.perOMMDescsOffset << "}, "
         << arg.value->pDesc->inputs.perOMMDescs.StrideInBytes << "}";

  stream << "}, {";
  printObjectKey(stream, arg.scratchOpacityMicromapArrayDataKey);
  stream << ", " << arg.scratchOpacityMicromapArrayDataOffset << "}, ";

  stream << arg.value->numPostbuildInfoDescs << ", ";
  if (!arg.value->pPostbuildInfoDescs) {
    stream << "nullptr";
  } else {
    stream << "[";
    for (unsigned i = 0; i < arg.destPostBuildBufferKeys.size(); ++i) {
      if (i > 0) {
        stream << ", ";
      }
      stream << "D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC{{";
      printObjectKey(stream, arg.destPostBuildBufferKeys[i]);
      stream << ", " << arg.destPostBuildBufferOffsets[i] << "}, ";
      stream << arg.value->pPostbuildInfoDescs[i].infoType;
      stream << "}";
    }
    stream << "]";
  }

  return stream;
}

FastOStream& operator<<(
    FastOStream& stream,
    PointerArgument<NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS>& arg) {
  if (!arg.value) {
    return stream << "nullptr";
  }
  stream << "NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS{";

  stream << arg.value->version << ", ";

  stream << arg.value->pDesc->inputs.maxArgCount << ", " << arg.value->pDesc->inputs.flags << ", "
         << arg.value->pDesc->inputs.type << ", " << arg.value->pDesc->inputs.mode << ", {";

  if (arg.value->pDesc->inputs.type ==
      NVAPI_D3D12_RAYTRACING_MULTI_INDIRECT_CLUSTER_OPERATION_TYPE_BUILD_BLAS_FROM_CLAS) {
    stream << arg.value->pDesc->inputs.clasDesc.maxTotalClasCount << ", "
           << arg.value->pDesc->inputs.clasDesc.maxClasCountPerArg;
  } else if (arg.value->pDesc->inputs.type ==
             NVAPI_D3D12_RAYTRACING_MULTI_INDIRECT_CLUSTER_OPERATION_TYPE_MOVE_CLUSTER_OBJECT) {
    stream << arg.value->pDesc->inputs.movesDesc.type << ", "
           << arg.value->pDesc->inputs.movesDesc.maxBytesMoved;
  } else {
    stream << static_cast<DXGI_FORMAT>(arg.value->pDesc->inputs.trianglesDesc.vertexFormat) << ", "
           << arg.value->pDesc->inputs.trianglesDesc.maxGeometryIndexValue << ", "
           << arg.value->pDesc->inputs.trianglesDesc.maxUniqueGeometryCountPerArg << ", "
           << arg.value->pDesc->inputs.trianglesDesc.maxTriangleCountPerArg << ", "
           << arg.value->pDesc->inputs.trianglesDesc.maxVertexCountPerArg << ", "
           << arg.value->pDesc->inputs.trianglesDesc.maxTotalTriangleCount << ", "
           << arg.value->pDesc->inputs.trianglesDesc.maxTotalVertexCount << ", "
           << arg.value->pDesc->inputs.trianglesDesc.minPositionTruncateBitCount;
  }

  stream << "}}, ";
  stream << static_cast<
                NVAPI_D3D12_RAYTRACING_MULTI_INDIRECT_CLUSTER_OPERATION_ADDRESS_RESOLUTION_FLAGS>(
                arg.value->pDesc->addressResolutionFlags)
         << ", {";

  printObjectKey(stream, arg.batchResultDataKey);
  stream << ", " << arg.batchResultDataOffset << "}, {";

  printObjectKey(stream, arg.batchScratchDataKey);
  stream << ", " << arg.batchScratchDataOffset << "}, {{";

  printObjectKey(stream, arg.destinationAddressArrayKey);
  stream << ", " << arg.destinationAddressArrayOffset << "}, "
         << arg.value->pDesc->destinationAddressArray.StrideInBytes << "}, {{";

  printObjectKey(stream, arg.resultSizeArrayKey);
  stream << ", " << arg.resultSizeArrayOffset << "}, "
         << arg.value->pDesc->resultSizeArray.StrideInBytes << "}, {{";

  printObjectKey(stream, arg.indirectArgArrayKey);
  stream << ", " << arg.indirectArgArrayOffset << "}, "
         << arg.value->pDesc->indirectArgArray.StrideInBytes << "}, {";

  printObjectKey(stream, arg.indirectArgCountKey);
  stream << ", " << arg.indirectArgCountOffset << "}";

  return stream;
}

} // namespace DirectX
} // namespace gits
