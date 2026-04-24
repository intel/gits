// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "argumentDecoders.h"
#include "dmlCodersAuto.h"
#include "log.h"

#include <d3dx12/d3dx12_pipeline_state_stream.h>

namespace gits {
namespace DirectX {

void Decode(char* src, unsigned& offset, BufferArgument& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }

  memcpy(&arg.Size, src + offset, sizeof(arg.Size));
  offset += sizeof(arg.Size);

  arg.Value = const_cast<char*>(src + offset);
  offset += arg.Size;
}

void Decode(char* src, unsigned& offset, OutputBufferArgument& arg) {
  memcpy(&arg.CaptureValue, src + offset, sizeof(void*));
  offset += sizeof(void*);
  if (arg.CaptureValue) {
    arg.Value = &arg.Data;
  } else {
    arg.Value = nullptr;
  }
}

void Decode(char* src, unsigned& offset, LPCWSTR_Argument& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }

  unsigned* len = reinterpret_cast<unsigned*>(src + offset);
  offset += sizeof(unsigned);
  arg.Value = reinterpret_cast<LPWSTR>(src + offset);
  offset += *len;
}

void Decode(char* src, unsigned& offset, LPCSTR_Argument& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }

  unsigned* len = reinterpret_cast<unsigned*>(src + offset);
  offset += sizeof(unsigned);
  arg.Value = reinterpret_cast<LPSTR>(src + offset);
  offset += *len;
}

void Decode(char* src, unsigned& offset, D3D12_GPU_VIRTUAL_ADDRESS_Argument& arg) {
  memcpy(&arg.Value, src + offset, sizeof(arg.Value));
  offset += sizeof(arg.Value);
  memcpy(&arg.InterfaceKey, src + offset, sizeof(arg.InterfaceKey));
  offset += sizeof(arg.InterfaceKey);
  memcpy(&arg.Offset, src + offset, sizeof(arg.Offset));
  offset += sizeof(arg.Offset);
}

void Decode(char* src, unsigned& offset, D3D12_GPU_VIRTUAL_ADDRESSs_Argument& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }
  memcpy(&arg.Size, src + offset, sizeof(arg.Size));
  offset += sizeof(arg.Size);

  arg.Value = reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS*>(src + offset);
  offset += sizeof(D3D12_GPU_VIRTUAL_ADDRESS) * arg.Size;

  arg.InterfaceKeys.resize(arg.Size);
  memcpy(arg.InterfaceKeys.data(), src + offset, arg.Size * sizeof(unsigned));
  offset += arg.Size * sizeof(unsigned);

  arg.Offsets.resize(arg.Size);
  memcpy(arg.Offsets.data(), src + offset, arg.Size * sizeof(unsigned));
  offset += arg.Size * sizeof(unsigned);
}

void Decode(char* src, unsigned& offset, ShaderIdentifierArgument& arg) {
  arg.Data.resize(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
  memcpy(arg.Data.data(), src + offset, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
  offset += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
  arg.Value = arg.Data.data();
}

void Decode(char* src, unsigned& offset, D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.Value = reinterpret_cast<D3D12_GRAPHICS_PIPELINE_STATE_DESC*>(src + offset);
  offset += sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC);

  auto decodeBytecode = [&](D3D12_SHADER_BYTECODE& bytecode) {
    if (bytecode.pShaderBytecode) {
      bytecode.pShaderBytecode = src + offset;
      offset += bytecode.BytecodeLength;
    }
  };
  decodeBytecode(arg.Value->VS);
  decodeBytecode(arg.Value->PS);
  decodeBytecode(arg.Value->DS);
  decodeBytecode(arg.Value->HS);
  decodeBytecode(arg.Value->GS);

  if (arg.Value->StreamOutput.pSODeclaration) {
    arg.Value->StreamOutput.pSODeclaration =
        reinterpret_cast<D3D12_SO_DECLARATION_ENTRY*>(src + offset);
    offset += arg.Value->StreamOutput.NumEntries * sizeof(D3D12_SO_DECLARATION_ENTRY);

    for (unsigned i = 0; i < arg.Value->StreamOutput.NumEntries; ++i) {
      unsigned* len = reinterpret_cast<unsigned*>(src + offset);
      offset += sizeof(unsigned);
      const_cast<D3D12_SO_DECLARATION_ENTRY*>(arg.Value->StreamOutput.pSODeclaration)[i]
          .SemanticName = reinterpret_cast<LPCSTR>(src + offset);
      offset += *len;
    }
  }
  if (arg.Value->StreamOutput.pBufferStrides) {
    arg.Value->StreamOutput.pBufferStrides = reinterpret_cast<UINT*>(src + offset);
    offset += arg.Value->StreamOutput.NumStrides * sizeof(UINT);
  }

  arg.Value->InputLayout.pInputElementDescs =
      reinterpret_cast<D3D12_INPUT_ELEMENT_DESC*>(src + offset);
  offset += sizeof(D3D12_INPUT_ELEMENT_DESC) * arg.Value->InputLayout.NumElements;

  for (unsigned i = 0; i < arg.Value->InputLayout.NumElements; ++i) {
    D3D12_INPUT_ELEMENT_DESC& inputElement =
        const_cast<D3D12_INPUT_ELEMENT_DESC&>(arg.Value->InputLayout.pInputElementDescs[i]);
    unsigned* len = reinterpret_cast<unsigned*>(src + offset);
    offset += sizeof(unsigned);
    inputElement.SemanticName = src + offset;
    offset += *len;
  }

  if (arg.Value->CachedPSO.pCachedBlob) {
    arg.Value->CachedPSO.pCachedBlob = src + offset;
    offset += arg.Value->CachedPSO.CachedBlobSizeInBytes;
  }

  memcpy(&arg.RootSignatureKey, src + offset, sizeof(arg.RootSignatureKey));
  offset += sizeof(arg.RootSignatureKey);
}

void Decode(char* src, unsigned& offset, D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.Value = reinterpret_cast<D3D12_COMPUTE_PIPELINE_STATE_DESC*>(src + offset);
  offset += sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC);

  if (arg.Value->CS.pShaderBytecode) {
    arg.Value->CS.pShaderBytecode = src + offset;
    offset += arg.Value->CS.BytecodeLength;
  }

  if (arg.Value->CachedPSO.pCachedBlob) {
    arg.Value->CachedPSO.pCachedBlob = src + offset;
    offset += arg.Value->CachedPSO.CachedBlobSizeInBytes;
  }

  memcpy(&arg.RootSignatureKey, src + offset, sizeof(arg.RootSignatureKey));
  offset += sizeof(arg.RootSignatureKey);
}

void Decode(char* src, unsigned& offset, D3D12_TEXTURE_COPY_LOCATION_Argument& arg) {

  arg.Value = reinterpret_cast<D3D12_TEXTURE_COPY_LOCATION*>(src + offset);
  offset += sizeof(D3D12_TEXTURE_COPY_LOCATION);

  memcpy(&arg.ResourceKey, src + offset, sizeof(arg.ResourceKey));
  offset += sizeof(arg.ResourceKey);
}

void Decode(char* src, unsigned& offset, D3D12_RESOURCE_BARRIERs_Argument& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }

  memcpy(&arg.Size, src + offset, sizeof(arg.Size));
  offset += sizeof(arg.Size);

  arg.Value = reinterpret_cast<D3D12_RESOURCE_BARRIER*>(src + offset);
  offset += arg.Size * sizeof(D3D12_RESOURCE_BARRIER);

  arg.ResourceKeys.resize(arg.Size);
  memcpy(arg.ResourceKeys.data(), src + offset, arg.Size * sizeof(unsigned));
  offset += arg.Size * sizeof(unsigned);

  arg.ResourceAfterKeys.resize(arg.Size);
  memcpy(arg.ResourceAfterKeys.data(), src + offset, arg.Size * sizeof(unsigned));
  offset += arg.Size * sizeof(unsigned);
}

