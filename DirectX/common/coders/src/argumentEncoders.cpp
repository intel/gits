// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "argumentEncoders.h"
#include "intelExtensions.h"
#include "dmlCodersAuto.h"
#include "gits.h"

#include <d3dx12/d3dx12_pipeline_state_stream.h>

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
  void* const* value = arg.captureValue ? &arg.captureValue : arg.value;
  if (value == nullptr) {
    // WA for nullptr input to ID3D12Resource::Map
    void* null{};
    memcpy(dest + offset, &null, sizeof(void*));
  } else {
    memcpy(dest + offset, value, sizeof(void*));
  }
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

unsigned getSize(const LPCSTR_Argument& arg) {
  if (!arg.value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(unsigned) + strlen(arg.value) + 1;
}

void encode(char* dest, unsigned& offset, const LPCSTR_Argument& arg) {
  if (encodeNullPtr(dest, offset, arg)) {
    return;
  }
  unsigned len = strlen(arg.value) + 1;
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

unsigned getSize(const D3D12_GPU_VIRTUAL_ADDRESSs_Argument& arg) {
  if (!arg.value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(arg.size) +
         (sizeof(D3D12_GPU_VIRTUAL_ADDRESS) + sizeof(unsigned) * 2) * arg.size;
}

void encode(char* dest, unsigned& offset, const D3D12_GPU_VIRTUAL_ADDRESSs_Argument& arg) {
  if (encodeNullPtr(dest, offset, arg)) {
    return;
  }
  memcpy(dest + offset, &arg.size, sizeof(arg.size));
  offset += sizeof(arg.size);

  memcpy(dest + offset, arg.value, sizeof(D3D12_GPU_VIRTUAL_ADDRESS) * arg.size);
  offset += sizeof(D3D12_GPU_VIRTUAL_ADDRESS) * arg.size;

  memcpy(dest + offset, arg.interfaceKeys.data(), sizeof(unsigned) * arg.size);
  offset += sizeof(unsigned) * arg.size;

  memcpy(dest + offset, arg.offsets.data(), sizeof(unsigned) * arg.size);
  offset += sizeof(unsigned) * arg.size;
}

unsigned getSize(const ShaderIdentifierArgument& arg) {
  return D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
}

void encode(char* dest, unsigned& offset, const ShaderIdentifierArgument& arg) {
  if (arg.value) {
    memcpy(dest + offset, arg.value, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
  } else {
    memset(dest + offset, 0, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
  }
  offset += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
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
      if (subobject->pSODeclaration) {
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
      }
      if (subobject->pBufferStrides) {
        memcpy(dest + offset, subobject->pBufferStrides, subobject->NumStrides * sizeof(UINT));
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
      if (subobject->pViewInstanceLocations) {
        memcpy(dest + offset, subobject->pViewInstanceLocations,
               subobject->ViewInstanceCount * sizeof(D3D12_VIEW_INSTANCE_LOCATION));
        offset += subobject->ViewInstanceCount * sizeof(D3D12_VIEW_INSTANCE_LOCATION);
      }
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

unsigned getSize(const PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS>& arg) {
  if (!arg.value) {
    return sizeof(void*);
  }

  unsigned size = sizeof(void*) + sizeof(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS);
  if (arg.value->Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
    if (arg.value->DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY) {
      size += arg.value->NumDescs * sizeof(D3D12_RAYTRACING_GEOMETRY_DESC);
      for (unsigned i = 0; i < arg.value->NumDescs; ++i) {
        if (arg.value->pGeometryDescs[i].Type == D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES) {
          if (arg.value->pGeometryDescs[i].OmmTriangles.pTriangles) {
            size += sizeof(D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC);
          }
          if (arg.value->pGeometryDescs[i].OmmTriangles.pOmmLinkage) {
            size += sizeof(D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC);
          }
        }
      }
    } else if (arg.value->DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS) {
      size += arg.value->NumDescs * sizeof(D3D12_RAYTRACING_GEOMETRY_DESC*);
      size += arg.value->NumDescs * sizeof(D3D12_RAYTRACING_GEOMETRY_DESC);
      for (unsigned i = 0; i < arg.value->NumDescs; ++i) {
        if (arg.value->ppGeometryDescs[i]->Type == D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES) {
          if (arg.value->ppGeometryDescs[i]->OmmTriangles.pTriangles) {
            size += sizeof(D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC);
          }
          if (arg.value->ppGeometryDescs[i]->OmmTriangles.pOmmLinkage) {
            size += sizeof(D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC);
          }
        }
      }
    }
  } else if (arg.value->Type ==
             D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_OPACITY_MICROMAP_ARRAY) {
    if (arg.value->pOpacityMicromapArrayDesc) {
      size += sizeof(D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC);
      if (arg.value->pOpacityMicromapArrayDesc->pOmmHistogram) {
        size += arg.value->pOpacityMicromapArrayDesc->NumOmmHistogramEntries *
                sizeof(D3D12_RAYTRACING_OPACITY_MICROMAP_HISTOGRAM_ENTRY);
      }
    }
  }

  size += sizeof(unsigned) + sizeof(unsigned) * arg.inputKeys.size() * 2;

  return size;
}

void encode(char* dest,
            unsigned& offset,
            const PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS>& arg) {
  if (encodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.value, sizeof(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS));
  offset += sizeof(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS);

  if (arg.value->Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
    if (arg.value->DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY) {
      if (arg.value->pGeometryDescs) {
        memcpy(dest + offset, arg.value->pGeometryDescs,
               sizeof(D3D12_RAYTRACING_GEOMETRY_DESC) * arg.value->NumDescs);
        offset += sizeof(D3D12_RAYTRACING_GEOMETRY_DESC) * arg.value->NumDescs;
        for (unsigned i = 0; i < arg.value->NumDescs; ++i) {
          if (arg.value->pGeometryDescs[i].Type == D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES) {
            if (arg.value->pGeometryDescs[i].OmmTriangles.pTriangles) {
              memcpy(dest + offset, arg.value->pGeometryDescs[i].OmmTriangles.pTriangles,
                     sizeof(D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC));
              offset += sizeof(D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC);
            }
            if (arg.value->pGeometryDescs[i].OmmTriangles.pOmmLinkage) {
              memcpy(dest + offset, arg.value->pGeometryDescs[i].OmmTriangles.pOmmLinkage,
                     sizeof(D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC));
              offset += sizeof(D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC);
            }
          }
        }
      }
    } else if (arg.value->DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS) {
      if (arg.value->ppGeometryDescs) {
        memcpy(dest + offset, arg.value->ppGeometryDescs,
               sizeof(D3D12_RAYTRACING_GEOMETRY_DESC*) * arg.value->NumDescs);
        offset += sizeof(D3D12_RAYTRACING_GEOMETRY_DESC*) * arg.value->NumDescs;
      }
      for (unsigned i = 0; i < arg.value->NumDescs; ++i) {
        memcpy(dest + offset, arg.value->ppGeometryDescs[i],
               sizeof(D3D12_RAYTRACING_GEOMETRY_DESC));
        offset += sizeof(D3D12_RAYTRACING_GEOMETRY_DESC);
        if (arg.value->ppGeometryDescs[i]->Type == D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES) {
          if (arg.value->ppGeometryDescs[i]->OmmTriangles.pTriangles) {
            memcpy(dest + offset, arg.value->ppGeometryDescs[i]->OmmTriangles.pTriangles,
                   sizeof(D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC));
            offset += sizeof(D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC);
          }
          if (arg.value->ppGeometryDescs[i]->OmmTriangles.pOmmLinkage) {
            memcpy(dest + offset, arg.value->ppGeometryDescs[i]->OmmTriangles.pOmmLinkage,
                   sizeof(D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC));
            offset += sizeof(D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC);
          }
        }
      }
    }
  } else if (arg.value->Type ==
             D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_OPACITY_MICROMAP_ARRAY) {
    if (arg.value->pOpacityMicromapArrayDesc) {
      memcpy(dest + offset, arg.value->pOpacityMicromapArrayDesc,
             sizeof(D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC));
      offset += sizeof(D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC);
      if (arg.value->pOpacityMicromapArrayDesc->pOmmHistogram) {
        memcpy(dest + offset, arg.value->pOpacityMicromapArrayDesc->pOmmHistogram,
               sizeof(D3D12_RAYTRACING_OPACITY_MICROMAP_HISTOGRAM_ENTRY) *
                   arg.value->pOpacityMicromapArrayDesc->NumOmmHistogramEntries);
        offset += sizeof(D3D12_RAYTRACING_OPACITY_MICROMAP_HISTOGRAM_ENTRY) *
                  arg.value->pOpacityMicromapArrayDesc->NumOmmHistogramEntries;
      }
    }
  }

  unsigned size = arg.inputKeys.size();
  memcpy(dest + offset, &size, sizeof(size));
  offset += sizeof(size);
  memcpy(dest + offset, arg.inputKeys.data(), sizeof(unsigned) * size);
  offset += sizeof(unsigned) * size;
  memcpy(dest + offset, arg.inputOffsets.data(), sizeof(unsigned) * size);
  offset += sizeof(unsigned) * size;
}

unsigned getSize(const PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC>& arg) {
  if (!arg.value) {
    return sizeof(void*);
  }
  unsigned size = sizeof(void*) + sizeof(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC);

  if (arg.value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
    if (arg.value->Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY) {
      size += arg.value->Inputs.NumDescs * sizeof(D3D12_RAYTRACING_GEOMETRY_DESC);
      for (unsigned i = 0; i < arg.value->Inputs.NumDescs; ++i) {
        if (arg.value->Inputs.pGeometryDescs[i].Type ==
            D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES) {
          if (arg.value->Inputs.pGeometryDescs[i].OmmTriangles.pTriangles) {
            size += sizeof(D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC);
          }
          if (arg.value->Inputs.pGeometryDescs[i].OmmTriangles.pOmmLinkage) {
            size += sizeof(D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC);
          }
        }
      }
    } else if (arg.value->Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS) {
      size += arg.value->Inputs.NumDescs * sizeof(D3D12_RAYTRACING_GEOMETRY_DESC*);
      size += arg.value->Inputs.NumDescs * sizeof(D3D12_RAYTRACING_GEOMETRY_DESC);
      for (unsigned i = 0; i < arg.value->Inputs.NumDescs; ++i) {
        if (arg.value->Inputs.ppGeometryDescs[i]->Type ==
            D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES) {
          if (arg.value->Inputs.ppGeometryDescs[i]->OmmTriangles.pTriangles) {
            size += sizeof(D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC);
          }
          if (arg.value->Inputs.ppGeometryDescs[i]->OmmTriangles.pOmmLinkage) {
            size += sizeof(D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC);
          }
        }
      }
    }
  } else if (arg.value->Inputs.Type ==
             D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_OPACITY_MICROMAP_ARRAY) {
    if (arg.value->Inputs.pOpacityMicromapArrayDesc) {
      size += sizeof(D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC);
      if (arg.value->Inputs.pOpacityMicromapArrayDesc->pOmmHistogram) {
        size += arg.value->Inputs.pOpacityMicromapArrayDesc->NumOmmHistogramEntries *
                sizeof(D3D12_RAYTRACING_OPACITY_MICROMAP_HISTOGRAM_ENTRY);
      }
    }
  }

  size += sizeof(unsigned) * 6;
  size += sizeof(unsigned) + sizeof(unsigned) * arg.inputKeys.size() * 2;

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
        for (unsigned i = 0; i < arg.value->Inputs.NumDescs; ++i) {
          if (arg.value->Inputs.pGeometryDescs[i].Type ==
              D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES) {
            if (arg.value->Inputs.pGeometryDescs[i].OmmTriangles.pTriangles) {
              memcpy(dest + offset, arg.value->Inputs.pGeometryDescs[i].OmmTriangles.pTriangles,
                     sizeof(D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC));
              offset += sizeof(D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC);
            }
            if (arg.value->Inputs.pGeometryDescs[i].OmmTriangles.pOmmLinkage) {
              memcpy(dest + offset, arg.value->Inputs.pGeometryDescs[i].OmmTriangles.pOmmLinkage,
                     sizeof(D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC));
              offset += sizeof(D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC);
            }
          }
        }
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
        if (arg.value->Inputs.ppGeometryDescs[i]->Type ==
            D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES) {
          if (arg.value->Inputs.ppGeometryDescs[i]->OmmTriangles.pTriangles) {
            memcpy(dest + offset, arg.value->Inputs.ppGeometryDescs[i]->OmmTriangles.pTriangles,
                   sizeof(D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC));
            offset += sizeof(D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC);
          }
          if (arg.value->Inputs.ppGeometryDescs[i]->OmmTriangles.pOmmLinkage) {
            memcpy(dest + offset, arg.value->Inputs.ppGeometryDescs[i]->OmmTriangles.pOmmLinkage,
                   sizeof(D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC));
            offset += sizeof(D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC);
          }
        }
      }
    }
  } else if (arg.value->Inputs.Type ==
             D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_OPACITY_MICROMAP_ARRAY) {
    if (arg.value->Inputs.pOpacityMicromapArrayDesc) {
      memcpy(dest + offset, arg.value->Inputs.pOpacityMicromapArrayDesc,
             sizeof(D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC));
      offset += sizeof(D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC);
      if (arg.value->Inputs.pOpacityMicromapArrayDesc->pOmmHistogram) {
        memcpy(dest + offset, arg.value->Inputs.pOpacityMicromapArrayDesc->pOmmHistogram,
               sizeof(D3D12_RAYTRACING_OPACITY_MICROMAP_HISTOGRAM_ENTRY) *
                   arg.value->Inputs.pOpacityMicromapArrayDesc->NumOmmHistogramEntries);
        offset += sizeof(D3D12_RAYTRACING_OPACITY_MICROMAP_HISTOGRAM_ENTRY) *
                  arg.value->Inputs.pOpacityMicromapArrayDesc->NumOmmHistogramEntries;
      }
    }
  }

  memcpy(dest + offset, &arg.destAccelerationStructureKey,
         sizeof(arg.destAccelerationStructureKey));
  offset += sizeof(arg.destAccelerationStructureKey);
  memcpy(dest + offset, &arg.destAccelerationStructureOffset,
         sizeof(arg.destAccelerationStructureOffset));
  offset += sizeof(arg.destAccelerationStructureOffset);

  memcpy(dest + offset, &arg.sourceAccelerationStructureKey,
         sizeof(arg.sourceAccelerationStructureKey));
  offset += sizeof(arg.sourceAccelerationStructureKey);
  memcpy(dest + offset, &arg.sourceAccelerationStructureOffset,
         sizeof(arg.sourceAccelerationStructureOffset));
  offset += sizeof(arg.sourceAccelerationStructureOffset);

  memcpy(dest + offset, &arg.scratchAccelerationStructureKey,
         sizeof(arg.scratchAccelerationStructureKey));
  offset += sizeof(arg.scratchAccelerationStructureKey);
  memcpy(dest + offset, &arg.scratchAccelerationStructureOffset,
         sizeof(arg.scratchAccelerationStructureOffset));
  offset += sizeof(arg.scratchAccelerationStructureOffset);

  unsigned size = arg.inputKeys.size();
  memcpy(dest + offset, &size, sizeof(size));
  offset += sizeof(size);
  memcpy(dest + offset, arg.inputKeys.data(), sizeof(unsigned) * size);
  offset += sizeof(unsigned) * size;
  memcpy(dest + offset, arg.inputOffsets.data(), sizeof(unsigned) * size);
  offset += sizeof(unsigned) * size;
}

unsigned getSize(const PointerArgument<D3D12_DISPATCH_RAYS_DESC>& arg) {
  if (!arg.value) {
    return sizeof(void*);
  }
  unsigned size = sizeof(void*) + sizeof(D3D12_DISPATCH_RAYS_DESC);
  size += sizeof(unsigned) * 8;
  return size;
}

void encode(char* dest, unsigned& offset, const PointerArgument<D3D12_DISPATCH_RAYS_DESC>& arg) {
  if (encodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.value, sizeof(D3D12_DISPATCH_RAYS_DESC));
  offset += sizeof(D3D12_DISPATCH_RAYS_DESC);

  memcpy(dest + offset, &arg.rayGenerationShaderRecordKey,
         sizeof(arg.rayGenerationShaderRecordKey));
  offset += sizeof(arg.rayGenerationShaderRecordKey);
  memcpy(dest + offset, &arg.rayGenerationShaderRecordOffset,
         sizeof(arg.rayGenerationShaderRecordOffset));
  offset += sizeof(arg.rayGenerationShaderRecordOffset);

  memcpy(dest + offset, &arg.missShaderTableKey, sizeof(arg.missShaderTableKey));
  offset += sizeof(arg.missShaderTableKey);
  memcpy(dest + offset, &arg.missShaderTableOffset, sizeof(arg.missShaderTableOffset));
  offset += sizeof(arg.missShaderTableOffset);

  memcpy(dest + offset, &arg.hitGroupTableKey, sizeof(arg.hitGroupTableKey));
  offset += sizeof(arg.hitGroupTableKey);
  memcpy(dest + offset, &arg.hitGroupTableOffset, sizeof(arg.hitGroupTableOffset));
  offset += sizeof(arg.hitGroupTableOffset);

  memcpy(dest + offset, &arg.callableShaderTableKey, sizeof(arg.callableShaderTableKey));
  offset += sizeof(arg.callableShaderTableKey);
  memcpy(dest + offset, &arg.callableShaderTableOffset, sizeof(arg.callableShaderTableOffset));
  offset += sizeof(arg.callableShaderTableOffset);
}

unsigned getSize(
    const ArrayArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>& arg) {
  if (!arg.value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(arg.size) +
         (sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC) +
          sizeof(unsigned) * 2) *
             arg.size;
}

void encode(char* dest,
            unsigned& offset,
            const ArrayArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>& arg) {
  if (encodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, &arg.size, sizeof(arg.size));
  offset += sizeof(arg.size);

  memcpy(dest + offset, arg.value,
         sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC) * arg.size);
  offset += sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC) * arg.size;

  memcpy(dest + offset, arg.destBufferKeys.data(), sizeof(unsigned) * arg.size);
  offset += sizeof(unsigned) * arg.size;

  memcpy(dest + offset, arg.destBufferOffsets.data(), sizeof(unsigned) * arg.size);
  offset += sizeof(unsigned) * arg.size;
}

unsigned getSize(
    const PointerArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>& arg) {
  if (!arg.value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC) +
         sizeof(unsigned) * 2;
}

void encode(
    char* dest,
    unsigned& offset,
    const PointerArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>& arg) {
  if (encodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.value,
         sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC));
  offset += sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC);

  memcpy(dest + offset, &arg.destBufferKey, sizeof(unsigned));
  offset += sizeof(unsigned);

  memcpy(dest + offset, &arg.destBufferOffset, sizeof(unsigned));
  offset += sizeof(unsigned);
}

