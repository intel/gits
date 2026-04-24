// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "argumentEncoders.h"
#include "intelExtensions.h"
#include "dmlCodersAuto.h"
#include "log.h"

#include <d3dx12/d3dx12_pipeline_state_stream.h>

namespace gits {
namespace DirectX {

unsigned GetSize(const BufferArgument& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(arg.Size) + arg.Size;
}

void Encode(char* dest, unsigned& offset, const BufferArgument& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }
  memcpy(dest + offset, &arg.Size, sizeof(arg.Size));
  offset += sizeof(arg.Size);
  memcpy(dest + offset, arg.Value, arg.Size);
  offset += arg.Size;
}

unsigned GetSize(const OutputBufferArgument& arg) {
  return sizeof(void*);
}

void Encode(char* dest, unsigned& offset, const OutputBufferArgument& arg) {
  void* const* value = arg.CaptureValue ? &arg.CaptureValue : arg.Value;
  if (value == nullptr) {
    // WA for nullptr input to ID3D12Resource::Map
    void* null{};
    memcpy(dest + offset, &null, sizeof(void*));
  } else {
    memcpy(dest + offset, value, sizeof(void*));
  }
  offset += sizeof(void*);
}

unsigned GetSize(const LPCWSTR_Argument& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(unsigned) + wcslen(arg.Value) * 2 + 2;
}

void Encode(char* dest, unsigned& offset, const LPCWSTR_Argument& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }
  unsigned len = wcslen(arg.Value) * 2 + 2;
  memcpy(dest + offset, &len, sizeof(len));
  offset += sizeof(unsigned);
  memcpy(dest + offset, arg.Value, len);
  offset += len;
}

unsigned GetSize(const LPCSTR_Argument& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(unsigned) + strlen(arg.Value) + 1;
}

void Encode(char* dest, unsigned& offset, const LPCSTR_Argument& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }
  unsigned len = strlen(arg.Value) + 1;
  memcpy(dest + offset, &len, sizeof(len));
  offset += sizeof(unsigned);
  memcpy(dest + offset, arg.Value, len);
  offset += len;
}

unsigned GetSize(const D3D12_GPU_VIRTUAL_ADDRESS_Argument& arg) {
  return sizeof(arg.Value) + sizeof(arg.InterfaceKey) + sizeof(arg.Offset);
}

void Encode(char* dest, unsigned& offset, const D3D12_GPU_VIRTUAL_ADDRESS_Argument& arg) {
  memcpy(dest + offset, &arg.Value, sizeof(arg.Value));
  offset += sizeof(arg.Value);
  memcpy(dest + offset, &arg.InterfaceKey, sizeof(arg.InterfaceKey));
  offset += sizeof(arg.InterfaceKey);
  memcpy(dest + offset, &arg.Offset, sizeof(arg.Offset));
  offset += sizeof(arg.Offset);
}

unsigned GetSize(const D3D12_GPU_VIRTUAL_ADDRESSs_Argument& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(arg.Size) +
         (sizeof(D3D12_GPU_VIRTUAL_ADDRESS) + sizeof(unsigned) * 2) * arg.Size;
}

void Encode(char* dest, unsigned& offset, const D3D12_GPU_VIRTUAL_ADDRESSs_Argument& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }
  memcpy(dest + offset, &arg.Size, sizeof(arg.Size));
  offset += sizeof(arg.Size);

  memcpy(dest + offset, arg.Value, sizeof(D3D12_GPU_VIRTUAL_ADDRESS) * arg.Size);
  offset += sizeof(D3D12_GPU_VIRTUAL_ADDRESS) * arg.Size;

  memcpy(dest + offset, arg.InterfaceKeys.data(), sizeof(unsigned) * arg.Size);
  offset += sizeof(unsigned) * arg.Size;

  memcpy(dest + offset, arg.Offsets.data(), sizeof(unsigned) * arg.Size);
  offset += sizeof(unsigned) * arg.Size;
}

unsigned GetSize(const ShaderIdentifierArgument& arg) {
  return D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
}

void Encode(char* dest, unsigned& offset, const ShaderIdentifierArgument& arg) {
  if (arg.Value) {
    memcpy(dest + offset, arg.Value, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
  } else {
    memset(dest + offset, 0, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
  }
  offset += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
}

unsigned GetSize(const D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  unsigned size = sizeof(void*) + sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC) +
                  arg.Value->VS.BytecodeLength + arg.Value->PS.BytecodeLength +
                  arg.Value->DS.BytecodeLength + arg.Value->HS.BytecodeLength +
                  arg.Value->GS.BytecodeLength;

  size += arg.Value->StreamOutput.NumEntries * sizeof(D3D12_SO_DECLARATION_ENTRY);
  for (unsigned i = 0; i < arg.Value->StreamOutput.NumEntries; ++i) {
    size += sizeof(unsigned) + strlen(arg.Value->StreamOutput.pSODeclaration[i].SemanticName) + 1;
  }
  size += arg.Value->StreamOutput.NumStrides * sizeof(UINT);

  size += sizeof(D3D12_INPUT_ELEMENT_DESC) * arg.Value->InputLayout.NumElements;
  for (unsigned i = 0; i < arg.Value->InputLayout.NumElements; ++i) {
    size +=
        sizeof(unsigned) + strlen(arg.Value->InputLayout.pInputElementDescs[i].SemanticName) + 1;
  }
  size += arg.Value->CachedPSO.CachedBlobSizeInBytes;
  size += sizeof(arg.RootSignatureKey);
  return size;
}

void Encode(char* dest, unsigned& offset, const D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.Value, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
  offset += sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC);

  auto encodeBytecode = [&](D3D12_SHADER_BYTECODE& bytecode) {
    if (bytecode.pShaderBytecode) {
      memcpy(dest + offset, bytecode.pShaderBytecode, bytecode.BytecodeLength);
      offset += bytecode.BytecodeLength;
    }
  };
  encodeBytecode(arg.Value->VS);
  encodeBytecode(arg.Value->PS);
  encodeBytecode(arg.Value->DS);
  encodeBytecode(arg.Value->HS);
  encodeBytecode(arg.Value->GS);

  if (arg.Value->StreamOutput.pSODeclaration) {
    memcpy(dest + offset, arg.Value->StreamOutput.pSODeclaration,
           arg.Value->StreamOutput.NumEntries * sizeof(D3D12_SO_DECLARATION_ENTRY));
    offset += arg.Value->StreamOutput.NumEntries * sizeof(D3D12_SO_DECLARATION_ENTRY);

    for (unsigned i = 0; i < arg.Value->StreamOutput.NumEntries; ++i) {
      unsigned len = strlen(arg.Value->StreamOutput.pSODeclaration[i].SemanticName) + 1;
      memcpy(dest + offset, &len, sizeof(len));
      offset += sizeof(len);
      memcpy(dest + offset, arg.Value->StreamOutput.pSODeclaration[i].SemanticName, len);
      offset += len;
    }
  }
  if (arg.Value->StreamOutput.pBufferStrides) {
    memcpy(dest + offset, arg.Value->StreamOutput.pBufferStrides,
           arg.Value->StreamOutput.NumStrides * sizeof(UINT));
    offset += arg.Value->StreamOutput.NumStrides * sizeof(UINT);
  }

  memcpy(dest + offset, arg.Value->InputLayout.pInputElementDescs,
         sizeof(D3D12_INPUT_ELEMENT_DESC) * arg.Value->InputLayout.NumElements);
  offset += sizeof(D3D12_INPUT_ELEMENT_DESC) * arg.Value->InputLayout.NumElements;

  for (unsigned i = 0; i < arg.Value->InputLayout.NumElements; ++i) {
    const D3D12_INPUT_ELEMENT_DESC& inputElement = arg.Value->InputLayout.pInputElementDescs[i];
    unsigned len = strlen(inputElement.SemanticName) + 1;
    memcpy(dest + offset, &len, sizeof(len));
    offset += sizeof(len);
    memcpy(dest + offset, inputElement.SemanticName, len);
    offset += len;
  }

  if (arg.Value->CachedPSO.pCachedBlob) {
    memcpy(dest + offset, arg.Value->CachedPSO.pCachedBlob,
           arg.Value->CachedPSO.CachedBlobSizeInBytes);
    offset += arg.Value->CachedPSO.CachedBlobSizeInBytes;
  }

  memcpy(dest + offset, &arg.RootSignatureKey, sizeof(arg.RootSignatureKey));
  offset += sizeof(arg.RootSignatureKey);
}

unsigned GetSize(const D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC) + arg.Value->CS.BytecodeLength +
         arg.Value->CachedPSO.CachedBlobSizeInBytes + sizeof(arg.RootSignatureKey);
}

void Encode(char* dest, unsigned& offset, const D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.Value, sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC));
  offset += sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC);

  if (arg.Value->CS.pShaderBytecode) {
    memcpy(dest + offset, arg.Value->CS.pShaderBytecode, arg.Value->CS.BytecodeLength);
    offset += arg.Value->CS.BytecodeLength;
  }

  if (arg.Value->CachedPSO.pCachedBlob) {
    memcpy(dest + offset, arg.Value->CachedPSO.pCachedBlob,
           arg.Value->CachedPSO.CachedBlobSizeInBytes);
    offset += arg.Value->CachedPSO.CachedBlobSizeInBytes;
  }

  memcpy(dest + offset, &arg.RootSignatureKey, sizeof(arg.RootSignatureKey));
  offset += sizeof(arg.RootSignatureKey);
}

unsigned GetSize(const D3D12_TEXTURE_COPY_LOCATION_Argument& arg) {
  return sizeof(D3D12_TEXTURE_COPY_LOCATION) + sizeof(arg.ResourceKey);
}

void Encode(char* dest, unsigned& offset, const D3D12_TEXTURE_COPY_LOCATION_Argument& arg) {

  memcpy(dest + offset, arg.Value, sizeof(D3D12_TEXTURE_COPY_LOCATION));
  offset += sizeof(D3D12_TEXTURE_COPY_LOCATION);

  memcpy(dest + offset, &arg.ResourceKey, sizeof(arg.ResourceKey));
  offset += sizeof(arg.ResourceKey);
}

unsigned GetSize(const D3D12_RESOURCE_BARRIERs_Argument& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(arg.Size) + sizeof(D3D12_RESOURCE_BARRIER) * arg.Size +
         sizeof(unsigned) * arg.Size * 2;
}

void Encode(char* dest, unsigned& offset, const D3D12_RESOURCE_BARRIERs_Argument& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }
  memcpy(dest + offset, &arg.Size, sizeof(arg.Size));
  offset += sizeof(arg.Size);

  memcpy(dest + offset, arg.Value, sizeof(D3D12_RESOURCE_BARRIER) * arg.Size);
  offset += sizeof(D3D12_RESOURCE_BARRIER) * arg.Size;

  memcpy(dest + offset, arg.ResourceKeys.data(), sizeof(unsigned) * arg.Size);
  offset += sizeof(unsigned) * arg.Size;

  memcpy(dest + offset, arg.ResourceAfterKeys.data(), sizeof(unsigned) * arg.Size);
  offset += sizeof(unsigned) * arg.Size;
}