void Decode(char* src, unsigned& offset, PointerArgument<D3D12_ROOT_SIGNATURE_DESC>& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.Value = reinterpret_cast<D3D12_ROOT_SIGNATURE_DESC*>(src + offset);
  offset += sizeof(D3D12_ROOT_SIGNATURE_DESC);

  arg.Value->pStaticSamplers = reinterpret_cast<D3D12_STATIC_SAMPLER_DESC*>(src + offset);
  offset += sizeof(D3D12_STATIC_SAMPLER_DESC) * arg.Value->NumStaticSamplers;

  arg.Value->pParameters = reinterpret_cast<D3D12_ROOT_PARAMETER*>(src + offset);
  offset += sizeof(D3D12_ROOT_PARAMETER) * arg.Value->NumParameters;
  for (unsigned i = 0; i < arg.Value->NumParameters; ++i) {
    if (arg.Value->pParameters[i].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {

      D3D12_ROOT_DESCRIPTOR_TABLE& descriptorTable =
          const_cast<D3D12_ROOT_DESCRIPTOR_TABLE&>(arg.Value->pParameters[i].DescriptorTable);
      descriptorTable.pDescriptorRanges = reinterpret_cast<D3D12_DESCRIPTOR_RANGE*>(src + offset);
      offset += sizeof(D3D12_DESCRIPTOR_RANGE) * descriptorTable.NumDescriptorRanges;
    }
  }
}

void Decode(char* src,
            unsigned& offset,
            PointerArgument<D3D12_VERSIONED_ROOT_SIGNATURE_DESC>& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.Value = reinterpret_cast<D3D12_VERSIONED_ROOT_SIGNATURE_DESC*>(src + offset);
  offset += sizeof(D3D12_VERSIONED_ROOT_SIGNATURE_DESC);

  switch (arg.Value->Version) {
  case D3D_ROOT_SIGNATURE_VERSION_1_0: {
    D3D12_ROOT_SIGNATURE_DESC& desc0 = arg.Value->Desc_1_0;

    desc0.pStaticSamplers = reinterpret_cast<D3D12_STATIC_SAMPLER_DESC*>(src + offset);
    offset += sizeof(D3D12_STATIC_SAMPLER_DESC) * desc0.NumStaticSamplers;

    desc0.pParameters = reinterpret_cast<D3D12_ROOT_PARAMETER*>(src + offset);
    offset += sizeof(D3D12_ROOT_PARAMETER) * desc0.NumParameters;
    for (unsigned i = 0; i < desc0.NumParameters; ++i) {
      if (desc0.pParameters[i].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {

        D3D12_ROOT_DESCRIPTOR_TABLE& descriptorTable =
            const_cast<D3D12_ROOT_DESCRIPTOR_TABLE&>(desc0.pParameters[i].DescriptorTable);
        descriptorTable.pDescriptorRanges = reinterpret_cast<D3D12_DESCRIPTOR_RANGE*>(src + offset);
        offset += sizeof(D3D12_DESCRIPTOR_RANGE) * descriptorTable.NumDescriptorRanges;
      }
    }
  } break;
  case D3D_ROOT_SIGNATURE_VERSION_1_1: {
    D3D12_ROOT_SIGNATURE_DESC1& desc1 = arg.Value->Desc_1_1;

    desc1.pStaticSamplers = reinterpret_cast<D3D12_STATIC_SAMPLER_DESC*>(src + offset);
    offset += sizeof(D3D12_STATIC_SAMPLER_DESC) * desc1.NumStaticSamplers;

    desc1.pParameters = reinterpret_cast<D3D12_ROOT_PARAMETER1*>(src + offset);
    offset += sizeof(D3D12_ROOT_PARAMETER1) * desc1.NumParameters;
    for (unsigned i = 0; i < desc1.NumParameters; ++i) {
      if (desc1.pParameters[i].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {

        D3D12_ROOT_DESCRIPTOR_TABLE1& descriptorTable =
            const_cast<D3D12_ROOT_DESCRIPTOR_TABLE1&>(desc1.pParameters[i].DescriptorTable);
        descriptorTable.pDescriptorRanges =
            reinterpret_cast<D3D12_DESCRIPTOR_RANGE1*>(src + offset);
        offset += sizeof(D3D12_DESCRIPTOR_RANGE1) * descriptorTable.NumDescriptorRanges;
      }
    }
  } break;
  case D3D_ROOT_SIGNATURE_VERSION_1_2: {
    D3D12_ROOT_SIGNATURE_DESC2& desc2 = arg.Value->Desc_1_2;

    desc2.pStaticSamplers = reinterpret_cast<D3D12_STATIC_SAMPLER_DESC1*>(src + offset);
    offset += sizeof(D3D12_STATIC_SAMPLER_DESC1) * desc2.NumStaticSamplers;

    desc2.pParameters = reinterpret_cast<D3D12_ROOT_PARAMETER1*>(src + offset);
    offset += sizeof(D3D12_ROOT_PARAMETER1) * desc2.NumParameters;
    for (unsigned i = 0; i < desc2.NumParameters; ++i) {
      if (desc2.pParameters[i].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {

        D3D12_ROOT_DESCRIPTOR_TABLE1& descriptorTable =
            const_cast<D3D12_ROOT_DESCRIPTOR_TABLE1&>(desc2.pParameters[i].DescriptorTable);
        descriptorTable.pDescriptorRanges =
            reinterpret_cast<D3D12_DESCRIPTOR_RANGE1*>(src + offset);
        offset += sizeof(D3D12_DESCRIPTOR_RANGE1) * descriptorTable.NumDescriptorRanges;
      }
    }
  } break;
  }
}

void Decode(char* src, unsigned& offset, PointerArgument<D3D12_COMMAND_SIGNATURE_DESC>& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.Value = reinterpret_cast<D3D12_COMMAND_SIGNATURE_DESC*>(src + offset);
  offset += sizeof(D3D12_COMMAND_SIGNATURE_DESC);

  arg.Value->pArgumentDescs = reinterpret_cast<D3D12_INDIRECT_ARGUMENT_DESC*>(src + offset);
  offset += sizeof(D3D12_INDIRECT_ARGUMENT_DESC) * arg.Value->NumArgumentDescs;
}

void Decode(char* src, unsigned& offset, D3D12_INDEX_BUFFER_VIEW_Argument& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.Value = reinterpret_cast<D3D12_INDEX_BUFFER_VIEW*>(src + offset);
  offset += sizeof(D3D12_INDEX_BUFFER_VIEW);

  memcpy(&arg.BufferLocationKey, src + offset, sizeof(arg.BufferLocationKey));
  offset += sizeof(arg.BufferLocationKey);
  memcpy(&arg.BufferLocationOffset, src + offset, sizeof(arg.BufferLocationOffset));
  offset += sizeof(arg.BufferLocationOffset);
}

void Decode(char* src, unsigned& offset, D3D12_CONSTANT_BUFFER_VIEW_DESC_Argument& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.Value = reinterpret_cast<D3D12_CONSTANT_BUFFER_VIEW_DESC*>(src + offset);
  offset += sizeof(D3D12_CONSTANT_BUFFER_VIEW_DESC);

  memcpy(&arg.BufferLocationKey, src + offset, sizeof(arg.BufferLocationKey));
  offset += sizeof(arg.BufferLocationKey);
  memcpy(&arg.BufferLocationOffset, src + offset, sizeof(arg.BufferLocationOffset));
  offset += sizeof(arg.BufferLocationOffset);
}

void Decode(char* src, unsigned& offset, D3D12_VERTEX_BUFFER_VIEWs_Argument& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }

  memcpy(&arg.Size, src + offset, sizeof(arg.Size));
  offset += sizeof(arg.Size);

  arg.Value = reinterpret_cast<D3D12_VERTEX_BUFFER_VIEW*>(src + offset);
  offset += arg.Size * sizeof(D3D12_VERTEX_BUFFER_VIEW);

  arg.BufferLocationKeys.resize(arg.Size);
  memcpy(arg.BufferLocationKeys.data(), src + offset, arg.Size * sizeof(unsigned));
  offset += arg.Size * sizeof(unsigned);

  arg.BufferLocationOffsets.resize(arg.Size);
  memcpy(arg.BufferLocationOffsets.data(), src + offset, arg.Size * sizeof(unsigned));
  offset += arg.Size * sizeof(unsigned);
}

void Decode(char* src, unsigned& offset, D3D12_STREAM_OUTPUT_BUFFER_VIEWs_Argument& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }

  memcpy(&arg.Size, src + offset, sizeof(arg.Size));
  offset += sizeof(arg.Size);

  arg.Value = reinterpret_cast<D3D12_STREAM_OUTPUT_BUFFER_VIEW*>(src + offset);
  offset += arg.Size * sizeof(D3D12_STREAM_OUTPUT_BUFFER_VIEW);

  arg.BufferLocationKeys.resize(arg.Size);
  memcpy(arg.BufferLocationKeys.data(), src + offset, arg.Size * sizeof(unsigned));
  offset += arg.Size * sizeof(unsigned);

  arg.BufferLocationOffsets.resize(arg.Size);
  memcpy(arg.BufferLocationOffsets.data(), src + offset, arg.Size * sizeof(unsigned));
  offset += arg.Size * sizeof(unsigned);

  arg.BufferFilledSizeLocationKeys.resize(arg.Size);
  memcpy(arg.BufferFilledSizeLocationKeys.data(), src + offset, arg.Size * sizeof(unsigned));
  offset += arg.Size * sizeof(unsigned);

  arg.BufferFilledSizeLocationOffsets.resize(arg.Size);
  memcpy(arg.BufferFilledSizeLocationOffsets.data(), src + offset, arg.Size * sizeof(unsigned));
  offset += arg.Size * sizeof(unsigned);
}

void Decode(char* src, unsigned& offset, D3D12_WRITEBUFFERIMMEDIATE_PARAMETERs_Argument& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }

  memcpy(&arg.Size, src + offset, sizeof(arg.Size));
  offset += sizeof(arg.Size);

  arg.Value = reinterpret_cast<D3D12_WRITEBUFFERIMMEDIATE_PARAMETER*>(src + offset);
  offset += arg.Size * sizeof(D3D12_WRITEBUFFERIMMEDIATE_PARAMETER);

  arg.DestKeys.resize(arg.Size);
  memcpy(arg.DestKeys.data(), src + offset, arg.Size * sizeof(unsigned));
  offset += arg.Size * sizeof(unsigned);

  arg.DestOffsets.resize(arg.Size);
  memcpy(arg.DestOffsets.data(), src + offset, arg.Size * sizeof(unsigned));
  offset += arg.Size * sizeof(unsigned);
}

void Decode(char* src, unsigned& offset, D3D12_STATE_OBJECT_DESC_Argument& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.Value = reinterpret_cast<D3D12_STATE_OBJECT_DESC*>(src + offset);
  offset += sizeof(D3D12_STATE_OBJECT_DESC);

  arg.Value->pSubobjects = reinterpret_cast<D3D12_STATE_SUBOBJECT*>(src + offset);
  offset += sizeof(D3D12_STATE_SUBOBJECT) * arg.Value->NumSubobjects;

  std::map<const D3D12_STATE_SUBOBJECT*, unsigned> subobjectIndexes;

  for (unsigned index = 0; index < arg.Value->NumSubobjects; ++index) {
    D3D12_STATE_SUBOBJECT& subobject =
        const_cast<D3D12_STATE_SUBOBJECT&>(arg.Value->pSubobjects[index]);

    subobjectIndexes[&subobject] = index;

    switch (subobject.Type) {
    case D3D12_STATE_SUBOBJECT_TYPE_STATE_OBJECT_CONFIG: {
      subobject.pDesc = src + offset;
      offset += sizeof(D3D12_STATE_OBJECT_CONFIG);
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE: {
      subobject.pDesc = src + offset;
      offset += sizeof(D3D12_GLOBAL_ROOT_SIGNATURE);
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE: {
      subobject.pDesc = src + offset;
      offset += sizeof(D3D12_LOCAL_ROOT_SIGNATURE);
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_NODE_MASK: {
      subobject.pDesc = src + offset;
      offset += sizeof(D3D12_NODE_MASK);
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY: {
      subobject.pDesc = src + offset;
      offset += sizeof(D3D12_DXIL_LIBRARY_DESC);

      D3D12_DXIL_LIBRARY_DESC* desc =
          static_cast<D3D12_DXIL_LIBRARY_DESC*>(const_cast<void*>(subobject.pDesc));

      desc->DXILLibrary.pShaderBytecode = src + offset;
      offset += desc->DXILLibrary.BytecodeLength;

      desc->pExports = reinterpret_cast<D3D12_EXPORT_DESC*>(src + offset);
      offset += sizeof(D3D12_EXPORT_DESC) * desc->NumExports;

      for (unsigned i = 0; i < desc->NumExports; ++i) {
        unsigned* len = reinterpret_cast<unsigned*>(src + offset);
        offset += sizeof(unsigned);
        const_cast<D3D12_EXPORT_DESC*>(desc->pExports)[i].Name =
            reinterpret_cast<LPCWSTR>(src + offset);
        offset += *len;

        if (desc->pExports[i].ExportToRename) {
          unsigned* len = reinterpret_cast<unsigned*>(src + offset);
          offset += sizeof(unsigned);
          const_cast<D3D12_EXPORT_DESC*>(desc->pExports)[i].ExportToRename =
              reinterpret_cast<LPCWSTR>(src + offset);
          offset += *len;
        }
      }
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION: {
      subobject.pDesc = src + offset;
      offset += sizeof(D3D12_EXISTING_COLLECTION_DESC);

      D3D12_EXISTING_COLLECTION_DESC* desc =
          static_cast<D3D12_EXISTING_COLLECTION_DESC*>(const_cast<void*>(subobject.pDesc));

      desc->pExports = reinterpret_cast<D3D12_EXPORT_DESC*>(src + offset);
      offset += sizeof(D3D12_EXPORT_DESC) * desc->NumExports;

      for (unsigned i = 0; i < desc->NumExports; ++i) {
        unsigned* len = reinterpret_cast<unsigned*>(src + offset);
        offset += sizeof(unsigned);
        const_cast<D3D12_EXPORT_DESC*>(desc->pExports)[i].Name =
            reinterpret_cast<LPCWSTR>(src + offset);
        offset += *len;

        if (desc->pExports[i].ExportToRename) {
          unsigned* len = reinterpret_cast<unsigned*>(src + offset);
          offset += sizeof(unsigned);
          const_cast<D3D12_EXPORT_DESC*>(desc->pExports)[i].ExportToRename =
              reinterpret_cast<LPCWSTR>(src + offset);
          offset += *len;
        }
      }
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION: {
      subobject.pDesc = src + offset;
      offset += sizeof(D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION);

      D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION* desc =
          static_cast<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(const_cast<void*>(subobject.pDesc));

      desc->pExports = reinterpret_cast<LPCWSTR*>(src + offset);
      offset += sizeof(LPCWSTR) * desc->NumExports;

      for (unsigned i = 0; i < desc->NumExports; ++i) {
        unsigned* len = reinterpret_cast<unsigned*>(src + offset);
        offset += sizeof(unsigned);
        const_cast<LPCWSTR*>(desc->pExports)[i] = reinterpret_cast<LPCWSTR>(src + offset);
        offset += *len;
      }
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION: {
      subobject.pDesc = src + offset;
      offset += sizeof(D3D12_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION);

      D3D12_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION* desc =
          static_cast<D3D12_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(
              const_cast<void*>(subobject.pDesc));

      unsigned* len = reinterpret_cast<unsigned*>(src + offset);
      offset += sizeof(unsigned);
      desc->SubobjectToAssociate = reinterpret_cast<LPCWSTR>(src + offset);
      offset += *len;

      desc->pExports = reinterpret_cast<LPCWSTR*>(src + offset);
      offset += sizeof(LPCWSTR) * desc->NumExports;

      for (unsigned i = 0; i < desc->NumExports; ++i) {
        unsigned* len = reinterpret_cast<unsigned*>(src + offset);
        offset += sizeof(unsigned);
        const_cast<LPCWSTR*>(desc->pExports)[i] = reinterpret_cast<LPCWSTR>(src + offset);
        offset += *len;
      }
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG: {
      subobject.pDesc = src + offset;
      offset += sizeof(D3D12_RAYTRACING_SHADER_CONFIG);
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG: {
      subobject.pDesc = src + offset;
      offset += sizeof(D3D12_RAYTRACING_PIPELINE_CONFIG);
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP: {
      subobject.pDesc = src + offset;
      offset += sizeof(D3D12_HIT_GROUP_DESC);

      D3D12_HIT_GROUP_DESC* desc =
          static_cast<D3D12_HIT_GROUP_DESC*>(const_cast<void*>(subobject.pDesc));

      unsigned* len = reinterpret_cast<unsigned*>(src + offset);
      offset += sizeof(unsigned);
      desc->HitGroupExport = reinterpret_cast<LPCWSTR>(src + offset);
      offset += *len;

      if (desc->AnyHitShaderImport) {
        unsigned* len = reinterpret_cast<unsigned*>(src + offset);
        offset += sizeof(unsigned);
        desc->AnyHitShaderImport = reinterpret_cast<LPCWSTR>(src + offset);
        offset += *len;
      }
      if (desc->ClosestHitShaderImport) {
        unsigned* len = reinterpret_cast<unsigned*>(src + offset);
        offset += sizeof(unsigned);
        desc->ClosestHitShaderImport = reinterpret_cast<LPCWSTR>(src + offset);
        offset += *len;
      }
      if (desc->IntersectionShaderImport) {
        unsigned* len = reinterpret_cast<unsigned*>(src + offset);
        offset += sizeof(unsigned);
        desc->IntersectionShaderImport = reinterpret_cast<LPCWSTR>(src + offset);
        offset += *len;
      }
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG1: {
      subobject.pDesc = src + offset;
      offset += sizeof(D3D12_RAYTRACING_PIPELINE_CONFIG1);
    } break;
    }
  }

  {
    unsigned* size = reinterpret_cast<unsigned*>(src + offset);
    offset += sizeof(unsigned);
    for (unsigned i = 0; i < *size; ++i) {
      unsigned* indexAssociation = reinterpret_cast<unsigned*>(src + offset);
      offset += sizeof(unsigned);
      unsigned* indexAssociated = reinterpret_cast<unsigned*>(src + offset);
      offset += sizeof(unsigned);

      D3D12_STATE_SUBOBJECT& subobject =
          const_cast<D3D12_STATE_SUBOBJECT&>(arg.Value->pSubobjects[*indexAssociation]);
      GITS_ASSERT(subobject.Type == D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION);
      D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION* desc =
          static_cast<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(const_cast<void*>(subobject.pDesc));
      desc->pSubobjectToAssociate = &arg.Value->pSubobjects[*indexAssociated];
    }
  }
  {
    unsigned* size = reinterpret_cast<unsigned*>(src + offset);
    offset += sizeof(unsigned);
    for (unsigned i = 0; i < *size; ++i) {
      unsigned* index = reinterpret_cast<unsigned*>(src + offset);
      offset += sizeof(unsigned);
      unsigned* key = reinterpret_cast<unsigned*>(src + offset);
      offset += sizeof(unsigned);

      arg.InterfaceKeysBySubobject[*index] = *key;
    }
  }
}

void Decode(char* src, unsigned& offset, D3D12_PIPELINE_STATE_STREAM_DESC_Argument& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.Value = reinterpret_cast<D3D12_PIPELINE_STATE_STREAM_DESC*>(src + offset);
  offset += sizeof(D3D12_PIPELINE_STATE_STREAM_DESC);

  arg.Value->pPipelineStateSubobjectStream = src + offset;
  offset += arg.Value->SizeInBytes;

  size_t stateOffset = 0;
  while (stateOffset < arg.Value->SizeInBytes) {
    void* subobjectData =
        static_cast<char*>(arg.Value->pPipelineStateSubobjectStream) + stateOffset;
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE subobjectType =
        *reinterpret_cast<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE*>(subobjectData);

    switch (subobjectType) {
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE:
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS: {
      D3D12_SHADER_BYTECODE* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_VS*>(subobjectData);
      if (subobject->pShaderBytecode) {
        subobject->pShaderBytecode = src + offset;
        offset += subobject->BytecodeLength;
      }
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_VS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS: {
      D3D12_SHADER_BYTECODE* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_PS*>(subobjectData);
      if (subobject->pShaderBytecode) {
        subobject->pShaderBytecode = src + offset;
        offset += subobject->BytecodeLength;
      }
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_PS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DS: {
      D3D12_SHADER_BYTECODE* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_DS*>(subobjectData);
      if (subobject->pShaderBytecode) {
        subobject->pShaderBytecode = src + offset;
        offset += subobject->BytecodeLength;
      }
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_DS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_HS: {
      D3D12_SHADER_BYTECODE* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_HS*>(subobjectData);
      if (subobject->pShaderBytecode) {
        subobject->pShaderBytecode = src + offset;
        offset += subobject->BytecodeLength;
      }
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_HS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_GS: {
      D3D12_SHADER_BYTECODE* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_GS*>(subobjectData);
      if (subobject->pShaderBytecode) {
        subobject->pShaderBytecode = src + offset;
        offset += subobject->BytecodeLength;
      }
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_GS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS: {
      D3D12_SHADER_BYTECODE* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_CS*>(subobjectData);
      if (subobject->pShaderBytecode) {
        subobject->pShaderBytecode = src + offset;
        offset += subobject->BytecodeLength;
      }
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_CS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS: {
      D3D12_SHADER_BYTECODE* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_AS*>(subobjectData);
      if (subobject->pShaderBytecode) {
        subobject->pShaderBytecode = src + offset;
        offset += subobject->BytecodeLength;
      }
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_AS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS: {
      D3D12_SHADER_BYTECODE* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_MS*>(subobjectData);
      if (subobject->pShaderBytecode) {
        subobject->pShaderBytecode = src + offset;
        offset += subobject->BytecodeLength;
      }
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_MS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_STREAM_OUTPUT: {
      D3D12_STREAM_OUTPUT_DESC* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_STREAM_OUTPUT*>(subobjectData);
      if (subobject->pSODeclaration) {
        subobject->pSODeclaration = reinterpret_cast<D3D12_SO_DECLARATION_ENTRY*>(src + offset);
        offset += subobject->NumEntries * sizeof(D3D12_SO_DECLARATION_ENTRY);

        for (unsigned i = 0; i < subobject->NumEntries; ++i) {
          unsigned* len = reinterpret_cast<unsigned*>(src + offset);
          offset += sizeof(unsigned);
          const_cast<D3D12_SO_DECLARATION_ENTRY*>(subobject->pSODeclaration)[i].SemanticName =
              reinterpret_cast<LPCSTR>(src + offset);
          offset += *len;
        }
      }
      if (subobject->pBufferStrides) {
        subobject->pBufferStrides = reinterpret_cast<UINT*>(src + offset);
        offset += subobject->NumStrides * sizeof(UINT);
      }
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_STREAM_OUTPUT);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND:
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_MASK:
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_MASK);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_NODE_MASK:
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_NODE_MASK);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER:
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER1:
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER1);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER2:
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER2);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL:
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL1:
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL1);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL2:
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL2);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT: {
      D3D12_INPUT_LAYOUT_DESC* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT*>(subobjectData);
      if (subobject->pInputElementDescs) {
        subobject->pInputElementDescs = reinterpret_cast<D3D12_INPUT_ELEMENT_DESC*>(src + offset);
        offset += subobject->NumElements * sizeof(D3D12_INPUT_ELEMENT_DESC);

        for (unsigned i = 0; i < subobject->NumElements; ++i) {
          unsigned* len = reinterpret_cast<unsigned*>(src + offset);
          offset += sizeof(unsigned);
          const_cast<D3D12_INPUT_ELEMENT_DESC*>(subobject->pInputElementDescs)[i].SemanticName =
              reinterpret_cast<LPCSTR>(src + offset);
          offset += *len;
        }
      }
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_IB_STRIP_CUT_VALUE:
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_IB_STRIP_CUT_VALUE);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY:
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS:
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT:
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC:
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CACHED_PSO: {
      D3D12_CACHED_PIPELINE_STATE* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_CACHED_PSO*>(subobjectData);
      if (subobject->pCachedBlob) {
        // Cached PSO blobs are not compatible accross different HW or SW
        subobject->pCachedBlob = nullptr;
        offset += subobject->CachedBlobSizeInBytes;
      }
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_CACHED_PSO);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_FLAGS:
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_FLAGS);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VIEW_INSTANCING: {
      D3D12_VIEW_INSTANCING_DESC* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_VIEW_INSTANCING*>(subobjectData);
      if (subobject->pViewInstanceLocations) {
        subobject->pViewInstanceLocations =
            reinterpret_cast<D3D12_VIEW_INSTANCE_LOCATION*>(src + offset);
        offset += subobject->ViewInstanceCount * sizeof(D3D12_VIEW_INSTANCE_LOCATION);
      }
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_VIEW_INSTANCING);
    } break;
    default:
      GITS_ASSERT(0 && "Unexpected subobject type");
      break;
    }
  }

  memcpy(&arg.RootSignatureKey, src + offset, sizeof(arg.RootSignatureKey));
  offset += sizeof(arg.RootSignatureKey);
}

void Decode(char* src, unsigned& offset, D3D12_BARRIER_GROUPs_Argument& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }

  memcpy(&arg.Size, src + offset, sizeof(arg.Size));
  offset += sizeof(arg.Size);

  arg.Value = reinterpret_cast<D3D12_BARRIER_GROUP*>(src + offset);
  offset += arg.Size * sizeof(D3D12_BARRIER_GROUP);

  unsigned numResourceKeys{};

  for (unsigned i = 0; i < arg.Size; ++i) {
    if (arg.Value[i].Type == D3D12_BARRIER_TYPE_GLOBAL) {
      arg.Value[i].pGlobalBarriers = reinterpret_cast<D3D12_GLOBAL_BARRIER*>(src + offset);
      offset += sizeof(D3D12_GLOBAL_BARRIER) * arg.Value[i].NumBarriers;
    } else if (arg.Value[i].Type == D3D12_BARRIER_TYPE_TEXTURE) {
      arg.Value[i].pTextureBarriers = reinterpret_cast<D3D12_TEXTURE_BARRIER*>(src + offset);
      offset += sizeof(D3D12_TEXTURE_BARRIER) * arg.Value[i].NumBarriers;
      numResourceKeys += arg.Value[i].NumBarriers;
    } else if (arg.Value[i].Type == D3D12_BARRIER_TYPE_BUFFER) {
      arg.Value[i].pBufferBarriers = reinterpret_cast<D3D12_BUFFER_BARRIER*>(src + offset);
      offset += sizeof(D3D12_BUFFER_BARRIER) * arg.Value[i].NumBarriers;
      numResourceKeys += arg.Value[i].NumBarriers;
    }
  }

  arg.ResourceKeys.resize(numResourceKeys);
  memcpy(arg.ResourceKeys.data(), src + offset, numResourceKeys * sizeof(unsigned));
  offset += numResourceKeys * sizeof(unsigned);
}

void Decode(char* src, unsigned& offset, DML_BINDING_DESC_Argument& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.Value = reinterpret_cast<DML_BINDING_DESC*>(src + offset);
  dml::Decode(arg.Value, 1, src, offset);

  memcpy(&arg.ResourceKeysSize, src + offset, sizeof(arg.ResourceKeysSize));
  offset += sizeof(arg.ResourceKeysSize);

  arg.ResourceKeys.resize(arg.ResourceKeysSize);
  memcpy(arg.ResourceKeys.data(), src + offset, arg.ResourceKeysSize * sizeof(unsigned));
  offset += arg.ResourceKeysSize * sizeof(unsigned);
}

void Decode(char* src, unsigned& offset, DML_BINDING_DESCs_Argument& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }

  memcpy(&arg.Size, src + offset, sizeof(arg.Size));
  offset += sizeof(arg.Size);

  arg.Value = reinterpret_cast<DML_BINDING_DESC*>(src + offset);
  dml::Decode(arg.Value, arg.Size, src, offset);

  memcpy(&arg.ResourceKeysSize, src + offset, sizeof(arg.ResourceKeysSize));
  offset += sizeof(arg.ResourceKeysSize);

  arg.ResourceKeys.resize(arg.ResourceKeysSize);
  memcpy(arg.ResourceKeys.data(), src + offset, arg.ResourceKeysSize * sizeof(unsigned));
  offset += arg.ResourceKeysSize * sizeof(unsigned);
}

void Decode(char* src, unsigned& offset, DML_BINDING_TABLE_DESC_Argument& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.Value = reinterpret_cast<DML_BINDING_TABLE_DESC*>(src + offset);
  offset += sizeof(DML_BINDING_TABLE_DESC);

  memcpy(&arg.TableFields, src + offset, sizeof(arg.TableFields));
  offset += sizeof(arg.TableFields);
}

void Decode(char* src, unsigned& offset, DML_OPERATOR_DESC_Argument& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }
  arg.Value = reinterpret_cast<DML_OPERATOR_DESC*>(src + offset);
  dml::Decode(arg.Value, 1, src, offset);
}

void Decode(char* src, unsigned& offset, DML_GRAPH_DESC_Argument& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }
  arg.Value = reinterpret_cast<DML_GRAPH_DESC*>(src + offset);
  dml::Decode(arg.Value, 1, src, offset);

  memcpy(&arg.OperatorKeysSize, src + offset, sizeof(unsigned));
  offset += sizeof(unsigned);

  arg.OperatorKeys.resize(arg.OperatorKeysSize);
  memcpy(arg.OperatorKeys.data(), src + offset, arg.OperatorKeysSize * sizeof(unsigned));
  offset += arg.OperatorKeysSize * sizeof(unsigned);
}

void Decode(char* src, unsigned& offset, DML_CheckFeatureSupport_BufferArgument& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }
  Decode(src, offset, arg.Size);
  Decode(src, offset, arg.feature);

  arg.Value = const_cast<char*>(src + offset);
  if (arg.feature == DML_FEATURE_FEATURE_LEVELS) {
    auto* featlevels = reinterpret_cast<DML_FEATURE_QUERY_FEATURE_LEVELS*>(arg.Value);
    offset += sizeof(DML_FEATURE_QUERY_FEATURE_LEVELS);
    featlevels->RequestedFeatureLevels = reinterpret_cast<const DML_FEATURE_LEVEL*>(src + offset);
    offset += featlevels->RequestedFeatureLevelCount * sizeof(DML_FEATURE_LEVEL);
  } else {
    offset += arg.Size;
  }
}

void Decode(char* src,
            unsigned& offset,
            PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS>& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.Value = reinterpret_cast<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS*>(src + offset);
  offset += sizeof(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS);

  if (arg.Value->Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
    if (arg.Value->DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY) {
      if (arg.Value->pGeometryDescs) {
        arg.Value->pGeometryDescs = reinterpret_cast<D3D12_RAYTRACING_GEOMETRY_DESC*>(src + offset);
        offset += sizeof(D3D12_RAYTRACING_GEOMETRY_DESC) * arg.Value->NumDescs;
        for (unsigned i = 0; i < arg.Value->NumDescs; ++i) {
          if (arg.Value->pGeometryDescs[i].Type == D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES) {
            auto& desc = const_cast<D3D12_RAYTRACING_GEOMETRY_DESC*>(arg.Value->pGeometryDescs)[i];
            if (desc.OmmTriangles.pTriangles) {
              desc.OmmTriangles.pTriangles =
                  reinterpret_cast<D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC*>(src + offset);
              offset += sizeof(D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC);
            }
            if (desc.OmmTriangles.pOmmLinkage) {
              desc.OmmTriangles.pOmmLinkage =
                  reinterpret_cast<D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC*>(src + offset);
              offset += sizeof(D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC);
            }
          }
        }
      }
    } else if (arg.Value->DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS) {
      if (arg.Value->ppGeometryDescs) {
        arg.Value->ppGeometryDescs =
            reinterpret_cast<D3D12_RAYTRACING_GEOMETRY_DESC**>(src + offset);
        offset += sizeof(D3D12_RAYTRACING_GEOMETRY_DESC*) * arg.Value->NumDescs;
      }
      for (unsigned i = 0; i < arg.Value->NumDescs; ++i) {
        const_cast<D3D12_RAYTRACING_GEOMETRY_DESC**>(arg.Value->ppGeometryDescs)[i] =
            reinterpret_cast<D3D12_RAYTRACING_GEOMETRY_DESC*>(src + offset);
        offset += sizeof(D3D12_RAYTRACING_GEOMETRY_DESC);
        if (arg.Value->ppGeometryDescs[i]->Type == D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES) {
          auto desc = const_cast<D3D12_RAYTRACING_GEOMETRY_DESC**>(arg.Value->ppGeometryDescs)[i];
          if (desc->OmmTriangles.pTriangles) {
            desc->OmmTriangles.pTriangles =
                reinterpret_cast<D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC*>(src + offset);
            offset += sizeof(D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC);
          }
          if (desc->OmmTriangles.pOmmLinkage) {
            desc->OmmTriangles.pOmmLinkage =
                reinterpret_cast<D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC*>(src + offset);
            offset += sizeof(D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC);
          }
        }
      }
    }
  } else if (arg.Value->Type ==
             D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_OPACITY_MICROMAP_ARRAY) {
    if (arg.Value->pOpacityMicromapArrayDesc) {
      arg.Value->pOpacityMicromapArrayDesc =
          reinterpret_cast<D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC*>(src + offset);
      offset += sizeof(D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC);
      if (arg.Value->pOpacityMicromapArrayDesc->pOmmHistogram) {
        auto desc = const_cast<D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC*>(
            arg.Value->pOpacityMicromapArrayDesc);
        desc->pOmmHistogram =
            reinterpret_cast<D3D12_RAYTRACING_OPACITY_MICROMAP_HISTOGRAM_ENTRY*>(src + offset);
        offset += sizeof(D3D12_RAYTRACING_OPACITY_MICROMAP_HISTOGRAM_ENTRY) *
                  desc->NumOmmHistogramEntries;
      }
    }
  }

  unsigned size{};
  memcpy(&size, src + offset, sizeof(size));
  offset += sizeof(size);

  arg.InputKeys.resize(size);
  memcpy(arg.InputKeys.data(), src + offset, size * sizeof(unsigned));
  offset += size * sizeof(unsigned);

  arg.InputOffsets.resize(size);
  memcpy(arg.InputOffsets.data(), src + offset, size * sizeof(unsigned));
  offset += size * sizeof(unsigned);
}

void Decode(char* src,
            unsigned& offset,
            PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC>& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.Value = reinterpret_cast<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC*>(src + offset);
  offset += sizeof(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC);

  if (arg.Value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
    if (arg.Value->Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY) {
      if (arg.Value->Inputs.pGeometryDescs) {
        arg.Value->Inputs.pGeometryDescs =
            reinterpret_cast<D3D12_RAYTRACING_GEOMETRY_DESC*>(src + offset);
        offset += sizeof(D3D12_RAYTRACING_GEOMETRY_DESC) * arg.Value->Inputs.NumDescs;
        for (unsigned i = 0; i < arg.Value->Inputs.NumDescs; ++i) {
          if (arg.Value->Inputs.pGeometryDescs[i].Type ==
              D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES) {
            auto& desc =
                const_cast<D3D12_RAYTRACING_GEOMETRY_DESC*>(arg.Value->Inputs.pGeometryDescs)[i];
            if (desc.OmmTriangles.pTriangles) {
              desc.OmmTriangles.pTriangles =
                  reinterpret_cast<D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC*>(src + offset);
              offset += sizeof(D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC);
            }
            if (desc.OmmTriangles.pOmmLinkage) {
              desc.OmmTriangles.pOmmLinkage =
                  reinterpret_cast<D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC*>(src + offset);
              offset += sizeof(D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC);
            }
          }
        }
      }
    } else if (arg.Value->Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS) {
      if (arg.Value->Inputs.ppGeometryDescs) {
        arg.Value->Inputs.ppGeometryDescs =
            reinterpret_cast<D3D12_RAYTRACING_GEOMETRY_DESC**>(src + offset);
        offset += sizeof(D3D12_RAYTRACING_GEOMETRY_DESC*) * arg.Value->Inputs.NumDescs;
      }
      for (unsigned i = 0; i < arg.Value->Inputs.NumDescs; ++i) {
        const_cast<D3D12_RAYTRACING_GEOMETRY_DESC**>(arg.Value->Inputs.ppGeometryDescs)[i] =
            reinterpret_cast<D3D12_RAYTRACING_GEOMETRY_DESC*>(src + offset);
        offset += sizeof(D3D12_RAYTRACING_GEOMETRY_DESC);
        if (arg.Value->Inputs.ppGeometryDescs[i]->Type ==
            D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES) {
          auto desc =
              const_cast<D3D12_RAYTRACING_GEOMETRY_DESC**>(arg.Value->Inputs.ppGeometryDescs)[i];
          if (desc->OmmTriangles.pTriangles) {
            desc->OmmTriangles.pTriangles =
                reinterpret_cast<D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC*>(src + offset);
            offset += sizeof(D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC);
          }
          if (desc->OmmTriangles.pOmmLinkage) {
            desc->OmmTriangles.pOmmLinkage =
                reinterpret_cast<D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC*>(src + offset);
            offset += sizeof(D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC);
          }
        }
      }
    }
  } else if (arg.Value->Inputs.Type ==
             D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_OPACITY_MICROMAP_ARRAY) {
    if (arg.Value->Inputs.pOpacityMicromapArrayDesc) {
      arg.Value->Inputs.pOpacityMicromapArrayDesc =
          reinterpret_cast<D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC*>(src + offset);
      offset += sizeof(D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC);
      if (arg.Value->Inputs.pOpacityMicromapArrayDesc->pOmmHistogram) {
        auto desc = const_cast<D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC*>(
            arg.Value->Inputs.pOpacityMicromapArrayDesc);
        desc->pOmmHistogram =
            reinterpret_cast<D3D12_RAYTRACING_OPACITY_MICROMAP_HISTOGRAM_ENTRY*>(src + offset);
        offset += sizeof(D3D12_RAYTRACING_OPACITY_MICROMAP_HISTOGRAM_ENTRY) *
                  desc->NumOmmHistogramEntries;
      }
    }
  }

  memcpy(&arg.DestAccelerationStructureKey, src + offset, sizeof(arg.DestAccelerationStructureKey));
  offset += sizeof(arg.DestAccelerationStructureKey);
  memcpy(&arg.DestAccelerationStructureOffset, src + offset,
         sizeof(arg.DestAccelerationStructureOffset));
  offset += sizeof(arg.DestAccelerationStructureOffset);

  memcpy(&arg.SourceAccelerationStructureKey, src + offset,
         sizeof(arg.SourceAccelerationStructureKey));
  offset += sizeof(arg.SourceAccelerationStructureKey);
  memcpy(&arg.SourceAccelerationStructureOffset, src + offset,
         sizeof(arg.SourceAccelerationStructureOffset));
  offset += sizeof(arg.SourceAccelerationStructureOffset);

  memcpy(&arg.ScratchAccelerationStructureKey, src + offset,
         sizeof(arg.ScratchAccelerationStructureKey));
  offset += sizeof(arg.ScratchAccelerationStructureKey);
  memcpy(&arg.ScratchAccelerationStructureOffset, src + offset,
         sizeof(arg.ScratchAccelerationStructureOffset));
  offset += sizeof(arg.ScratchAccelerationStructureOffset);

  unsigned size{};
  memcpy(&size, src + offset, sizeof(size));
  offset += sizeof(size);

  arg.InputKeys.resize(size);
  memcpy(arg.InputKeys.data(), src + offset, size * sizeof(unsigned));
  offset += size * sizeof(unsigned);

  arg.InputOffsets.resize(size);
  memcpy(arg.InputOffsets.data(), src + offset, size * sizeof(unsigned));
  offset += size * sizeof(unsigned);
}

void Decode(char* src, unsigned& offset, PointerArgument<D3D12_DISPATCH_RAYS_DESC>& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.Value = reinterpret_cast<D3D12_DISPATCH_RAYS_DESC*>(src + offset);
  offset += sizeof(D3D12_DISPATCH_RAYS_DESC);

  memcpy(&arg.RayGenerationShaderRecordKey, src + offset, sizeof(arg.RayGenerationShaderRecordKey));
  offset += sizeof(arg.RayGenerationShaderRecordKey);
  memcpy(&arg.RayGenerationShaderRecordOffset, src + offset,
         sizeof(arg.RayGenerationShaderRecordOffset));
  offset += sizeof(arg.RayGenerationShaderRecordOffset);

  memcpy(&arg.MissShaderTableKey, src + offset, sizeof(arg.MissShaderTableKey));
  offset += sizeof(arg.MissShaderTableKey);
  memcpy(&arg.MissShaderTableOffset, src + offset, sizeof(arg.MissShaderTableOffset));
  offset += sizeof(arg.MissShaderTableOffset);

  memcpy(&arg.HitGroupTableKey, src + offset, sizeof(arg.HitGroupTableKey));
  offset += sizeof(arg.HitGroupTableKey);
  memcpy(&arg.HitGroupTableOffset, src + offset, sizeof(arg.HitGroupTableOffset));
  offset += sizeof(arg.HitGroupTableOffset);

  memcpy(&arg.CallableShaderTableKey, src + offset, sizeof(arg.CallableShaderTableKey));
  offset += sizeof(arg.CallableShaderTableKey);
  memcpy(&arg.CallableShaderTableOffset, src + offset, sizeof(arg.CallableShaderTableOffset));
  offset += sizeof(arg.CallableShaderTableOffset);
}

void Decode(char* src,
            unsigned& offset,
            ArrayArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }

  memcpy(&arg.Size, src + offset, sizeof(arg.Size));
  offset += sizeof(arg.Size);

  arg.Value =
      reinterpret_cast<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC*>(src + offset);
  offset += sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC) * arg.Size;

  arg.DestBufferKeys.resize(arg.Size);
  memcpy(arg.DestBufferKeys.data(), src + offset, arg.Size * sizeof(unsigned));
  offset += arg.Size * sizeof(unsigned);

  arg.DestBufferOffsets.resize(arg.Size);
  memcpy(arg.DestBufferOffsets.data(), src + offset, arg.Size * sizeof(unsigned));
  offset += arg.Size * sizeof(unsigned);
}

void Decode(char* src,
            unsigned& offset,
            PointerArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.Value =
      reinterpret_cast<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC*>(src + offset);
  offset += sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC);

  memcpy(&arg.destBufferKey, src + offset, sizeof(unsigned));
  offset += sizeof(unsigned);

  memcpy(&arg.destBufferOffset, src + offset, sizeof(unsigned));
  offset += sizeof(unsigned);
}

void Decode(char* src, unsigned& offset, D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }

  memcpy(&arg.Size, src + offset, sizeof(arg.Size));
  offset += sizeof(arg.Size);

  arg.Value = reinterpret_cast<D3D12_RENDER_PASS_RENDER_TARGET_DESC*>(src + offset);
  offset += sizeof(D3D12_RENDER_PASS_RENDER_TARGET_DESC) * arg.Size;

  for (unsigned i = 0; i < arg.Size; ++i) {
    if (arg.Value[i].EndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE) {
      arg.Value[i].EndingAccess.Resolve.pSubresourceParameters =
          reinterpret_cast<D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS*>(src +
                                                                                            offset);
      offset += sizeof(D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS) *
                arg.Value[i].EndingAccess.Resolve.SubresourceCount;
    }
  }

  arg.DescriptorKeys.resize(arg.Size);
  memcpy(arg.DescriptorKeys.data(), src + offset, arg.Size * sizeof(unsigned));
  offset += arg.Size * sizeof(unsigned);

  arg.DescriptorIndexes.resize(arg.Size);
  memcpy(arg.DescriptorIndexes.data(), src + offset, arg.Size * sizeof(unsigned));
  offset += arg.Size * sizeof(unsigned);

  unsigned size{};
  memcpy(&size, src + offset, sizeof(size));
  offset += sizeof(size);

  arg.ResolveSrcResourceKeys.resize(size);
  memcpy(arg.ResolveSrcResourceKeys.data(), src + offset, size * sizeof(unsigned));
  offset += size * sizeof(unsigned);

  arg.ResolveDstResourceKeys.resize(size);
  memcpy(arg.ResolveDstResourceKeys.data(), src + offset, size * sizeof(unsigned));
  offset += size * sizeof(unsigned);
}

void Decode(char* src, unsigned& offset, D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.Value = reinterpret_cast<D3D12_RENDER_PASS_DEPTH_STENCIL_DESC*>(src + offset);
  offset += sizeof(D3D12_RENDER_PASS_DEPTH_STENCIL_DESC);

  if (arg.Value->DepthEndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE) {
    arg.Value->DepthEndingAccess.Resolve.pSubresourceParameters =
        reinterpret_cast<D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS*>(src +
                                                                                          offset);
    offset += sizeof(D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS) *
              arg.Value->DepthEndingAccess.Resolve.SubresourceCount;
  }
  if (arg.Value->StencilEndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE) {
    arg.Value->StencilEndingAccess.Resolve.pSubresourceParameters =
        reinterpret_cast<D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS*>(src +
                                                                                          offset);
    offset += sizeof(D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS) *
              arg.Value->StencilEndingAccess.Resolve.SubresourceCount;
  }

  memcpy(&arg.DescriptorKey, src + offset, sizeof(unsigned));
  offset += sizeof(unsigned);
  memcpy(&arg.DescriptorIndex, src + offset, sizeof(unsigned));
  offset += sizeof(unsigned);

  memcpy(&arg.ResolveSrcDepthKey, src + offset, sizeof(unsigned));
  offset += sizeof(unsigned);
  memcpy(&arg.ResolveDstDepthKey, src + offset, sizeof(unsigned));
  offset += sizeof(unsigned);
  memcpy(&arg.ResolveSrcStencilKey, src + offset, sizeof(unsigned));
  offset += sizeof(unsigned);
  memcpy(&arg.ResolveDstStencilKey, src + offset, sizeof(unsigned));
  offset += sizeof(unsigned);
}

void Decode(char* src, unsigned& offset, D3D12_SHADER_RESOURCE_VIEW_DESC_Argument& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }
  arg.Value = reinterpret_cast<D3D12_SHADER_RESOURCE_VIEW_DESC*>(const_cast<char*>(src + offset));
  offset += sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC);

  if (arg.Value->ViewDimension == D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE) {
    memcpy(&arg.RaytracingLocationKey, src + offset, sizeof(unsigned));
    offset += sizeof(unsigned);
    memcpy(&arg.RaytracingLocationOffset, src + offset, sizeof(unsigned));
    offset += sizeof(unsigned);
  }
}

void Decode(char* src,
            unsigned& offset,
            ArrayArgument<D3D12_LINEAR_ALGEBRA_MATRIX_CONVERSION_INFO>& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }

  memcpy(&arg.Size, src + offset, sizeof(arg.Size));
  offset += sizeof(arg.Size);

  arg.Value = reinterpret_cast<D3D12_LINEAR_ALGEBRA_MATRIX_CONVERSION_INFO*>(src + offset);
  offset += arg.Size * sizeof(D3D12_LINEAR_ALGEBRA_MATRIX_CONVERSION_INFO);

  arg.DestKey.resize(arg.Size);
  memcpy(arg.DestKey.data(), src + offset, arg.Size * sizeof(unsigned));
  offset += arg.Size * sizeof(unsigned);
  arg.DestOffset.resize(arg.Size);
  memcpy(arg.DestOffset.data(), src + offset, arg.Size * sizeof(unsigned));
  offset += arg.Size * sizeof(unsigned);

  arg.SourceKey.resize(arg.Size);
  memcpy(arg.SourceKey.data(), src + offset, arg.Size * sizeof(unsigned));
  offset += arg.Size * sizeof(unsigned);
  arg.SourceOffset.resize(arg.Size);
  memcpy(arg.SourceOffset.data(), src + offset, arg.Size * sizeof(unsigned));
  offset += arg.Size * sizeof(unsigned);
}