unsigned getSize(const D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument& arg) {
  if (!arg.value) {
    return sizeof(void*);
  }
  unsigned size =
      sizeof(void*) + sizeof(arg.size) + sizeof(D3D12_RENDER_PASS_RENDER_TARGET_DESC) * arg.size;

  for (unsigned i = 0; i < arg.size; ++i) {
    if (arg.value[i].EndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE) {
      size += sizeof(D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS) *
              arg.value[i].EndingAccess.Resolve.SubresourceCount;
    }
  }

  size += arg.descriptorKeys.size() * sizeof(unsigned) +
          arg.descriptorIndexes.size() * sizeof(unsigned) + sizeof(unsigned) +
          arg.resolveSrcResourceKeys.size() * sizeof(unsigned) +
          arg.resolveDstResourceKeys.size() * sizeof(unsigned);
  return size;
}

void encode(char* dest,
            unsigned& offset,
            const D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument& arg) {
  if (encodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, &arg.size, sizeof(arg.size));
  offset += sizeof(arg.size);

  memcpy(dest + offset, arg.value, sizeof(D3D12_RENDER_PASS_RENDER_TARGET_DESC) * arg.size);
  offset += sizeof(D3D12_RENDER_PASS_RENDER_TARGET_DESC) * arg.size;

  for (unsigned i = 0; i < arg.size; ++i) {
    if (arg.value[i].EndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE) {
      memcpy(dest + offset, arg.value[i].EndingAccess.Resolve.pSubresourceParameters,
             sizeof(D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS) *
                 arg.value[i].EndingAccess.Resolve.SubresourceCount);
      offset += sizeof(D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS) *
                arg.value[i].EndingAccess.Resolve.SubresourceCount;
    }
  }

  memcpy(dest + offset, arg.descriptorKeys.data(), sizeof(unsigned) * arg.size);
  offset += sizeof(unsigned) * arg.size;

  memcpy(dest + offset, arg.descriptorIndexes.data(), sizeof(unsigned) * arg.size);
  offset += sizeof(unsigned) * arg.size;

  unsigned size = arg.resolveSrcResourceKeys.size();
  memcpy(dest + offset, &size, sizeof(size));
  offset += sizeof(size);
  memcpy(dest + offset, arg.resolveSrcResourceKeys.data(), sizeof(unsigned) * size);
  offset += sizeof(unsigned) * size;
  memcpy(dest + offset, arg.resolveDstResourceKeys.data(), sizeof(unsigned) * size);
  offset += sizeof(unsigned) * size;
}