unsigned GetSize(const PointerArgument<D3D12_ROOT_SIGNATURE_DESC>& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  unsigned size = sizeof(void*) + sizeof(D3D12_ROOT_SIGNATURE_DESC);

  size += arg.Value->NumStaticSamplers * sizeof(D3D12_STATIC_SAMPLER_DESC);
  size += arg.Value->NumParameters * sizeof(D3D12_ROOT_PARAMETER);
  for (unsigned i = 0; i < arg.Value->NumParameters; ++i) {
    if (arg.Value->pParameters[i].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
      size += arg.Value->pParameters[i].DescriptorTable.NumDescriptorRanges *
              sizeof(D3D12_DESCRIPTOR_RANGE);
    }
  }
  return size;
}

void Encode(char* dest, unsigned& offset, const PointerArgument<D3D12_ROOT_SIGNATURE_DESC>& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.Value, sizeof(D3D12_ROOT_SIGNATURE_DESC));
  offset += sizeof(D3D12_ROOT_SIGNATURE_DESC);

  memcpy(dest + offset, arg.Value->pStaticSamplers,
         sizeof(D3D12_STATIC_SAMPLER_DESC) * arg.Value->NumStaticSamplers);
  offset += sizeof(D3D12_STATIC_SAMPLER_DESC) * arg.Value->NumStaticSamplers;

  memcpy(dest + offset, arg.Value->pParameters,
         sizeof(D3D12_ROOT_PARAMETER) * arg.Value->NumParameters);
  offset += sizeof(D3D12_ROOT_PARAMETER) * arg.Value->NumParameters;
  for (unsigned i = 0; i < arg.Value->NumParameters; ++i) {
    if (arg.Value->pParameters[i].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {

      memcpy(dest + offset, arg.Value->pParameters[i].DescriptorTable.pDescriptorRanges,
             sizeof(D3D12_DESCRIPTOR_RANGE) *
                 arg.Value->pParameters[i].DescriptorTable.NumDescriptorRanges);
      offset += sizeof(D3D12_DESCRIPTOR_RANGE) *
                arg.Value->pParameters[i].DescriptorTable.NumDescriptorRanges;
    }
  }
}

unsigned GetSize(const PointerArgument<D3D12_VERSIONED_ROOT_SIGNATURE_DESC>& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  unsigned size = sizeof(void*) + sizeof(D3D12_VERSIONED_ROOT_SIGNATURE_DESC);

  switch (arg.Value->Version) {
  case D3D_ROOT_SIGNATURE_VERSION_1_0: {
    D3D12_ROOT_SIGNATURE_DESC& desc0 = arg.Value->Desc_1_0;
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
    D3D12_ROOT_SIGNATURE_DESC1& desc1 = arg.Value->Desc_1_1;
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
    D3D12_ROOT_SIGNATURE_DESC2& desc2 = arg.Value->Desc_1_2;
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

void Encode(char* dest,
            unsigned& offset,
            const PointerArgument<D3D12_VERSIONED_ROOT_SIGNATURE_DESC>& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.Value, sizeof(D3D12_VERSIONED_ROOT_SIGNATURE_DESC));
  offset += sizeof(D3D12_VERSIONED_ROOT_SIGNATURE_DESC);

  switch (arg.Value->Version) {
  case D3D_ROOT_SIGNATURE_VERSION_1_0: {
    D3D12_ROOT_SIGNATURE_DESC& desc0 = arg.Value->Desc_1_0;

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
    D3D12_ROOT_SIGNATURE_DESC1& desc1 = arg.Value->Desc_1_1;

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
    D3D12_ROOT_SIGNATURE_DESC2& desc2 = arg.Value->Desc_1_2;

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

unsigned GetSize(const PointerArgument<D3D12_COMMAND_SIGNATURE_DESC>& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(D3D12_COMMAND_SIGNATURE_DESC) * arg.Value->NumArgumentDescs *
                             sizeof(D3D12_INDIRECT_ARGUMENT_DESC);
}

void Encode(char* dest,
            unsigned& offset,
            const PointerArgument<D3D12_COMMAND_SIGNATURE_DESC>& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.Value, sizeof(D3D12_COMMAND_SIGNATURE_DESC));
  offset += sizeof(D3D12_COMMAND_SIGNATURE_DESC);

  memcpy(dest + offset, arg.Value->pArgumentDescs,
         sizeof(D3D12_INDIRECT_ARGUMENT_DESC) * arg.Value->NumArgumentDescs);
  offset += sizeof(D3D12_INDIRECT_ARGUMENT_DESC) * arg.Value->NumArgumentDescs;
}

unsigned GetSize(const D3D12_INDEX_BUFFER_VIEW_Argument& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(D3D12_INDEX_BUFFER_VIEW) + sizeof(unsigned) * 2;
}

void Encode(char* dest, unsigned& offset, const D3D12_INDEX_BUFFER_VIEW_Argument& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.Value, sizeof(D3D12_INDEX_BUFFER_VIEW));
  offset += sizeof(D3D12_INDEX_BUFFER_VIEW);

  memcpy(dest + offset, &arg.BufferLocationKey, sizeof(arg.BufferLocationKey));
  offset += sizeof(arg.BufferLocationKey);
  memcpy(dest + offset, &arg.BufferLocationOffset, sizeof(arg.BufferLocationOffset));
  offset += sizeof(arg.BufferLocationOffset);
}

unsigned GetSize(const D3D12_CONSTANT_BUFFER_VIEW_DESC_Argument& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(D3D12_CONSTANT_BUFFER_VIEW_DESC) + sizeof(unsigned) * 2;
}

void Encode(char* dest, unsigned& offset, const D3D12_CONSTANT_BUFFER_VIEW_DESC_Argument& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.Value, sizeof(D3D12_CONSTANT_BUFFER_VIEW_DESC));
  offset += sizeof(D3D12_CONSTANT_BUFFER_VIEW_DESC);

  memcpy(dest + offset, &arg.BufferLocationKey, sizeof(arg.BufferLocationKey));
  offset += sizeof(arg.BufferLocationKey);
  memcpy(dest + offset, &arg.BufferLocationOffset, sizeof(arg.BufferLocationOffset));
  offset += sizeof(arg.BufferLocationOffset);
}

unsigned GetSize(const D3D12_VERTEX_BUFFER_VIEWs_Argument& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(arg.Size) + sizeof(D3D12_VERTEX_BUFFER_VIEW) * arg.Size +
         sizeof(unsigned) * arg.Size * 2;
}

void Encode(char* dest, unsigned& offset, const D3D12_VERTEX_BUFFER_VIEWs_Argument& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, &arg.Size, sizeof(arg.Size));
  offset += sizeof(arg.Size);

  memcpy(dest + offset, arg.Value, sizeof(D3D12_VERTEX_BUFFER_VIEW) * arg.Size);
  offset += sizeof(D3D12_VERTEX_BUFFER_VIEW) * arg.Size;

  memcpy(dest + offset, arg.BufferLocationKeys.data(), sizeof(unsigned) * arg.Size);
  offset += sizeof(unsigned) * arg.Size;

  memcpy(dest + offset, arg.BufferLocationOffsets.data(), sizeof(unsigned) * arg.Size);
  offset += sizeof(unsigned) * arg.Size;
}

unsigned GetSize(const D3D12_STREAM_OUTPUT_BUFFER_VIEWs_Argument& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(arg.Size) + sizeof(D3D12_STREAM_OUTPUT_BUFFER_VIEW) * arg.Size +
         sizeof(unsigned) * arg.Size * 4;
}

void Encode(char* dest, unsigned& offset, const D3D12_STREAM_OUTPUT_BUFFER_VIEWs_Argument& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, &arg.Size, sizeof(arg.Size));
  offset += sizeof(arg.Size);

  memcpy(dest + offset, arg.Value, sizeof(D3D12_STREAM_OUTPUT_BUFFER_VIEW) * arg.Size);
  offset += sizeof(D3D12_STREAM_OUTPUT_BUFFER_VIEW) * arg.Size;

  memcpy(dest + offset, arg.BufferLocationKeys.data(), sizeof(unsigned) * arg.Size);
  offset += sizeof(unsigned) * arg.Size;

  memcpy(dest + offset, arg.BufferLocationOffsets.data(), sizeof(unsigned) * arg.Size);
  offset += sizeof(unsigned) * arg.Size;

  memcpy(dest + offset, arg.BufferFilledSizeLocationKeys.data(), sizeof(unsigned) * arg.Size);
  offset += sizeof(unsigned) * arg.Size;

  memcpy(dest + offset, arg.BufferFilledSizeLocationOffsets.data(), sizeof(unsigned) * arg.Size);
  offset += sizeof(unsigned) * arg.Size;
}

unsigned GetSize(const D3D12_WRITEBUFFERIMMEDIATE_PARAMETERs_Argument& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(arg.Size) +
         sizeof(D3D12_WRITEBUFFERIMMEDIATE_PARAMETER) * arg.Size + sizeof(unsigned) * arg.Size * 2;
}

void Encode(char* dest,
            unsigned& offset,
            const D3D12_WRITEBUFFERIMMEDIATE_PARAMETERs_Argument& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, &arg.Size, sizeof(arg.Size));
  offset += sizeof(arg.Size);

  memcpy(dest + offset, arg.Value, sizeof(D3D12_WRITEBUFFERIMMEDIATE_PARAMETER) * arg.Size);
  offset += sizeof(D3D12_WRITEBUFFERIMMEDIATE_PARAMETER) * arg.Size;

  memcpy(dest + offset, arg.DestKeys.data(), sizeof(unsigned) * arg.Size);
  offset += sizeof(unsigned) * arg.Size;

  memcpy(dest + offset, arg.DestOffsets.data(), sizeof(unsigned) * arg.Size);
  offset += sizeof(unsigned) * arg.Size;
}