// This Decode function does not have a GetSize/Encode counterpart
// Set pDeviceDriverDesc and pDeviceDriverVersion to 0 (the capture time strings are NOT encoded in the stream)
// Note: Encoding and decoding these values would cause compatibility issues with already existing streams
void Decode(char* src, unsigned& offset, PointerArgument<INTCExtensionInfo>& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.Value = reinterpret_cast<INTCExtensionInfo*>(src + offset);
  offset += sizeof(INTCExtensionInfo);

  // Always set pDeviceDriverDesc and pDeviceDriverVersion to 0
  arg.Value->pDeviceDriverDesc = 0;
  arg.Value->pDeviceDriverVersion = 0;
}

void Decode(char* src, unsigned& offset, PointerArgument<INTCExtensionAppInfo>& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.Value = reinterpret_cast<INTCExtensionAppInfo*>(src + offset);
  offset += sizeof(INTCExtensionAppInfo);

  if (arg.Value->pApplicationName) {
    unsigned* len = reinterpret_cast<unsigned*>(src + offset);
    offset += sizeof(unsigned);
    arg.Value->pApplicationName = reinterpret_cast<const wchar_t*>(src + offset);
    arg.ApplicationName = arg.Value->pApplicationName;
    offset += *len;
  }
  if (arg.Value->pEngineName) {
    unsigned* len = reinterpret_cast<unsigned*>(src + offset);
    offset += sizeof(unsigned);
    arg.Value->pEngineName = reinterpret_cast<const wchar_t*>(src + offset);
    arg.EngineName = arg.Value->pEngineName;
    offset += *len;
  }
}