unsigned getSize(const D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument& arg) {
  if (!arg.value) {
    return sizeof(void*);
  }
  unsigned size = sizeof(void*) + sizeof(D3D12_RENDER_PASS_DEPTH_STENCIL_DESC);

  if (arg.value->DepthEndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE) {
    size += sizeof(D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS) *
            arg.value->DepthEndingAccess.Resolve.SubresourceCount;
  }
  if (arg.value->StencilEndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE) {
    size += sizeof(D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS) *
            arg.value->StencilEndingAccess.Resolve.SubresourceCount;
  }

  size += sizeof(unsigned) * 6;
  return size;
}

void encode(char* dest,
            unsigned& offset,
            const D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument& arg) {
  if (encodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.value, sizeof(D3D12_RENDER_PASS_DEPTH_STENCIL_DESC));
  offset += sizeof(D3D12_RENDER_PASS_DEPTH_STENCIL_DESC);

  if (arg.value->DepthEndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE) {
    memcpy(dest + offset, arg.value->DepthEndingAccess.Resolve.pSubresourceParameters,
           sizeof(D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS) *
               arg.value->DepthEndingAccess.Resolve.SubresourceCount);
    offset += sizeof(D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS) *
              arg.value->DepthEndingAccess.Resolve.SubresourceCount;
  }
  if (arg.value->StencilEndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE) {
    memcpy(dest + offset, arg.value->StencilEndingAccess.Resolve.pSubresourceParameters,
           sizeof(D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS) *
               arg.value->StencilEndingAccess.Resolve.SubresourceCount);
    offset += sizeof(D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS) *
              arg.value->StencilEndingAccess.Resolve.SubresourceCount;
  }

  memcpy(dest + offset, &arg.descriptorKey, sizeof(unsigned));
  offset += sizeof(unsigned);
  memcpy(dest + offset, &arg.descriptorIndex, sizeof(unsigned));
  offset += sizeof(unsigned);

  memcpy(dest + offset, &arg.resolveSrcDepthKey, sizeof(unsigned));
  offset += sizeof(unsigned);
  memcpy(dest + offset, &arg.resolveDstDepthKey, sizeof(unsigned));
  offset += sizeof(unsigned);
  memcpy(dest + offset, &arg.resolveSrcStencilKey, sizeof(unsigned));
  offset += sizeof(unsigned);
  memcpy(dest + offset, &arg.resolveDstStencilKey, sizeof(unsigned));
  offset += sizeof(unsigned);
}