unsigned GetSize(const D3D12_STATE_OBJECT_DESC_Argument& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  unsigned size = sizeof(void*) + sizeof(D3D12_STATE_OBJECT_DESC) +
                  sizeof(D3D12_STATE_SUBOBJECT) * arg.Value->NumSubobjects;

  unsigned associationsCount = 0;

  for (unsigned index = 0; index < arg.Value->NumSubobjects; ++index) {
    const D3D12_STATE_SUBOBJECT& subobject = arg.Value->pSubobjects[index];

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

  size += sizeof(unsigned) + arg.InterfaceKeysBySubobject.size() * sizeof(unsigned) * 2;

  return size;
}

void Encode(char* dest, unsigned& offset, const D3D12_STATE_OBJECT_DESC_Argument& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.Value, sizeof(D3D12_STATE_OBJECT_DESC));
  offset += sizeof(D3D12_STATE_OBJECT_DESC);

  memcpy(dest + offset, arg.Value->pSubobjects,
         sizeof(D3D12_STATE_SUBOBJECT) * arg.Value->NumSubobjects);
  offset += sizeof(D3D12_STATE_SUBOBJECT) * arg.Value->NumSubobjects;

  std::map<const D3D12_STATE_SUBOBJECT*, unsigned> subobjectIndexes;

  for (unsigned index = 0; index < arg.Value->NumSubobjects; ++index) {
    const D3D12_STATE_SUBOBJECT& subobject = arg.Value->pSubobjects[index];

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
    for (unsigned index = 0; index < arg.Value->NumSubobjects; ++index) {
      D3D12_STATE_SUBOBJECT& subobject =
          const_cast<D3D12_STATE_SUBOBJECT&>(arg.Value->pSubobjects[index]);

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
    unsigned size = arg.InterfaceKeysBySubobject.size();
    memcpy(dest + offset, &size, sizeof(unsigned));
    offset += sizeof(unsigned);
    for (auto& it : arg.InterfaceKeysBySubobject) {
      memcpy(dest + offset, &it.first, sizeof(unsigned));
      offset += sizeof(unsigned);
      memcpy(dest + offset, &it.second, sizeof(unsigned));
      offset += sizeof(unsigned);
    }
  }
}

unsigned GetSize(const D3D12_PIPELINE_STATE_STREAM_DESC_Argument& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  unsigned size = sizeof(void*) + sizeof(D3D12_PIPELINE_STATE_STREAM_DESC) + arg.Value->SizeInBytes;

  size_t offset = 0;
  while (offset < arg.Value->SizeInBytes) {
    void* subobjectData = static_cast<char*>(arg.Value->pPipelineStateSubobjectStream) + offset;
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

  size += sizeof(arg.RootSignatureKey);
  return size;
}

void Encode(char* dest, unsigned& offset, const D3D12_PIPELINE_STATE_STREAM_DESC_Argument& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.Value, sizeof(D3D12_PIPELINE_STATE_STREAM_DESC));
  offset += sizeof(D3D12_PIPELINE_STATE_STREAM_DESC);

  memcpy(dest + offset, arg.Value->pPipelineStateSubobjectStream, arg.Value->SizeInBytes);
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

  memcpy(dest + offset, &arg.RootSignatureKey, sizeof(arg.RootSignatureKey));
  offset += sizeof(arg.RootSignatureKey);
}

unsigned GetSize(const PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS>& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }

  unsigned size = sizeof(void*) + sizeof(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS);
  if (arg.Value->Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
    if (arg.Value->DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY) {
      size += arg.Value->NumDescs * sizeof(D3D12_RAYTRACING_GEOMETRY_DESC);
      for (unsigned i = 0; i < arg.Value->NumDescs; ++i) {
        if (arg.Value->pGeometryDescs[i].Type == D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES) {
          if (arg.Value->pGeometryDescs[i].OmmTriangles.pTriangles) {
            size += sizeof(D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC);
          }
          if (arg.Value->pGeometryDescs[i].OmmTriangles.pOmmLinkage) {
            size += sizeof(D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC);
          }
        }
      }
    } else if (arg.Value->DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS) {
      size += arg.Value->NumDescs * sizeof(D3D12_RAYTRACING_GEOMETRY_DESC*);
      size += arg.Value->NumDescs * sizeof(D3D12_RAYTRACING_GEOMETRY_DESC);
      for (unsigned i = 0; i < arg.Value->NumDescs; ++i) {
        if (arg.Value->ppGeometryDescs[i]->Type == D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES) {
          if (arg.Value->ppGeometryDescs[i]->OmmTriangles.pTriangles) {
            size += sizeof(D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC);
          }
          if (arg.Value->ppGeometryDescs[i]->OmmTriangles.pOmmLinkage) {
            size += sizeof(D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC);
          }
        }
      }
    }
  } else if (arg.Value->Type ==
             D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_OPACITY_MICROMAP_ARRAY) {
    if (arg.Value->pOpacityMicromapArrayDesc) {
      size += sizeof(D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC);
      if (arg.Value->pOpacityMicromapArrayDesc->pOmmHistogram) {
        size += arg.Value->pOpacityMicromapArrayDesc->NumOmmHistogramEntries *
                sizeof(D3D12_RAYTRACING_OPACITY_MICROMAP_HISTOGRAM_ENTRY);
      }
    }
  }

  size += sizeof(unsigned) + sizeof(unsigned) * arg.InputKeys.size() * 2;

  return size;
}

void Encode(char* dest,
            unsigned& offset,
            const PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS>& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.Value, sizeof(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS));
  offset += sizeof(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS);

  if (arg.Value->Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
    if (arg.Value->DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY) {
      if (arg.Value->pGeometryDescs) {
        memcpy(dest + offset, arg.Value->pGeometryDescs,
               sizeof(D3D12_RAYTRACING_GEOMETRY_DESC) * arg.Value->NumDescs);
        offset += sizeof(D3D12_RAYTRACING_GEOMETRY_DESC) * arg.Value->NumDescs;
        for (unsigned i = 0; i < arg.Value->NumDescs; ++i) {
          if (arg.Value->pGeometryDescs[i].Type == D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES) {
            if (arg.Value->pGeometryDescs[i].OmmTriangles.pTriangles) {
              memcpy(dest + offset, arg.Value->pGeometryDescs[i].OmmTriangles.pTriangles,
                     sizeof(D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC));
              offset += sizeof(D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC);
            }
            if (arg.Value->pGeometryDescs[i].OmmTriangles.pOmmLinkage) {
              memcpy(dest + offset, arg.Value->pGeometryDescs[i].OmmTriangles.pOmmLinkage,
                     sizeof(D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC));
              offset += sizeof(D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC);
            }
          }
        }
      }
    } else if (arg.Value->DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS) {
      if (arg.Value->ppGeometryDescs) {
        memcpy(dest + offset, arg.Value->ppGeometryDescs,
               sizeof(D3D12_RAYTRACING_GEOMETRY_DESC*) * arg.Value->NumDescs);
        offset += sizeof(D3D12_RAYTRACING_GEOMETRY_DESC*) * arg.Value->NumDescs;
      }
      for (unsigned i = 0; i < arg.Value->NumDescs; ++i) {
        memcpy(dest + offset, arg.Value->ppGeometryDescs[i],
               sizeof(D3D12_RAYTRACING_GEOMETRY_DESC));
        offset += sizeof(D3D12_RAYTRACING_GEOMETRY_DESC);
        if (arg.Value->ppGeometryDescs[i]->Type == D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES) {
          if (arg.Value->ppGeometryDescs[i]->OmmTriangles.pTriangles) {
            memcpy(dest + offset, arg.Value->ppGeometryDescs[i]->OmmTriangles.pTriangles,
                   sizeof(D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC));
            offset += sizeof(D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC);
          }
          if (arg.Value->ppGeometryDescs[i]->OmmTriangles.pOmmLinkage) {
            memcpy(dest + offset, arg.Value->ppGeometryDescs[i]->OmmTriangles.pOmmLinkage,
                   sizeof(D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC));
            offset += sizeof(D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC);
          }
        }
      }
    }
  } else if (arg.Value->Type ==
             D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_OPACITY_MICROMAP_ARRAY) {
    if (arg.Value->pOpacityMicromapArrayDesc) {
      memcpy(dest + offset, arg.Value->pOpacityMicromapArrayDesc,
             sizeof(D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC));
      offset += sizeof(D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC);
      if (arg.Value->pOpacityMicromapArrayDesc->pOmmHistogram) {
        memcpy(dest + offset, arg.Value->pOpacityMicromapArrayDesc->pOmmHistogram,
               sizeof(D3D12_RAYTRACING_OPACITY_MICROMAP_HISTOGRAM_ENTRY) *
                   arg.Value->pOpacityMicromapArrayDesc->NumOmmHistogramEntries);
        offset += sizeof(D3D12_RAYTRACING_OPACITY_MICROMAP_HISTOGRAM_ENTRY) *
                  arg.Value->pOpacityMicromapArrayDesc->NumOmmHistogramEntries;
      }
    }
  }

  unsigned size = arg.InputKeys.size();
  memcpy(dest + offset, &size, sizeof(size));
  offset += sizeof(size);
  memcpy(dest + offset, arg.InputKeys.data(), sizeof(unsigned) * size);
  offset += sizeof(unsigned) * size;
  memcpy(dest + offset, arg.InputOffsets.data(), sizeof(unsigned) * size);
  offset += sizeof(unsigned) * size;
}

unsigned GetSize(const PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC>& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  unsigned size = sizeof(void*) + sizeof(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC);

  if (arg.Value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
    if (arg.Value->Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY) {
      size += arg.Value->Inputs.NumDescs * sizeof(D3D12_RAYTRACING_GEOMETRY_DESC);
      for (unsigned i = 0; i < arg.Value->Inputs.NumDescs; ++i) {
        if (arg.Value->Inputs.pGeometryDescs[i].Type ==
            D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES) {
          if (arg.Value->Inputs.pGeometryDescs[i].OmmTriangles.pTriangles) {
            size += sizeof(D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC);
          }
          if (arg.Value->Inputs.pGeometryDescs[i].OmmTriangles.pOmmLinkage) {
            size += sizeof(D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC);
          }
        }
      }
    } else if (arg.Value->Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS) {
      size += arg.Value->Inputs.NumDescs * sizeof(D3D12_RAYTRACING_GEOMETRY_DESC*);
      size += arg.Value->Inputs.NumDescs * sizeof(D3D12_RAYTRACING_GEOMETRY_DESC);
      for (unsigned i = 0; i < arg.Value->Inputs.NumDescs; ++i) {
        if (arg.Value->Inputs.ppGeometryDescs[i]->Type ==
            D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES) {
          if (arg.Value->Inputs.ppGeometryDescs[i]->OmmTriangles.pTriangles) {
            size += sizeof(D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC);
          }
          if (arg.Value->Inputs.ppGeometryDescs[i]->OmmTriangles.pOmmLinkage) {
            size += sizeof(D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC);
          }
        }
      }
    }
  } else if (arg.Value->Inputs.Type ==
             D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_OPACITY_MICROMAP_ARRAY) {
    if (arg.Value->Inputs.pOpacityMicromapArrayDesc) {
      size += sizeof(D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC);
      if (arg.Value->Inputs.pOpacityMicromapArrayDesc->pOmmHistogram) {
        size += arg.Value->Inputs.pOpacityMicromapArrayDesc->NumOmmHistogramEntries *
                sizeof(D3D12_RAYTRACING_OPACITY_MICROMAP_HISTOGRAM_ENTRY);
      }
    }
  }

  size += sizeof(unsigned) * 6;
  size += sizeof(unsigned) + sizeof(unsigned) * arg.InputKeys.size() * 2;

  return size;
}