void Decode(char* src, unsigned& offset, PointerArgument<INTCExtensionAppInfo1>& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.Value = reinterpret_cast<INTCExtensionAppInfo1*>(src + offset);
  offset += sizeof(INTCExtensionAppInfo1);

  if (arg.Value->pApplicationName) {
    unsigned* len = reinterpret_cast<unsigned*>(src + offset);
    offset += sizeof(unsigned);
    arg.Value->pApplicationName = reinterpret_cast<const wchar_t*>(src + offset);
    arg.ApplicationName = arg.Value->pApplicationName;
    offset += *len;
  }
  if (arg.Value->pEngineName) {
    unsigned* len = reinterpret_cast<unsigned*>(src + offset);
    offset += sizeof(unsigned);
    arg.Value->pEngineName = reinterpret_cast<const wchar_t*>(src + offset);
    arg.EngineName = arg.Value->pEngineName;
    offset += *len;
  }
}

void Decode(char* src,
            unsigned& offset,
            PointerArgument<INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC>& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.Value = reinterpret_cast<INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC*>(src + offset);
  offset += sizeof(INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC);

  arg.Value->pD3D12Desc = reinterpret_cast<D3D12_COMPUTE_PIPELINE_STATE_DESC*>(src + offset);
  offset += sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC);

  arg.Value->pD3D12Desc->CS.pShaderBytecode = nullptr;
  arg.Value->pD3D12Desc->CS.BytecodeLength = 0;

  if (arg.Value->CS.pShaderBytecode) {
    arg.Value->CS.pShaderBytecode = src + offset;
    offset += arg.Value->CS.BytecodeLength;
  }

  if (arg.Value->CompileOptions) {
    unsigned* len = reinterpret_cast<unsigned*>(src + offset);
    offset += sizeof(unsigned);
    arg.Value->CompileOptions = src + offset;
    offset += *len;
  }

  if (arg.Value->InternalOptions) {
    unsigned* len = reinterpret_cast<unsigned*>(src + offset);
    offset += sizeof(unsigned);
    arg.Value->InternalOptions = src + offset;
    offset += *len;
  }

  memcpy(&arg.RootSignatureKey, src + offset, sizeof(arg.RootSignatureKey));
  offset += sizeof(arg.RootSignatureKey);
}

