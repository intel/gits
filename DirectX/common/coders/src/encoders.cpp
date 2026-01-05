// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "argumentEncoders.h"
#include "intelExtensions.h"
#include "gits.h"

#include <d3dx12_pipeline_state_stream.h>

namespace gits {
namespace DirectX {

unsigned getSize(const BufferArgument& arg) {
  if (!arg.value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(arg.size) + arg.size;
}

void encode(char* dest, unsigned& offset, const BufferArgument& arg) {
  if (encodeNullPtr(dest, offset, arg)) {
    return;
  }
  memcpy(dest + offset, &arg.size, sizeof(arg.size));
  offset += sizeof(arg.size);
  memcpy(dest + offset, arg.value, arg.size);
  offset += arg.size;
}

unsigned getSize(const OutputBufferArgument& arg) {
  return sizeof(void*);
}

void encode(char* dest, unsigned& offset, const OutputBufferArgument& arg) {
  memcpy(dest + offset, arg.captureValue ? &arg.captureValue : arg.value, sizeof(void*));
  offset += sizeof(void*);
}

unsigned getSize(const LPCWSTR_Argument& arg) {
  if (!arg.value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(unsigned) + wcslen(arg.value) * 2 + 2;
}

void encode(char* dest, unsigned& offset, const LPCWSTR_Argument& arg) {
  if (encodeNullPtr(dest, offset, arg)) {
    return;
  }
  unsigned len = wcslen(arg.value) * 2 + 2;
  memcpy(dest + offset, &len, sizeof(len));
  offset += sizeof(unsigned);
  memcpy(dest + offset, arg.value, len);
  offset += len;
}

unsigned getSize(const D3D12_GPU_VIRTUAL_ADDRESS_Argument& arg) {
  return sizeof(arg.value) + sizeof(arg.interfaceKey) + sizeof(arg.offset);
}

void encode(char* dest, unsigned& offset, const D3D12_GPU_VIRTUAL_ADDRESS_Argument& arg) {
  memcpy(dest + offset, &arg.value, sizeof(arg.value));
  offset += sizeof(arg.value);
  memcpy(dest + offset, &arg.interfaceKey, sizeof(arg.interfaceKey));
  offset += sizeof(arg.interfaceKey);
  memcpy(dest + offset, &arg.offset, sizeof(arg.offset));
  offset += sizeof(arg.offset);
}

unsigned getSize(const D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument& arg) {
  if (!arg.value) {
    return sizeof(void*);
  }
  unsigned size = sizeof(void*) + sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC) +
                  arg.value->VS.BytecodeLength + arg.value->PS.BytecodeLength +
                  arg.value->DS.BytecodeLength + arg.value->HS.BytecodeLength +
                  arg.value->GS.BytecodeLength;

  size += arg.value->StreamOutput.NumEntries * sizeof(D3D12_SO_DECLARATION_ENTRY);
  for (unsigned i = 0; i < arg.value->StreamOutput.NumEntries; ++i) {
    size += sizeof(unsigned) + strlen(arg.value->StreamOutput.pSODeclaration[i].SemanticName) + 1;
  }
  size += arg.value->StreamOutput.NumStrides * sizeof(UINT);

  size += sizeof(D3D12_INPUT_ELEMENT_DESC) * arg.value->InputLayout.NumElements;
  for (unsigned i = 0; i < arg.value->InputLayout.NumElements; ++i) {
    size +=
        sizeof(unsigned) + strlen(arg.value->InputLayout.pInputElementDescs[i].SemanticName) + 1;
  }
  size += arg.value->CachedPSO.CachedBlobSizeInBytes;
  size += sizeof(arg.rootSignatureKey);
  return size;
}

void encode(char* dest, unsigned& offset, const D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument& arg) {
  if (encodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.value, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
  offset += sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC);

  auto encodeBytecode = [&](D3D12_SHADER_BYTECODE& bytecode) {
    if (bytecode.pShaderBytecode) {
      memcpy(dest + offset, bytecode.pShaderBytecode, bytecode.BytecodeLength);
      offset += bytecode.BytecodeLength;
    }
  };
  encodeBytecode(arg.value->VS);
  encodeBytecode(arg.value->PS);
  encodeBytecode(arg.value->DS);
  encodeBytecode(arg.value->HS);
  encodeBytecode(arg.value->GS);

  if (arg.value->StreamOutput.pSODeclaration) {
    memcpy(dest + offset, arg.value->StreamOutput.pSODeclaration,
           arg.value->StreamOutput.NumEntries * sizeof(D3D12_SO_DECLARATION_ENTRY));
    offset += arg.value->StreamOutput.NumEntries * sizeof(D3D12_SO_DECLARATION_ENTRY);

    for (unsigned i = 0; i < arg.value->StreamOutput.NumEntries; ++i) {
      unsigned len = strlen(arg.value->StreamOutput.pSODeclaration[i].SemanticName) + 1;
      memcpy(dest + offset, &len, sizeof(len));
      offset += sizeof(len);
      memcpy(dest + offset, arg.value->StreamOutput.pSODeclaration[i].SemanticName, len);
      offset += len;
    }
  }
  if (arg.value->StreamOutput.pBufferStrides) {
    memcpy(dest + offset, arg.value->StreamOutput.pBufferStrides,
           arg.value->StreamOutput.NumStrides * sizeof(UINT));
    offset += arg.value->StreamOutput.NumStrides * sizeof(UINT);
  }

  memcpy(dest + offset, arg.value->InputLayout.pInputElementDescs,
         sizeof(D3D12_INPUT_ELEMENT_DESC) * arg.value->InputLayout.NumElements);
  offset += sizeof(D3D12_INPUT_ELEMENT_DESC) * arg.value->InputLayout.NumElements;

  for (unsigned i = 0; i < arg.value->InputLayout.NumElements; ++i) {
    const D3D12_INPUT_ELEMENT_DESC& inputElement = arg.value->InputLayout.pInputElementDescs[i];
    unsigned len = strlen(inputElement.SemanticName) + 1;
    memcpy(dest + offset, &len, sizeof(len));
    offset += sizeof(len);
    memcpy(dest + offset, inputElement.SemanticName, len);
    offset += len;
  }

  if (arg.value->CachedPSO.pCachedBlob) {
    memcpy(dest + offset, arg.value->CachedPSO.pCachedBlob,
           arg.value->CachedPSO.CachedBlobSizeInBytes);
    offset += arg.value->CachedPSO.CachedBlobSizeInBytes;
  }

  memcpy(dest + offset, &arg.rootSignatureKey, sizeof(arg.rootSignatureKey));
  offset += sizeof(arg.rootSignatureKey);
}

unsigned getSize(const D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument& arg) {
  if (!arg.value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC) + arg.value->CS.BytecodeLength +
         arg.value->CachedPSO.CachedBlobSizeInBytes + sizeof(arg.rootSignatureKey);
}

void encode(char* dest, unsigned& offset, const D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument& arg) {
  if (encodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.value, sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC));
  offset += sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC);