void Encode(char* dest,
            unsigned& offset,
            const PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC>& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.Value, sizeof(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC));
  offset += sizeof(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC);

  if (arg.Value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
    if (arg.Value->Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY) {
      if (arg.Value->Inputs.pGeometryDescs) {
        memcpy(dest + offset, arg.Value->Inputs.pGeometryDescs,
               sizeof(D3D12_RAYTRACING_GEOMETRY_DESC) * arg.Value->Inputs.NumDescs);
        offset += sizeof(D3D12_RAYTRACING_GEOMETRY_DESC) * arg.Value->Inputs.NumDescs;
        for (unsigned i = 0; i < arg.Value->Inputs.NumDescs; ++i) {
          if (arg.Value->Inputs.pGeometryDescs[i].Type ==
              D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES) {
            if (arg.Value->Inputs.pGeometryDescs[i].OmmTriangles.pTriangles) {
              memcpy(dest + offset, arg.Value->Inputs.pGeometryDescs[i].OmmTriangles.pTriangles,
                     sizeof(D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC));
              offset += sizeof(D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC);
            }
            if (arg.Value->Inputs.pGeometryDescs[i].OmmTriangles.pOmmLinkage) {
              memcpy(dest + offset, arg.Value->Inputs.pGeometryDescs[i].OmmTriangles.pOmmLinkage,
                     sizeof(D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC));
              offset += sizeof(D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC);
            }
          }
        }
      }
    } else if (arg.Value->Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS) {
      if (arg.Value->Inputs.ppGeometryDescs) {
        memcpy(dest + offset, arg.Value->Inputs.ppGeometryDescs,
               sizeof(D3D12_RAYTRACING_GEOMETRY_DESC*) * arg.Value->Inputs.NumDescs);
        offset += sizeof(D3D12_RAYTRACING_GEOMETRY_DESC*) * arg.Value->Inputs.NumDescs;
      }
      for (unsigned i = 0; i < arg.Value->Inputs.NumDescs; ++i) {
        memcpy(dest + offset, arg.Value->Inputs.ppGeometryDescs[i],
               sizeof(D3D12_RAYTRACING_GEOMETRY_DESC));
        offset += sizeof(D3D12_RAYTRACING_GEOMETRY_DESC);
        if (arg.Value->Inputs.ppGeometryDescs[i]->Type ==
            D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES) {
          if (arg.Value->Inputs.ppGeometryDescs[i]->OmmTriangles.pTriangles) {
            memcpy(dest + offset, arg.Value->Inputs.ppGeometryDescs[i]->OmmTriangles.pTriangles,
                   sizeof(D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC));
            offset += sizeof(D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC);
          }
          if (arg.Value->Inputs.ppGeometryDescs[i]->OmmTriangles.pOmmLinkage) {
            memcpy(dest + offset, arg.Value->Inputs.ppGeometryDescs[i]->OmmTriangles.pOmmLinkage,
                   sizeof(D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC));
            offset += sizeof(D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC);
          }
        }
      }
    }
  } else if (arg.Value->Inputs.Type ==
             D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_OPACITY_MICROMAP_ARRAY) {
    if (arg.Value->Inputs.pOpacityMicromapArrayDesc) {
      memcpy(dest + offset, arg.Value->Inputs.pOpacityMicromapArrayDesc,
             sizeof(D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC));
      offset += sizeof(D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC);
      if (arg.Value->Inputs.pOpacityMicromapArrayDesc->pOmmHistogram) {
        memcpy(dest + offset, arg.Value->Inputs.pOpacityMicromapArrayDesc->pOmmHistogram,
               sizeof(D3D12_RAYTRACING_OPACITY_MICROMAP_HISTOGRAM_ENTRY) *
                   arg.Value->Inputs.pOpacityMicromapArrayDesc->NumOmmHistogramEntries);
        offset += sizeof(D3D12_RAYTRACING_OPACITY_MICROMAP_HISTOGRAM_ENTRY) *
                  arg.Value->Inputs.pOpacityMicromapArrayDesc->NumOmmHistogramEntries;
      }
    }
  }

  memcpy(dest + offset, &arg.DestAccelerationStructureKey,
         sizeof(arg.DestAccelerationStructureKey));
  offset += sizeof(arg.DestAccelerationStructureKey);
  memcpy(dest + offset, &arg.DestAccelerationStructureOffset,
         sizeof(arg.DestAccelerationStructureOffset));
  offset += sizeof(arg.DestAccelerationStructureOffset);

  memcpy(dest + offset, &arg.SourceAccelerationStructureKey,
         sizeof(arg.SourceAccelerationStructureKey));
  offset += sizeof(arg.SourceAccelerationStructureKey);
  memcpy(dest + offset, &arg.SourceAccelerationStructureOffset,
         sizeof(arg.SourceAccelerationStructureOffset));
  offset += sizeof(arg.SourceAccelerationStructureOffset);

  memcpy(dest + offset, &arg.ScratchAccelerationStructureKey,
         sizeof(arg.ScratchAccelerationStructureKey));
  offset += sizeof(arg.ScratchAccelerationStructureKey);
  memcpy(dest + offset, &arg.ScratchAccelerationStructureOffset,
         sizeof(arg.ScratchAccelerationStructureOffset));
  offset += sizeof(arg.ScratchAccelerationStructureOffset);

  unsigned size = arg.InputKeys.size();
  memcpy(dest + offset, &size, sizeof(size));
  offset += sizeof(size);
  memcpy(dest + offset, arg.InputKeys.data(), sizeof(unsigned) * size);
  offset += sizeof(unsigned) * size;
  memcpy(dest + offset, arg.InputOffsets.data(), sizeof(unsigned) * size);
  offset += sizeof(unsigned) * size;
}

unsigned GetSize(const PointerArgument<D3D12_DISPATCH_RAYS_DESC>& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  unsigned size = sizeof(void*) + sizeof(D3D12_DISPATCH_RAYS_DESC);
  size += sizeof(unsigned) * 8;
  return size;
}

void Encode(char* dest, unsigned& offset, const PointerArgument<D3D12_DISPATCH_RAYS_DESC>& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.Value, sizeof(D3D12_DISPATCH_RAYS_DESC));
  offset += sizeof(D3D12_DISPATCH_RAYS_DESC);

  memcpy(dest + offset, &arg.RayGenerationShaderRecordKey,
         sizeof(arg.RayGenerationShaderRecordKey));
  offset += sizeof(arg.RayGenerationShaderRecordKey);
  memcpy(dest + offset, &arg.RayGenerationShaderRecordOffset,
         sizeof(arg.RayGenerationShaderRecordOffset));
  offset += sizeof(arg.RayGenerationShaderRecordOffset);

  memcpy(dest + offset, &arg.MissShaderTableKey, sizeof(arg.MissShaderTableKey));
  offset += sizeof(arg.MissShaderTableKey);
  memcpy(dest + offset, &arg.MissShaderTableOffset, sizeof(arg.MissShaderTableOffset));
  offset += sizeof(arg.MissShaderTableOffset);

  memcpy(dest + offset, &arg.HitGroupTableKey, sizeof(arg.HitGroupTableKey));
  offset += sizeof(arg.HitGroupTableKey);
  memcpy(dest + offset, &arg.HitGroupTableOffset, sizeof(arg.HitGroupTableOffset));
  offset += sizeof(arg.HitGroupTableOffset);

  memcpy(dest + offset, &arg.CallableShaderTableKey, sizeof(arg.CallableShaderTableKey));
  offset += sizeof(arg.CallableShaderTableKey);
  memcpy(dest + offset, &arg.CallableShaderTableOffset, sizeof(arg.CallableShaderTableOffset));
  offset += sizeof(arg.CallableShaderTableOffset);
}

unsigned GetSize(
    const ArrayArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(arg.Size) +
         (sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC) +
          sizeof(unsigned) * 2) *
             arg.Size;
}

void Encode(char* dest,
            unsigned& offset,
            const ArrayArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, &arg.Size, sizeof(arg.Size));
  offset += sizeof(arg.Size);

  memcpy(dest + offset, arg.Value,
         sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC) * arg.Size);
  offset += sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC) * arg.Size;

  memcpy(dest + offset, arg.DestBufferKeys.data(), sizeof(unsigned) * arg.Size);
  offset += sizeof(unsigned) * arg.Size;

  memcpy(dest + offset, arg.DestBufferOffsets.data(), sizeof(unsigned) * arg.Size);
  offset += sizeof(unsigned) * arg.Size;
}

unsigned GetSize(
    const PointerArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC) +
         sizeof(unsigned) * 2;
}

void Encode(
    char* dest,
    unsigned& offset,
    const PointerArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.Value,
         sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC));
  offset += sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC);

  memcpy(dest + offset, &arg.destBufferKey, sizeof(unsigned));
  offset += sizeof(unsigned);

  memcpy(dest + offset, &arg.destBufferOffset, sizeof(unsigned));
  offset += sizeof(unsigned);
}

unsigned GetSize(const D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  unsigned size =
      sizeof(void*) + sizeof(arg.Size) + sizeof(D3D12_RENDER_PASS_RENDER_TARGET_DESC) * arg.Size;

  for (unsigned i = 0; i < arg.Size; ++i) {
    if (arg.Value[i].EndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE) {
      size += sizeof(D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS) *
              arg.Value[i].EndingAccess.Resolve.SubresourceCount;
    }
  }

  size += arg.DescriptorKeys.size() * sizeof(unsigned) +
          arg.DescriptorIndexes.size() * sizeof(unsigned) + sizeof(unsigned) +
          arg.ResolveSrcResourceKeys.size() * sizeof(unsigned) +
          arg.ResolveDstResourceKeys.size() * sizeof(unsigned);
  return size;
}

void Encode(char* dest,
            unsigned& offset,
            const D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, &arg.Size, sizeof(arg.Size));
  offset += sizeof(arg.Size);

  memcpy(dest + offset, arg.Value, sizeof(D3D12_RENDER_PASS_RENDER_TARGET_DESC) * arg.Size);
  offset += sizeof(D3D12_RENDER_PASS_RENDER_TARGET_DESC) * arg.Size;

  for (unsigned i = 0; i < arg.Size; ++i) {
    if (arg.Value[i].EndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE) {
      memcpy(dest + offset, arg.Value[i].EndingAccess.Resolve.pSubresourceParameters,
             sizeof(D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS) *
                 arg.Value[i].EndingAccess.Resolve.SubresourceCount);
      offset += sizeof(D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS) *
                arg.Value[i].EndingAccess.Resolve.SubresourceCount;
    }
  }

  memcpy(dest + offset, arg.DescriptorKeys.data(), sizeof(unsigned) * arg.Size);
  offset += sizeof(unsigned) * arg.Size;

  memcpy(dest + offset, arg.DescriptorIndexes.data(), sizeof(unsigned) * arg.Size);
  offset += sizeof(unsigned) * arg.Size;

  unsigned size = arg.ResolveSrcResourceKeys.size();
  memcpy(dest + offset, &size, sizeof(size));
  offset += sizeof(size);
  memcpy(dest + offset, arg.ResolveSrcResourceKeys.data(), sizeof(unsigned) * size);
  offset += sizeof(unsigned) * size;
  memcpy(dest + offset, arg.ResolveDstResourceKeys.data(), sizeof(unsigned) * size);
  offset += sizeof(unsigned) * size;
}