void Decode(char* src, unsigned& offset, PointerArgument<INTC_D3D12_HEAP_DESC>& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.Value = reinterpret_cast<INTC_D3D12_HEAP_DESC*>(src + offset);
  offset += sizeof(INTC_D3D12_HEAP_DESC);

  arg.Value->pD3D12Desc = reinterpret_cast<D3D12_HEAP_DESC*>(src + offset);
  offset += sizeof(D3D12_HEAP_DESC);
}

void Decode(char* src, unsigned& offset, PointerArgument<INTC_D3D12_RESOURCE_DESC_0001>& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.Value = reinterpret_cast<INTC_D3D12_RESOURCE_DESC_0001*>(src + offset);
  offset += sizeof(INTC_D3D12_RESOURCE_DESC_0001);

  arg.Value->pD3D12Desc = reinterpret_cast<D3D12_RESOURCE_DESC*>(src + offset);
  offset += sizeof(D3D12_RESOURCE_DESC);
}

void Decode(char* src, unsigned& offset, xess_d3d12_init_params_t_Argument& arg) {
  arg.Value = reinterpret_cast<xess_d3d12_init_params_t*>(src + offset);
  offset += sizeof(xess_d3d12_init_params_t);

  memcpy(&arg.Key, src + offset, sizeof(arg.Key));
  offset += sizeof(arg.Key);

  memcpy(&arg.TempBufferHeapKey, src + offset, sizeof(arg.TempBufferHeapKey));
  offset += sizeof(arg.TempBufferHeapKey);

  memcpy(&arg.TempTextureHeapKey, src + offset, sizeof(arg.TempTextureHeapKey));
  offset += sizeof(arg.TempTextureHeapKey);

  memcpy(&arg.PipelineLibraryKey, src + offset, sizeof(arg.PipelineLibraryKey));
  offset += sizeof(arg.PipelineLibraryKey);
}