unsigned getSize(const D3D12_SHADER_RESOURCE_VIEW_DESC_Argument& arg) {
  if (!arg.value) {
    return sizeof(void*);
  }
  unsigned size = sizeof(void*) + sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC);

  if (arg.value->ViewDimension == D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE) {
    size += sizeof(unsigned) * 2;
  }

  return size;
}

void encode(char* dest, unsigned& offset, const D3D12_SHADER_RESOURCE_VIEW_DESC_Argument& arg) {
  if (encodeNullPtr(dest, offset, arg)) {
    return;
  }
  memcpy(dest + offset, arg.value, sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC));
  offset += sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC);

  if (arg.value->ViewDimension == D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE) {
    memcpy(dest + offset, &arg.raytracingLocationKey, sizeof(unsigned));
    offset += sizeof(unsigned);
    memcpy(dest + offset, &arg.raytracingLocationOffset, sizeof(unsigned));
    offset += sizeof(unsigned);
  }
}

unsigned getSize(const D3D12_BARRIER_GROUPs_Argument& arg) {
  if (!arg.value) {
    return sizeof(void*);
  }
  unsigned size = sizeof(void*) + sizeof(arg.size) + sizeof(D3D12_BARRIER_GROUP) * arg.size;

  for (unsigned i = 0; i < arg.size; ++i) {
    if (arg.value[i].Type == D3D12_BARRIER_TYPE_GLOBAL) {
      size += sizeof(D3D12_GLOBAL_BARRIER) * arg.value[i].NumBarriers;
    } else if (arg.value[i].Type == D3D12_BARRIER_TYPE_TEXTURE) {
      size += sizeof(D3D12_TEXTURE_BARRIER) * arg.value[i].NumBarriers;
    } else if (arg.value[i].Type == D3D12_BARRIER_TYPE_BUFFER) {
      size += sizeof(D3D12_BUFFER_BARRIER) * arg.value[i].NumBarriers;
    }
  }

  size += sizeof(unsigned) * arg.resourceKeys.size();

  return size;
}

void encode(char* dest, unsigned& offset, const D3D12_BARRIER_GROUPs_Argument& arg) {
  if (encodeNullPtr(dest, offset, arg)) {
    return;
  }
  memcpy(dest + offset, &arg.size, sizeof(arg.size));
  offset += sizeof(arg.size);

  memcpy(dest + offset, arg.value, sizeof(D3D12_BARRIER_GROUP) * arg.size);
  offset += sizeof(D3D12_BARRIER_GROUP) * arg.size;

  for (unsigned i = 0; i < arg.size; ++i) {
    if (arg.value[i].Type == D3D12_BARRIER_TYPE_GLOBAL) {
      memcpy(dest + offset, arg.value[i].pGlobalBarriers,
             sizeof(D3D12_GLOBAL_BARRIER) * arg.value[i].NumBarriers);
      offset += sizeof(D3D12_GLOBAL_BARRIER) * arg.value[i].NumBarriers;
    } else if (arg.value[i].Type == D3D12_BARRIER_TYPE_TEXTURE) {
      memcpy(dest + offset, arg.value[i].pTextureBarriers,
             sizeof(D3D12_TEXTURE_BARRIER) * arg.value[i].NumBarriers);
      offset += sizeof(D3D12_TEXTURE_BARRIER) * arg.value[i].NumBarriers;
    } else if (arg.value[i].Type == D3D12_BARRIER_TYPE_BUFFER) {
      memcpy(dest + offset, arg.value[i].pBufferBarriers,
             sizeof(D3D12_BUFFER_BARRIER) * arg.value[i].NumBarriers);
      offset += sizeof(D3D12_BUFFER_BARRIER) * arg.value[i].NumBarriers;
    }
  }

  memcpy(dest + offset, arg.resourceKeys.data(), sizeof(unsigned) * arg.resourceKeys.size());
  offset += sizeof(unsigned) * arg.resourceKeys.size();
}

unsigned getSize(const DML_BINDING_DESC_Argument& arg) {
  if (!arg.value) {
    return sizeof(void*);
  }
  return sizeof(void*) + dml::getSize(arg.value, 1) + sizeof(arg.resourceKeysSize) +
         (sizeof(unsigned) * arg.resourceKeysSize);
}

void encode(char* dest, unsigned& offset, const DML_BINDING_DESC_Argument& arg) {
  if (encodeNullPtr(dest, offset, arg)) {
    return;
  }

  dml::encode(arg.value, 1, dest, offset);

  memcpy(dest + offset, &arg.resourceKeysSize, sizeof(arg.resourceKeysSize));
  offset += sizeof(arg.resourceKeysSize);

  memcpy(dest + offset, arg.resourceKeys.data(), sizeof(unsigned) * arg.resourceKeysSize);
  offset += sizeof(unsigned) * arg.resourceKeysSize;
}