unsigned GetSize(const D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  unsigned size = sizeof(void*) + sizeof(D3D12_RENDER_PASS_DEPTH_STENCIL_DESC);

  if (arg.Value->DepthEndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE) {
    size += sizeof(D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS) *
            arg.Value->DepthEndingAccess.Resolve.SubresourceCount;
  }
  if (arg.Value->StencilEndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE) {
    size += sizeof(D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS) *
            arg.Value->StencilEndingAccess.Resolve.SubresourceCount;
  }

  size += sizeof(unsigned) * 6;
  return size;
}

void Encode(char* dest,
            unsigned& offset,
            const D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.Value, sizeof(D3D12_RENDER_PASS_DEPTH_STENCIL_DESC));
  offset += sizeof(D3D12_RENDER_PASS_DEPTH_STENCIL_DESC);

  if (arg.Value->DepthEndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE) {
    memcpy(dest + offset, arg.Value->DepthEndingAccess.Resolve.pSubresourceParameters,
           sizeof(D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS) *
               arg.Value->DepthEndingAccess.Resolve.SubresourceCount);
    offset += sizeof(D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS) *
              arg.Value->DepthEndingAccess.Resolve.SubresourceCount;
  }
  if (arg.Value->StencilEndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE) {
    memcpy(dest + offset, arg.Value->StencilEndingAccess.Resolve.pSubresourceParameters,
           sizeof(D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS) *
               arg.Value->StencilEndingAccess.Resolve.SubresourceCount);
    offset += sizeof(D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS) *
              arg.Value->StencilEndingAccess.Resolve.SubresourceCount;
  }

  memcpy(dest + offset, &arg.DescriptorKey, sizeof(unsigned));
  offset += sizeof(unsigned);
  memcpy(dest + offset, &arg.DescriptorIndex, sizeof(unsigned));
  offset += sizeof(unsigned);

  memcpy(dest + offset, &arg.ResolveSrcDepthKey, sizeof(unsigned));
  offset += sizeof(unsigned);
  memcpy(dest + offset, &arg.ResolveDstDepthKey, sizeof(unsigned));
  offset += sizeof(unsigned);
  memcpy(dest + offset, &arg.ResolveSrcStencilKey, sizeof(unsigned));
  offset += sizeof(unsigned);
  memcpy(dest + offset, &arg.ResolveDstStencilKey, sizeof(unsigned));
  offset += sizeof(unsigned);
}

unsigned GetSize(const D3D12_SHADER_RESOURCE_VIEW_DESC_Argument& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  unsigned size = sizeof(void*) + sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC);

  if (arg.Value->ViewDimension == D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE) {
    size += sizeof(unsigned) * 2;
  }

  return size;
}

void Encode(char* dest, unsigned& offset, const D3D12_SHADER_RESOURCE_VIEW_DESC_Argument& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }
  memcpy(dest + offset, arg.Value, sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC));
  offset += sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC);

  if (arg.Value->ViewDimension == D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE) {
    memcpy(dest + offset, &arg.RaytracingLocationKey, sizeof(unsigned));
    offset += sizeof(unsigned);
    memcpy(dest + offset, &arg.RaytracingLocationOffset, sizeof(unsigned));
    offset += sizeof(unsigned);
  }
}

unsigned GetSize(const D3D12_BARRIER_GROUPs_Argument& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  unsigned size = sizeof(void*) + sizeof(arg.Size) + sizeof(D3D12_BARRIER_GROUP) * arg.Size;

  for (unsigned i = 0; i < arg.Size; ++i) {
    if (arg.Value[i].Type == D3D12_BARRIER_TYPE_GLOBAL) {
      size += sizeof(D3D12_GLOBAL_BARRIER) * arg.Value[i].NumBarriers;
    } else if (arg.Value[i].Type == D3D12_BARRIER_TYPE_TEXTURE) {
      size += sizeof(D3D12_TEXTURE_BARRIER) * arg.Value[i].NumBarriers;
    } else if (arg.Value[i].Type == D3D12_BARRIER_TYPE_BUFFER) {
      size += sizeof(D3D12_BUFFER_BARRIER) * arg.Value[i].NumBarriers;
    }
  }

  size += sizeof(unsigned) * arg.ResourceKeys.size();

  return size;
}

void Encode(char* dest, unsigned& offset, const D3D12_BARRIER_GROUPs_Argument& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }
  memcpy(dest + offset, &arg.Size, sizeof(arg.Size));
  offset += sizeof(arg.Size);

  memcpy(dest + offset, arg.Value, sizeof(D3D12_BARRIER_GROUP) * arg.Size);
  offset += sizeof(D3D12_BARRIER_GROUP) * arg.Size;

  for (unsigned i = 0; i < arg.Size; ++i) {
    if (arg.Value[i].Type == D3D12_BARRIER_TYPE_GLOBAL) {
      memcpy(dest + offset, arg.Value[i].pGlobalBarriers,
             sizeof(D3D12_GLOBAL_BARRIER) * arg.Value[i].NumBarriers);
      offset += sizeof(D3D12_GLOBAL_BARRIER) * arg.Value[i].NumBarriers;
    } else if (arg.Value[i].Type == D3D12_BARRIER_TYPE_TEXTURE) {
      memcpy(dest + offset, arg.Value[i].pTextureBarriers,
             sizeof(D3D12_TEXTURE_BARRIER) * arg.Value[i].NumBarriers);
      offset += sizeof(D3D12_TEXTURE_BARRIER) * arg.Value[i].NumBarriers;
    } else if (arg.Value[i].Type == D3D12_BARRIER_TYPE_BUFFER) {
      memcpy(dest + offset, arg.Value[i].pBufferBarriers,
             sizeof(D3D12_BUFFER_BARRIER) * arg.Value[i].NumBarriers);
      offset += sizeof(D3D12_BUFFER_BARRIER) * arg.Value[i].NumBarriers;
    }
  }

  memcpy(dest + offset, arg.ResourceKeys.data(), sizeof(unsigned) * arg.ResourceKeys.size());
  offset += sizeof(unsigned) * arg.ResourceKeys.size();
}

unsigned GetSize(const ArrayArgument<D3D12_LINEAR_ALGEBRA_MATRIX_CONVERSION_INFO>& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  unsigned size = sizeof(void*) + sizeof(arg.Size) +
                  arg.Size * sizeof(D3D12_LINEAR_ALGEBRA_MATRIX_CONVERSION_INFO);
  size += 4 * arg.Size * sizeof(unsigned);
  return size;
}

void Encode(char* dest,
            unsigned& offset,
            const ArrayArgument<D3D12_LINEAR_ALGEBRA_MATRIX_CONVERSION_INFO>& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, &arg.Size, sizeof(arg.Size));
  offset += sizeof(arg.Size);

  memcpy(dest + offset, arg.Value, arg.Size * sizeof(D3D12_LINEAR_ALGEBRA_MATRIX_CONVERSION_INFO));
  offset += arg.Size * sizeof(D3D12_LINEAR_ALGEBRA_MATRIX_CONVERSION_INFO);

  memcpy(dest + offset, arg.DestKey.data(), arg.Size * sizeof(unsigned));
  offset += arg.Size * sizeof(unsigned);
  memcpy(dest + offset, arg.DestOffset.data(), arg.Size * sizeof(unsigned));
  offset += arg.Size * sizeof(unsigned);

  memcpy(dest + offset, arg.SourceKey.data(), arg.Size * sizeof(unsigned));
  offset += arg.Size * sizeof(unsigned);
  memcpy(dest + offset, arg.SourceOffset.data(), arg.Size * sizeof(unsigned));
  offset += arg.Size * sizeof(unsigned);
}

unsigned GetSize(const PointerArgument<D3D12_UNORDERED_ACCESS_VIEW_DESC>& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }

  // WA for D3D12_UNORDERED_ACCESS_VIEW_DESC size change in Agility SDK 1.719.1-preview
  constexpr unsigned sizeAdjustment =
      (sizeof(D3D12_BUFFER_UAV_BYTE_OFFSET) - sizeof(D3D12_BUFFER_UAV));
  return sizeof(void*) + sizeof(D3D12_UNORDERED_ACCESS_VIEW_DESC) - sizeAdjustment;
}

void Encode(char* dest,
            unsigned& offset,
            const PointerArgument<D3D12_UNORDERED_ACCESS_VIEW_DESC>& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }

  // WA for D3D12_UNORDERED_ACCESS_VIEW_DESC size change in Agility SDK 1.719.1-preview
  GITS_ASSERT(arg.Value->ViewDimension != D3D12_UAV_DIMENSION_BUFFER_BYTE_OFFSET);
  constexpr unsigned sizeAdjustment =
      (sizeof(D3D12_BUFFER_UAV_BYTE_OFFSET) - sizeof(D3D12_BUFFER_UAV));
  constexpr unsigned size = sizeof(D3D12_UNORDERED_ACCESS_VIEW_DESC) - sizeAdjustment;
  memcpy(dest + offset, arg.Value, size);
  offset += size;
}

unsigned GetSize(const DML_BINDING_DESC_Argument& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  return sizeof(void*) + dml::GetSize(arg.Value, 1) + sizeof(arg.ResourceKeysSize) +
         (sizeof(unsigned) * arg.ResourceKeysSize);
}

void Encode(char* dest, unsigned& offset, const DML_BINDING_DESC_Argument& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }

  dml::Encode(arg.Value, 1, dest, offset);

  memcpy(dest + offset, &arg.ResourceKeysSize, sizeof(arg.ResourceKeysSize));
  offset += sizeof(arg.ResourceKeysSize);

  memcpy(dest + offset, arg.ResourceKeys.data(), sizeof(unsigned) * arg.ResourceKeysSize);
  offset += sizeof(unsigned) * arg.ResourceKeysSize;
}

unsigned GetSize(const DML_BINDING_DESCs_Argument& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(arg.Size) + dml::GetSize(arg.Value, arg.Size) +
         sizeof(arg.ResourceKeysSize) + (sizeof(unsigned) * arg.ResourceKeysSize);
}

void Encode(char* dest, unsigned& offset, const DML_BINDING_DESCs_Argument& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, &arg.Size, sizeof(arg.Size));
  offset += sizeof(arg.Size);

  dml::Encode(arg.Value, arg.Size, dest, offset);

  memcpy(dest + offset, &arg.ResourceKeysSize, sizeof(arg.ResourceKeysSize));
  offset += sizeof(arg.ResourceKeysSize);

  memcpy(dest + offset, arg.ResourceKeys.data(), sizeof(unsigned) * arg.ResourceKeysSize);
  offset += sizeof(unsigned) * arg.ResourceKeysSize;
}

unsigned GetSize(const DML_BINDING_TABLE_DESC_Argument& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(DML_BINDING_TABLE_DESC) +
         sizeof(DML_BINDING_TABLE_DESC_Argument::BindingTableFields);
}

void Encode(char* dest, unsigned& offset, const DML_BINDING_TABLE_DESC_Argument& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.Value, sizeof(DML_BINDING_TABLE_DESC));
  offset += sizeof(DML_BINDING_TABLE_DESC);

  memcpy(dest + offset, &arg.TableFields, sizeof(arg.TableFields));
  offset += sizeof(arg.TableFields);
}