void Decode(char* src, unsigned& offset, xess_d3d12_execute_params_t_Argument& arg) {
  arg.Value = reinterpret_cast<xess_d3d12_execute_params_t*>(src + offset);
  offset += sizeof(xess_d3d12_execute_params_t);

  memcpy(&arg.ColorTextureKey, src + offset, sizeof(arg.ColorTextureKey));
  offset += sizeof(arg.ColorTextureKey);

  memcpy(&arg.VelocityTextureKey, src + offset, sizeof(arg.VelocityTextureKey));
  offset += sizeof(arg.VelocityTextureKey);

  memcpy(&arg.DepthTextureKey, src + offset, sizeof(arg.DepthTextureKey));
  offset += sizeof(arg.DepthTextureKey);

  memcpy(&arg.ExposureScaleTextureKey, src + offset, sizeof(arg.ExposureScaleTextureKey));
  offset += sizeof(arg.ExposureScaleTextureKey);

  memcpy(&arg.ResponsivePixelMaskTextureKey, src + offset,
         sizeof(arg.ResponsivePixelMaskTextureKey));
  offset += sizeof(arg.ResponsivePixelMaskTextureKey);

  memcpy(&arg.OutputTextureKey, src + offset, sizeof(arg.OutputTextureKey));
  offset += sizeof(arg.OutputTextureKey);

  memcpy(&arg.DescriptorHeapKey, src + offset, sizeof(arg.DescriptorHeapKey));
  offset += sizeof(arg.DescriptorHeapKey);
}