unsigned getSize(const DML_BINDING_DESCs_Argument& arg) {
  if (!arg.value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(arg.size) + dml::getSize(arg.value, arg.size) +
         sizeof(arg.resourceKeysSize) + (sizeof(unsigned) * arg.resourceKeysSize);
}

void encode(char* dest, unsigned& offset, const DML_BINDING_DESCs_Argument& arg) {
  if (encodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, &arg.size, sizeof(arg.size));
  offset += sizeof(arg.size);

  dml::encode(arg.value, arg.size, dest, offset);

  memcpy(dest + offset, &arg.resourceKeysSize, sizeof(arg.resourceKeysSize));
  offset += sizeof(arg.resourceKeysSize);

  memcpy(dest + offset, arg.resourceKeys.data(), sizeof(unsigned) * arg.resourceKeysSize);
  offset += sizeof(unsigned) * arg.resourceKeysSize;
}

unsigned getSize(const DML_BINDING_TABLE_DESC_Argument& arg) {
  if (!arg.value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(DML_BINDING_TABLE_DESC) +
         sizeof(DML_BINDING_TABLE_DESC_Argument::Data);
}

void encode(char* dest, unsigned& offset, const DML_BINDING_TABLE_DESC_Argument& arg) {
  if (encodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.value, sizeof(DML_BINDING_TABLE_DESC));
  offset += sizeof(DML_BINDING_TABLE_DESC);

  memcpy(dest + offset, &arg.data, sizeof(arg.data));
  offset += sizeof(arg.data);
}

unsigned getSize(const DML_OPERATOR_DESC_Argument& arg) {
  if (!arg.value) {
    return sizeof(void*);
  }
  return sizeof(void*) + dml::getSize(arg.value, 1);
}

void encode(char* dest, unsigned& offset, const DML_OPERATOR_DESC_Argument& arg) {
  if (encodeNullPtr(dest, offset, arg)) {
    return;
  }
  dml::encode(arg.value, 1, dest, offset);
}

unsigned getSize(const DML_GRAPH_DESC_Argument& arg) {
  if (!arg.value) {
    return sizeof(void*);
  }
  return sizeof(void*) + dml::getSize(arg.value, 1) + sizeof(unsigned) +
         (sizeof(unsigned) * arg.operatorKeysSize);
}

void encode(char* dest, unsigned& offset, const DML_GRAPH_DESC_Argument& arg) {
  if (encodeNullPtr(dest, offset, arg)) {
    return;
  }

  dml::encode(arg.value, 1, dest, offset);

  memcpy(dest + offset, &arg.operatorKeysSize, sizeof(unsigned));
  offset += sizeof(unsigned);

  memcpy(dest + offset, arg.operatorKeys.data(), sizeof(unsigned) * arg.operatorKeysSize);
  offset += sizeof(unsigned) * arg.operatorKeysSize;
}

unsigned getSize(const DML_CheckFeatureSupport_BufferArgument& arg) {
  if (!arg.value) {
    return sizeof(void*);
  }

  size_t totalSize = sizeof(void*) + sizeof(arg.size) + arg.size + sizeof(arg.feature);
  if (arg.feature == DML_FEATURE_FEATURE_LEVELS) {
    auto* featlevels = reinterpret_cast<DML_FEATURE_QUERY_FEATURE_LEVELS*>(arg.value);
    totalSize += featlevels->RequestedFeatureLevelCount * sizeof(DML_FEATURE_LEVEL);
  }

  return totalSize;
}

void encode(char* dest, unsigned& offset, const DML_CheckFeatureSupport_BufferArgument& arg) {
  if (encodeNullPtr(dest, offset, arg)) {
    return;
  }
  encode(dest, offset, arg.size);
  encode(dest, offset, arg.feature);

  if (arg.feature == DML_FEATURE_FEATURE_LEVELS) {
    auto* featlevels = reinterpret_cast<DML_FEATURE_QUERY_FEATURE_LEVELS*>(arg.value);
    encode(dest, offset, *featlevels);
    if (featlevels->RequestedFeatureLevelCount > 0) {
      dml::encode(featlevels->RequestedFeatureLevels, featlevels->RequestedFeatureLevelCount, dest,
                  offset);
    }
  } else {
    memcpy(dest + offset, arg.value, arg.size);
    offset += arg.size;
  }
}

unsigned getSize(const PointerArgument<INTCExtensionAppInfo>& arg) {
  if (!arg.value) {
    return sizeof(void*);
  }
  unsigned size = sizeof(void*) + sizeof(INTCExtensionAppInfo);

  if (arg.value->pApplicationName) {
    size += sizeof(unsigned) + wcslen(arg.pApplicationName) * 2 + 2;
  }
  if (arg.value->pEngineName) {
    size += sizeof(unsigned) + wcslen(arg.pEngineName) * 2 + 2;
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
    unsigned len = wcslen(arg.pApplicationName) * 2 + 2;
    memcpy(dest + offset, &len, sizeof(len));
    offset += sizeof(unsigned);
    memcpy(dest + offset, arg.pApplicationName, len);
    offset += len;
  }
  if (arg.value->pEngineName) {
    unsigned len = wcslen(arg.pEngineName) * 2 + 2;
    memcpy(dest + offset, &len, sizeof(len));
    offset += sizeof(unsigned);
    memcpy(dest + offset, arg.pEngineName, len);
    offset += len;
  }
}

unsigned getSize(const PointerArgument<INTCExtensionAppInfo1>& arg) {
  if (!arg.value) {
    return sizeof(void*);
  }
  unsigned size = sizeof(void*) + sizeof(INTCExtensionAppInfo1);

  if (arg.pApplicationName) {
    size += sizeof(unsigned) + wcslen(arg.pApplicationName) * 2 + 2;
  }
  if (arg.pEngineName) {
    size += sizeof(unsigned) + wcslen(arg.pEngineName) * 2 + 2;
  }

  return size;
}

void encode(char* dest, unsigned& offset, const PointerArgument<INTCExtensionAppInfo1>& arg) {
  if (encodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.value, sizeof(INTCExtensionAppInfo1));
  offset += sizeof(INTCExtensionAppInfo1);

  if (arg.value->pApplicationName) {
    unsigned len = wcslen(arg.pApplicationName) * 2 + 2;
    memcpy(dest + offset, &len, sizeof(len));
    offset += sizeof(unsigned);
    memcpy(dest + offset, arg.pApplicationName, len);
    offset += len;
  }
  if (arg.value->pEngineName) {
    unsigned len = wcslen(arg.pEngineName) * 2 + 2;
    memcpy(dest + offset, &len, sizeof(len));
    offset += sizeof(unsigned);
    memcpy(dest + offset, arg.pEngineName, len);
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

unsigned getSize(const PointerArgument<INTC_D3D12_HEAP_DESC>& arg) {
  if (!arg.value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(INTC_D3D12_HEAP_DESC) + sizeof(D3D12_HEAP_DESC);
}

void encode(char* dest, unsigned& offset, const PointerArgument<INTC_D3D12_HEAP_DESC>& arg) {
  if (encodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.value, sizeof(INTC_D3D12_HEAP_DESC));
  offset += sizeof(INTC_D3D12_HEAP_DESC);

  memcpy(dest + offset, arg.value->pD3D12Desc, sizeof(D3D12_HEAP_DESC));
  offset += sizeof(D3D12_HEAP_DESC);
}

unsigned getSize(const xess_d3d12_init_params_t_Argument& arg) {
  return sizeof(xess_d3d12_init_params_t) + sizeof(arg.key) + sizeof(arg.tempBufferHeapKey) +
         sizeof(arg.tempTextureHeapKey) + sizeof(arg.pipelineLibraryKey);
}

void encode(char* dest, unsigned& offset, const xess_d3d12_init_params_t_Argument& arg) {
  memcpy(dest + offset, arg.value, sizeof(xess_d3d12_init_params_t));
  offset += sizeof(xess_d3d12_init_params_t);

  memcpy(dest + offset, &arg.key, sizeof(arg.key));
  offset += sizeof(arg.key);

  memcpy(dest + offset, &arg.tempBufferHeapKey, sizeof(arg.tempBufferHeapKey));
  offset += sizeof(arg.tempBufferHeapKey);

  memcpy(dest + offset, &arg.tempTextureHeapKey, sizeof(arg.tempTextureHeapKey));
  offset += sizeof(arg.tempTextureHeapKey);

  memcpy(dest + offset, &arg.pipelineLibraryKey, sizeof(arg.pipelineLibraryKey));
  offset += sizeof(arg.pipelineLibraryKey);
}

unsigned getSize(const xess_d3d12_execute_params_t_Argument& arg) {
  return sizeof(xess_d3d12_execute_params_t) + sizeof(arg.colorTextureKey) +
         sizeof(arg.velocityTextureKey) + sizeof(arg.depthTextureKey) +
         sizeof(arg.exposureScaleTextureKey) + sizeof(arg.responsivePixelMaskTextureKey) +
         sizeof(arg.outputTextureKey) + sizeof(arg.descriptorHeapKey);
}

void encode(char* dest, unsigned& offset, const xess_d3d12_execute_params_t_Argument& arg) {
  memcpy(dest + offset, arg.value, sizeof(xess_d3d12_execute_params_t));
  offset += sizeof(xess_d3d12_execute_params_t);

  memcpy(dest + offset, &arg.colorTextureKey, sizeof(arg.colorTextureKey));
  offset += sizeof(arg.colorTextureKey);

  memcpy(dest + offset, &arg.velocityTextureKey, sizeof(arg.velocityTextureKey));
  offset += sizeof(arg.velocityTextureKey);

  memcpy(dest + offset, &arg.depthTextureKey, sizeof(arg.depthTextureKey));
  offset += sizeof(arg.depthTextureKey);

  memcpy(dest + offset, &arg.exposureScaleTextureKey, sizeof(arg.exposureScaleTextureKey));
  offset += sizeof(arg.exposureScaleTextureKey);

  memcpy(dest + offset, &arg.responsivePixelMaskTextureKey,
         sizeof(arg.responsivePixelMaskTextureKey));
  offset += sizeof(arg.responsivePixelMaskTextureKey);

  memcpy(dest + offset, &arg.outputTextureKey, sizeof(arg.outputTextureKey));
  offset += sizeof(arg.outputTextureKey);

  memcpy(dest + offset, &arg.descriptorHeapKey, sizeof(arg.descriptorHeapKey));
  offset += sizeof(arg.descriptorHeapKey);
}

unsigned getSize(const ArrayArgument<D3D12_META_COMMAND_DESC>& arg) {
  if (!arg.value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(arg.size) + (sizeof(D3D12_META_COMMAND_DESC) * arg.size);
}

void encode(char* dest, unsigned& offset, const ArrayArgument<D3D12_META_COMMAND_DESC>& arg) {
  if (encodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, &arg.size, sizeof(arg.size));
  offset += sizeof(arg.size);

  auto* values = (D3D12_META_COMMAND_DESC*)(dest + offset);
  memcpy(dest + offset, arg.value, sizeof(D3D12_META_COMMAND_DESC) * arg.size);
  offset += sizeof(D3D12_META_COMMAND_DESC) * arg.size;

  // D3D12_META_COMMAND_DESC has an LPCWSTR Name field
  // This field is allocated in the driver and we don't need to encode it in the stream since this would break Trace::pre
  for (unsigned i = 0; i < arg.size; ++i) {
    values[i].Name = '\0';
  }
}

unsigned getSize(const DSTORAGE_QUEUE_DESC_Argument& arg) {
  if (!arg.value) {
    return sizeof(void*);
  }

  unsigned size = sizeof(void*) + sizeof(DSTORAGE_QUEUE_DESC) + sizeof(arg.deviceKey);
  if (arg.value->Name) {
    size += sizeof(unsigned) + strlen(arg.value->Name) + 1;
  }
  return size;
}

void encode(char* dest, unsigned& offset, const DSTORAGE_QUEUE_DESC_Argument& arg) {
  if (encodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.value, sizeof(DSTORAGE_QUEUE_DESC));
  offset += sizeof(DSTORAGE_QUEUE_DESC);

  memcpy(dest + offset, &arg.deviceKey, sizeof(arg.deviceKey));
  offset += sizeof(arg.deviceKey);

  if (arg.value->Name) {
    unsigned len = strlen(arg.value->Name) + 1;
    memcpy(dest + offset, &len, sizeof(len));
    offset += sizeof(unsigned);
    memcpy(dest + offset, arg.value->Name, len);
    offset += len;
  }
}

unsigned getSize(const DSTORAGE_REQUEST_Argument& arg) {
  if (!arg.value) {
    return sizeof(void*);
  }

  unsigned size = sizeof(void*) + sizeof(DSTORAGE_REQUEST) + sizeof(arg.fileKey) +
                  sizeof(arg.resourceKey) + sizeof(arg.newOffset);
  if (arg.value->Name) {
    size += sizeof(unsigned) + strlen(arg.value->Name) + 1;
  }
  return size;
}

void encode(char* dest, unsigned& offset, const DSTORAGE_REQUEST_Argument& arg) {
  if (encodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.value, sizeof(DSTORAGE_REQUEST));
  offset += sizeof(DSTORAGE_REQUEST);

  memcpy(dest + offset, &arg.fileKey, sizeof(arg.fileKey));
  offset += sizeof(arg.fileKey);

  memcpy(dest + offset, &arg.resourceKey, sizeof(arg.resourceKey));
  offset += sizeof(arg.resourceKey);

  memcpy(dest + offset, &arg.newOffset, sizeof(arg.newOffset));
  offset += sizeof(arg.newOffset);

  if (arg.value->Name) {
    unsigned len = strlen(arg.value->Name) + 1;
    memcpy(dest + offset, &len, sizeof(len));
    offset += sizeof(unsigned);
    memcpy(dest + offset, arg.value->Name, len);
    offset += len;
  }
}

unsigned getSize(
    const PointerArgument<NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS>& arg) {
  if (!arg.value) {
    return sizeof(void*);
  }
  unsigned size = sizeof(void*) + sizeof(NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS);

  size += sizeof(NVAPI_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC_EX);

  if (arg.value->pDesc->inputs.type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
    if (arg.value->pDesc->inputs.descsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY) {
      if (arg.value->pDesc->inputs.pGeometryDescs) {
        size +=
            arg.value->pDesc->inputs.numDescs * arg.value->pDesc->inputs.geometryDescStrideInBytes;

        for (unsigned i = 0; i < arg.value->pDesc->inputs.numDescs; ++i) {
          const NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX& desc =
              *(NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX*)((char*)(arg.value->pDesc->inputs
                                                                      .pGeometryDescs) +
                                                          arg.value->pDesc->inputs
                                                                  .geometryDescStrideInBytes *
                                                              i);
          if (desc.type == NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES_EX) {
            size += sizeof(NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_USAGE_COUNT) *
                    desc.ommTriangles.ommAttachment.numOMMUsageCounts;
          } else if (desc.type == NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_DMM_TRIANGLES_EX) {
            size += sizeof(NVAPI_D3D12_RAYTRACING_DISPLACEMENT_MICROMAP_USAGE_COUNT) *
                    desc.dmmTriangles.dmmAttachment.numDMMUsageCounts;
          }
        }
      }
    } else if (arg.value->pDesc->inputs.descsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS) {
      if (arg.value->pDesc->inputs.ppGeometryDescs) {
        size +=
            arg.value->pDesc->inputs.numDescs * sizeof(NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX*);
        size += arg.value->pDesc->inputs.numDescs * sizeof(NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX);

        for (unsigned i = 0; i < arg.value->pDesc->inputs.numDescs; ++i) {
          if (arg.value->pDesc->inputs.ppGeometryDescs[i]->type ==
              NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES_EX) {
            size += sizeof(NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_USAGE_COUNT) *
                    arg.value->pDesc->inputs.ppGeometryDescs[i]
                        ->ommTriangles.ommAttachment.numOMMUsageCounts;
          } else if (arg.value->pDesc->inputs.ppGeometryDescs[i]->type ==
                     NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_DMM_TRIANGLES_EX) {
            size += sizeof(NVAPI_D3D12_RAYTRACING_DISPLACEMENT_MICROMAP_USAGE_COUNT) *
                    arg.value->pDesc->inputs.ppGeometryDescs[i]
                        ->dmmTriangles.dmmAttachment.numDMMUsageCounts;
          }
        }
      }
    }
  }

  size += sizeof(unsigned) * 6;
  size += sizeof(unsigned) + sizeof(unsigned) * arg.inputKeys.size() * 2;

  if (arg.value->pPostbuildInfoDescs) {
    size += (sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC) +
             sizeof(unsigned) * 2) *
            arg.value->numPostbuildInfoDescs;

    size += sizeof(unsigned) * arg.destPostBuildBufferKeys.size() * 2;
  }

  return size;
}

void encode(char* dest,
            unsigned& offset,
            const PointerArgument<NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS>& arg) {
  if (encodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.value, sizeof(NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS));
  offset += sizeof(NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS);

  if (arg.value->pDesc) {
    memcpy(dest + offset, arg.value->pDesc,
           sizeof(NVAPI_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC_EX));
    offset += sizeof(NVAPI_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC_EX);

    if (arg.value->pDesc->inputs.type ==
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
      if (arg.value->pDesc->inputs.descsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY) {
        if (arg.value->pDesc->inputs.pGeometryDescs) {
          memcpy(dest + offset, arg.value->pDesc->inputs.pGeometryDescs,
                 arg.value->pDesc->inputs.numDescs *
                     arg.value->pDesc->inputs.geometryDescStrideInBytes);
          offset += arg.value->pDesc->inputs.numDescs *
                    arg.value->pDesc->inputs.geometryDescStrideInBytes;

          for (unsigned i = 0; i < arg.value->pDesc->inputs.numDescs; ++i) {
            const NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX& desc =
                *(NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX*)((char*)(arg.value->pDesc->inputs
                                                                        .pGeometryDescs) +
                                                            arg.value->pDesc->inputs
                                                                    .geometryDescStrideInBytes *
                                                                i);
            if (desc.type == NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES_EX) {
              memcpy(dest + offset, desc.ommTriangles.ommAttachment.pOMMUsageCounts,
                     sizeof(NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_USAGE_COUNT) *
                         desc.ommTriangles.ommAttachment.numOMMUsageCounts);
              offset += sizeof(NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_USAGE_COUNT) *
                        desc.ommTriangles.ommAttachment.numOMMUsageCounts;
            } else if (desc.type == NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_DMM_TRIANGLES_EX) {
              memcpy(dest + offset, desc.dmmTriangles.dmmAttachment.pDMMUsageCounts,
                     sizeof(NVAPI_D3D12_RAYTRACING_DISPLACEMENT_MICROMAP_USAGE_COUNT) *
                         desc.dmmTriangles.dmmAttachment.numDMMUsageCounts);
              offset += sizeof(NVAPI_D3D12_RAYTRACING_DISPLACEMENT_MICROMAP_USAGE_COUNT) *
                        desc.dmmTriangles.dmmAttachment.numDMMUsageCounts;
            }
          }
        }
      } else if (arg.value->pDesc->inputs.descsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS) {
        if (arg.value->pDesc->inputs.ppGeometryDescs) {
          memcpy(dest + offset, arg.value->pDesc->inputs.ppGeometryDescs,
                 sizeof(NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX*) *
                     arg.value->pDesc->inputs.numDescs);
          offset +=
              sizeof(NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX*) * arg.value->pDesc->inputs.numDescs;
        }
        for (unsigned i = 0; i < arg.value->pDesc->inputs.numDescs; ++i) {
          memcpy(dest + offset, arg.value->pDesc->inputs.ppGeometryDescs[i],
                 sizeof(NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX));
          offset += sizeof(NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX);

          if (arg.value->pDesc->inputs.ppGeometryDescs[i]->type ==
              NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES_EX) {
            memcpy(dest + offset,
                   arg.value->pDesc->inputs.ppGeometryDescs[i]
                       ->ommTriangles.ommAttachment.pOMMUsageCounts,
                   sizeof(NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_USAGE_COUNT) *
                       arg.value->pDesc->inputs.ppGeometryDescs[i]
                           ->ommTriangles.ommAttachment.numOMMUsageCounts);
            offset += sizeof(NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_USAGE_COUNT) *
                      arg.value->pDesc->inputs.ppGeometryDescs[i]
                          ->ommTriangles.ommAttachment.numOMMUsageCounts;
          } else if (arg.value->pDesc->inputs.ppGeometryDescs[i]->type ==
                     NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_DMM_TRIANGLES_EX) {
            memcpy(dest + offset,
                   arg.value->pDesc->inputs.ppGeometryDescs[i]
                       ->dmmTriangles.dmmAttachment.pDMMUsageCounts,
                   sizeof(NVAPI_D3D12_RAYTRACING_DISPLACEMENT_MICROMAP_USAGE_COUNT) *
                       arg.value->pDesc->inputs.ppGeometryDescs[i]
                           ->dmmTriangles.dmmAttachment.numDMMUsageCounts);
            offset += sizeof(NVAPI_D3D12_RAYTRACING_DISPLACEMENT_MICROMAP_USAGE_COUNT) *
                      arg.value->pDesc->inputs.ppGeometryDescs[i]
                          ->dmmTriangles.dmmAttachment.numDMMUsageCounts;
          }
        }
      }
    }
  }

  memcpy(dest + offset, &arg.destAccelerationStructureKey,
         sizeof(arg.destAccelerationStructureKey));
  offset += sizeof(arg.destAccelerationStructureKey);
  memcpy(dest + offset, &arg.destAccelerationStructureOffset,
         sizeof(arg.destAccelerationStructureOffset));
  offset += sizeof(arg.destAccelerationStructureOffset);

  memcpy(dest + offset, &arg.sourceAccelerationStructureKey,
         sizeof(arg.sourceAccelerationStructureKey));
  offset += sizeof(arg.sourceAccelerationStructureKey);
  memcpy(dest + offset, &arg.sourceAccelerationStructureOffset,
         sizeof(arg.sourceAccelerationStructureOffset));
  offset += sizeof(arg.sourceAccelerationStructureOffset);

  memcpy(dest + offset, &arg.scratchAccelerationStructureKey,
         sizeof(arg.scratchAccelerationStructureKey));
  offset += sizeof(arg.scratchAccelerationStructureKey);
  memcpy(dest + offset, &arg.scratchAccelerationStructureOffset,
         sizeof(arg.scratchAccelerationStructureOffset));
  offset += sizeof(arg.scratchAccelerationStructureOffset);

  unsigned size = arg.inputKeys.size();
  memcpy(dest + offset, &size, sizeof(size));
  offset += sizeof(size);
  memcpy(dest + offset, arg.inputKeys.data(), sizeof(unsigned) * size);
  offset += sizeof(unsigned) * size;
  memcpy(dest + offset, arg.inputOffsets.data(), sizeof(unsigned) * size);
  offset += sizeof(unsigned) * size;

  if (arg.value->pPostbuildInfoDescs) {
    memcpy(dest + offset, arg.value->pPostbuildInfoDescs,
           sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC) *
               arg.value->numPostbuildInfoDescs);
    offset += sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC) *
              arg.value->numPostbuildInfoDescs;

    memcpy(dest + offset, arg.destPostBuildBufferKeys.data(),
           sizeof(unsigned) * arg.value->numPostbuildInfoDescs);
    offset += sizeof(unsigned) * arg.value->numPostbuildInfoDescs;

    memcpy(dest + offset, arg.destPostBuildBufferOffsets.data(),
           sizeof(unsigned) * arg.value->numPostbuildInfoDescs);
    offset += sizeof(unsigned) * arg.value->numPostbuildInfoDescs;
  }
}

unsigned getSize(const PointerArgument<NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS>& arg) {
  if (!arg.value) {
    return sizeof(void*);
  }
  unsigned size = sizeof(void*) + sizeof(NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS);

  if (arg.value->pDesc) {
    size += sizeof(NVAPI_D3D12_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC);

    if (arg.value->pDesc->inputs.pOMMUsageCounts) {
      size += sizeof(NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_USAGE_COUNT) *
              arg.value->pDesc->inputs.numOMMUsageCounts;
    }
  }

  size += sizeof(unsigned) * 8;

  if (arg.value->pPostbuildInfoDescs) {
    size += (sizeof(NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_POSTBUILD_INFO_DESC) +
             sizeof(unsigned) * 2) *
            arg.value->numPostbuildInfoDescs;

    size += sizeof(unsigned) * arg.destPostBuildBufferKeys.size() * 2;
  }

  return size;
}

void encode(char* dest,
            unsigned& offset,
            const PointerArgument<NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS>& arg) {
  if (encodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.value, sizeof(NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS));
  offset += sizeof(NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS);

  if (arg.value->pDesc) {
    memcpy(dest + offset, arg.value->pDesc,
           sizeof(NVAPI_D3D12_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC));
    offset += sizeof(NVAPI_D3D12_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC);

    if (arg.value->pDesc->inputs.pOMMUsageCounts) {
      memcpy(dest + offset, arg.value->pDesc->inputs.pOMMUsageCounts,
             sizeof(NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_USAGE_COUNT) *
                 arg.value->pDesc->inputs.numOMMUsageCounts);
      offset += sizeof(NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_USAGE_COUNT) *
                arg.value->pDesc->inputs.numOMMUsageCounts;
    }
  }

  memcpy(dest + offset, &arg.destOpacityMicromapArrayDataKey,
         sizeof(arg.destOpacityMicromapArrayDataKey));
  offset += sizeof(arg.destOpacityMicromapArrayDataKey);
  memcpy(dest + offset, &arg.destOpacityMicromapArrayDataOffset,
         sizeof(arg.destOpacityMicromapArrayDataOffset));
  offset += sizeof(arg.destOpacityMicromapArrayDataOffset);

  memcpy(dest + offset, &arg.inputBufferKey, sizeof(arg.inputBufferKey));
  offset += sizeof(arg.inputBufferKey);
  memcpy(dest + offset, &arg.inputBufferOffset, sizeof(arg.inputBufferOffset));
  offset += sizeof(arg.inputBufferOffset);

  memcpy(dest + offset, &arg.perOMMDescsKey, sizeof(arg.perOMMDescsKey));
  offset += sizeof(arg.perOMMDescsKey);
  memcpy(dest + offset, &arg.perOMMDescsOffset, sizeof(arg.perOMMDescsOffset));
  offset += sizeof(arg.perOMMDescsOffset);

  memcpy(dest + offset, &arg.scratchOpacityMicromapArrayDataKey,
         sizeof(arg.scratchOpacityMicromapArrayDataKey));
  offset += sizeof(arg.scratchOpacityMicromapArrayDataKey);
  memcpy(dest + offset, &arg.scratchOpacityMicromapArrayDataOffset,
         sizeof(arg.scratchOpacityMicromapArrayDataOffset));
  offset += sizeof(arg.scratchOpacityMicromapArrayDataOffset);

  if (arg.value->pPostbuildInfoDescs) {
    memcpy(dest + offset, arg.value->pPostbuildInfoDescs,
           sizeof(NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_POSTBUILD_INFO_DESC) *
               arg.value->numPostbuildInfoDescs);
    offset += sizeof(NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_POSTBUILD_INFO_DESC) *
              arg.value->numPostbuildInfoDescs;

    memcpy(dest + offset, arg.destPostBuildBufferKeys.data(),
           sizeof(unsigned) * arg.value->numPostbuildInfoDescs);
    offset += sizeof(unsigned) * arg.value->numPostbuildInfoDescs;

    memcpy(dest + offset, arg.destPostBuildBufferOffsets.data(),
           sizeof(unsigned) * arg.value->numPostbuildInfoDescs);
    offset += sizeof(unsigned) * arg.value->numPostbuildInfoDescs;
  }
}

unsigned getSize(
    const PointerArgument<NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS>& arg) {
  if (!arg.value) {
    return sizeof(void*);
  }
  unsigned size =
      sizeof(void*) + sizeof(NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS);

  if (arg.value->pDesc) {
    size += sizeof(NVAPI_D3D12_RAYTRACING_MULTI_INDIRECT_CLUSTER_OPERATION_DESC);
  }

  size += sizeof(unsigned) * 12;

  return size;
}

void encode(
    char* dest,
    unsigned& offset,
    const PointerArgument<NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS>& arg) {
  if (encodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.value,
         sizeof(NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS));
  offset += sizeof(NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS);

  if (arg.value->pDesc) {
    memcpy(dest + offset, arg.value->pDesc,
           sizeof(NVAPI_D3D12_RAYTRACING_MULTI_INDIRECT_CLUSTER_OPERATION_DESC));
    offset += sizeof(NVAPI_D3D12_RAYTRACING_MULTI_INDIRECT_CLUSTER_OPERATION_DESC);
  }

  memcpy(dest + offset, &arg.batchResultDataKey, sizeof(arg.batchResultDataKey));
  offset += sizeof(arg.batchResultDataKey);
  memcpy(dest + offset, &arg.batchResultDataOffset, sizeof(arg.batchResultDataOffset));
  offset += sizeof(arg.batchResultDataOffset);

  memcpy(dest + offset, &arg.batchScratchDataKey, sizeof(arg.batchScratchDataKey));
  offset += sizeof(arg.batchScratchDataKey);
  memcpy(dest + offset, &arg.batchScratchDataOffset, sizeof(arg.batchScratchDataOffset));
  offset += sizeof(arg.batchScratchDataOffset);

  memcpy(dest + offset, &arg.destinationAddressArrayKey, sizeof(arg.destinationAddressArrayKey));
  offset += sizeof(arg.destinationAddressArrayKey);
  memcpy(dest + offset, &arg.destinationAddressArrayOffset,
         sizeof(arg.destinationAddressArrayOffset));
  offset += sizeof(arg.destinationAddressArrayOffset);

  memcpy(dest + offset, &arg.resultSizeArrayKey, sizeof(arg.resultSizeArrayKey));
  offset += sizeof(arg.resultSizeArrayKey);
  memcpy(dest + offset, &arg.resultSizeArrayOffset, sizeof(arg.resultSizeArrayOffset));
  offset += sizeof(arg.resultSizeArrayOffset);

  memcpy(dest + offset, &arg.indirectArgArrayKey, sizeof(arg.indirectArgArrayKey));
  offset += sizeof(arg.indirectArgArrayKey);
  memcpy(dest + offset, &arg.indirectArgArrayOffset, sizeof(arg.indirectArgArrayOffset));
  offset += sizeof(arg.indirectArgArrayOffset);

  memcpy(dest + offset, &arg.indirectArgCountKey, sizeof(arg.indirectArgCountKey));
  offset += sizeof(arg.indirectArgCountKey);
  memcpy(dest + offset, &arg.indirectArgCountOffset, sizeof(arg.indirectArgCountOffset));
  offset += sizeof(arg.indirectArgCountOffset);
}

} // namespace DirectX
} // namespace gits