unsigned GetSize(const DML_OPERATOR_DESC_Argument& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  return sizeof(void*) + dml::GetSize(arg.Value, 1);
}

void Encode(char* dest, unsigned& offset, const DML_OPERATOR_DESC_Argument& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }
  dml::Encode(arg.Value, 1, dest, offset);
}

unsigned GetSize(const DML_GRAPH_DESC_Argument& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  return sizeof(void*) + dml::GetSize(arg.Value, 1) + sizeof(unsigned) +
         (sizeof(unsigned) * arg.OperatorKeysSize);
}

void Encode(char* dest, unsigned& offset, const DML_GRAPH_DESC_Argument& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }

  dml::Encode(arg.Value, 1, dest, offset);

  memcpy(dest + offset, &arg.OperatorKeysSize, sizeof(unsigned));
  offset += sizeof(unsigned);

  memcpy(dest + offset, arg.OperatorKeys.data(), sizeof(unsigned) * arg.OperatorKeysSize);
  offset += sizeof(unsigned) * arg.OperatorKeysSize;
}

unsigned GetSize(const DML_CheckFeatureSupport_BufferArgument& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }

  size_t totalSize = sizeof(void*) + sizeof(arg.Size) + arg.Size + sizeof(arg.feature);
  if (arg.feature == DML_FEATURE_FEATURE_LEVELS) {
    auto* featlevels = reinterpret_cast<DML_FEATURE_QUERY_FEATURE_LEVELS*>(arg.Value);
    totalSize += featlevels->RequestedFeatureLevelCount * sizeof(DML_FEATURE_LEVEL);
  }

  return totalSize;
}

void Encode(char* dest, unsigned& offset, const DML_CheckFeatureSupport_BufferArgument& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }
  Encode(dest, offset, arg.Size);
  Encode(dest, offset, arg.feature);

  if (arg.feature == DML_FEATURE_FEATURE_LEVELS) {
    auto* featlevels = reinterpret_cast<DML_FEATURE_QUERY_FEATURE_LEVELS*>(arg.Value);
    Encode(dest, offset, *featlevels);
    if (featlevels->RequestedFeatureLevelCount > 0) {
      dml::Encode(featlevels->RequestedFeatureLevels, featlevels->RequestedFeatureLevelCount, dest,
                  offset);
    }
  } else {
    memcpy(dest + offset, arg.Value, arg.Size);
    offset += arg.Size;
  }
}

unsigned GetSize(const PointerArgument<INTCExtensionAppInfo>& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  unsigned size = sizeof(void*) + sizeof(INTCExtensionAppInfo);

  if (arg.Value->pApplicationName) {
    size += sizeof(unsigned) + wcslen(arg.ApplicationName) * 2 + 2;
  }
  if (arg.Value->pEngineName) {
    size += sizeof(unsigned) + wcslen(arg.EngineName) * 2 + 2;
  }

  return size;
}

void Encode(char* dest, unsigned& offset, const PointerArgument<INTCExtensionAppInfo>& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.Value, sizeof(INTCExtensionAppInfo));
  offset += sizeof(INTCExtensionAppInfo);

  if (arg.Value->pApplicationName) {
    unsigned len = wcslen(arg.ApplicationName) * 2 + 2;
    memcpy(dest + offset, &len, sizeof(len));
    offset += sizeof(unsigned);
    memcpy(dest + offset, arg.ApplicationName, len);
    offset += len;
  }
  if (arg.Value->pEngineName) {
    unsigned len = wcslen(arg.EngineName) * 2 + 2;
    memcpy(dest + offset, &len, sizeof(len));
    offset += sizeof(unsigned);
    memcpy(dest + offset, arg.EngineName, len);
    offset += len;
  }
}

unsigned GetSize(const PointerArgument<INTCExtensionAppInfo1>& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  unsigned size = sizeof(void*) + sizeof(INTCExtensionAppInfo1);

  if (arg.ApplicationName) {
    size += sizeof(unsigned) + wcslen(arg.ApplicationName) * 2 + 2;
  }
  if (arg.EngineName) {
    size += sizeof(unsigned) + wcslen(arg.EngineName) * 2 + 2;
  }

  return size;
}

void Encode(char* dest, unsigned& offset, const PointerArgument<INTCExtensionAppInfo1>& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.Value, sizeof(INTCExtensionAppInfo1));
  offset += sizeof(INTCExtensionAppInfo1);

  if (arg.Value->pApplicationName) {
    unsigned len = wcslen(arg.ApplicationName) * 2 + 2;
    memcpy(dest + offset, &len, sizeof(len));
    offset += sizeof(unsigned);
    memcpy(dest + offset, arg.ApplicationName, len);
    offset += len;
  }
  if (arg.Value->pEngineName) {
    unsigned len = wcslen(arg.EngineName) * 2 + 2;
    memcpy(dest + offset, &len, sizeof(len));
    offset += sizeof(unsigned);
    memcpy(dest + offset, arg.EngineName, len);
    offset += len;
  }
}

unsigned GetSize(const PointerArgument<INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC>& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  unsigned size = sizeof(void*) + sizeof(INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC) +
                  sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC) + arg.Value->CS.BytecodeLength;

  if (arg.Value->CompileOptions) {
    size += sizeof(unsigned) + strlen(static_cast<const char*>(arg.CompileOptions)) + 1;
  }
  if (arg.Value->InternalOptions) {
    size += sizeof(unsigned) + strlen(static_cast<const char*>(arg.InternalOptions)) + 1;
  }
  size += sizeof(arg.RootSignatureKey);

  return size;
}

void Encode(char* dest,
            unsigned& offset,
            const PointerArgument<INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC>& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.Value, sizeof(INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC));
  offset += sizeof(INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC);

  memcpy(dest + offset, arg.Value->pD3D12Desc, sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC));
  offset += sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC);

  memcpy(dest + offset, arg.Cs, arg.Value->CS.BytecodeLength);
  offset += arg.Value->CS.BytecodeLength;

  if (arg.Value->CompileOptions) {
    unsigned len = strlen(static_cast<const char*>(arg.CompileOptions)) + 1;
    memcpy(dest + offset, &len, sizeof(len));
    offset += sizeof(len);
    memcpy(dest + offset, arg.CompileOptions, len);
    offset += len;
  }

  if (arg.Value->InternalOptions) {
    unsigned len = strlen(static_cast<const char*>(arg.InternalOptions)) + 1;
    memcpy(dest + offset, &len, sizeof(len));
    offset += sizeof(len);
    memcpy(dest + offset, arg.InternalOptions, len);
    offset += len;
  }

  memcpy(dest + offset, &arg.RootSignatureKey, sizeof(arg.RootSignatureKey));
  offset += sizeof(arg.RootSignatureKey);
}

unsigned GetSize(const PointerArgument<INTC_D3D12_RESOURCE_DESC_0001>& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(INTC_D3D12_RESOURCE_DESC_0001) + sizeof(D3D12_RESOURCE_DESC);
}

void Encode(char* dest,
            unsigned& offset,
            const PointerArgument<INTC_D3D12_RESOURCE_DESC_0001>& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.Value, sizeof(INTC_D3D12_RESOURCE_DESC_0001));
  offset += sizeof(INTC_D3D12_RESOURCE_DESC_0001);

  memcpy(dest + offset, arg.Value->pD3D12Desc, sizeof(D3D12_RESOURCE_DESC));
  offset += sizeof(D3D12_RESOURCE_DESC);
}

unsigned GetSize(const PointerArgument<INTC_D3D12_HEAP_DESC>& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(INTC_D3D12_HEAP_DESC) + sizeof(D3D12_HEAP_DESC);
}

void Encode(char* dest, unsigned& offset, const PointerArgument<INTC_D3D12_HEAP_DESC>& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.Value, sizeof(INTC_D3D12_HEAP_DESC));
  offset += sizeof(INTC_D3D12_HEAP_DESC);

  memcpy(dest + offset, arg.Value->pD3D12Desc, sizeof(D3D12_HEAP_DESC));
  offset += sizeof(D3D12_HEAP_DESC);
}

unsigned GetSize(const xess_d3d12_init_params_t_Argument& arg) {
  return sizeof(xess_d3d12_init_params_t) + sizeof(arg.Key) + sizeof(arg.TempBufferHeapKey) +
         sizeof(arg.TempTextureHeapKey) + sizeof(arg.PipelineLibraryKey);
}

void Encode(char* dest, unsigned& offset, const xess_d3d12_init_params_t_Argument& arg) {
  memcpy(dest + offset, arg.Value, sizeof(xess_d3d12_init_params_t));
  offset += sizeof(xess_d3d12_init_params_t);

  memcpy(dest + offset, &arg.Key, sizeof(arg.Key));
  offset += sizeof(arg.Key);

  memcpy(dest + offset, &arg.TempBufferHeapKey, sizeof(arg.TempBufferHeapKey));
  offset += sizeof(arg.TempBufferHeapKey);

  memcpy(dest + offset, &arg.TempTextureHeapKey, sizeof(arg.TempTextureHeapKey));
  offset += sizeof(arg.TempTextureHeapKey);

  memcpy(dest + offset, &arg.PipelineLibraryKey, sizeof(arg.PipelineLibraryKey));
  offset += sizeof(arg.PipelineLibraryKey);
}

unsigned GetSize(const xess_d3d12_execute_params_t_Argument& arg) {
  return sizeof(xess_d3d12_execute_params_t) + sizeof(arg.ColorTextureKey) +
         sizeof(arg.VelocityTextureKey) + sizeof(arg.DepthTextureKey) +
         sizeof(arg.ExposureScaleTextureKey) + sizeof(arg.ResponsivePixelMaskTextureKey) +
         sizeof(arg.OutputTextureKey) + sizeof(arg.DescriptorHeapKey);
}

void Encode(char* dest, unsigned& offset, const xess_d3d12_execute_params_t_Argument& arg) {
  memcpy(dest + offset, arg.Value, sizeof(xess_d3d12_execute_params_t));
  offset += sizeof(xess_d3d12_execute_params_t);

  memcpy(dest + offset, &arg.ColorTextureKey, sizeof(arg.ColorTextureKey));
  offset += sizeof(arg.ColorTextureKey);

  memcpy(dest + offset, &arg.VelocityTextureKey, sizeof(arg.VelocityTextureKey));
  offset += sizeof(arg.VelocityTextureKey);

  memcpy(dest + offset, &arg.DepthTextureKey, sizeof(arg.DepthTextureKey));
  offset += sizeof(arg.DepthTextureKey);

  memcpy(dest + offset, &arg.ExposureScaleTextureKey, sizeof(arg.ExposureScaleTextureKey));
  offset += sizeof(arg.ExposureScaleTextureKey);

  memcpy(dest + offset, &arg.ResponsivePixelMaskTextureKey,
         sizeof(arg.ResponsivePixelMaskTextureKey));
  offset += sizeof(arg.ResponsivePixelMaskTextureKey);

  memcpy(dest + offset, &arg.OutputTextureKey, sizeof(arg.OutputTextureKey));
  offset += sizeof(arg.OutputTextureKey);

  memcpy(dest + offset, &arg.DescriptorHeapKey, sizeof(arg.DescriptorHeapKey));
  offset += sizeof(arg.DescriptorHeapKey);
}