void Decode(char* src, unsigned& offset, DSTORAGE_QUEUE_DESC_Argument& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.Value = reinterpret_cast<DSTORAGE_QUEUE_DESC*>(src + offset);
  offset += sizeof(DSTORAGE_QUEUE_DESC);

  memcpy(&arg.DeviceKey, src + offset, sizeof(arg.DeviceKey));
  offset += sizeof(arg.DeviceKey);

  if (arg.Value->Name) {
    unsigned* len = reinterpret_cast<unsigned*>(src + offset);
    offset += sizeof(unsigned);
    arg.Value->Name = reinterpret_cast<const char*>(src + offset);
    offset += *len;
  }
}

void Decode(char* src, unsigned& offset, DSTORAGE_REQUEST_Argument& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.Value = reinterpret_cast<DSTORAGE_REQUEST*>(src + offset);
  offset += sizeof(DSTORAGE_REQUEST);

  memcpy(&arg.FileKey, src + offset, sizeof(arg.FileKey));
  offset += sizeof(arg.FileKey);

  memcpy(&arg.ResourceKey, src + offset, sizeof(arg.ResourceKey));
  offset += sizeof(arg.ResourceKey);

  memcpy(&arg.NewOffset, src + offset, sizeof(arg.NewOffset));
  offset += sizeof(arg.NewOffset);

  if (arg.Value->Name) {
    unsigned* len = reinterpret_cast<unsigned*>(src + offset);
    offset += sizeof(unsigned);
    arg.Value->Name = reinterpret_cast<const char*>(src + offset);
    offset += *len;
  }
}

