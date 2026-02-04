// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "argumentDecoders.h"
#include "dmlCodersAuto.h"
#include "gits.h"

#include <d3dx12/d3dx12_pipeline_state_stream.h>

namespace gits {
namespace DirectX {

void decode(char* src, unsigned& offset, BufferArgument& arg) {
  if (decodeNullPtr(src, offset, arg)) {
    return;
  }

  memcpy(&arg.size, src + offset, sizeof(arg.size));
  offset += sizeof(arg.size);

  arg.value = const_cast<char*>(src + offset);
  offset += arg.size;
}

void decode(char* src, unsigned& offset, OutputBufferArgument& arg) {
  memcpy(&arg.captureValue, src + offset, sizeof(void*));
  offset += sizeof(void*);
  if (arg.captureValue) {
    arg.value = &arg.data;
  } else {
    arg.value = nullptr;
  }
}

void decode(char* src, unsigned& offset, LPCWSTR_Argument& arg) {
  if (decodeNullPtr(src, offset, arg)) {
    return;
  }

  unsigned* len = reinterpret_cast<unsigned*>(src + offset);
  offset += sizeof(unsigned);
  arg.value = reinterpret_cast<LPWSTR>(src + offset);
  offset += *len;
}

void decode(char* src, unsigned& offset, LPCSTR_Argument& arg) {
  if (decodeNullPtr(src, offset, arg)) {
    return;
  }

  unsigned* len = reinterpret_cast<unsigned*>(src + offset);
  offset += sizeof(unsigned);
  arg.value = reinterpret_cast<LPSTR>(src + offset);
  offset += *len;
}

void decode(char* src, unsigned& offset, D3D12_GPU_VIRTUAL_ADDRESS_Argument& arg) {
  memcpy(&arg.value, src + offset, sizeof(arg.value));
  offset += sizeof(arg.value);
  memcpy(&arg.interfaceKey, src + offset, sizeof(arg.interfaceKey));
  offset += sizeof(arg.interfaceKey);
  memcpy(&arg.offset, src + offset, sizeof(arg.offset));
  offset += sizeof(arg.offset);
}

void decode(char* src, unsigned& offset, D3D12_GPU_VIRTUAL_ADDRESSs_Argument& arg) {
  if (decodeNullPtr(src, offset, arg)) {
    return;
  }
  memcpy(&arg.size, src + offset, sizeof(arg.size));
  offset += sizeof(arg.size);

  arg.value = reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS*>(src + offset);
  offset += sizeof(D3D12_GPU_VIRTUAL_ADDRESS) * arg.size;

  arg.interfaceKeys.resize(arg.size);
  memcpy(arg.interfaceKeys.data(), src + offset, arg.size * sizeof(unsigned));
  offset += arg.size * sizeof(unsigned);

  arg.offsets.resize(arg.size);
  memcpy(arg.offsets.data(), src + offset, arg.size * sizeof(unsigned));
  offset += arg.size * sizeof(unsigned);
}

void decode(char* src, unsigned& offset, ShaderIdentifierArgument& arg) {
  arg.data.resize(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
  memcpy(arg.data.data(), src + offset, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
  offset += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
  arg.value = arg.data.data();
}

void decode(char* src, unsigned& offset, D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument& arg) {
  if (decodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.value = reinterpret_cast<D3D12_GRAPHICS_PIPELINE_STATE_DESC*>(src + offset);
  offset += sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC);

  auto decodeBytecode = [&](D3D12_SHADER_BYTECODE& bytecode) {
    if (bytecode.pShaderBytecode) {
      bytecode.pShaderBytecode = src + offset;
      offset += bytecode.BytecodeLength;
    }
  };
  decodeBytecode(arg.value->VS);
  decodeBytecode(arg.value->PS);
  decodeBytecode(arg.value->DS);
  decodeBytecode(arg.value->HS);
  decodeBytecode(arg.value->GS);

  if (arg.value->StreamOutput.pSODeclaration) {
    arg.value->StreamOutput.pSODeclaration =
        reinterpret_cast<D3D12_SO_DECLARATION_ENTRY*>(src + offset);
    offset += arg.value->StreamOutput.NumEntries * sizeof(D3D12_SO_DECLARATION_ENTRY);

    for (unsigned i = 0; i < arg.value->StreamOutput.NumEntries; ++i) {
      unsigned* len = reinterpret_cast<unsigned*>(src + offset);
      offset += sizeof(unsigned);
      const_cast<D3D12_SO_DECLARATION_ENTRY*>(arg.value->StreamOutput.pSODeclaration)[i]
          .SemanticName = reinterpret_cast<LPCSTR>(src + offset);
      offset += *len;
    }
  }
  if (arg.value->StreamOutput.pBufferStrides) {
    arg.value->StreamOutput.pBufferStrides = reinterpret_cast<UINT*>(src + offset);
    offset += arg.value->StreamOutput.NumStrides * sizeof(UINT);
  }

  arg.value->InputLayout.pInputElementDescs =
      reinterpret_cast<D3D12_INPUT_ELEMENT_DESC*>(src + offset);
  offset += sizeof(D3D12_INPUT_ELEMENT_DESC) * arg.value->InputLayout.NumElements;

  for (unsigned i = 0; i < arg.value->InputLayout.NumElements; ++i) {
    D3D12_INPUT_ELEMENT_DESC& inputElement =
        const_cast<D3D12_INPUT_ELEMENT_DESC&>(arg.value->InputLayout.pInputElementDescs[i]);
    unsigned* len = reinterpret_cast<unsigned*>(src + offset);
    offset += sizeof(unsigned);
    inputElement.SemanticName = src + offset;
    offset += *len;
  }

  if (arg.value->CachedPSO.pCachedBlob) {
    arg.value->CachedPSO.pCachedBlob = src + offset;
    offset += arg.value->CachedPSO.CachedBlobSizeInBytes;
  }

  memcpy(&arg.rootSignatureKey, src + offset, sizeof(arg.rootSignatureKey));
  offset += sizeof(arg.rootSignatureKey);
}

void decode(char* src, unsigned& offset, D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument& arg) {
  if (decodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.value = reinterpret_cast<D3D12_COMPUTE_PIPELINE_STATE_DESC*>(src + offset);
  offset += sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC);

  if (arg.value->CS.pShaderBytecode) {
    arg.value->CS.pShaderBytecode = src + offset;
    offset += arg.value->CS.BytecodeLength;
  }

  if (arg.value->CachedPSO.pCachedBlob) {
    arg.value->CachedPSO.pCachedBlob = src + offset;
    offset += arg.value->CachedPSO.CachedBlobSizeInBytes;
  }

  memcpy(&arg.rootSignatureKey, src + offset, sizeof(arg.rootSignatureKey));
  offset += sizeof(arg.rootSignatureKey);
}

void decode(char* src, unsigned& offset, D3D12_TEXTURE_COPY_LOCATION_Argument& arg) {

  arg.value = reinterpret_cast<D3D12_TEXTURE_COPY_LOCATION*>(src + offset);
  offset += sizeof(D3D12_TEXTURE_COPY_LOCATION);

  memcpy(&arg.resourceKey, src + offset, sizeof(arg.resourceKey));
  offset += sizeof(arg.resourceKey);
}

void decode(char* src, unsigned& offset, D3D12_RESOURCE_BARRIERs_Argument& arg) {
  if (decodeNullPtr(src, offset, arg)) {
    return;
  }

  memcpy(&arg.size, src + offset, sizeof(arg.size));
  offset += sizeof(arg.size);

  arg.value = reinterpret_cast<D3D12_RESOURCE_BARRIER*>(src + offset);
  offset += arg.size * sizeof(D3D12_RESOURCE_BARRIER);

  arg.resourceKeys.resize(arg.size);
  memcpy(arg.resourceKeys.data(), src + offset, arg.size * sizeof(unsigned));
  offset += arg.size * sizeof(unsigned);

  arg.resourceAfterKeys.resize(arg.size);
  memcpy(arg.resourceAfterKeys.data(), src + offset, arg.size * sizeof(unsigned));
  offset += arg.size * sizeof(unsigned);
}

void decode(char* src, unsigned& offset, PointerArgument<D3D12_ROOT_SIGNATURE_DESC>& arg) {
  if (decodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.value = reinterpret_cast<D3D12_ROOT_SIGNATURE_DESC*>(src + offset);
  offset += sizeof(D3D12_ROOT_SIGNATURE_DESC);

  arg.value->pStaticSamplers = reinterpret_cast<D3D12_STATIC_SAMPLER_DESC*>(src + offset);
  offset += sizeof(D3D12_STATIC_SAMPLER_DESC) * arg.value->NumStaticSamplers;

  arg.value->pParameters = reinterpret_cast<D3D12_ROOT_PARAMETER*>(src + offset);
  offset += sizeof(D3D12_ROOT_PARAMETER) * arg.value->NumParameters;
  for (unsigned i = 0; i < arg.value->NumParameters; ++i) {
    if (arg.value->pParameters[i].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {

      D3D12_ROOT_DESCRIPTOR_TABLE& descriptorTable =
          const_cast<D3D12_ROOT_DESCRIPTOR_TABLE&>(arg.value->pParameters[i].DescriptorTable);
      descriptorTable.pDescriptorRanges = reinterpret_cast<D3D12_DESCRIPTOR_RANGE*>(src + offset);
      offset += sizeof(D3D12_DESCRIPTOR_RANGE) * descriptorTable.NumDescriptorRanges;
    }
  }
}

void decode(char* src,
            unsigned& offset,
            PointerArgument<D3D12_VERSIONED_ROOT_SIGNATURE_DESC>& arg) {
  if (decodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.value = reinterpret_cast<D3D12_VERSIONED_ROOT_SIGNATURE_DESC*>(src + offset);
  offset += sizeof(D3D12_VERSIONED_ROOT_SIGNATURE_DESC);

  switch (arg.value->Version) {
  case D3D_ROOT_SIGNATURE_VERSION_1_0: {
    D3D12_ROOT_SIGNATURE_DESC& desc0 = arg.value->Desc_1_0;

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
    D3D12_ROOT_SIGNATURE_DESC1& desc1 = arg.value->Desc_1_1;

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
    D3D12_ROOT_SIGNATURE_DESC2& desc2 = arg.value->Desc_1_2;

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

void decode(char* src, unsigned& offset, PointerArgument<D3D12_COMMAND_SIGNATURE_DESC>& arg) {
  if (decodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.value = reinterpret_cast<D3D12_COMMAND_SIGNATURE_DESC*>(src + offset);
  offset += sizeof(D3D12_COMMAND_SIGNATURE_DESC);

  arg.value->pArgumentDescs = reinterpret_cast<D3D12_INDIRECT_ARGUMENT_DESC*>(src + offset);
  offset += sizeof(D3D12_INDIRECT_ARGUMENT_DESC) * arg.value->NumArgumentDescs;
}

void decode(char* src, unsigned& offset, D3D12_INDEX_BUFFER_VIEW_Argument& arg) {
  if (decodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.value = reinterpret_cast<D3D12_INDEX_BUFFER_VIEW*>(src + offset);
  offset += sizeof(D3D12_INDEX_BUFFER_VIEW);

  memcpy(&arg.bufferLocationKey, src + offset, sizeof(arg.bufferLocationKey));
  offset += sizeof(arg.bufferLocationKey);
  memcpy(&arg.bufferLocationOffset, src + offset, sizeof(arg.bufferLocationOffset));
  offset += sizeof(arg.bufferLocationOffset);
}

void decode(char* src, unsigned& offset, D3D12_CONSTANT_BUFFER_VIEW_DESC_Argument& arg) {
  if (decodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.value = reinterpret_cast<D3D12_CONSTANT_BUFFER_VIEW_DESC*>(src + offset);
  offset += sizeof(D3D12_CONSTANT_BUFFER_VIEW_DESC);

  memcpy(&arg.bufferLocationKey, src + offset, sizeof(arg.bufferLocationKey));
  offset += sizeof(arg.bufferLocationKey);
  memcpy(&arg.bufferLocationOffset, src + offset, sizeof(arg.bufferLocationOffset));
  offset += sizeof(arg.bufferLocationOffset);
}

void decode(char* src, unsigned& offset, D3D12_VERTEX_BUFFER_VIEWs_Argument& arg) {
  if (decodeNullPtr(src, offset, arg)) {
    return;
  }

  memcpy(&arg.size, src + offset, sizeof(arg.size));
  offset += sizeof(arg.size);

  arg.value = reinterpret_cast<D3D12_VERTEX_BUFFER_VIEW*>(src + offset);
  offset += arg.size * sizeof(D3D12_VERTEX_BUFFER_VIEW);

  arg.bufferLocationKeys.resize(arg.size);
  memcpy(arg.bufferLocationKeys.data(), src + offset, arg.size * sizeof(unsigned));
  offset += arg.size * sizeof(unsigned);

  arg.bufferLocationOffsets.resize(arg.size);
  memcpy(arg.bufferLocationOffsets.data(), src + offset, arg.size * sizeof(unsigned));
  offset += arg.size * sizeof(unsigned);
}

void decode(char* src, unsigned& offset, D3D12_STREAM_OUTPUT_BUFFER_VIEWs_Argument& arg) {
  if (decodeNullPtr(src, offset, arg)) {
    return;
  }

  memcpy(&arg.size, src + offset, sizeof(arg.size));
  offset += sizeof(arg.size);

  arg.value = reinterpret_cast<D3D12_STREAM_OUTPUT_BUFFER_VIEW*>(src + offset);
  offset += arg.size * sizeof(D3D12_STREAM_OUTPUT_BUFFER_VIEW);

  arg.bufferLocationKeys.resize(arg.size);
  memcpy(arg.bufferLocationKeys.data(), src + offset, arg.size * sizeof(unsigned));
  offset += arg.size * sizeof(unsigned);

  arg.bufferLocationOffsets.resize(arg.size);
  memcpy(arg.bufferLocationOffsets.data(), src + offset, arg.size * sizeof(unsigned));
  offset += arg.size * sizeof(unsigned);

  arg.bufferFilledSizeLocationKeys.resize(arg.size);
  memcpy(arg.bufferFilledSizeLocationKeys.data(), src + offset, arg.size * sizeof(unsigned));
  offset += arg.size * sizeof(unsigned);

  arg.bufferFilledSizeLocationOffsets.resize(arg.size);
  memcpy(arg.bufferFilledSizeLocationOffsets.data(), src + offset, arg.size * sizeof(unsigned));
  offset += arg.size * sizeof(unsigned);
}

void decode(char* src, unsigned& offset, D3D12_WRITEBUFFERIMMEDIATE_PARAMETERs_Argument& arg) {
  if (decodeNullPtr(src, offset, arg)) {
    return;
  }

  memcpy(&arg.size, src + offset, sizeof(arg.size));
  offset += sizeof(arg.size);

  arg.value = reinterpret_cast<D3D12_WRITEBUFFERIMMEDIATE_PARAMETER*>(src + offset);
  offset += arg.size * sizeof(D3D12_WRITEBUFFERIMMEDIATE_PARAMETER);

  arg.destKeys.resize(arg.size);
  memcpy(arg.destKeys.data(), src + offset, arg.size * sizeof(unsigned));
  offset += arg.size * sizeof(unsigned);

  arg.destOffsets.resize(arg.size);
  memcpy(arg.destOffsets.data(), src + offset, arg.size * sizeof(unsigned));
  offset += arg.size * sizeof(unsigned);
}

void decode(char* src, unsigned& offset, D3D12_STATE_OBJECT_DESC_Argument& arg) {
  if (decodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.value = reinterpret_cast<D3D12_STATE_OBJECT_DESC*>(src + offset);
  offset += sizeof(D3D12_STATE_OBJECT_DESC);

  arg.value->pSubobjects = reinterpret_cast<D3D12_STATE_SUBOBJECT*>(src + offset);
  offset += sizeof(D3D12_STATE_SUBOBJECT) * arg.value->NumSubobjects;

  std::map<const D3D12_STATE_SUBOBJECT*, unsigned> subobjectIndexes;

  for (unsigned index = 0; index < arg.value->NumSubobjects; ++index) {
    D3D12_STATE_SUBOBJECT& subobject =
        const_cast<D3D12_STATE_SUBOBJECT&>(arg.value->pSubobjects[index]);

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
          const_cast<D3D12_STATE_SUBOBJECT&>(arg.value->pSubobjects[*indexAssociation]);
      GITS_ASSERT(subobject.Type == D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION);
      D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION* desc =
          static_cast<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(const_cast<void*>(subobject.pDesc));
      desc->pSubobjectToAssociate = &arg.value->pSubobjects[*indexAssociated];
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

      arg.interfaceKeysBySubobject[*index] = *key;
    }
  }
}

void decode(char* src, unsigned& offset, D3D12_PIPELINE_STATE_STREAM_DESC_Argument& arg) {
  if (decodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.value = reinterpret_cast<D3D12_PIPELINE_STATE_STREAM_DESC*>(src + offset);
  offset += sizeof(D3D12_PIPELINE_STATE_STREAM_DESC);

  arg.value->pPipelineStateSubobjectStream = src + offset;
  offset += arg.value->SizeInBytes;

  size_t stateOffset = 0;
  while (stateOffset < arg.value->SizeInBytes) {
    void* subobjectData =
        static_cast<char*>(arg.value->pPipelineStateSubobjectStream) + stateOffset;
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

  memcpy(&arg.rootSignatureKey, src + offset, sizeof(arg.rootSignatureKey));
  offset += sizeof(arg.rootSignatureKey);
}

void decode(char* src, unsigned& offset, D3D12_BARRIER_GROUPs_Argument& arg) {
  if (decodeNullPtr(src, offset, arg)) {
    return;
  }

  memcpy(&arg.size, src + offset, sizeof(arg.size));
  offset += sizeof(arg.size);

  arg.value = reinterpret_cast<D3D12_BARRIER_GROUP*>(src + offset);
  offset += arg.size * sizeof(D3D12_BARRIER_GROUP);

  unsigned numResourceKeys{};

  for (unsigned i = 0; i < arg.size; ++i) {
    if (arg.value[i].Type == D3D12_BARRIER_TYPE_GLOBAL) {
      arg.value[i].pGlobalBarriers = reinterpret_cast<D3D12_GLOBAL_BARRIER*>(src + offset);
      offset += sizeof(D3D12_GLOBAL_BARRIER) * arg.value[i].NumBarriers;
    } else if (arg.value[i].Type == D3D12_BARRIER_TYPE_TEXTURE) {
      arg.value[i].pTextureBarriers = reinterpret_cast<D3D12_TEXTURE_BARRIER*>(src + offset);
      offset += sizeof(D3D12_TEXTURE_BARRIER) * arg.value[i].NumBarriers;
      numResourceKeys += arg.value[i].NumBarriers;
    } else if (arg.value[i].Type == D3D12_BARRIER_TYPE_BUFFER) {
      arg.value[i].pBufferBarriers = reinterpret_cast<D3D12_BUFFER_BARRIER*>(src + offset);
      offset += sizeof(D3D12_BUFFER_BARRIER) * arg.value[i].NumBarriers;
      numResourceKeys += arg.value[i].NumBarriers;
    }
  }

  arg.resourceKeys.resize(numResourceKeys);
  memcpy(arg.resourceKeys.data(), src + offset, numResourceKeys * sizeof(unsigned));
  offset += numResourceKeys * sizeof(unsigned);
}

void decode(char* src, unsigned& offset, DML_BINDING_DESC_Argument& arg) {
  if (decodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.value = reinterpret_cast<DML_BINDING_DESC*>(src + offset);
  dml::decode(arg.value, 1, src, offset);

  memcpy(&arg.resourceKeysSize, src + offset, sizeof(arg.resourceKeysSize));
  offset += sizeof(arg.resourceKeysSize);

  arg.resourceKeys.resize(arg.resourceKeysSize);
  memcpy(arg.resourceKeys.data(), src + offset, arg.resourceKeysSize * sizeof(unsigned));
  offset += arg.resourceKeysSize * sizeof(unsigned);
}

void decode(char* src, unsigned& offset, DML_BINDING_DESCs_Argument& arg) {
  if (decodeNullPtr(src, offset, arg)) {
    return;
  }

  memcpy(&arg.size, src + offset, sizeof(arg.size));
  offset += sizeof(arg.size);

  arg.value = reinterpret_cast<DML_BINDING_DESC*>(src + offset);
  dml::decode(arg.value, arg.size, src, offset);

  memcpy(&arg.resourceKeysSize, src + offset, sizeof(arg.resourceKeysSize));
  offset += sizeof(arg.resourceKeysSize);

  arg.resourceKeys.resize(arg.resourceKeysSize);
  memcpy(arg.resourceKeys.data(), src + offset, arg.resourceKeysSize * sizeof(unsigned));
  offset += arg.resourceKeysSize * sizeof(unsigned);
}

void decode(char* src, unsigned& offset, DML_BINDING_TABLE_DESC_Argument& arg) {
  if (decodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.value = reinterpret_cast<DML_BINDING_TABLE_DESC*>(src + offset);
  offset += sizeof(DML_BINDING_TABLE_DESC);

  memcpy(&arg.data, src + offset, sizeof(arg.data));
  offset += sizeof(arg.data);
}

void decode(char* src, unsigned& offset, DML_OPERATOR_DESC_Argument& arg) {
  if (decodeNullPtr(src, offset, arg)) {
    return;
  }
  arg.value = reinterpret_cast<DML_OPERATOR_DESC*>(src + offset);
  dml::decode(arg.value, 1, src, offset);
}

void decode(char* src, unsigned& offset, DML_GRAPH_DESC_Argument& arg) {
  if (decodeNullPtr(src, offset, arg)) {
    return;
  }
  arg.value = reinterpret_cast<DML_GRAPH_DESC*>(src + offset);
  dml::decode(arg.value, 1, src, offset);

  memcpy(&arg.operatorKeysSize, src + offset, sizeof(unsigned));
  offset += sizeof(unsigned);

  arg.operatorKeys.resize(arg.operatorKeysSize);
  memcpy(arg.operatorKeys.data(), src + offset, arg.operatorKeysSize * sizeof(unsigned));
  offset += arg.operatorKeysSize * sizeof(unsigned);
}

void decode(char* src, unsigned& offset, DML_CheckFeatureSupport_BufferArgument& arg) {
  if (decodeNullPtr(src, offset, arg)) {
    return;
  }
  decode(src, offset, arg.size);
  decode(src, offset, arg.feature);

  arg.value = const_cast<char*>(src + offset);
  if (arg.feature == DML_FEATURE_FEATURE_LEVELS) {
    auto* featlevels = reinterpret_cast<DML_FEATURE_QUERY_FEATURE_LEVELS*>(arg.value);
    offset += sizeof(DML_FEATURE_QUERY_FEATURE_LEVELS);
    featlevels->RequestedFeatureLevels = reinterpret_cast<const DML_FEATURE_LEVEL*>(src + offset);
    offset += featlevels->RequestedFeatureLevelCount * sizeof(DML_FEATURE_LEVEL);
  } else {
    offset += arg.size;
  }
}

void decode(char* src,
            unsigned& offset,
            PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS>& arg) {
  if (decodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.value = reinterpret_cast<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS*>(src + offset);
  offset += sizeof(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS);

  if (arg.value->Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
    if (arg.value->DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY) {
      if (arg.value->pGeometryDescs) {
        arg.value->pGeometryDescs = reinterpret_cast<D3D12_RAYTRACING_GEOMETRY_DESC*>(src + offset);
        offset += sizeof(D3D12_RAYTRACING_GEOMETRY_DESC) * arg.value->NumDescs;
        for (unsigned i = 0; i < arg.value->NumDescs; ++i) {
          if (arg.value->pGeometryDescs[i].Type == D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES) {
            auto& desc = const_cast<D3D12_RAYTRACING_GEOMETRY_DESC*>(arg.value->pGeometryDescs)[i];
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
    } else if (arg.value->DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS) {
      if (arg.value->ppGeometryDescs) {
        arg.value->ppGeometryDescs =
            reinterpret_cast<D3D12_RAYTRACING_GEOMETRY_DESC**>(src + offset);
        offset += sizeof(D3D12_RAYTRACING_GEOMETRY_DESC*) * arg.value->NumDescs;
      }
      for (unsigned i = 0; i < arg.value->NumDescs; ++i) {
        const_cast<D3D12_RAYTRACING_GEOMETRY_DESC**>(arg.value->ppGeometryDescs)[i] =
            reinterpret_cast<D3D12_RAYTRACING_GEOMETRY_DESC*>(src + offset);
        offset += sizeof(D3D12_RAYTRACING_GEOMETRY_DESC);
        if (arg.value->ppGeometryDescs[i]->Type == D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES) {
          auto desc = const_cast<D3D12_RAYTRACING_GEOMETRY_DESC**>(arg.value->ppGeometryDescs)[i];
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
  } else if (arg.value->Type ==
             D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_OPACITY_MICROMAP_ARRAY) {
    if (arg.value->pOpacityMicromapArrayDesc) {
      arg.value->pOpacityMicromapArrayDesc =
          reinterpret_cast<D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC*>(src + offset);
      offset += sizeof(D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC);
      if (arg.value->pOpacityMicromapArrayDesc->pOmmHistogram) {
        auto desc = const_cast<D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC*>(
            arg.value->pOpacityMicromapArrayDesc);
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

  arg.inputKeys.resize(size);
  memcpy(arg.inputKeys.data(), src + offset, size * sizeof(unsigned));
  offset += size * sizeof(unsigned);

  arg.inputOffsets.resize(size);
  memcpy(arg.inputOffsets.data(), src + offset, size * sizeof(unsigned));
  offset += size * sizeof(unsigned);
}

void decode(char* src,
            unsigned& offset,
            PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC>& arg) {
  if (decodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.value = reinterpret_cast<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC*>(src + offset);
  offset += sizeof(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC);

  if (arg.value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
    if (arg.value->Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY) {
      if (arg.value->Inputs.pGeometryDescs) {
        arg.value->Inputs.pGeometryDescs =
            reinterpret_cast<D3D12_RAYTRACING_GEOMETRY_DESC*>(src + offset);
        offset += sizeof(D3D12_RAYTRACING_GEOMETRY_DESC) * arg.value->Inputs.NumDescs;
        for (unsigned i = 0; i < arg.value->Inputs.NumDescs; ++i) {
          if (arg.value->Inputs.pGeometryDescs[i].Type ==
              D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES) {
            auto& desc =
                const_cast<D3D12_RAYTRACING_GEOMETRY_DESC*>(arg.value->Inputs.pGeometryDescs)[i];
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
    } else if (arg.value->Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS) {
      if (arg.value->Inputs.ppGeometryDescs) {
        arg.value->Inputs.ppGeometryDescs =
            reinterpret_cast<D3D12_RAYTRACING_GEOMETRY_DESC**>(src + offset);
        offset += sizeof(D3D12_RAYTRACING_GEOMETRY_DESC*) * arg.value->Inputs.NumDescs;
      }
      for (unsigned i = 0; i < arg.value->Inputs.NumDescs; ++i) {
        const_cast<D3D12_RAYTRACING_GEOMETRY_DESC**>(arg.value->Inputs.ppGeometryDescs)[i] =
            reinterpret_cast<D3D12_RAYTRACING_GEOMETRY_DESC*>(src + offset);
        offset += sizeof(D3D12_RAYTRACING_GEOMETRY_DESC);
        if (arg.value->Inputs.ppGeometryDescs[i]->Type ==
            D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES) {
          auto desc =
              const_cast<D3D12_RAYTRACING_GEOMETRY_DESC**>(arg.value->Inputs.ppGeometryDescs)[i];
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
  } else if (arg.value->Inputs.Type ==
             D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_OPACITY_MICROMAP_ARRAY) {
    if (arg.value->Inputs.pOpacityMicromapArrayDesc) {
      arg.value->Inputs.pOpacityMicromapArrayDesc =
          reinterpret_cast<D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC*>(src + offset);
      offset += sizeof(D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC);
      if (arg.value->Inputs.pOpacityMicromapArrayDesc->pOmmHistogram) {
        auto desc = const_cast<D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC*>(
            arg.value->Inputs.pOpacityMicromapArrayDesc);
        desc->pOmmHistogram =
            reinterpret_cast<D3D12_RAYTRACING_OPACITY_MICROMAP_HISTOGRAM_ENTRY*>(src + offset);
        offset += sizeof(D3D12_RAYTRACING_OPACITY_MICROMAP_HISTOGRAM_ENTRY) *
                  desc->NumOmmHistogramEntries;
      }
    }
  }

  memcpy(&arg.destAccelerationStructureKey, src + offset, sizeof(arg.destAccelerationStructureKey));
  offset += sizeof(arg.destAccelerationStructureKey);
  memcpy(&arg.destAccelerationStructureOffset, src + offset,
         sizeof(arg.destAccelerationStructureOffset));
  offset += sizeof(arg.destAccelerationStructureOffset);

  memcpy(&arg.sourceAccelerationStructureKey, src + offset,
         sizeof(arg.sourceAccelerationStructureKey));
  offset += sizeof(arg.sourceAccelerationStructureKey);
  memcpy(&arg.sourceAccelerationStructureOffset, src + offset,
         sizeof(arg.sourceAccelerationStructureOffset));
  offset += sizeof(arg.sourceAccelerationStructureOffset);

  memcpy(&arg.scratchAccelerationStructureKey, src + offset,
         sizeof(arg.scratchAccelerationStructureKey));
  offset += sizeof(arg.scratchAccelerationStructureKey);
  memcpy(&arg.scratchAccelerationStructureOffset, src + offset,
         sizeof(arg.scratchAccelerationStructureOffset));
  offset += sizeof(arg.scratchAccelerationStructureOffset);

  unsigned size{};
  memcpy(&size, src + offset, sizeof(size));
  offset += sizeof(size);

  arg.inputKeys.resize(size);
  memcpy(arg.inputKeys.data(), src + offset, size * sizeof(unsigned));
  offset += size * sizeof(unsigned);

  arg.inputOffsets.resize(size);
  memcpy(arg.inputOffsets.data(), src + offset, size * sizeof(unsigned));
  offset += size * sizeof(unsigned);
}

void decode(char* src, unsigned& offset, PointerArgument<D3D12_DISPATCH_RAYS_DESC>& arg) {
  if (decodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.value = reinterpret_cast<D3D12_DISPATCH_RAYS_DESC*>(src + offset);
  offset += sizeof(D3D12_DISPATCH_RAYS_DESC);

  memcpy(&arg.rayGenerationShaderRecordKey, src + offset, sizeof(arg.rayGenerationShaderRecordKey));
  offset += sizeof(arg.rayGenerationShaderRecordKey);
  memcpy(&arg.rayGenerationShaderRecordOffset, src + offset,
         sizeof(arg.rayGenerationShaderRecordOffset));
  offset += sizeof(arg.rayGenerationShaderRecordOffset);

  memcpy(&arg.missShaderTableKey, src + offset, sizeof(arg.missShaderTableKey));
  offset += sizeof(arg.missShaderTableKey);
  memcpy(&arg.missShaderTableOffset, src + offset, sizeof(arg.missShaderTableOffset));
  offset += sizeof(arg.missShaderTableOffset);

  memcpy(&arg.hitGroupTableKey, src + offset, sizeof(arg.hitGroupTableKey));
  offset += sizeof(arg.hitGroupTableKey);
  memcpy(&arg.hitGroupTableOffset, src + offset, sizeof(arg.hitGroupTableOffset));
  offset += sizeof(arg.hitGroupTableOffset);

  memcpy(&arg.callableShaderTableKey, src + offset, sizeof(arg.callableShaderTableKey));
  offset += sizeof(arg.callableShaderTableKey);
  memcpy(&arg.callableShaderTableOffset, src + offset, sizeof(arg.callableShaderTableOffset));
  offset += sizeof(arg.callableShaderTableOffset);
}

void decode(char* src,
            unsigned& offset,
            ArrayArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>& arg) {
  if (decodeNullPtr(src, offset, arg)) {
    return;
  }

  memcpy(&arg.size, src + offset, sizeof(arg.size));
  offset += sizeof(arg.size);

  arg.value =
      reinterpret_cast<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC*>(src + offset);
  offset += sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC) * arg.size;

  arg.destBufferKeys.resize(arg.size);
  memcpy(arg.destBufferKeys.data(), src + offset, arg.size * sizeof(unsigned));
  offset += arg.size * sizeof(unsigned);

  arg.destBufferOffsets.resize(arg.size);
  memcpy(arg.destBufferOffsets.data(), src + offset, arg.size * sizeof(unsigned));
  offset += arg.size * sizeof(unsigned);
}

void decode(char* src,
            unsigned& offset,
            PointerArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>& arg) {
  if (decodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.value =
      reinterpret_cast<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC*>(src + offset);
  offset += sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC);

  memcpy(&arg.destBufferKey, src + offset, sizeof(unsigned));
  offset += sizeof(unsigned);

  memcpy(&arg.destBufferOffset, src + offset, sizeof(unsigned));
  offset += sizeof(unsigned);
}

void decode(char* src, unsigned& offset, D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument& arg) {
  if (decodeNullPtr(src, offset, arg)) {
    return;
  }

  memcpy(&arg.size, src + offset, sizeof(arg.size));
  offset += sizeof(arg.size);

  arg.value = reinterpret_cast<D3D12_RENDER_PASS_RENDER_TARGET_DESC*>(src + offset);
  offset += sizeof(D3D12_RENDER_PASS_RENDER_TARGET_DESC) * arg.size;

  for (unsigned i = 0; i < arg.size; ++i) {
    if (arg.value[i].EndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE) {
      arg.value[i].EndingAccess.Resolve.pSubresourceParameters =
          reinterpret_cast<D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS*>(src +
                                                                                            offset);
      offset += sizeof(D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS) *
                arg.value[i].EndingAccess.Resolve.SubresourceCount;
    }
  }

  arg.descriptorKeys.resize(arg.size);
  memcpy(arg.descriptorKeys.data(), src + offset, arg.size * sizeof(unsigned));
  offset += arg.size * sizeof(unsigned);

  arg.descriptorIndexes.resize(arg.size);
  memcpy(arg.descriptorIndexes.data(), src + offset, arg.size * sizeof(unsigned));
  offset += arg.size * sizeof(unsigned);

  unsigned size{};
  memcpy(&size, src + offset, sizeof(size));
  offset += sizeof(size);

  arg.resolveSrcResourceKeys.resize(size);
  memcpy(arg.resolveSrcResourceKeys.data(), src + offset, size * sizeof(unsigned));
  offset += size * sizeof(unsigned);

  arg.resolveDstResourceKeys.resize(size);
  memcpy(arg.resolveDstResourceKeys.data(), src + offset, size * sizeof(unsigned));
  offset += size * sizeof(unsigned);
}

void decode(char* src, unsigned& offset, D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument& arg) {
  if (decodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.value = reinterpret_cast<D3D12_RENDER_PASS_DEPTH_STENCIL_DESC*>(src + offset);
  offset += sizeof(D3D12_RENDER_PASS_DEPTH_STENCIL_DESC);

  if (arg.value->DepthEndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE) {
    arg.value->DepthEndingAccess.Resolve.pSubresourceParameters =
        reinterpret_cast<D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS*>(src +
                                                                                          offset);
    offset += sizeof(D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS) *
              arg.value->DepthEndingAccess.Resolve.SubresourceCount;
  }
  if (arg.value->StencilEndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE) {
    arg.value->StencilEndingAccess.Resolve.pSubresourceParameters =
        reinterpret_cast<D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS*>(src +
                                                                                          offset);
    offset += sizeof(D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS) *
              arg.value->StencilEndingAccess.Resolve.SubresourceCount;
  }

  memcpy(&arg.descriptorKey, src + offset, sizeof(unsigned));
  offset += sizeof(unsigned);
  memcpy(&arg.descriptorIndex, src + offset, sizeof(unsigned));
  offset += sizeof(unsigned);

  memcpy(&arg.resolveSrcDepthKey, src + offset, sizeof(unsigned));
  offset += sizeof(unsigned);
  memcpy(&arg.resolveDstDepthKey, src + offset, sizeof(unsigned));
  offset += sizeof(unsigned);
  memcpy(&arg.resolveSrcStencilKey, src + offset, sizeof(unsigned));
  offset += sizeof(unsigned);
  memcpy(&arg.resolveDstStencilKey, src + offset, sizeof(unsigned));
  offset += sizeof(unsigned);
}

void decode(char* src, unsigned& offset, D3D12_SHADER_RESOURCE_VIEW_DESC_Argument& arg) {
  if (decodeNullPtr(src, offset, arg)) {
    return;
  }
  arg.value = reinterpret_cast<D3D12_SHADER_RESOURCE_VIEW_DESC*>(const_cast<char*>(src + offset));
  offset += sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC);

  if (arg.value->ViewDimension == D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE) {
    memcpy(&arg.raytracingLocationKey, src + offset, sizeof(unsigned));
    offset += sizeof(unsigned);
    memcpy(&arg.raytracingLocationOffset, src + offset, sizeof(unsigned));
    offset += sizeof(unsigned);
  }
}

void decode(char* src,
            unsigned& offset,
            ArrayArgument<D3D12_LINEAR_ALGEBRA_MATRIX_CONVERSION_INFO>& arg) {
  if (decodeNullPtr(src, offset, arg)) {
    return;
  }

  memcpy(&arg.size, src + offset, sizeof(arg.size));
  offset += sizeof(arg.size);

  arg.value = reinterpret_cast<D3D12_LINEAR_ALGEBRA_MATRIX_CONVERSION_INFO*>(src + offset);
  offset += arg.size * sizeof(D3D12_LINEAR_ALGEBRA_MATRIX_CONVERSION_INFO);

  arg.destKey.resize(arg.size);
  memcpy(arg.destKey.data(), src + offset, arg.size * sizeof(unsigned));
  offset += arg.size * sizeof(unsigned);
  arg.destOffset.resize(arg.size);
  memcpy(arg.destOffset.data(), src + offset, arg.size * sizeof(unsigned));
  offset += arg.size * sizeof(unsigned);

  arg.sourceKey.resize(arg.size);
  memcpy(arg.sourceKey.data(), src + offset, arg.size * sizeof(unsigned));
  offset += arg.size * sizeof(unsigned);
  arg.sourceOffset.resize(arg.size);
  memcpy(arg.sourceOffset.data(), src + offset, arg.size * sizeof(unsigned));
  offset += arg.size * sizeof(unsigned);
}

// This decode function does not have a getSize/encode counterpart
// Set pDeviceDriverDesc and pDeviceDriverVersion to 0 (the capture time strings are NOT encoded in the stream)
// Note: Encoding and decoding these values would cause compatibility issues with already existing streams
void decode(char* src, unsigned& offset, PointerArgument<INTCExtensionInfo>& arg) {
  if (decodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.value = reinterpret_cast<INTCExtensionInfo*>(src + offset);
  offset += sizeof(INTCExtensionInfo);

  // Always set pDeviceDriverDesc and pDeviceDriverVersion to 0
  arg.value->pDeviceDriverDesc = 0;
  arg.value->pDeviceDriverVersion = 0;
}

void decode(char* src, unsigned& offset, PointerArgument<INTCExtensionAppInfo>& arg) {
  if (decodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.value = reinterpret_cast<INTCExtensionAppInfo*>(src + offset);
  offset += sizeof(INTCExtensionAppInfo);

  if (arg.value->pApplicationName) {
    unsigned* len = reinterpret_cast<unsigned*>(src + offset);
    offset += sizeof(unsigned);
    arg.value->pApplicationName = reinterpret_cast<const wchar_t*>(src + offset);
    arg.pApplicationName = arg.value->pApplicationName;
    offset += *len;
  }
  if (arg.value->pEngineName) {
    unsigned* len = reinterpret_cast<unsigned*>(src + offset);
    offset += sizeof(unsigned);
    arg.value->pEngineName = reinterpret_cast<const wchar_t*>(src + offset);
    arg.pEngineName = arg.value->pEngineName;
    offset += *len;
  }
}

void decode(char* src, unsigned& offset, PointerArgument<INTCExtensionAppInfo1>& arg) {
  if (decodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.value = reinterpret_cast<INTCExtensionAppInfo1*>(src + offset);
  offset += sizeof(INTCExtensionAppInfo1);

  if (arg.value->pApplicationName) {
    unsigned* len = reinterpret_cast<unsigned*>(src + offset);
    offset += sizeof(unsigned);
    arg.value->pApplicationName = reinterpret_cast<const wchar_t*>(src + offset);
    arg.pApplicationName = arg.value->pApplicationName;
    offset += *len;
  }
  if (arg.value->pEngineName) {
    unsigned* len = reinterpret_cast<unsigned*>(src + offset);
    offset += sizeof(unsigned);
    arg.value->pEngineName = reinterpret_cast<const wchar_t*>(src + offset);
    arg.pEngineName = arg.value->pEngineName;
    offset += *len;
  }
}

void decode(char* src,
            unsigned& offset,
            PointerArgument<INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC>& arg) {
  if (decodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.value = reinterpret_cast<INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC*>(src + offset);
  offset += sizeof(INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC);

  arg.value->pD3D12Desc = reinterpret_cast<D3D12_COMPUTE_PIPELINE_STATE_DESC*>(src + offset);
  offset += sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC);

  arg.value->pD3D12Desc->CS.pShaderBytecode = nullptr;
  arg.value->pD3D12Desc->CS.BytecodeLength = 0;

  if (arg.value->CS.pShaderBytecode) {
    arg.value->CS.pShaderBytecode = src + offset;
    offset += arg.value->CS.BytecodeLength;
  }

  if (arg.value->CompileOptions) {
    unsigned* len = reinterpret_cast<unsigned*>(src + offset);
    offset += sizeof(unsigned);
    arg.value->CompileOptions = src + offset;
    offset += *len;
  }

  if (arg.value->InternalOptions) {
    unsigned* len = reinterpret_cast<unsigned*>(src + offset);
    offset += sizeof(unsigned);
    arg.value->InternalOptions = src + offset;
    offset += *len;
  }

  memcpy(&arg.rootSignatureKey, src + offset, sizeof(arg.rootSignatureKey));
  offset += sizeof(arg.rootSignatureKey);
}

void decode(char* src, unsigned& offset, PointerArgument<INTC_D3D12_HEAP_DESC>& arg) {
  if (decodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.value = reinterpret_cast<INTC_D3D12_HEAP_DESC*>(src + offset);
  offset += sizeof(INTC_D3D12_HEAP_DESC);

  arg.value->pD3D12Desc = reinterpret_cast<D3D12_HEAP_DESC*>(src + offset);
  offset += sizeof(D3D12_HEAP_DESC);
}

void decode(char* src, unsigned& offset, PointerArgument<INTC_D3D12_RESOURCE_DESC_0001>& arg) {
  if (decodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.value = reinterpret_cast<INTC_D3D12_RESOURCE_DESC_0001*>(src + offset);
  offset += sizeof(INTC_D3D12_RESOURCE_DESC_0001);

  arg.value->pD3D12Desc = reinterpret_cast<D3D12_RESOURCE_DESC*>(src + offset);
  offset += sizeof(D3D12_RESOURCE_DESC);
}

void decode(char* src, unsigned& offset, xess_d3d12_init_params_t_Argument& arg) {
  arg.value = reinterpret_cast<xess_d3d12_init_params_t*>(src + offset);
  offset += sizeof(xess_d3d12_init_params_t);

  memcpy(&arg.key, src + offset, sizeof(arg.key));
  offset += sizeof(arg.key);

  memcpy(&arg.tempBufferHeapKey, src + offset, sizeof(arg.tempBufferHeapKey));
  offset += sizeof(arg.tempBufferHeapKey);

  memcpy(&arg.tempTextureHeapKey, src + offset, sizeof(arg.tempTextureHeapKey));
  offset += sizeof(arg.tempTextureHeapKey);

  memcpy(&arg.pipelineLibraryKey, src + offset, sizeof(arg.pipelineLibraryKey));
  offset += sizeof(arg.pipelineLibraryKey);
}

void decode(char* src, unsigned& offset, xess_d3d12_execute_params_t_Argument& arg) {
  arg.value = reinterpret_cast<xess_d3d12_execute_params_t*>(src + offset);
  offset += sizeof(xess_d3d12_execute_params_t);

  memcpy(&arg.colorTextureKey, src + offset, sizeof(arg.colorTextureKey));
  offset += sizeof(arg.colorTextureKey);

  memcpy(&arg.velocityTextureKey, src + offset, sizeof(arg.velocityTextureKey));
  offset += sizeof(arg.velocityTextureKey);

  memcpy(&arg.depthTextureKey, src + offset, sizeof(arg.depthTextureKey));
  offset += sizeof(arg.depthTextureKey);

  memcpy(&arg.exposureScaleTextureKey, src + offset, sizeof(arg.exposureScaleTextureKey));
  offset += sizeof(arg.exposureScaleTextureKey);

  memcpy(&arg.responsivePixelMaskTextureKey, src + offset,
         sizeof(arg.responsivePixelMaskTextureKey));
  offset += sizeof(arg.responsivePixelMaskTextureKey);

  memcpy(&arg.outputTextureKey, src + offset, sizeof(arg.outputTextureKey));
  offset += sizeof(arg.outputTextureKey);

  memcpy(&arg.descriptorHeapKey, src + offset, sizeof(arg.descriptorHeapKey));
  offset += sizeof(arg.descriptorHeapKey);
}

void decode(char* src, unsigned& offset, DSTORAGE_QUEUE_DESC_Argument& arg) {
  if (decodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.value = reinterpret_cast<DSTORAGE_QUEUE_DESC*>(src + offset);
  offset += sizeof(DSTORAGE_QUEUE_DESC);

  memcpy(&arg.deviceKey, src + offset, sizeof(arg.deviceKey));
  offset += sizeof(arg.deviceKey);

  if (arg.value->Name) {
    unsigned* len = reinterpret_cast<unsigned*>(src + offset);
    offset += sizeof(unsigned);
    arg.value->Name = reinterpret_cast<const char*>(src + offset);
    offset += *len;
  }
}

void decode(char* src, unsigned& offset, DSTORAGE_REQUEST_Argument& arg) {
  if (decodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.value = reinterpret_cast<DSTORAGE_REQUEST*>(src + offset);
  offset += sizeof(DSTORAGE_REQUEST);

  memcpy(&arg.fileKey, src + offset, sizeof(arg.fileKey));
  offset += sizeof(arg.fileKey);

  memcpy(&arg.resourceKey, src + offset, sizeof(arg.resourceKey));
  offset += sizeof(arg.resourceKey);

  memcpy(&arg.newOffset, src + offset, sizeof(arg.newOffset));
  offset += sizeof(arg.newOffset);

  if (arg.value->Name) {
    unsigned* len = reinterpret_cast<unsigned*>(src + offset);
    offset += sizeof(unsigned);
    arg.value->Name = reinterpret_cast<const char*>(src + offset);
    offset += *len;
  }
}

void decode(char* src,
            unsigned& offset,
            PointerArgument<NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS>& arg) {
  if (decodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.value =
      reinterpret_cast<NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS*>(src + offset);
  offset += sizeof(NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS);

  if (arg.value->pDesc) {
    arg.value->pDesc =
        reinterpret_cast<NVAPI_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC_EX*>(src +
                                                                                       offset);
    offset += sizeof(NVAPI_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC_EX);

    NVAPI_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC_EX* pDescMod =
        const_cast<NVAPI_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC_EX*>(arg.value->pDesc);

    if (arg.value->pDesc->inputs.type ==
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
      if (arg.value->pDesc->inputs.descsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY) {
        if (arg.value->pDesc->inputs.pGeometryDescs) {
          pDescMod->inputs.pGeometryDescs =
              reinterpret_cast<const NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX*>(src + offset);
          offset += arg.value->pDesc->inputs.numDescs *
                    arg.value->pDesc->inputs.geometryDescStrideInBytes;

          for (unsigned i = 0; i < arg.value->pDesc->inputs.numDescs; ++i) {
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
      } else if (arg.value->pDesc->inputs.descsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS) {
        if (arg.value->pDesc->inputs.ppGeometryDescs) {
          const_cast<NVAPI_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC_EX*>(arg.value->pDesc)
              ->inputs.ppGeometryDescs =
              reinterpret_cast<const NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX**>(src + offset);
          offset +=
              sizeof(NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX*) * arg.value->pDesc->inputs.numDescs;
        }
        for (unsigned i = 0; i < arg.value->pDesc->inputs.numDescs; ++i) {
          const_cast<NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX**>(
              arg.value->pDesc->inputs.ppGeometryDescs)[i] =
              reinterpret_cast<NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX*>(src + offset);
          offset += sizeof(NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX);

          if (arg.value->pDesc->inputs.ppGeometryDescs[i]->type ==
              NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES_EX) {

            const_cast<NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX*>(
                pDescMod->inputs.ppGeometryDescs[i])
                ->ommTriangles.ommAttachment.pOMMUsageCounts =
                reinterpret_cast<const NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_USAGE_COUNT*>(
                    src + offset);
            offset += sizeof(NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_USAGE_COUNT) *
                      arg.value->pDesc->inputs.ppGeometryDescs[i]
                          ->ommTriangles.ommAttachment.numOMMUsageCounts;
          } else if (arg.value->pDesc->inputs.ppGeometryDescs[i]->type ==
                     NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_DMM_TRIANGLES_EX) {
            const_cast<NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX*>(
                pDescMod->inputs.ppGeometryDescs[i])
                ->dmmTriangles.dmmAttachment.pDMMUsageCounts =
                reinterpret_cast<const NVAPI_D3D12_RAYTRACING_DISPLACEMENT_MICROMAP_USAGE_COUNT*>(
                    src + offset);
            offset += sizeof(NVAPI_D3D12_RAYTRACING_DISPLACEMENT_MICROMAP_USAGE_COUNT) *
                      arg.value->pDesc->inputs.ppGeometryDescs[i]
                          ->dmmTriangles.dmmAttachment.numDMMUsageCounts;
          }
        }
      }
    }
  }

  memcpy(&arg.destAccelerationStructureKey, src + offset, sizeof(arg.destAccelerationStructureKey));
  offset += sizeof(arg.destAccelerationStructureKey);
  memcpy(&arg.destAccelerationStructureOffset, src + offset,
         sizeof(arg.destAccelerationStructureOffset));
  offset += sizeof(arg.destAccelerationStructureOffset);

  memcpy(&arg.sourceAccelerationStructureKey, src + offset,
         sizeof(arg.sourceAccelerationStructureKey));
  offset += sizeof(arg.sourceAccelerationStructureKey);
  memcpy(&arg.sourceAccelerationStructureOffset, src + offset,
         sizeof(arg.sourceAccelerationStructureOffset));
  offset += sizeof(arg.sourceAccelerationStructureOffset);

  memcpy(&arg.scratchAccelerationStructureKey, src + offset,
         sizeof(arg.scratchAccelerationStructureKey));
  offset += sizeof(arg.scratchAccelerationStructureKey);
  memcpy(&arg.scratchAccelerationStructureOffset, src + offset,
         sizeof(arg.scratchAccelerationStructureOffset));
  offset += sizeof(arg.scratchAccelerationStructureOffset);

  unsigned size{};
  memcpy(&size, src + offset, sizeof(size));
  offset += sizeof(size);

  arg.inputKeys.resize(size);
  memcpy(arg.inputKeys.data(), src + offset, size * sizeof(unsigned));
  offset += size * sizeof(unsigned);

  arg.inputOffsets.resize(size);
  memcpy(arg.inputOffsets.data(), src + offset, size * sizeof(unsigned));
  offset += size * sizeof(unsigned);

  if (arg.value->pPostbuildInfoDescs) {
    arg.value->pPostbuildInfoDescs =
        reinterpret_cast<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC*>(src +
                                                                                       offset);
    offset += sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC) *
              arg.value->numPostbuildInfoDescs;

    arg.destPostBuildBufferKeys.resize(arg.value->numPostbuildInfoDescs);
    memcpy(arg.destPostBuildBufferKeys.data(), src + offset,
           sizeof(unsigned) * arg.value->numPostbuildInfoDescs);
    offset += sizeof(unsigned) * arg.value->numPostbuildInfoDescs;

    arg.destPostBuildBufferOffsets.resize(arg.value->numPostbuildInfoDescs);
    memcpy(arg.destPostBuildBufferOffsets.data(), src + offset,
           sizeof(unsigned) * arg.value->numPostbuildInfoDescs);
    offset += sizeof(unsigned) * arg.value->numPostbuildInfoDescs;
  }
}
void decode(char* src,
            unsigned& offset,
            PointerArgument<NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS>& arg) {
  if (decodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.value = reinterpret_cast<NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS*>(src + offset);
  offset += sizeof(NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS);

  if (arg.value->pDesc) {
    arg.value->pDesc =
        reinterpret_cast<NVAPI_D3D12_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC*>(src + offset);
    offset += sizeof(NVAPI_D3D12_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC);

    if (arg.value->pDesc->inputs.pOMMUsageCounts) {
      const_cast<NVAPI_D3D12_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC*>(arg.value->pDesc)
          ->inputs.pOMMUsageCounts =
          reinterpret_cast<NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_USAGE_COUNT*>(src + offset);
      offset += sizeof(NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_USAGE_COUNT) *
                arg.value->pDesc->inputs.numOMMUsageCounts;
    }
  }

  memcpy(&arg.destOpacityMicromapArrayDataKey, src + offset,
         sizeof(arg.destOpacityMicromapArrayDataKey));
  offset += sizeof(arg.destOpacityMicromapArrayDataKey);
  memcpy(&arg.destOpacityMicromapArrayDataOffset, src + offset,
         sizeof(arg.destOpacityMicromapArrayDataOffset));
  offset += sizeof(arg.destOpacityMicromapArrayDataOffset);

  memcpy(&arg.inputBufferKey, src + offset, sizeof(arg.inputBufferKey));
  offset += sizeof(arg.inputBufferKey);
  memcpy(&arg.inputBufferOffset, src + offset, sizeof(arg.inputBufferOffset));
  offset += sizeof(arg.inputBufferOffset);

  memcpy(&arg.perOMMDescsKey, src + offset, sizeof(arg.perOMMDescsKey));
  offset += sizeof(arg.perOMMDescsKey);
  memcpy(&arg.perOMMDescsOffset, src + offset, sizeof(arg.perOMMDescsOffset));
  offset += sizeof(arg.perOMMDescsOffset);

  memcpy(&arg.scratchOpacityMicromapArrayDataKey, src + offset,
         sizeof(arg.scratchOpacityMicromapArrayDataKey));
  offset += sizeof(arg.scratchOpacityMicromapArrayDataKey);
  memcpy(&arg.scratchOpacityMicromapArrayDataOffset, src + offset,
         sizeof(arg.scratchOpacityMicromapArrayDataOffset));
  offset += sizeof(arg.scratchOpacityMicromapArrayDataOffset);

  if (arg.value->pPostbuildInfoDescs) {
    arg.value->pPostbuildInfoDescs =
        reinterpret_cast<NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_POSTBUILD_INFO_DESC*>(
            src + offset);
    offset += sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC) *
              arg.value->numPostbuildInfoDescs;

    arg.destPostBuildBufferKeys.resize(arg.value->numPostbuildInfoDescs);
    memcpy(arg.destPostBuildBufferKeys.data(), src + offset,
           sizeof(unsigned) * arg.value->numPostbuildInfoDescs);
    offset += sizeof(unsigned) * arg.value->numPostbuildInfoDescs;

    arg.destPostBuildBufferOffsets.resize(arg.value->numPostbuildInfoDescs);
    memcpy(arg.destPostBuildBufferOffsets.data(), src + offset,
           sizeof(unsigned) * arg.value->numPostbuildInfoDescs);
    offset += sizeof(unsigned) * arg.value->numPostbuildInfoDescs;
  }
}

void decode(
    char* src,
    unsigned& offset,
    PointerArgument<NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS>& arg) {
  if (decodeNullPtr(src, offset, arg)) {
    return;
  }

  arg.value = reinterpret_cast<NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS*>(
      src + offset);
  offset += sizeof(NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS);
  if (arg.value->pDesc) {
    arg.value->pDesc =
        reinterpret_cast<NVAPI_D3D12_RAYTRACING_MULTI_INDIRECT_CLUSTER_OPERATION_DESC*>(src +
                                                                                        offset);
    offset += sizeof(NVAPI_D3D12_RAYTRACING_MULTI_INDIRECT_CLUSTER_OPERATION_DESC);
  }

  memcpy(&arg.batchResultDataKey, src + offset, sizeof(arg.batchResultDataKey));
  offset += sizeof(arg.batchResultDataKey);
  memcpy(&arg.batchResultDataOffset, src + offset, sizeof(arg.batchResultDataOffset));
  offset += sizeof(arg.batchResultDataOffset);

  memcpy(&arg.batchScratchDataKey, src + offset, sizeof(arg.batchScratchDataKey));
  offset += sizeof(arg.batchScratchDataKey);
  memcpy(&arg.batchScratchDataOffset, src + offset, sizeof(arg.batchScratchDataOffset));
  offset += sizeof(arg.batchScratchDataOffset);

  memcpy(&arg.destinationAddressArrayKey, src + offset, sizeof(arg.destinationAddressArrayKey));
  offset += sizeof(arg.destinationAddressArrayKey);
  memcpy(&arg.destinationAddressArrayOffset, src + offset,
         sizeof(arg.destinationAddressArrayOffset));
  offset += sizeof(arg.destinationAddressArrayOffset);

  memcpy(&arg.resultSizeArrayKey, src + offset, sizeof(arg.resultSizeArrayKey));
  offset += sizeof(arg.resultSizeArrayKey);
  memcpy(&arg.resultSizeArrayOffset, src + offset, sizeof(arg.resultSizeArrayOffset));
  offset += sizeof(arg.resultSizeArrayOffset);

  memcpy(&arg.indirectArgArrayKey, src + offset, sizeof(arg.indirectArgArrayKey));
  offset += sizeof(arg.indirectArgArrayKey);
  memcpy(&arg.indirectArgArrayOffset, src + offset, sizeof(arg.indirectArgArrayOffset));
  offset += sizeof(arg.indirectArgArrayOffset);

  memcpy(&arg.indirectArgCountKey, src + offset, sizeof(arg.indirectArgCountKey));
  offset += sizeof(arg.indirectArgCountKey);
  memcpy(&arg.indirectArgCountOffset, src + offset, sizeof(arg.indirectArgCountOffset));
  offset += sizeof(arg.indirectArgCountOffset);
}

void decode(char* src, unsigned& offset, xell_frame_report_t_Argument& arg) {
  if (decodeNullPtr(src, offset, arg)) {
    return;
  }
  arg.value = reinterpret_cast<xell_frame_report_t*>(src + offset);
  offset += sizeof(xell_frame_report_t) * arg.FRAME_REPORTS_COUNT;
}

void decode(char* src, unsigned& offset, xefg_swapchain_d3d12_init_params_t_Argument& arg) {
  if (decodeNullPtr(src, offset, arg)) {
    return;
  }
  arg.value = reinterpret_cast<xefg_swapchain_d3d12_init_params_t*>(src + offset);
  offset += sizeof(xefg_swapchain_d3d12_init_params_t);
  memcpy(&arg.key, src + offset, sizeof(arg.key));
  offset += sizeof(arg.key);
  memcpy(&arg.applicationSwapChainKey, src + offset, sizeof(arg.applicationSwapChainKey));
  offset += sizeof(arg.applicationSwapChainKey);
  memcpy(&arg.tempBufferHeapKey, src + offset, sizeof(arg.tempBufferHeapKey));
  offset += sizeof(arg.tempBufferHeapKey);
  memcpy(&arg.tempTextureHeapKey, src + offset, sizeof(arg.tempTextureHeapKey));
  offset += sizeof(arg.tempTextureHeapKey);
  memcpy(&arg.pipelineLibraryKey, src + offset, sizeof(arg.pipelineLibraryKey));
  offset += sizeof(arg.pipelineLibraryKey);
}

void decode(char* src, unsigned& offset, xefg_swapchain_d3d12_resource_data_t_Argument& arg) {
  if (decodeNullPtr(src, offset, arg)) {
    return;
  }
  arg.value = reinterpret_cast<xefg_swapchain_d3d12_resource_data_t*>(src + offset);
  offset += sizeof(xefg_swapchain_d3d12_resource_data_t);
  memcpy(&arg.resourceKey, src + offset, sizeof(arg.resourceKey));
  offset += sizeof(arg.resourceKey);
}

} // namespace DirectX
} // namespace gits