unsigned GetSize(const ArrayArgument<D3D12_META_COMMAND_DESC>& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(arg.Size) + (sizeof(D3D12_META_COMMAND_DESC) * arg.Size);
}

void Encode(char* dest, unsigned& offset, const ArrayArgument<D3D12_META_COMMAND_DESC>& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, &arg.Size, sizeof(arg.Size));
  offset += sizeof(arg.Size);

  auto* values = (D3D12_META_COMMAND_DESC*)(dest + offset);
  memcpy(dest + offset, arg.Value, sizeof(D3D12_META_COMMAND_DESC) * arg.Size);
  offset += sizeof(D3D12_META_COMMAND_DESC) * arg.Size;

  // D3D12_META_COMMAND_DESC has an LPCWSTR Name field
  // This field is allocated in the driver and we don't need to Encode it in the stream since this would break Trace::pre
  for (unsigned i = 0; i < arg.Size; ++i) {
    values[i].Name = nullptr;
  }
}

unsigned GetSize(const DSTORAGE_QUEUE_DESC_Argument& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }

  unsigned size = sizeof(void*) + sizeof(DSTORAGE_QUEUE_DESC) + sizeof(arg.DeviceKey);
  if (arg.Value->Name) {
    size += sizeof(unsigned) + strlen(arg.Value->Name) + 1;
  }
  return size;
}

void Encode(char* dest, unsigned& offset, const DSTORAGE_QUEUE_DESC_Argument& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.Value, sizeof(DSTORAGE_QUEUE_DESC));
  offset += sizeof(DSTORAGE_QUEUE_DESC);

  memcpy(dest + offset, &arg.DeviceKey, sizeof(arg.DeviceKey));
  offset += sizeof(arg.DeviceKey);

  if (arg.Value->Name) {
    unsigned len = strlen(arg.Value->Name) + 1;
    memcpy(dest + offset, &len, sizeof(len));
    offset += sizeof(unsigned);
    memcpy(dest + offset, arg.Value->Name, len);
    offset += len;
  }
}

unsigned GetSize(const DSTORAGE_REQUEST_Argument& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }

  unsigned size = sizeof(void*) + sizeof(DSTORAGE_REQUEST) + sizeof(arg.FileKey) +
                  sizeof(arg.ResourceKey) + sizeof(arg.NewOffset);
  if (arg.Value->Name) {
    size += sizeof(unsigned) + strlen(arg.Value->Name) + 1;
  }
  return size;
}

void Encode(char* dest, unsigned& offset, const DSTORAGE_REQUEST_Argument& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.Value, sizeof(DSTORAGE_REQUEST));
  offset += sizeof(DSTORAGE_REQUEST);

  memcpy(dest + offset, &arg.FileKey, sizeof(arg.FileKey));
  offset += sizeof(arg.FileKey);

  memcpy(dest + offset, &arg.ResourceKey, sizeof(arg.ResourceKey));
  offset += sizeof(arg.ResourceKey);

  memcpy(dest + offset, &arg.NewOffset, sizeof(arg.NewOffset));
  offset += sizeof(arg.NewOffset);

  if (arg.Value->Name) {
    unsigned len = strlen(arg.Value->Name) + 1;
    memcpy(dest + offset, &len, sizeof(len));
    offset += sizeof(unsigned);
    memcpy(dest + offset, arg.Value->Name, len);
    offset += len;
  }
}

