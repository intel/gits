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
  memcpy(dest + offset, arg.CaptureValue ? &arg.CaptureValue : arg.Value, sizeof(void*));
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

  memcpy(dest + offset, &arg.RootSignatureKey, sizeof(arg.RootSignatureKey));
  offset += sizeof(arg.RootSignatureKey);
}

unsigned GetSize(const PointerArgument<D3D12_HEAP_PROPERTIES>& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(D3D12_HEAP_PROPERTIES);
}

void Encode(char* dest, unsigned& offset, const PointerArgument<D3D12_HEAP_PROPERTIES>& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.Value, sizeof(D3D12_HEAP_PROPERTIES));
  offset += sizeof(D3D12_HEAP_PROPERTIES);
}

unsigned GetSize(const PointerArgument<D3D12_HEAP_DESC>& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  return sizeof(void*) + sizeof(D3D12_HEAP_DESC);
}

void Encode(char* dest, unsigned& offset, const PointerArgument<D3D12_HEAP_DESC>& arg) {
  if (EncodeNullPtr(dest, offset, arg)) {
    return;
  }

  memcpy(dest + offset, arg.Value, sizeof(D3D12_HEAP_DESC));
  offset += sizeof(D3D12_HEAP_DESC);
}

unsigned GetSize(const Argument<D3D12_HEAP_FLAGS>& arg) {
  return sizeof(D3D12_HEAP_FLAGS);
}

void Encode(char* dest, unsigned& offset, const Argument<D3D12_HEAP_FLAGS>& arg) {
  memcpy(dest + offset, &arg.Value, sizeof(D3D12_HEAP_FLAGS));
  offset += sizeof(D3D12_HEAP_FLAGS);
}

unsigned GetSize(const PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC>& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  unsigned size = sizeof(void*) + sizeof(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC);

  if (arg.Value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
    if (arg.Value->Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY) {
      size += arg.Value->Inputs.NumDescs * sizeof(D3D12_RAYTRACING_GEOMETRY_DESC);
    } else if (arg.Value->Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS) {
      size += arg.Value->Inputs.NumDescs * sizeof(D3D12_RAYTRACING_GEOMETRY_DESC*);
      size += arg.Value->Inputs.NumDescs * sizeof(D3D12_RAYTRACING_GEOMETRY_DESC);
    }
  }

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
      }
    }
  }
}

unsigned GetSize(const PointerArgument<INTCExtensionAppInfo>& arg) {
  if (!arg.Value) {
    return sizeof(void*);
  }
  unsigned size = sizeof(void*) + sizeof(INTCExtensionAppInfo);

  if (arg.Value->pApplicationName) {
    size += sizeof(unsigned) + wcslen(arg.Value->pApplicationName) * 2 + 2;
  }
  if (arg.Value->pEngineName) {
    size += sizeof(unsigned) + wcslen(arg.Value->pEngineName) * 2 + 2;
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
    unsigned len = wcslen(arg.Value->pApplicationName) * 2 + 2;
    memcpy(dest + offset, &len, sizeof(len));
    offset += sizeof(unsigned);
    memcpy(dest + offset, arg.Value->pApplicationName, len);
    offset += len;
  }
  if (arg.Value->pEngineName) {
    unsigned len = wcslen(arg.Value->pEngineName) * 2 + 2;
    memcpy(dest + offset, &len, sizeof(len));
    offset += sizeof(unsigned);
    memcpy(dest + offset, arg.Value->pEngineName, len);
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

unsigned GetSize(const INTCExtensionContextOutputArgument& arg) {
  return sizeof(arg.Key);
}

void Encode(char* dest, unsigned& offset, const INTCExtensionContextOutputArgument& arg) {
  memcpy(dest + offset, &arg.Key, sizeof(arg.Key));
  offset += sizeof(arg.Key);
}

unsigned GetSize(const INTCExtensionContextArgument& arg) {
  return sizeof(arg.Key);
}

void Encode(char* dest, unsigned& offset, const INTCExtensionContextArgument& arg) {
  memcpy(dest + offset, &arg.Key, sizeof(arg.Key));
  offset += sizeof(arg.Key);
}

} // namespace DirectX
} // namespace gits