  if (arg.value->CS.pShaderBytecode) {
    memcpy(dest + offset, arg.value->CS.pShaderBytecode, arg.value->CS.BytecodeLength);
    offset += arg.value->CS.BytecodeLength;
  }

  if (arg.value->CachedPSO.pCachedBlob) {
    memcpy(dest + offset, arg.value->CachedPSO.pCachedBlob,
           arg.value->CachedPSO.CachedBlobSizeInBytes);
    offset += arg.value->CachedPSO.CachedBlobSizeInBytes;
  }

  memcpy(dest + offset, &arg.rootSignatureKey, sizeof(arg.rootSignatureKey));
  offset += sizeof(arg.rootSignatureKey);
}

unsigned getSize(const D3D12_TEXTURE_COPY_LOCATION_Argument& arg) {
  return sizeof(D3D12_TEXTURE_COPY_LOCATION) + sizeof(arg.resourceKey);
}

void encode(char* dest, unsigned& offset, const D3D12_TEXTURE_COPY_LOCATION_Argument& arg) {

  memcpy(dest + offset, arg.value, sizeof(D3D12_TEXTURE_COPY_LOCATION));
  offset += sizeof(D3D12_TEXTURE_COPY_LOCATION);

  memcpy(dest + offset, &arg.resourceKey, sizeof(arg.resourceKey));
  offset += sizeof(arg.resourceKey);
}

unsigned getSize(const D3D12_RESOURCE_BARRIERs_Argument& arg) {
  if (!arg.value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(arg.size) + sizeof(D3D12_RESOURCE_BARRIER) * arg.size +
         sizeof(unsigned) * arg.size * 2;
}

void encode(char* dest, unsigned& offset, const D3D12_RESOURCE_BARRIERs_Argument& arg) {
  if (encodeNullPtr(dest, offset, arg)) {
    return;
  }
  memcpy(dest + offset, &arg.size, sizeof(arg.size));
  offset += sizeof(arg.size);

  memcpy(dest + offset, arg.value, sizeof(D3D12_RESOURCE_BARRIER) * arg.size);
  offset += sizeof(D3D12_RESOURCE_BARRIER) * arg.size;

  memcpy(dest + offset, arg.resourceKeys.data(), sizeof(unsigned) * arg.size);
  offset += sizeof(unsigned) * arg.size;

  memcpy(dest + offset, arg.resourceAfterKeys.data(), sizeof(unsigned) * arg.size);
  offset += sizeof(unsigned) * arg.size;
}

unsigned getSize(const PointerArgument<D3D12_ROOT_SIGNATURE_DESC>& arg) {
  if (!arg.value) {
    return sizeof(void*);
  }
  unsigned size = sizeof(void*) + sizeof(D3D12_ROOT_SIGNATURE_DESC);

  size += arg.value->NumStaticSamplers * sizeof(D3D12_STATIC_SAMPLER_DESC);
  size += arg.value->NumParameters * sizeof(D3D12_ROOT_PARAMETER);
  for (unsigned i = 0; i < arg.value->NumParameters; ++i) {
    if (arg.value->pParameters[i].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
      size += arg.value->pParameters[i].DescriptorTable.NumDescriptorRanges *
              sizeof(D3D12_DESCRIPTOR_RANGE);
    }
  }
  return size;
}

void encode(char* dest, unsigned& offset, const PointerArgument<D3D12_ROOT_SIGNATURE_DESC>& arg) {
  if (encodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.value, sizeof(D3D12_ROOT_SIGNATURE_DESC));
  offset += sizeof(D3D12_ROOT_SIGNATURE_DESC);

  memcpy(dest + offset, arg.value->pStaticSamplers,
         sizeof(D3D12_STATIC_SAMPLER_DESC) * arg.value->NumStaticSamplers);
  offset += sizeof(D3D12_STATIC_SAMPLER_DESC) * arg.value->NumStaticSamplers;

  memcpy(dest + offset, arg.value->pParameters,
         sizeof(D3D12_ROOT_PARAMETER) * arg.value->NumParameters);
  offset += sizeof(D3D12_ROOT_PARAMETER) * arg.value->NumParameters;
  for (unsigned i = 0; i < arg.value->NumParameters; ++i) {
    if (arg.value->pParameters[i].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {

      memcpy(dest + offset, arg.value->pParameters[i].DescriptorTable.pDescriptorRanges,
             sizeof(D3D12_DESCRIPTOR_RANGE) *
                 arg.value->pParameters[i].DescriptorTable.NumDescriptorRanges);
      offset += sizeof(D3D12_DESCRIPTOR_RANGE) *
                arg.value->pParameters[i].DescriptorTable.NumDescriptorRanges;
    }
  }
}

unsigned getSize(const PointerArgument<D3D12_VERSIONED_ROOT_SIGNATURE_DESC>& arg) {
  if (!arg.value) {
    return sizeof(void*);
  }
  unsigned size = sizeof(void*) + sizeof(D3D12_VERSIONED_ROOT_SIGNATURE_DESC);

  switch (arg.value->Version) {
  case D3D_ROOT_SIGNATURE_VERSION_1_0: {
    D3D12_ROOT_SIGNATURE_DESC& desc0 = arg.value->Desc_1_0;
    size += desc0.NumStaticSamplers * sizeof(D3D12_STATIC_SAMPLER_DESC);
    size += desc0.NumParameters * sizeof(D3D12_ROOT_PARAMETER);
    for (unsigned i = 0; i < desc0.NumParameters; ++i) {
      if (desc0.pParameters[i].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
        size += desc0.pParameters[i].DescriptorTable.NumDescriptorRanges *
                sizeof(D3D12_DESCRIPTOR_RANGE);
      }
    }
  } break;
  case D3D_ROOT_SIGNATURE_VERSION_1_1: {
    D3D12_ROOT_SIGNATURE_DESC1& desc1 = arg.value->Desc_1_1;
    size += desc1.NumStaticSamplers * sizeof(D3D12_STATIC_SAMPLER_DESC);
    size += desc1.NumParameters * sizeof(D3D12_ROOT_PARAMETER1);
    for (unsigned i = 0; i < desc1.NumParameters; ++i) {
      if (desc1.pParameters[i].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
        size += desc1.pParameters[i].DescriptorTable.NumDescriptorRanges *
                sizeof(D3D12_DESCRIPTOR_RANGE1);
      }
    }
  } break;
  case D3D_ROOT_SIGNATURE_VERSION_1_2: {
    D3D12_ROOT_SIGNATURE_DESC2& desc2 = arg.value->Desc_1_2;
    size += desc2.NumStaticSamplers * sizeof(D3D12_STATIC_SAMPLER_DESC1);
    size += desc2.NumParameters * sizeof(D3D12_ROOT_PARAMETER1);
    for (unsigned i = 0; i < desc2.NumParameters; ++i) {
      if (desc2.pParameters[i].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
        size += desc2.pParameters[i].DescriptorTable.NumDescriptorRanges *
                sizeof(D3D12_DESCRIPTOR_RANGE1);
      }
    }
  } break;
  }
  return size;
}

void encode(char* dest,
            unsigned& offset,
            const PointerArgument<D3D12_VERSIONED_ROOT_SIGNATURE_DESC>& arg) {
  if (encodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.value, sizeof(D3D12_VERSIONED_ROOT_SIGNATURE_DESC));
  offset += sizeof(D3D12_VERSIONED_ROOT_SIGNATURE_DESC);

  switch (arg.value->Version) {
  case D3D_ROOT_SIGNATURE_VERSION_1_0: {
    D3D12_ROOT_SIGNATURE_DESC& desc0 = arg.value->Desc_1_0;

    memcpy(dest + offset, desc0.pStaticSamplers,
           sizeof(D3D12_STATIC_SAMPLER_DESC) * desc0.NumStaticSamplers);
    offset += sizeof(D3D12_STATIC_SAMPLER_DESC) * desc0.NumStaticSamplers;

    memcpy(dest + offset, desc0.pParameters, sizeof(D3D12_ROOT_PARAMETER) * desc0.NumParameters);
    offset += sizeof(D3D12_ROOT_PARAMETER) * desc0.NumParameters;
    for (unsigned i = 0; i < desc0.NumParameters; ++i) {
      if (desc0.pParameters[i].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {

        memcpy(dest + offset, desc0.pParameters[i].DescriptorTable.pDescriptorRanges,
               sizeof(D3D12_DESCRIPTOR_RANGE) *
                   desc0.pParameters[i].DescriptorTable.NumDescriptorRanges);
        offset += sizeof(D3D12_DESCRIPTOR_RANGE) *
                  desc0.pParameters[i].DescriptorTable.NumDescriptorRanges;
      }
    }
  } break;
  case D3D_ROOT_SIGNATURE_VERSION_1_1: {
    D3D12_ROOT_SIGNATURE_DESC1& desc1 = arg.value->Desc_1_1;

    memcpy(dest + offset, desc1.pStaticSamplers,
           sizeof(D3D12_STATIC_SAMPLER_DESC) * desc1.NumStaticSamplers);
    offset += sizeof(D3D12_STATIC_SAMPLER_DESC) * desc1.NumStaticSamplers;

    memcpy(dest + offset, desc1.pParameters, sizeof(D3D12_ROOT_PARAMETER1) * desc1.NumParameters);
    offset += sizeof(D3D12_ROOT_PARAMETER1) * desc1.NumParameters;
    for (unsigned i = 0; i < desc1.NumParameters; ++i) {
      if (desc1.pParameters[i].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {

        memcpy(dest + offset, desc1.pParameters[i].DescriptorTable.pDescriptorRanges,
               sizeof(D3D12_DESCRIPTOR_RANGE1) *
                   desc1.pParameters[i].DescriptorTable.NumDescriptorRanges);
        offset += sizeof(D3D12_DESCRIPTOR_RANGE1) *
                  desc1.pParameters[i].DescriptorTable.NumDescriptorRanges;
      }
    }
  } break;
  case D3D_ROOT_SIGNATURE_VERSION_1_2: {
    D3D12_ROOT_SIGNATURE_DESC2& desc2 = arg.value->Desc_1_2;

    memcpy(dest + offset, desc2.pStaticSamplers,
           sizeof(D3D12_STATIC_SAMPLER_DESC1) * desc2.NumStaticSamplers);
    offset += sizeof(D3D12_STATIC_SAMPLER_DESC1) * desc2.NumStaticSamplers;

    memcpy(dest + offset, desc2.pParameters, sizeof(D3D12_ROOT_PARAMETER1) * desc2.NumParameters);
    offset += sizeof(D3D12_ROOT_PARAMETER1) * desc2.NumParameters;
    for (unsigned i = 0; i < desc2.NumParameters; ++i) {
      if (desc2.pParameters[i].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {

        memcpy(dest + offset, desc2.pParameters[i].DescriptorTable.pDescriptorRanges,
               sizeof(D3D12_DESCRIPTOR_RANGE1) *
                   desc2.pParameters[i].DescriptorTable.NumDescriptorRanges);
        offset += sizeof(D3D12_DESCRIPTOR_RANGE1) *
                  desc2.pParameters[i].DescriptorTable.NumDescriptorRanges;
      }
    }
  } break;
  }
}

unsigned getSize(const PointerArgument<D3D12_COMMAND_SIGNATURE_DESC>& arg) {
  if (!arg.value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(D3D12_COMMAND_SIGNATURE_DESC) * arg.value->NumArgumentDescs *
                             sizeof(D3D12_INDIRECT_ARGUMENT_DESC);
}

void encode(char* dest,
            unsigned& offset,
            const PointerArgument<D3D12_COMMAND_SIGNATURE_DESC>& arg) {
  if (encodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.value, sizeof(D3D12_COMMAND_SIGNATURE_DESC));
  offset += sizeof(D3D12_COMMAND_SIGNATURE_DESC);

  memcpy(dest + offset, arg.value->pArgumentDescs,
         sizeof(D3D12_INDIRECT_ARGUMENT_DESC) * arg.value->NumArgumentDescs);
  offset += sizeof(D3D12_INDIRECT_ARGUMENT_DESC) * arg.value->NumArgumentDescs;
}

unsigned getSize(const D3D12_INDEX_BUFFER_VIEW_Argument& arg) {
  if (!arg.value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(D3D12_INDEX_BUFFER_VIEW) + sizeof(unsigned) * 2;
}

void encode(char* dest, unsigned& offset, const D3D12_INDEX_BUFFER_VIEW_Argument& arg) {
  if (encodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.value, sizeof(D3D12_INDEX_BUFFER_VIEW));
  offset += sizeof(D3D12_INDEX_BUFFER_VIEW);

  memcpy(dest + offset, &arg.bufferLocationKey, sizeof(arg.bufferLocationKey));
  offset += sizeof(arg.bufferLocationKey);
  memcpy(dest + offset, &arg.bufferLocationOffset, sizeof(arg.bufferLocationOffset));
  offset += sizeof(arg.bufferLocationOffset);
}

unsigned getSize(const D3D12_CONSTANT_BUFFER_VIEW_DESC_Argument& arg) {
  if (!arg.value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(D3D12_CONSTANT_BUFFER_VIEW_DESC) + sizeof(unsigned) * 2;
}

void encode(char* dest, unsigned& offset, const D3D12_CONSTANT_BUFFER_VIEW_DESC_Argument& arg) {
  if (encodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.value, sizeof(D3D12_CONSTANT_BUFFER_VIEW_DESC));
  offset += sizeof(D3D12_CONSTANT_BUFFER_VIEW_DESC);

  memcpy(dest + offset, &arg.bufferLocationKey, sizeof(arg.bufferLocationKey));
  offset += sizeof(arg.bufferLocationKey);
  memcpy(dest + offset, &arg.bufferLocationOffset, sizeof(arg.bufferLocationOffset));
  offset += sizeof(arg.bufferLocationOffset);
}

unsigned getSize(const D3D12_VERTEX_BUFFER_VIEWs_Argument& arg) {
  if (!arg.value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(arg.size) + sizeof(D3D12_VERTEX_BUFFER_VIEW) * arg.size +
         sizeof(unsigned) * arg.size * 2;
}

void encode(char* dest, unsigned& offset, const D3D12_VERTEX_BUFFER_VIEWs_Argument& arg) {
  if (encodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, &arg.size, sizeof(arg.size));
  offset += sizeof(arg.size);

  memcpy(dest + offset, arg.value, sizeof(D3D12_VERTEX_BUFFER_VIEW) * arg.size);
  offset += sizeof(D3D12_VERTEX_BUFFER_VIEW) * arg.size;

  memcpy(dest + offset, arg.bufferLocationKeys.data(), sizeof(unsigned) * arg.size);
  offset += sizeof(unsigned) * arg.size;

  memcpy(dest + offset, arg.bufferLocationOffsets.data(), sizeof(unsigned) * arg.size);
  offset += sizeof(unsigned) * arg.size;
}

unsigned getSize(const D3D12_STREAM_OUTPUT_BUFFER_VIEWs_Argument& arg) {
  if (!arg.value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(arg.size) + sizeof(D3D12_STREAM_OUTPUT_BUFFER_VIEW) * arg.size +
         sizeof(unsigned) * arg.size * 4;
}

void encode(char* dest, unsigned& offset, const D3D12_STREAM_OUTPUT_BUFFER_VIEWs_Argument& arg) {
  if (encodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, &arg.size, sizeof(arg.size));
  offset += sizeof(arg.size);

  memcpy(dest + offset, arg.value, sizeof(D3D12_STREAM_OUTPUT_BUFFER_VIEW) * arg.size);
  offset += sizeof(D3D12_STREAM_OUTPUT_BUFFER_VIEW) * arg.size;

  memcpy(dest + offset, arg.bufferLocationKeys.data(), sizeof(unsigned) * arg.size);
  offset += sizeof(unsigned) * arg.size;

  memcpy(dest + offset, arg.bufferLocationOffsets.data(), sizeof(unsigned) * arg.size);
  offset += sizeof(unsigned) * arg.size;

  memcpy(dest + offset, arg.bufferFilledSizeLocationKeys.data(), sizeof(unsigned) * arg.size);
  offset += sizeof(unsigned) * arg.size;

  memcpy(dest + offset, arg.bufferFilledSizeLocationOffsets.data(), sizeof(unsigned) * arg.size);
  offset += sizeof(unsigned) * arg.size;
}

unsigned getSize(const D3D12_WRITEBUFFERIMMEDIATE_PARAMETERs_Argument& arg) {
  if (!arg.value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(arg.size) +
         sizeof(D3D12_WRITEBUFFERIMMEDIATE_PARAMETER) * arg.size + sizeof(unsigned) * arg.size * 2;
}

void encode(char* dest,
            unsigned& offset,
            const D3D12_WRITEBUFFERIMMEDIATE_PARAMETERs_Argument& arg) {
  if (encodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, &arg.size, sizeof(arg.size));
  offset += sizeof(arg.size);

  memcpy(dest + offset, arg.value, sizeof(D3D12_WRITEBUFFERIMMEDIATE_PARAMETER) * arg.size);
  offset += sizeof(D3D12_WRITEBUFFERIMMEDIATE_PARAMETER) * arg.size;

  memcpy(dest + offset, arg.destKeys.data(), sizeof(unsigned) * arg.size);
  offset += sizeof(unsigned) * arg.size;

  memcpy(dest + offset, arg.destOffsets.data(), sizeof(unsigned) * arg.size);
  offset += sizeof(unsigned) * arg.size;
}

unsigned getSize(const D3D12_STATE_OBJECT_DESC_Argument& arg) {
  if (!arg.value) {
    return sizeof(void*);
  }
  unsigned size = sizeof(void*) + sizeof(D3D12_STATE_OBJECT_DESC) +
                  sizeof(D3D12_STATE_SUBOBJECT) * arg.value->NumSubobjects;

  unsigned associationsCount = 0;

  for (unsigned index = 0; index < arg.value->NumSubobjects; ++index) {
    const D3D12_STATE_SUBOBJECT& subobject = arg.value->pSubobjects[index];

    switch (subobject.Type) {
    case D3D12_STATE_SUBOBJECT_TYPE_STATE_OBJECT_CONFIG: {
      size += sizeof(D3D12_STATE_OBJECT_CONFIG);
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE: {
      size += sizeof(D3D12_GLOBAL_ROOT_SIGNATURE);
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE: {
      size += sizeof(D3D12_LOCAL_ROOT_SIGNATURE);
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_NODE_MASK: {
      size += sizeof(D3D12_NODE_MASK);
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY: {
      const D3D12_DXIL_LIBRARY_DESC* desc =
          static_cast<const D3D12_DXIL_LIBRARY_DESC*>(subobject.pDesc);
      size += sizeof(D3D12_DXIL_LIBRARY_DESC);
      size += desc->DXILLibrary.BytecodeLength;
      size += sizeof(D3D12_EXPORT_DESC) * desc->NumExports;
      for (unsigned i = 0; i < desc->NumExports; ++i) {
        size += sizeof(unsigned) + wcslen(desc->pExports[i].Name) * 2 + 2;
        if (desc->pExports[i].ExportToRename) {
          size += sizeof(unsigned) + wcslen(desc->pExports[i].ExportToRename) * 2 + 2;
        }
      }
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION: {
      const D3D12_EXISTING_COLLECTION_DESC* desc =
          static_cast<const D3D12_EXISTING_COLLECTION_DESC*>(subobject.pDesc);
      size += sizeof(D3D12_EXISTING_COLLECTION_DESC);
      size += sizeof(D3D12_EXPORT_DESC) * desc->NumExports;
      for (unsigned i = 0; i < desc->NumExports; ++i) {
        size += sizeof(unsigned) + wcslen(desc->pExports[i].Name) * 2 + 2;
        if (desc->pExports[i].ExportToRename) {
          size += sizeof(unsigned) + wcslen(desc->pExports[i].ExportToRename) * 2 + 2;
        }
      }
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION: {
      const D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION* desc =
          static_cast<const D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(subobject.pDesc);
      size += sizeof(D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION);
      size += sizeof(LPCWSTR) * desc->NumExports;
      for (unsigned i = 0; i < desc->NumExports; ++i) {
        size += sizeof(unsigned) + wcslen(desc->pExports[i]) * 2 + 2;
      }
      ++associationsCount;
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION: {
      const D3D12_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION* desc =
          static_cast<const D3D12_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(subobject.pDesc);
      size += sizeof(D3D12_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION);
      size += sizeof(unsigned) + wcslen(desc->SubobjectToAssociate) * 2 + 2;
      size += sizeof(LPCWSTR) * desc->NumExports;
      for (unsigned i = 0; i < desc->NumExports; ++i) {
        size += sizeof(unsigned) + wcslen(desc->pExports[i]) * 2 + 2;
      }
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG: {
      size += sizeof(D3D12_RAYTRACING_SHADER_CONFIG);
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG: {
      size += sizeof(D3D12_RAYTRACING_PIPELINE_CONFIG);
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP: {
      const D3D12_HIT_GROUP_DESC* desc = static_cast<const D3D12_HIT_GROUP_DESC*>(subobject.pDesc);
      size += sizeof(D3D12_HIT_GROUP_DESC);
      size += sizeof(unsigned) + wcslen(desc->HitGroupExport) * 2 + 2;
      if (desc->AnyHitShaderImport) {
        size += sizeof(unsigned) + wcslen(desc->AnyHitShaderImport) * 2 + 2;
      }
      if (desc->ClosestHitShaderImport) {
        size += sizeof(unsigned) + wcslen(desc->ClosestHitShaderImport) * 2 + 2;
      }
      if (desc->IntersectionShaderImport) {
        size += sizeof(unsigned) + wcslen(desc->IntersectionShaderImport) * 2 + 2;
      }
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG1: {
      size += sizeof(D3D12_RAYTRACING_PIPELINE_CONFIG1);
    } break;
    }
  }

  size += sizeof(unsigned) + associationsCount * sizeof(unsigned) * 2;

  size += sizeof(unsigned) + arg.interfaceKeysBySubobject.size() * sizeof(unsigned) * 2;

  return size;
}

void encode(char* dest, unsigned& offset, const D3D12_STATE_OBJECT_DESC_Argument& arg) {
  if (encodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.value, sizeof(D3D12_STATE_OBJECT_DESC));
  offset += sizeof(D3D12_STATE_OBJECT_DESC);

  memcpy(dest + offset, arg.value->pSubobjects,
         sizeof(D3D12_STATE_SUBOBJECT) * arg.value->NumSubobjects);
  offset += sizeof(D3D12_STATE_SUBOBJECT) * arg.value->NumSubobjects;

  std::map<const D3D12_STATE_SUBOBJECT*, unsigned> subobjectIndexes;

  for (unsigned index = 0; index < arg.value->NumSubobjects; ++index) {
    const D3D12_STATE_SUBOBJECT& subobject = arg.value->pSubobjects[index];

    subobjectIndexes[&subobject] = index;

    switch (subobject.Type) {
    case D3D12_STATE_SUBOBJECT_TYPE_STATE_OBJECT_CONFIG: {
      memcpy(dest + offset, subobject.pDesc, sizeof(D3D12_STATE_OBJECT_CONFIG));
      offset += sizeof(D3D12_STATE_OBJECT_CONFIG);
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE: {
      memcpy(dest + offset, subobject.pDesc, sizeof(D3D12_GLOBAL_ROOT_SIGNATURE));
      offset += sizeof(D3D12_GLOBAL_ROOT_SIGNATURE);
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE: {
      memcpy(dest + offset, subobject.pDesc, sizeof(D3D12_LOCAL_ROOT_SIGNATURE));
      offset += sizeof(D3D12_LOCAL_ROOT_SIGNATURE);
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_NODE_MASK: {
      memcpy(dest + offset, subobject.pDesc, sizeof(D3D12_NODE_MASK));
      offset += sizeof(D3D12_NODE_MASK);
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY: {
      const D3D12_DXIL_LIBRARY_DESC* desc =
          static_cast<const D3D12_DXIL_LIBRARY_DESC*>(subobject.pDesc);

      memcpy(dest + offset, subobject.pDesc, sizeof(D3D12_DXIL_LIBRARY_DESC));
      offset += sizeof(D3D12_DXIL_LIBRARY_DESC);

      memcpy(dest + offset, desc->DXILLibrary.pShaderBytecode, desc->DXILLibrary.BytecodeLength);
      offset += desc->DXILLibrary.BytecodeLength;

      memcpy(dest + offset, desc->pExports, sizeof(D3D12_EXPORT_DESC) * desc->NumExports);
      offset += sizeof(D3D12_EXPORT_DESC) * desc->NumExports;

      for (unsigned i = 0; i < desc->NumExports; ++i) {
        unsigned len = wcslen(desc->pExports[i].Name) * 2 + 2;
        memcpy(dest + offset, &len, sizeof(len));
        offset += sizeof(unsigned);
        memcpy(dest + offset, desc->pExports[i].Name, len);
        offset += len;

        if (desc->pExports[i].ExportToRename) {
          unsigned len = wcslen(desc->pExports[i].ExportToRename) * 2 + 2;
          memcpy(dest + offset, &len, sizeof(len));
          offset += sizeof(unsigned);
          memcpy(dest + offset, desc->pExports[i].ExportToRename, len);
          offset += len;
        }
      }
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION: {
      const D3D12_EXISTING_COLLECTION_DESC* desc =
          static_cast<const D3D12_EXISTING_COLLECTION_DESC*>(subobject.pDesc);

      memcpy(dest + offset, subobject.pDesc, sizeof(D3D12_EXISTING_COLLECTION_DESC));
      offset += sizeof(D3D12_EXISTING_COLLECTION_DESC);

      memcpy(dest + offset, desc->pExports, sizeof(D3D12_EXPORT_DESC) * desc->NumExports);
      offset += sizeof(D3D12_EXPORT_DESC) * desc->NumExports;

      for (unsigned i = 0; i < desc->NumExports; ++i) {
        unsigned len = wcslen(desc->pExports[i].Name) * 2 + 2;
        memcpy(dest + offset, &len, sizeof(len));
        offset += sizeof(unsigned);
        memcpy(dest + offset, desc->pExports[i].Name, len);
        offset += len;

        if (desc->pExports[i].ExportToRename) {
          unsigned len = wcslen(desc->pExports[i].ExportToRename) * 2 + 2;
          memcpy(dest + offset, &len, sizeof(len));
          offset += sizeof(len);
          memcpy(dest + offset, desc->pExports[i].ExportToRename, len);
          offset += len;
        }
      }
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION: {
      const D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION* desc =
          static_cast<const D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(subobject.pDesc);

      memcpy(dest + offset, subobject.pDesc, sizeof(D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION));
      offset += sizeof(D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION);

      memcpy(dest + offset, desc->pExports, sizeof(LPCWSTR) * desc->NumExports);
      offset += sizeof(LPCWSTR) * desc->NumExports;

      for (unsigned i = 0; i < desc->NumExports; ++i) {
        unsigned len = wcslen(desc->pExports[i]) * 2 + 2;
        memcpy(dest + offset, &len, sizeof(len));
        offset += sizeof(len);
        memcpy(dest + offset, desc->pExports[i], len);
        offset += len;
      }
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION: {
      const D3D12_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION* desc =
          static_cast<const D3D12_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(subobject.pDesc);

      memcpy(dest + offset, subobject.pDesc, sizeof(D3D12_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION));
      offset += sizeof(D3D12_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION);

      unsigned len = wcslen(desc->SubobjectToAssociate) * 2 + 2;
      memcpy(dest + offset, &len, sizeof(len));
      offset += sizeof(len);
      memcpy(dest + offset, desc->SubobjectToAssociate, len);
      offset += len;

      memcpy(dest + offset, desc->pExports, sizeof(LPCWSTR) * desc->NumExports);
      offset += sizeof(LPCWSTR) * desc->NumExports;

      for (unsigned i = 0; i < desc->NumExports; ++i) {
        unsigned len = wcslen(desc->pExports[i]) * 2 + 2;
        memcpy(dest + offset, &len, sizeof(len));
        offset += sizeof(len);
        memcpy(dest + offset, desc->pExports[i], len);
        offset += len;
      }
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG: {
      memcpy(dest + offset, subobject.pDesc, sizeof(D3D12_RAYTRACING_SHADER_CONFIG));
      offset += sizeof(D3D12_RAYTRACING_SHADER_CONFIG);
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG: {
      memcpy(dest + offset, subobject.pDesc, sizeof(D3D12_RAYTRACING_PIPELINE_CONFIG));
      offset += sizeof(D3D12_RAYTRACING_PIPELINE_CONFIG);
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP: {
      const D3D12_HIT_GROUP_DESC* desc = static_cast<const D3D12_HIT_GROUP_DESC*>(subobject.pDesc);

      memcpy(dest + offset, subobject.pDesc, sizeof(D3D12_HIT_GROUP_DESC));
      offset += sizeof(D3D12_HIT_GROUP_DESC);

      unsigned len = wcslen(desc->HitGroupExport) * 2 + 2;
      memcpy(dest + offset, &len, sizeof(len));
      offset += sizeof(len);
      memcpy(dest + offset, desc->HitGroupExport, len);
      offset += len;

      if (desc->AnyHitShaderImport) {
        unsigned len = wcslen(desc->AnyHitShaderImport) * 2 + 2;
        memcpy(dest + offset, &len, sizeof(len));
        offset += sizeof(len);
        memcpy(dest + offset, desc->AnyHitShaderImport, len);
        offset += len;
      }
      if (desc->ClosestHitShaderImport) {
        unsigned len = wcslen(desc->ClosestHitShaderImport) * 2 + 2;
        memcpy(dest + offset, &len, sizeof(len));
        offset += sizeof(len);
        memcpy(dest + offset, desc->ClosestHitShaderImport, len);
        offset += len;
      }
      if (desc->IntersectionShaderImport) {
        unsigned len = wcslen(desc->IntersectionShaderImport) * 2 + 2;
        memcpy(dest + offset, &len, sizeof(len));
        offset += sizeof(len);
        memcpy(dest + offset, desc->IntersectionShaderImport, len);
        offset += len;
      }
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG1: {
      memcpy(dest + offset, subobject.pDesc, sizeof(D3D12_RAYTRACING_PIPELINE_CONFIG1));
      offset += sizeof(D3D12_RAYTRACING_PIPELINE_CONFIG1);
    } break;
    }
  }

  {
    std::map<unsigned, unsigned> subobjectAssociations;
    for (unsigned index = 0; index < arg.value->NumSubobjects; ++index) {
      D3D12_STATE_SUBOBJECT& subobject =
          const_cast<D3D12_STATE_SUBOBJECT&>(arg.value->pSubobjects[index]);

      if (subobject.Type == D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION) {
        D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION* desc =
            static_cast<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(
                const_cast<void*>(subobject.pDesc));

        auto it = subobjectIndexes.find(desc->pSubobjectToAssociate);
        GITS_ASSERT(it != subobjectIndexes.end());
        subobjectAssociations[index] = it->second;
      }
    }

    unsigned size = subobjectAssociations.size();
    memcpy(dest + offset, &size, sizeof(size));
    offset += sizeof(size);

    for (auto& it : subobjectAssociations) {
      memcpy(dest + offset, &it.first, sizeof(unsigned));
      offset += sizeof(unsigned);
      memcpy(dest + offset, &it.second, sizeof(unsigned));
      offset += sizeof(unsigned);
    }
  }
  {
    unsigned size = arg.interfaceKeysBySubobject.size();
    memcpy(dest + offset, &size, sizeof(unsigned));
    offset += sizeof(unsigned);
    for (auto& it : arg.interfaceKeysBySubobject) {
      memcpy(dest + offset, &it.first, sizeof(unsigned));
      offset += sizeof(unsigned);
      memcpy(dest + offset, &it.second, sizeof(unsigned));
      offset += sizeof(unsigned);
    }
  }
}

unsigned getSize(const D3D12_PIPELINE_STATE_STREAM_DESC_Argument& arg) {
  if (!arg.value) {
    return sizeof(void*);
  }
  unsigned size = sizeof(void*) + sizeof(D3D12_PIPELINE_STATE_STREAM_DESC) + arg.value->SizeInBytes;

  size_t offset = 0;
  while (offset < arg.value->SizeInBytes) {
    void* subobjectData = static_cast<char*>(arg.value->pPipelineStateSubobjectStream) + offset;
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE subobjectType =
        *reinterpret_cast<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE*>(subobjectData);

    switch (subobjectType) {
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE:
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS: {
      D3D12_SHADER_BYTECODE* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_VS*>(subobjectData);
      size += subobject->BytecodeLength;
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_VS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS: {
      D3D12_SHADER_BYTECODE* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_PS*>(subobjectData);
      size += subobject->BytecodeLength;
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_PS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DS: {
      D3D12_SHADER_BYTECODE* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_DS*>(subobjectData);
      size += subobject->BytecodeLength;
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_DS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_HS: {
      D3D12_SHADER_BYTECODE* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_HS*>(subobjectData);
      size += subobject->BytecodeLength;
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_HS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_GS: {
      D3D12_SHADER_BYTECODE* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_GS*>(subobjectData);
      size += subobject->BytecodeLength;
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_GS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS: {
      D3D12_SHADER_BYTECODE* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_CS*>(subobjectData);
      size += subobject->BytecodeLength;
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_CS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS: {
      D3D12_SHADER_BYTECODE* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_AS*>(subobjectData);
      size += subobject->BytecodeLength;
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_AS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS: {
      D3D12_SHADER_BYTECODE* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_MS*>(subobjectData);
      size += subobject->BytecodeLength;
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_MS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_STREAM_OUTPUT: {
      D3D12_STREAM_OUTPUT_DESC* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_STREAM_OUTPUT*>(subobjectData);
      size += subobject->NumEntries * sizeof(D3D12_SO_DECLARATION_ENTRY);
      for (unsigned i = 0; i < subobject->NumEntries; ++i) {
        size += sizeof(unsigned) + strlen(subobject->pSODeclaration[i].SemanticName) + 1;
      }
      size += subobject->NumStrides * sizeof(UINT);
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_STREAM_OUTPUT);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND:
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_MASK:
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_MASK);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_NODE_MASK:
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_NODE_MASK);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER:
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER1:
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER1);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER2:
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER2);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL:
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL1:
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL1);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL2:
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL2);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT: {
      D3D12_INPUT_LAYOUT_DESC* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT*>(subobjectData);
      size += subobject->NumElements * sizeof(D3D12_INPUT_ELEMENT_DESC);
      for (unsigned i = 0; i < subobject->NumElements; ++i) {
        size += sizeof(unsigned) + strlen(subobject->pInputElementDescs[i].SemanticName) + 1;
      }
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_IB_STRIP_CUT_VALUE:
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_IB_STRIP_CUT_VALUE);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY:
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS:
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT:
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC:
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CACHED_PSO: {
      D3D12_CACHED_PIPELINE_STATE* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_CACHED_PSO*>(subobjectData);
      size += subobject->CachedBlobSizeInBytes;
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_CACHED_PSO);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_FLAGS:
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_FLAGS);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VIEW_INSTANCING: {
      D3D12_VIEW_INSTANCING_DESC* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_VIEW_INSTANCING*>(subobjectData);
      size += subobject->ViewInstanceCount * sizeof(D3D12_VIEW_INSTANCE_LOCATION);
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_VIEW_INSTANCING);
    } break;
    default:
      GITS_ASSERT(0 && "Unexpected subobject type");
      break;
    }
  }

  size += sizeof(arg.rootSignatureKey);
  return size;
}

void encode(char* dest, unsigned& offset, const D3D12_PIPELINE_STATE_STREAM_DESC_Argument& arg) {
  if (encodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.value, sizeof(D3D12_PIPELINE_STATE_STREAM_DESC));
  offset += sizeof(D3D12_PIPELINE_STATE_STREAM_DESC);

  memcpy(dest + offset, arg.value->pPipelineStateSubobjectStream, arg.value->SizeInBytes);
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
        memcpy(dest + offset, subobject->pShaderBytecode, subobject->BytecodeLength);
        offset += subobject->BytecodeLength;
      }
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_VS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS: {
      D3D12_SHADER_BYTECODE* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_PS*>(subobjectData);
      if (subobject->pShaderBytecode) {
        memcpy(dest + offset, subobject->pShaderBytecode, subobject->BytecodeLength);
        offset += subobject->BytecodeLength;
      }
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_PS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DS: {
      D3D12_SHADER_BYTECODE* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_DS*>(subobjectData);
      if (subobject->pShaderBytecode) {
        memcpy(dest + offset, subobject->pShaderBytecode, subobject->BytecodeLength);
        offset += subobject->BytecodeLength;
      }
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_DS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_HS: {
      D3D12_SHADER_BYTECODE* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_HS*>(subobjectData);
      if (subobject->pShaderBytecode) {
        memcpy(dest + offset, subobject->pShaderBytecode, subobject->BytecodeLength);
        offset += subobject->BytecodeLength;
      }
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_HS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_GS: {
      D3D12_SHADER_BYTECODE* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_GS*>(subobjectData);
      if (subobject->pShaderBytecode) {
        memcpy(dest + offset, subobject->pShaderBytecode, subobject->BytecodeLength);
        offset += subobject->BytecodeLength;
      }
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_GS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS: {
      D3D12_SHADER_BYTECODE* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_CS*>(subobjectData);
      if (subobject->pShaderBytecode) {
        memcpy(dest + offset, subobject->pShaderBytecode, subobject->BytecodeLength);
        offset += subobject->BytecodeLength;
      }
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_CS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS: {
      D3D12_SHADER_BYTECODE* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_AS*>(subobjectData);
      if (subobject->pShaderBytecode) {
        memcpy(dest + offset, subobject->pShaderBytecode, subobject->BytecodeLength);
        offset += subobject->BytecodeLength;
      }
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_AS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS: {
      D3D12_SHADER_BYTECODE* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_MS*>(subobjectData);
      if (subobject->pShaderBytecode) {
        memcpy(dest + offset, subobject->pShaderBytecode, subobject->BytecodeLength);
        offset += subobject->BytecodeLength;
      }
      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_MS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_STREAM_OUTPUT: {
      D3D12_STREAM_OUTPUT_DESC* subobject =
          &*static_cast<CD3DX12_PIPELINE_STATE_STREAM_STREAM_OUTPUT*>(subobjectData);

      memcpy(dest + offset, subobject->pSODeclaration,
             subobject->NumEntries * sizeof(D3D12_SO_DECLARATION_ENTRY));
      offset += subobject->NumEntries * sizeof(D3D12_SO_DECLARATION_ENTRY);

      for (unsigned i = 0; i < subobject->NumEntries; ++i) {
        unsigned len = strlen(subobject->pSODeclaration[i].SemanticName) + 1;
        memcpy(dest + offset, &len, sizeof(len));
        offset += sizeof(len);
        memcpy(dest + offset, subobject->pSODeclaration[i].SemanticName, len);
        offset += len;
      }
      memcpy(dest + offset, subobject->pBufferStrides, subobject->NumStrides * sizeof(UINT));
      offset += subobject->NumStrides * sizeof(UINT);

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

      memcpy(dest + offset, subobject->pInputElementDescs,
             subobject->NumElements * sizeof(D3D12_INPUT_ELEMENT_DESC));
      offset += subobject->NumElements * sizeof(D3D12_INPUT_ELEMENT_DESC);

      for (unsigned i = 0; i < subobject->NumElements; ++i) {
        unsigned len = strlen(subobject->pInputElementDescs[i].SemanticName) + 1;
        memcpy(dest + offset, &len, sizeof(len));
        offset += sizeof(len);
        memcpy(dest + offset, subobject->pInputElementDescs[i].SemanticName, len);
        offset += len;
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
      if (subobject->CachedBlobSizeInBytes) {
        memcpy(dest + offset, subobject->pCachedBlob, subobject->CachedBlobSizeInBytes);
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
      memcpy(dest + offset, subobject->pViewInstanceLocations,
             subobject->ViewInstanceCount * sizeof(D3D12_VIEW_INSTANCE_LOCATION));
      offset += subobject->ViewInstanceCount * sizeof(D3D12_VIEW_INSTANCE_LOCATION);

      stateOffset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_VIEW_INSTANCING);
    } break;
    default:
      GITS_ASSERT(0 && "Unexpected subobject type");
      break;
    }
  }

  memcpy(dest + offset, &arg.rootSignatureKey, sizeof(arg.rootSignatureKey));
  offset += sizeof(arg.rootSignatureKey);
}

unsigned getSize(const PointerArgument<D3D12_HEAP_PROPERTIES>& arg) {
  if (!arg.value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(D3D12_HEAP_PROPERTIES);
}

void encode(char* dest, unsigned& offset, const PointerArgument<D3D12_HEAP_PROPERTIES>& arg) {
  if (encodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.value, sizeof(D3D12_HEAP_PROPERTIES));
  offset += sizeof(D3D12_HEAP_PROPERTIES);
}

unsigned getSize(const PointerArgument<D3D12_HEAP_DESC>& arg) {
  if (!arg.value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(D3D12_HEAP_DESC);
}

void encode(char* dest, unsigned& offset, const PointerArgument<D3D12_HEAP_DESC>& arg) {
  if (encodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.value, sizeof(D3D12_HEAP_DESC));
  offset += sizeof(D3D12_HEAP_DESC);
}

unsigned getSize(const Argument<D3D12_HEAP_FLAGS>& arg) {
  return sizeof(D3D12_HEAP_FLAGS);
}

void encode(char* dest, unsigned& offset, const Argument<D3D12_HEAP_FLAGS>& arg) {
  memcpy(dest + offset, &arg.value, sizeof(D3D12_HEAP_FLAGS));
  offset += sizeof(D3D12_HEAP_FLAGS);
}

unsigned getSize(const PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC>& arg) {
  if (!arg.value) {
    return sizeof(void*);
  }
  unsigned size = sizeof(void*) + sizeof(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC);

  if (arg.value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
    if (arg.value->Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY) {
      size += arg.value->Inputs.NumDescs * sizeof(D3D12_RAYTRACING_GEOMETRY_DESC);
    } else if (arg.value->Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS) {
      size += arg.value->Inputs.NumDescs * sizeof(D3D12_RAYTRACING_GEOMETRY_DESC*);
      size += arg.value->Inputs.NumDescs * sizeof(D3D12_RAYTRACING_GEOMETRY_DESC);
    }
  }

  return size;
}

void encode(char* dest,
            unsigned& offset,
            const PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC>& arg) {
  if (encodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.value, sizeof(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC));
  offset += sizeof(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC);

  if (arg.value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
    if (arg.value->Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY) {
      if (arg.value->Inputs.pGeometryDescs) {
        memcpy(dest + offset, arg.value->Inputs.pGeometryDescs,
               sizeof(D3D12_RAYTRACING_GEOMETRY_DESC) * arg.value->Inputs.NumDescs);
        offset += sizeof(D3D12_RAYTRACING_GEOMETRY_DESC) * arg.value->Inputs.NumDescs;
      }
    } else if (arg.value->Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS) {
      if (arg.value->Inputs.ppGeometryDescs) {
        memcpy(dest + offset, arg.value->Inputs.ppGeometryDescs,
               sizeof(D3D12_RAYTRACING_GEOMETRY_DESC*) * arg.value->Inputs.NumDescs);
        offset += sizeof(D3D12_RAYTRACING_GEOMETRY_DESC*) * arg.value->Inputs.NumDescs;
      }
      for (unsigned i = 0; i < arg.value->Inputs.NumDescs; ++i) {
        memcpy(dest + offset, arg.value->Inputs.ppGeometryDescs[i],
               sizeof(D3D12_RAYTRACING_GEOMETRY_DESC));
        offset += sizeof(D3D12_RAYTRACING_GEOMETRY_DESC);
      }
    }
  }
}

unsigned getSize(const PointerArgument<INTCExtensionAppInfo>& arg) {
  if (!arg.value) {
    return sizeof(void*);
  }
  unsigned size = sizeof(void*) + sizeof(INTCExtensionAppInfo);

  if (arg.value->pApplicationName) {
    size += sizeof(unsigned) + wcslen(arg.value->pApplicationName) * 2 + 2;
  }
  if (arg.value->pEngineName) {
    size += sizeof(unsigned) + wcslen(arg.value->pEngineName) * 2 + 2;
  }

  return size;
}

void encode(char* dest, unsigned& offset, const PointerArgument<INTCExtensionAppInfo>& arg) {
  if (encodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.value, sizeof(INTCExtensionAppInfo));
  offset += sizeof(INTCExtensionAppInfo);

  if (arg.value->pApplicationName) {
    unsigned len = wcslen(arg.value->pApplicationName) * 2 + 2;
    memcpy(dest + offset, &len, sizeof(len));
    offset += sizeof(unsigned);
    memcpy(dest + offset, arg.value->pApplicationName, len);
    offset += len;
  }
  if (arg.value->pEngineName) {
    unsigned len = wcslen(arg.value->pEngineName) * 2 + 2;
    memcpy(dest + offset, &len, sizeof(len));
    offset += sizeof(unsigned);
    memcpy(dest + offset, arg.value->pEngineName, len);
    offset += len;
  }
}

unsigned getSize(const PointerArgument<INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC>& arg) {
  if (!arg.value) {
    return sizeof(void*);
  }
  unsigned size = sizeof(void*) + sizeof(INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC) +
                  sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC) + arg.value->CS.BytecodeLength;

  if (arg.value->CompileOptions) {
    size += sizeof(unsigned) + strlen(static_cast<const char*>(arg.compileOptions)) + 1;
  }
  if (arg.value->InternalOptions) {
    size += sizeof(unsigned) + strlen(static_cast<const char*>(arg.internalOptions)) + 1;
  }
  size += sizeof(arg.rootSignatureKey);

  return size;
}

void encode(char* dest,
            unsigned& offset,
            const PointerArgument<INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC>& arg) {
  if (encodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.value, sizeof(INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC));
  offset += sizeof(INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC);

  memcpy(dest + offset, arg.value->pD3D12Desc, sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC));
  offset += sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC);

  memcpy(dest + offset, arg.cs, arg.value->CS.BytecodeLength);
  offset += arg.value->CS.BytecodeLength;

  if (arg.value->CompileOptions) {
    unsigned len = strlen(static_cast<const char*>(arg.compileOptions)) + 1;
    memcpy(dest + offset, &len, sizeof(len));
    offset += sizeof(len);
    memcpy(dest + offset, arg.compileOptions, len);
    offset += len;
  }

  if (arg.value->InternalOptions) {
    unsigned len = strlen(static_cast<const char*>(arg.internalOptions)) + 1;
    memcpy(dest + offset, &len, sizeof(len));
    offset += sizeof(len);
    memcpy(dest + offset, arg.internalOptions, len);
    offset += len;
  }

  memcpy(dest + offset, &arg.rootSignatureKey, sizeof(arg.rootSignatureKey));
  offset += sizeof(arg.rootSignatureKey);
}

unsigned getSize(const PointerArgument<INTC_D3D12_RESOURCE_DESC_0001>& arg) {
  if (!arg.value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(INTC_D3D12_RESOURCE_DESC_0001) + sizeof(D3D12_RESOURCE_DESC);
}

void encode(char* dest,
            unsigned& offset,
            const PointerArgument<INTC_D3D12_RESOURCE_DESC_0001>& arg) {
  if (encodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.value, sizeof(INTC_D3D12_RESOURCE_DESC_0001));
  offset += sizeof(INTC_D3D12_RESOURCE_DESC_0001);

  memcpy(dest + offset, arg.value->pD3D12Desc, sizeof(D3D12_RESOURCE_DESC));
  offset += sizeof(D3D12_RESOURCE_DESC);
}

unsigned getSize(const INTCExtensionContextOutputArgument& arg) {
  return sizeof(arg.key);
}

void encode(char* dest, unsigned& offset, const INTCExtensionContextOutputArgument& arg) {
  memcpy(dest + offset, &arg.key, sizeof(arg.key));
  offset += sizeof(arg.key);
}

unsigned getSize(const INTCExtensionContextArgument& arg) {
  return sizeof(arg.key);
}

void encode(char* dest, unsigned& offset, const INTCExtensionContextArgument& arg) {
  memcpy(dest + offset, &arg.key, sizeof(arg.key));
  offset += sizeof(arg.key);
}

} // namespace DirectX
} // namespace gits