unsigned GetSize(
    const PointerArgument<NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS>& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  unsigned size = sizeof(void*) + sizeof(NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS);

  size += sizeof(NVAPI_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC_EX);

  if (arg.Value->pDesc->inputs.type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
    if (arg.Value->pDesc->inputs.descsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY) {
      if (arg.Value->pDesc->inputs.pGeometryDescs) {
        size +=
            arg.Value->pDesc->inputs.numDescs * arg.Value->pDesc->inputs.geometryDescStrideInBytes;

        for (unsigned i = 0; i < arg.Value->pDesc->inputs.numDescs; ++i) {
          const NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX& desc =
              *(NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX*)((char*)(arg.Value->pDesc->inputs
                                                                      .pGeometryDescs) +
                                                          arg.Value->pDesc->inputs
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
    } else if (arg.Value->pDesc->inputs.descsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS) {
      if (arg.Value->pDesc->inputs.ppGeometryDescs) {
        size +=
            arg.Value->pDesc->inputs.numDescs * sizeof(NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX*);
        size += arg.Value->pDesc->inputs.numDescs * sizeof(NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX);

        for (unsigned i = 0; i < arg.Value->pDesc->inputs.numDescs; ++i) {
          if (arg.Value->pDesc->inputs.ppGeometryDescs[i]->type ==
              NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES_EX) {
            size += sizeof(NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_USAGE_COUNT) *
                    arg.Value->pDesc->inputs.ppGeometryDescs[i]
                        ->ommTriangles.ommAttachment.numOMMUsageCounts;
          } else if (arg.Value->pDesc->inputs.ppGeometryDescs[i]->type ==
                     NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_DMM_TRIANGLES_EX) {
            size += sizeof(NVAPI_D3D12_RAYTRACING_DISPLACEMENT_MICROMAP_USAGE_COUNT) *
                    arg.Value->pDesc->inputs.ppGeometryDescs[i]
                        ->dmmTriangles.dmmAttachment.numDMMUsageCounts;
          }
        }
      }
    }
  }

  size += sizeof(unsigned) * 6;
  size += sizeof(unsigned) + sizeof(unsigned) * arg.InputKeys.size() * 2;

  if (arg.Value->pPostbuildInfoDescs) {
    size += (sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC) +
             sizeof(unsigned) * 2) *
            arg.Value->numPostbuildInfoDescs;

    size += sizeof(unsigned) * arg.DestPostBuildBufferKeys.size() * 2;
  }

  return size;
}

void Encode(char* dest,
            unsigned& offset,
            const PointerArgument<NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS>& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.Value, sizeof(NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS));
  offset += sizeof(NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS);

  if (arg.Value->pDesc) {
    memcpy(dest + offset, arg.Value->pDesc,
           sizeof(NVAPI_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC_EX));
    offset += sizeof(NVAPI_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC_EX);

    if (arg.Value->pDesc->inputs.type ==
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
      if (arg.Value->pDesc->inputs.descsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY) {
        if (arg.Value->pDesc->inputs.pGeometryDescs) {
          memcpy(dest + offset, arg.Value->pDesc->inputs.pGeometryDescs,
                 arg.Value->pDesc->inputs.numDescs *
                     arg.Value->pDesc->inputs.geometryDescStrideInBytes);
          offset += arg.Value->pDesc->inputs.numDescs *
                    arg.Value->pDesc->inputs.geometryDescStrideInBytes;

          for (unsigned i = 0; i < arg.Value->pDesc->inputs.numDescs; ++i) {
            const NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX& desc =
                *(NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX*)((char*)(arg.Value->pDesc->inputs
                                                                        .pGeometryDescs) +
                                                            arg.Value->pDesc->inputs
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
      } else if (arg.Value->pDesc->inputs.descsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS) {
        if (arg.Value->pDesc->inputs.ppGeometryDescs) {
          memcpy(dest + offset, arg.Value->pDesc->inputs.ppGeometryDescs,
                 sizeof(NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX*) *
                     arg.Value->pDesc->inputs.numDescs);
          offset +=
              sizeof(NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX*) * arg.Value->pDesc->inputs.numDescs;
        }
        for (unsigned i = 0; i < arg.Value->pDesc->inputs.numDescs; ++i) {
          memcpy(dest + offset, arg.Value->pDesc->inputs.ppGeometryDescs[i],
                 sizeof(NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX));
          offset += sizeof(NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX);

          if (arg.Value->pDesc->inputs.ppGeometryDescs[i]->type ==
              NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES_EX) {
            memcpy(dest + offset,
                   arg.Value->pDesc->inputs.ppGeometryDescs[i]
                       ->ommTriangles.ommAttachment.pOMMUsageCounts,
                   sizeof(NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_USAGE_COUNT) *
                       arg.Value->pDesc->inputs.ppGeometryDescs[i]
                           ->ommTriangles.ommAttachment.numOMMUsageCounts);
            offset += sizeof(NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_USAGE_COUNT) *
                      arg.Value->pDesc->inputs.ppGeometryDescs[i]
                          ->ommTriangles.ommAttachment.numOMMUsageCounts;
          } else if (arg.Value->pDesc->inputs.ppGeometryDescs[i]->type ==
                     NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_DMM_TRIANGLES_EX) {
            memcpy(dest + offset,
                   arg.Value->pDesc->inputs.ppGeometryDescs[i]
                       ->dmmTriangles.dmmAttachment.pDMMUsageCounts,
                   sizeof(NVAPI_D3D12_RAYTRACING_DISPLACEMENT_MICROMAP_USAGE_COUNT) *
                       arg.Value->pDesc->inputs.ppGeometryDescs[i]
                           ->dmmTriangles.dmmAttachment.numDMMUsageCounts);
            offset += sizeof(NVAPI_D3D12_RAYTRACING_DISPLACEMENT_MICROMAP_USAGE_COUNT) *
                      arg.Value->pDesc->inputs.ppGeometryDescs[i]
                          ->dmmTriangles.dmmAttachment.numDMMUsageCounts;
          }
        }
      }
    }
  }

  memcpy(dest + offset, &arg.DestAccelerationStructureKey,
         sizeof(arg.DestAccelerationStructureKey));
  offset += sizeof(arg.DestAccelerationStructureKey);
  memcpy(dest + offset, &arg.DestAccelerationStructureOffset,
         sizeof(arg.DestAccelerationStructureOffset));
  offset += sizeof(arg.DestAccelerationStructureOffset);

  memcpy(dest + offset, &arg.SourceAccelerationStructureKey,
         sizeof(arg.SourceAccelerationStructureKey));
  offset += sizeof(arg.SourceAccelerationStructureKey);
  memcpy(dest + offset, &arg.SourceAccelerationStructureOffset,
         sizeof(arg.SourceAccelerationStructureOffset));
  offset += sizeof(arg.SourceAccelerationStructureOffset);

  memcpy(dest + offset, &arg.ScratchAccelerationStructureKey,
         sizeof(arg.ScratchAccelerationStructureKey));
  offset += sizeof(arg.ScratchAccelerationStructureKey);
  memcpy(dest + offset, &arg.ScratchAccelerationStructureOffset,
         sizeof(arg.ScratchAccelerationStructureOffset));
  offset += sizeof(arg.ScratchAccelerationStructureOffset);

  unsigned size = arg.InputKeys.size();
  memcpy(dest + offset, &size, sizeof(size));
  offset += sizeof(size);
  memcpy(dest + offset, arg.InputKeys.data(), sizeof(unsigned) * size);
  offset += sizeof(unsigned) * size;
  memcpy(dest + offset, arg.InputOffsets.data(), sizeof(unsigned) * size);
  offset += sizeof(unsigned) * size;

  if (arg.Value->pPostbuildInfoDescs) {
    memcpy(dest + offset, arg.Value->pPostbuildInfoDescs,
           sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC) *
               arg.Value->numPostbuildInfoDescs);
    offset += sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC) *
              arg.Value->numPostbuildInfoDescs;

    memcpy(dest + offset, arg.DestPostBuildBufferKeys.data(),
           sizeof(unsigned) * arg.Value->numPostbuildInfoDescs);
    offset += sizeof(unsigned) * arg.Value->numPostbuildInfoDescs;

    memcpy(dest + offset, arg.DestPostBuildBufferOffsets.data(),
           sizeof(unsigned) * arg.Value->numPostbuildInfoDescs);
    offset += sizeof(unsigned) * arg.Value->numPostbuildInfoDescs;
  }
}

unsigned GetSize(const PointerArgument<NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS>& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  unsigned size = sizeof(void*) + sizeof(NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS);

  if (arg.Value->pDesc) {
    size += sizeof(NVAPI_D3D12_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC);

    if (arg.Value->pDesc->inputs.pOMMUsageCounts) {
      size += sizeof(NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_USAGE_COUNT) *
              arg.Value->pDesc->inputs.numOMMUsageCounts;
    }
  }

  size += sizeof(unsigned) * 8;

  if (arg.Value->pPostbuildInfoDescs) {
    size += (sizeof(NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_POSTBUILD_INFO_DESC) +
             sizeof(unsigned) * 2) *
            arg.Value->numPostbuildInfoDescs;

    size += sizeof(unsigned) * arg.DestPostBuildBufferKeys.size() * 2;
  }

  return size;
}

void Encode(char* dest,
            unsigned& offset,
            const PointerArgument<NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS>& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.Value, sizeof(NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS));
  offset += sizeof(NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS);

  if (arg.Value->pDesc) {
    memcpy(dest + offset, arg.Value->pDesc,
           sizeof(NVAPI_D3D12_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC));
    offset += sizeof(NVAPI_D3D12_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC);

    if (arg.Value->pDesc->inputs.pOMMUsageCounts) {
      memcpy(dest + offset, arg.Value->pDesc->inputs.pOMMUsageCounts,
             sizeof(NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_USAGE_COUNT) *
                 arg.Value->pDesc->inputs.numOMMUsageCounts);
      offset += sizeof(NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_USAGE_COUNT) *
                arg.Value->pDesc->inputs.numOMMUsageCounts;
    }
  }

  memcpy(dest + offset, &arg.DestOpacityMicromapArrayDataKey,
         sizeof(arg.DestOpacityMicromapArrayDataKey));
  offset += sizeof(arg.DestOpacityMicromapArrayDataKey);
  memcpy(dest + offset, &arg.DestOpacityMicromapArrayDataOffset,
         sizeof(arg.DestOpacityMicromapArrayDataOffset));
  offset += sizeof(arg.DestOpacityMicromapArrayDataOffset);

  memcpy(dest + offset, &arg.InputBufferKey, sizeof(arg.InputBufferKey));
  offset += sizeof(arg.InputBufferKey);
  memcpy(dest + offset, &arg.InputBufferOffset, sizeof(arg.InputBufferOffset));
  offset += sizeof(arg.InputBufferOffset);

  memcpy(dest + offset, &arg.PerOMMDescsKey, sizeof(arg.PerOMMDescsKey));
  offset += sizeof(arg.PerOMMDescsKey);
  memcpy(dest + offset, &arg.PerOMMDescsOffset, sizeof(arg.PerOMMDescsOffset));
  offset += sizeof(arg.PerOMMDescsOffset);

  memcpy(dest + offset, &arg.ScratchOpacityMicromapArrayDataKey,
         sizeof(arg.ScratchOpacityMicromapArrayDataKey));
  offset += sizeof(arg.ScratchOpacityMicromapArrayDataKey);
  memcpy(dest + offset, &arg.ScratchOpacityMicromapArrayDataOffset,
         sizeof(arg.ScratchOpacityMicromapArrayDataOffset));
  offset += sizeof(arg.ScratchOpacityMicromapArrayDataOffset);

  if (arg.Value->pPostbuildInfoDescs) {
    memcpy(dest + offset, arg.Value->pPostbuildInfoDescs,
           sizeof(NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_POSTBUILD_INFO_DESC) *
               arg.Value->numPostbuildInfoDescs);
    offset += sizeof(NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_POSTBUILD_INFO_DESC) *
              arg.Value->numPostbuildInfoDescs;

    memcpy(dest + offset, arg.DestPostBuildBufferKeys.data(),
           sizeof(unsigned) * arg.Value->numPostbuildInfoDescs);
    offset += sizeof(unsigned) * arg.Value->numPostbuildInfoDescs;

    memcpy(dest + offset, arg.DestPostBuildBufferOffsets.data(),
           sizeof(unsigned) * arg.Value->numPostbuildInfoDescs);
    offset += sizeof(unsigned) * arg.Value->numPostbuildInfoDescs;
  }
}

unsigned GetSize(
    const PointerArgument<NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS>& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  unsigned size =
      sizeof(void*) + sizeof(NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS);

  if (arg.Value->pDesc) {
    size += sizeof(NVAPI_D3D12_RAYTRACING_MULTI_INDIRECT_CLUSTER_OPERATION_DESC);
  }

  size += sizeof(unsigned) * 12;

  return size;
}

void Encode(
    char* dest,
    unsigned& offset,
    const PointerArgument<NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS>& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.Value,
         sizeof(NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS));
  offset += sizeof(NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS);

  if (arg.Value->pDesc) {
    memcpy(dest + offset, arg.Value->pDesc,
           sizeof(NVAPI_D3D12_RAYTRACING_MULTI_INDIRECT_CLUSTER_OPERATION_DESC));
    offset += sizeof(NVAPI_D3D12_RAYTRACING_MULTI_INDIRECT_CLUSTER_OPERATION_DESC);
  }

  memcpy(dest + offset, &arg.BatchResultDataKey, sizeof(arg.BatchResultDataKey));
  offset += sizeof(arg.BatchResultDataKey);
  memcpy(dest + offset, &arg.BatchResultDataOffset, sizeof(arg.BatchResultDataOffset));
  offset += sizeof(arg.BatchResultDataOffset);

  memcpy(dest + offset, &arg.BatchScratchDataKey, sizeof(arg.BatchScratchDataKey));
  offset += sizeof(arg.BatchScratchDataKey);
  memcpy(dest + offset, &arg.BatchScratchDataOffset, sizeof(arg.BatchScratchDataOffset));
  offset += sizeof(arg.BatchScratchDataOffset);

  memcpy(dest + offset, &arg.DestinationAddressArrayKey, sizeof(arg.DestinationAddressArrayKey));
  offset += sizeof(arg.DestinationAddressArrayKey);
  memcpy(dest + offset, &arg.DestinationAddressArrayOffset,
         sizeof(arg.DestinationAddressArrayOffset));
  offset += sizeof(arg.DestinationAddressArrayOffset);

  memcpy(dest + offset, &arg.ResultSizeArrayKey, sizeof(arg.ResultSizeArrayKey));
  offset += sizeof(arg.ResultSizeArrayKey);
  memcpy(dest + offset, &arg.ResultSizeArrayOffset, sizeof(arg.ResultSizeArrayOffset));
  offset += sizeof(arg.ResultSizeArrayOffset);

  memcpy(dest + offset, &arg.IndirectArgArrayKey, sizeof(arg.IndirectArgArrayKey));
  offset += sizeof(arg.IndirectArgArrayKey);
  memcpy(dest + offset, &arg.IndirectArgArrayOffset, sizeof(arg.IndirectArgArrayOffset));
  offset += sizeof(arg.IndirectArgArrayOffset);

  memcpy(dest + offset, &arg.IndirectArgCountKey, sizeof(arg.IndirectArgCountKey));
  offset += sizeof(arg.IndirectArgCountKey);
  memcpy(dest + offset, &arg.IndirectArgCountOffset, sizeof(arg.IndirectArgCountOffset));
  offset += sizeof(arg.IndirectArgCountOffset);
}

unsigned GetSize(const xell_frame_report_t_Argument& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(xell_frame_report_t) * arg.FRAME_REPORTS_COUNT;
}

void Encode(char* dest, unsigned& offset, const xell_frame_report_t_Argument& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }
  memcpy(dest + offset, arg.Value, sizeof(xell_frame_report_t) * arg.FRAME_REPORTS_COUNT);
  offset += sizeof(xell_frame_report_t) * arg.FRAME_REPORTS_COUNT;
}

unsigned GetSize(const xefg_swapchain_d3d12_init_params_t_Argument& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(xefg_swapchain_d3d12_init_params_t) + sizeof(arg.Key) +
         sizeof(arg.ApplicationSwapChainKey) + sizeof(arg.TempBufferHeapKey) +
         sizeof(arg.TempTextureHeapKey) + sizeof(arg.PipelineLibraryKey);
}

void Encode(char* dest, unsigned& offset, const xefg_swapchain_d3d12_init_params_t_Argument& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }
  memcpy(dest + offset, arg.Value, sizeof(xefg_swapchain_d3d12_init_params_t));
  offset += sizeof(xefg_swapchain_d3d12_init_params_t);

  memcpy(dest + offset, &arg.Key, sizeof(arg.Key));
  offset += sizeof(arg.Key);

  memcpy(dest + offset, &arg.ApplicationSwapChainKey, sizeof(arg.ApplicationSwapChainKey));
  offset += sizeof(arg.ApplicationSwapChainKey);

  memcpy(dest + offset, &arg.TempBufferHeapKey, sizeof(arg.TempBufferHeapKey));
  offset += sizeof(arg.TempBufferHeapKey);

  memcpy(dest + offset, &arg.TempTextureHeapKey, sizeof(arg.TempTextureHeapKey));
  offset += sizeof(arg.TempTextureHeapKey);

  memcpy(dest + offset, &arg.PipelineLibraryKey, sizeof(arg.PipelineLibraryKey));
  offset += sizeof(arg.PipelineLibraryKey);
}

unsigned GetSize(const xefg_swapchain_d3d12_resource_data_t_Argument& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(xefg_swapchain_d3d12_resource_data_t) + sizeof(arg.ResourceKey);
}

void Encode(char* dest,
            unsigned& offset,
            const xefg_swapchain_d3d12_resource_data_t_Argument& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }
  memcpy(dest + offset, arg.Value, sizeof(xefg_swapchain_d3d12_resource_data_t));
  offset += sizeof(xefg_swapchain_d3d12_resource_data_t);
  memcpy(dest + offset, &arg.ResourceKey, sizeof(arg.ResourceKey));
  offset += sizeof(arg.ResourceKey);
}

} // namespace DirectX
} // namespace gits