void Decode(char* src,
            unsigned& offset,
            PointerArgument<NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS>& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.Value =
      reinterpret_cast<NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS*>(src + offset);
  offset += sizeof(NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS);

  if (arg.Value->pDesc) {
    arg.Value->pDesc =
        reinterpret_cast<NVAPI_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC_EX*>(src +
                                                                                       offset);
    offset += sizeof(NVAPI_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC_EX);

    NVAPI_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC_EX* pDescMod =
        const_cast<NVAPI_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC_EX*>(arg.Value->pDesc);

    if (arg.Value->pDesc->inputs.type ==
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
      if (arg.Value->pDesc->inputs.descsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY) {
        if (arg.Value->pDesc->inputs.pGeometryDescs) {
          pDescMod->inputs.pGeometryDescs =
              reinterpret_cast<const NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX*>(src + offset);
          offset += arg.Value->pDesc->inputs.numDescs *
                    arg.Value->pDesc->inputs.geometryDescStrideInBytes;

          for (unsigned i = 0; i < arg.Value->pDesc->inputs.numDescs; ++i) {
            NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX& desc = *(
                NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX*)((char*)(pDescMod->inputs.pGeometryDescs) +
                                                          pDescMod->inputs
                                                                  .geometryDescStrideInBytes *
                                                              i);
            if (desc.type == NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES_EX) {
              desc.ommTriangles.ommAttachment.pOMMUsageCounts =
                  reinterpret_cast<const NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_USAGE_COUNT*>(
                      src + offset);
              offset += sizeof(NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_USAGE_COUNT) *
                        desc.ommTriangles.ommAttachment.numOMMUsageCounts;
            } else if (desc.type == NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_DMM_TRIANGLES_EX) {
              desc.dmmTriangles.dmmAttachment.pDMMUsageCounts =
                  reinterpret_cast<const NVAPI_D3D12_RAYTRACING_DISPLACEMENT_MICROMAP_USAGE_COUNT*>(
                      src + offset);
              offset += sizeof(NVAPI_D3D12_RAYTRACING_DISPLACEMENT_MICROMAP_USAGE_COUNT) *
                        desc.dmmTriangles.dmmAttachment.numDMMUsageCounts;
            }
          }
        }
      } else if (arg.Value->pDesc->inputs.descsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS) {
        if (arg.Value->pDesc->inputs.ppGeometryDescs) {
          const_cast<NVAPI_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC_EX*>(arg.Value->pDesc)
              ->inputs.ppGeometryDescs =
              reinterpret_cast<const NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX**>(src + offset);
          offset +=
              sizeof(NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX*) * arg.Value->pDesc->inputs.numDescs;
        }
        for (unsigned i = 0; i < arg.Value->pDesc->inputs.numDescs; ++i) {
          const_cast<NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX**>(
              arg.Value->pDesc->inputs.ppGeometryDescs)[i] =
              reinterpret_cast<NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX*>(src + offset);
          offset += sizeof(NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX);

          if (arg.Value->pDesc->inputs.ppGeometryDescs[i]->type ==
              NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES_EX) {

            const_cast<NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX*>(
                pDescMod->inputs.ppGeometryDescs[i])
                ->ommTriangles.ommAttachment.pOMMUsageCounts =
                reinterpret_cast<const NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_USAGE_COUNT*>(
                    src + offset);
            offset += sizeof(NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_USAGE_COUNT) *
                      arg.Value->pDesc->inputs.ppGeometryDescs[i]
                          ->ommTriangles.ommAttachment.numOMMUsageCounts;
          } else if (arg.Value->pDesc->inputs.ppGeometryDescs[i]->type ==
                     NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_DMM_TRIANGLES_EX) {
            const_cast<NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX*>(
                pDescMod->inputs.ppGeometryDescs[i])
                ->dmmTriangles.dmmAttachment.pDMMUsageCounts =
                reinterpret_cast<const NVAPI_D3D12_RAYTRACING_DISPLACEMENT_MICROMAP_USAGE_COUNT*>(
                    src + offset);
            offset += sizeof(NVAPI_D3D12_RAYTRACING_DISPLACEMENT_MICROMAP_USAGE_COUNT) *
                      arg.Value->pDesc->inputs.ppGeometryDescs[i]
                          ->dmmTriangles.dmmAttachment.numDMMUsageCounts;
          }
        }
      }
    }
  }

  memcpy(&arg.DestAccelerationStructureKey, src + offset, sizeof(arg.DestAccelerationStructureKey));
  offset += sizeof(arg.DestAccelerationStructureKey);
  memcpy(&arg.DestAccelerationStructureOffset, src + offset,
         sizeof(arg.DestAccelerationStructureOffset));
  offset += sizeof(arg.DestAccelerationStructureOffset);

  memcpy(&arg.SourceAccelerationStructureKey, src + offset,
         sizeof(arg.SourceAccelerationStructureKey));
  offset += sizeof(arg.SourceAccelerationStructureKey);
  memcpy(&arg.SourceAccelerationStructureOffset, src + offset,
         sizeof(arg.SourceAccelerationStructureOffset));
  offset += sizeof(arg.SourceAccelerationStructureOffset);

  memcpy(&arg.ScratchAccelerationStructureKey, src + offset,
         sizeof(arg.ScratchAccelerationStructureKey));
  offset += sizeof(arg.ScratchAccelerationStructureKey);
  memcpy(&arg.ScratchAccelerationStructureOffset, src + offset,
         sizeof(arg.ScratchAccelerationStructureOffset));
  offset += sizeof(arg.ScratchAccelerationStructureOffset);

  unsigned size{};
  memcpy(&size, src + offset, sizeof(size));
  offset += sizeof(size);

  arg.InputKeys.resize(size);
  memcpy(arg.InputKeys.data(), src + offset, size * sizeof(unsigned));
  offset += size * sizeof(unsigned);

  arg.InputOffsets.resize(size);
  memcpy(arg.InputOffsets.data(), src + offset, size * sizeof(unsigned));
  offset += size * sizeof(unsigned);

  if (arg.Value->pPostbuildInfoDescs) {
    arg.Value->pPostbuildInfoDescs =
        reinterpret_cast<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC*>(src +
                                                                                       offset);
    offset += sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC) *
              arg.Value->numPostbuildInfoDescs;

    arg.DestPostBuildBufferKeys.resize(arg.Value->numPostbuildInfoDescs);
    memcpy(arg.DestPostBuildBufferKeys.data(), src + offset,
           sizeof(unsigned) * arg.Value->numPostbuildInfoDescs);
    offset += sizeof(unsigned) * arg.Value->numPostbuildInfoDescs;

    arg.DestPostBuildBufferOffsets.resize(arg.Value->numPostbuildInfoDescs);
    memcpy(arg.DestPostBuildBufferOffsets.data(), src + offset,
           sizeof(unsigned) * arg.Value->numPostbuildInfoDescs);
    offset += sizeof(unsigned) * arg.Value->numPostbuildInfoDescs;
  }
}
void Decode(char* src,
            unsigned& offset,
            PointerArgument<NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS>& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.Value = reinterpret_cast<NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS*>(src + offset);
  offset += sizeof(NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS);

  if (arg.Value->pDesc) {
    arg.Value->pDesc =
        reinterpret_cast<NVAPI_D3D12_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC*>(src + offset);
    offset += sizeof(NVAPI_D3D12_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC);

    if (arg.Value->pDesc->inputs.pOMMUsageCounts) {
      const_cast<NVAPI_D3D12_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC*>(arg.Value->pDesc)
          ->inputs.pOMMUsageCounts =
          reinterpret_cast<NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_USAGE_COUNT*>(src + offset);
      offset += sizeof(NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_USAGE_COUNT) *
                arg.Value->pDesc->inputs.numOMMUsageCounts;
    }
  }

  memcpy(&arg.DestOpacityMicromapArrayDataKey, src + offset,
         sizeof(arg.DestOpacityMicromapArrayDataKey));
  offset += sizeof(arg.DestOpacityMicromapArrayDataKey);
  memcpy(&arg.DestOpacityMicromapArrayDataOffset, src + offset,
         sizeof(arg.DestOpacityMicromapArrayDataOffset));
  offset += sizeof(arg.DestOpacityMicromapArrayDataOffset);

  memcpy(&arg.InputBufferKey, src + offset, sizeof(arg.InputBufferKey));
  offset += sizeof(arg.InputBufferKey);
  memcpy(&arg.InputBufferOffset, src + offset, sizeof(arg.InputBufferOffset));
  offset += sizeof(arg.InputBufferOffset);

  memcpy(&arg.PerOMMDescsKey, src + offset, sizeof(arg.PerOMMDescsKey));
  offset += sizeof(arg.PerOMMDescsKey);
  memcpy(&arg.PerOMMDescsOffset, src + offset, sizeof(arg.PerOMMDescsOffset));
  offset += sizeof(arg.PerOMMDescsOffset);

  memcpy(&arg.ScratchOpacityMicromapArrayDataKey, src + offset,
         sizeof(arg.ScratchOpacityMicromapArrayDataKey));
  offset += sizeof(arg.ScratchOpacityMicromapArrayDataKey);
  memcpy(&arg.ScratchOpacityMicromapArrayDataOffset, src + offset,
         sizeof(arg.ScratchOpacityMicromapArrayDataOffset));
  offset += sizeof(arg.ScratchOpacityMicromapArrayDataOffset);

  if (arg.Value->pPostbuildInfoDescs) {
    arg.Value->pPostbuildInfoDescs =
        reinterpret_cast<NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_POSTBUILD_INFO_DESC*>(
            src + offset);
    offset += sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC) *
              arg.Value->numPostbuildInfoDescs;

    arg.DestPostBuildBufferKeys.resize(arg.Value->numPostbuildInfoDescs);
    memcpy(arg.DestPostBuildBufferKeys.data(), src + offset,
           sizeof(unsigned) * arg.Value->numPostbuildInfoDescs);
    offset += sizeof(unsigned) * arg.Value->numPostbuildInfoDescs;

    arg.DestPostBuildBufferOffsets.resize(arg.Value->numPostbuildInfoDescs);
    memcpy(arg.DestPostBuildBufferOffsets.data(), src + offset,
           sizeof(unsigned) * arg.Value->numPostbuildInfoDescs);
    offset += sizeof(unsigned) * arg.Value->numPostbuildInfoDescs;
  }
}

void Decode(
    char* src,
    unsigned& offset,
    PointerArgument<NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS>& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.Value = reinterpret_cast<NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS*>(
      src + offset);
  offset += sizeof(NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS);
  if (arg.Value->pDesc) {
    arg.Value->pDesc =
        reinterpret_cast<NVAPI_D3D12_RAYTRACING_MULTI_INDIRECT_CLUSTER_OPERATION_DESC*>(src +
                                                                                        offset);
    offset += sizeof(NVAPI_D3D12_RAYTRACING_MULTI_INDIRECT_CLUSTER_OPERATION_DESC);
  }

  memcpy(&arg.BatchResultDataKey, src + offset, sizeof(arg.BatchResultDataKey));
  offset += sizeof(arg.BatchResultDataKey);
  memcpy(&arg.BatchResultDataOffset, src + offset, sizeof(arg.BatchResultDataOffset));
  offset += sizeof(arg.BatchResultDataOffset);

  memcpy(&arg.BatchScratchDataKey, src + offset, sizeof(arg.BatchScratchDataKey));
  offset += sizeof(arg.BatchScratchDataKey);
  memcpy(&arg.BatchScratchDataOffset, src + offset, sizeof(arg.BatchScratchDataOffset));
  offset += sizeof(arg.BatchScratchDataOffset);

  memcpy(&arg.DestinationAddressArrayKey, src + offset, sizeof(arg.DestinationAddressArrayKey));
  offset += sizeof(arg.DestinationAddressArrayKey);
  memcpy(&arg.DestinationAddressArrayOffset, src + offset,
         sizeof(arg.DestinationAddressArrayOffset));
  offset += sizeof(arg.DestinationAddressArrayOffset);

  memcpy(&arg.ResultSizeArrayKey, src + offset, sizeof(arg.ResultSizeArrayKey));
  offset += sizeof(arg.ResultSizeArrayKey);
  memcpy(&arg.ResultSizeArrayOffset, src + offset, sizeof(arg.ResultSizeArrayOffset));
  offset += sizeof(arg.ResultSizeArrayOffset);

  memcpy(&arg.IndirectArgArrayKey, src + offset, sizeof(arg.IndirectArgArrayKey));
  offset += sizeof(arg.IndirectArgArrayKey);
  memcpy(&arg.IndirectArgArrayOffset, src + offset, sizeof(arg.IndirectArgArrayOffset));
  offset += sizeof(arg.IndirectArgArrayOffset);

  memcpy(&arg.IndirectArgCountKey, src + offset, sizeof(arg.IndirectArgCountKey));
  offset += sizeof(arg.IndirectArgCountKey);
  memcpy(&arg.IndirectArgCountOffset, src + offset, sizeof(arg.IndirectArgCountOffset));
  offset += sizeof(arg.IndirectArgCountOffset);
}

void Decode(char* src, unsigned& offset, xell_frame_report_t_Argument& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }
  arg.Value = reinterpret_cast<xell_frame_report_t*>(src + offset);
  offset += sizeof(xell_frame_report_t) * arg.FRAME_REPORTS_COUNT;
}

void Decode(char* src, unsigned& offset, xefg_swapchain_d3d12_init_params_t_Argument& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }
  arg.Value = reinterpret_cast<xefg_swapchain_d3d12_init_params_t*>(src + offset);
  offset += sizeof(xefg_swapchain_d3d12_init_params_t);
  memcpy(&arg.Key, src + offset, sizeof(arg.Key));
  offset += sizeof(arg.Key);
  memcpy(&arg.ApplicationSwapChainKey, src + offset, sizeof(arg.ApplicationSwapChainKey));
  offset += sizeof(arg.ApplicationSwapChainKey);
  memcpy(&arg.TempBufferHeapKey, src + offset, sizeof(arg.TempBufferHeapKey));
  offset += sizeof(arg.TempBufferHeapKey);
  memcpy(&arg.TempTextureHeapKey, src + offset, sizeof(arg.TempTextureHeapKey));
  offset += sizeof(arg.TempTextureHeapKey);
  memcpy(&arg.PipelineLibraryKey, src + offset, sizeof(arg.PipelineLibraryKey));
  offset += sizeof(arg.PipelineLibraryKey);
}

void Decode(char* src, unsigned& offset, xefg_swapchain_d3d12_resource_data_t_Argument& arg) {
  if (DecodeNullPtr(src, offset, arg)) {
    return;
  }
  arg.Value = reinterpret_cast<xefg_swapchain_d3d12_resource_data_t*>(src + offset);
  offset += sizeof(xefg_swapchain_d3d12_resource_data_t);
  memcpy(&arg.ResourceKey, src + offset, sizeof(arg.ResourceKey));
  offset += sizeof(arg.ResourceKey);
}

} // namespace DirectX
} // namespace gits
