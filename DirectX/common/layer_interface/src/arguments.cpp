// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "arguments.h"
#include "intelExtensions.h"
#include <d3dx12/d3dx12_pipeline_state_stream.h>
#include "log.h"

namespace gits {
namespace DirectX {

#pragma region Generic

BufferArgument::BufferArgument(const BufferArgument& arg) {
  Size = arg.Size;
  if (arg.Value) {
    Value = new uint8_t[Size];
    memcpy(Value, arg.Value, Size);
  }
  Copy = true;
}

BufferArgument::~BufferArgument() {
  if (Copy) {
    delete static_cast<uint8_t*>(Value);
  }
}

OutputBufferArgument::OutputBufferArgument(const OutputBufferArgument& arg) {
  Data = *arg.Value;
  Value = &Data;
  CaptureValue = arg.CaptureValue;
}

LPCWSTR_Argument::LPCWSTR_Argument(const LPCWSTR_Argument& arg) {
  if (arg.Value) {
    unsigned len = wcslen(arg.Value);
    Value = new wchar_t[len + 1];
    memcpy(Value, arg.Value, len * 2 + 2);
  }
  Copy = true;
}

LPCWSTR_Argument::~LPCWSTR_Argument() {
  if (Copy) {
    delete[] Value;
  }
}

LPCSTR_Argument::LPCSTR_Argument(const LPCSTR_Argument& arg) {
  if (arg.Value) {
    unsigned len = strlen(arg.Value);
    Value = new char[len + 1];
    memcpy(Value, arg.Value, len + 1);
  }
  Copy = true;
}

LPCSTR_Argument::~LPCSTR_Argument() {
  if (Copy) {
    delete[] Value;
  }
}

#pragma endregion

#pragma region D3D12

D3D12_GPU_VIRTUAL_ADDRESSs_Argument::D3D12_GPU_VIRTUAL_ADDRESSs_Argument(
    const D3D12_GPU_VIRTUAL_ADDRESSs_Argument& arg) {
  Size = arg.Size;
  if (arg.Value) {
    Value = new D3D12_GPU_VIRTUAL_ADDRESS[Size];
    memcpy(Value, arg.Value, Size * sizeof(D3D12_GPU_VIRTUAL_ADDRESS));
  }
  InterfaceKeys = arg.InterfaceKeys;
  Offsets = arg.Offsets;
  Copy = true;
}

D3D12_GPU_VIRTUAL_ADDRESSs_Argument::~D3D12_GPU_VIRTUAL_ADDRESSs_Argument() {
  if (Copy) {
    delete[] Value;
  }
}

ShaderIdentifierArgument::ShaderIdentifierArgument(const ShaderIdentifierArgument& arg) {
  if (arg.Value) {
    Data.resize(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    memcpy(Data.data(), arg.Value, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    Value = Data.data();
  }
}

PointerArgument<D3D12_ROOT_SIGNATURE_DESC>::PointerArgument(
    const PointerArgument<D3D12_ROOT_SIGNATURE_DESC>& arg) {
  if (!arg.Value) {
    return;
  }
  Value = new D3D12_ROOT_SIGNATURE_DESC;
  *Value = *arg.Value;
  if (Value->NumParameters) {
    Value->pParameters = new D3D12_ROOT_PARAMETER[Value->NumParameters];
    for (unsigned i = 0; i < Value->NumParameters; ++i) {
      const_cast<D3D12_ROOT_PARAMETER*>(Value->pParameters)[i] = arg.Value->pParameters[i];
      if (Value->pParameters[i].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
        const_cast<D3D12_ROOT_PARAMETER*>(Value->pParameters)[i].DescriptorTable.pDescriptorRanges =
            new D3D12_DESCRIPTOR_RANGE[Value->pParameters[i].DescriptorTable.NumDescriptorRanges];
        for (unsigned j = 0; j < Value->pParameters[i].DescriptorTable.NumDescriptorRanges; ++j) {
          const_cast<D3D12_DESCRIPTOR_RANGE*>(
              Value->pParameters[i].DescriptorTable.pDescriptorRanges)[j] =
              arg.Value->pParameters[i].DescriptorTable.pDescriptorRanges[j];
        }
      }
    }
  }
  if (Value->NumStaticSamplers) {
    Value->pStaticSamplers = new D3D12_STATIC_SAMPLER_DESC[Value->NumStaticSamplers];
    for (unsigned i = 0; i < Value->NumStaticSamplers; ++i) {
      const_cast<D3D12_STATIC_SAMPLER_DESC*>(Value->pStaticSamplers)[i] =
          arg.Value->pStaticSamplers[i];
    }
  }
  Copy = true;
}

PointerArgument<D3D12_ROOT_SIGNATURE_DESC>::~PointerArgument() {
  if (Copy) {
    if (Value->NumParameters) {
      for (unsigned i = 0; i < Value->NumParameters; ++i) {
        if (Value->pParameters[i].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
          delete[] Value->pParameters[i].DescriptorTable.pDescriptorRanges;
        }
      }
      delete[] Value->pParameters;
    }
    if (Value->NumStaticSamplers) {
      delete[] Value->pStaticSamplers;
    }
    delete Value;
  }
}

PointerArgument<D3D12_VERSIONED_ROOT_SIGNATURE_DESC>::PointerArgument(
    const PointerArgument<D3D12_VERSIONED_ROOT_SIGNATURE_DESC>& arg) {
  if (!arg.Value) {
    return;
  }
  Value = new D3D12_VERSIONED_ROOT_SIGNATURE_DESC();
  *Value = *arg.Value;
  switch (Value->Version) {
  case D3D_ROOT_SIGNATURE_VERSION_1_0: {
    D3D12_ROOT_SIGNATURE_DESC& dest = Value->Desc_1_0;
    D3D12_ROOT_SIGNATURE_DESC& src = arg.Value->Desc_1_0;
    if (dest.NumParameters) {
      dest.pParameters = new D3D12_ROOT_PARAMETER[dest.NumParameters];
      for (unsigned i = 0; i < dest.NumParameters; ++i) {
        const_cast<D3D12_ROOT_PARAMETER*>(dest.pParameters)[i] = src.pParameters[i];
        if (dest.pParameters[i].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
          const_cast<D3D12_ROOT_PARAMETER*>(dest.pParameters)[i].DescriptorTable.pDescriptorRanges =
              new D3D12_DESCRIPTOR_RANGE[dest.pParameters[i].DescriptorTable.NumDescriptorRanges];
          for (unsigned j = 0; j < dest.pParameters[i].DescriptorTable.NumDescriptorRanges; ++j) {
            const_cast<D3D12_DESCRIPTOR_RANGE*>(
                dest.pParameters[i].DescriptorTable.pDescriptorRanges)[j] =
                src.pParameters[i].DescriptorTable.pDescriptorRanges[j];
          }
        }
      }
    }
    if (dest.NumStaticSamplers) {
      dest.pStaticSamplers = new D3D12_STATIC_SAMPLER_DESC[dest.NumStaticSamplers];
      for (unsigned i = 0; i < dest.NumStaticSamplers; ++i) {
        const_cast<D3D12_STATIC_SAMPLER_DESC*>(dest.pStaticSamplers)[i] = src.pStaticSamplers[i];
      }
    }
  } break;
  case D3D_ROOT_SIGNATURE_VERSION_1_1: {
    D3D12_ROOT_SIGNATURE_DESC1& dest = Value->Desc_1_1;
    D3D12_ROOT_SIGNATURE_DESC1& src = arg.Value->Desc_1_1;
    if (dest.NumParameters) {
      dest.pParameters = new D3D12_ROOT_PARAMETER1[dest.NumParameters];
      for (unsigned i = 0; i < dest.NumParameters; ++i) {
        const_cast<D3D12_ROOT_PARAMETER1*>(dest.pParameters)[i] = src.pParameters[i];
        if (dest.pParameters[i].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
          const_cast<D3D12_ROOT_PARAMETER1*>(dest.pParameters)[i]
              .DescriptorTable.pDescriptorRanges =
              new D3D12_DESCRIPTOR_RANGE1[dest.pParameters[i].DescriptorTable.NumDescriptorRanges];
          for (unsigned j = 0; j < dest.pParameters[i].DescriptorTable.NumDescriptorRanges; ++j) {
            const_cast<D3D12_DESCRIPTOR_RANGE1*>(
                dest.pParameters[i].DescriptorTable.pDescriptorRanges)[j] =
                src.pParameters[i].DescriptorTable.pDescriptorRanges[j];
          }
        }
      }
    }
    if (dest.NumStaticSamplers) {
      dest.pStaticSamplers = new D3D12_STATIC_SAMPLER_DESC[dest.NumStaticSamplers];
      for (unsigned i = 0; i < dest.NumStaticSamplers; ++i) {
        const_cast<D3D12_STATIC_SAMPLER_DESC*>(dest.pStaticSamplers)[i] = src.pStaticSamplers[i];
      }
    }
  } break;
  case D3D_ROOT_SIGNATURE_VERSION_1_2: {
    D3D12_ROOT_SIGNATURE_DESC2& dest = Value->Desc_1_2;
    D3D12_ROOT_SIGNATURE_DESC2& src = arg.Value->Desc_1_2;
    if (dest.NumParameters) {
      dest.pParameters = new D3D12_ROOT_PARAMETER1[dest.NumParameters];
      for (unsigned i = 0; i < dest.NumParameters; ++i) {
        const_cast<D3D12_ROOT_PARAMETER1*>(dest.pParameters)[i] = src.pParameters[i];
        if (dest.pParameters[i].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
          const_cast<D3D12_ROOT_PARAMETER1*>(dest.pParameters)[i]
              .DescriptorTable.pDescriptorRanges =
              new D3D12_DESCRIPTOR_RANGE1[dest.pParameters[i].DescriptorTable.NumDescriptorRanges];
          for (unsigned j = 0; j < dest.pParameters[i].DescriptorTable.NumDescriptorRanges; ++j) {
            const_cast<D3D12_DESCRIPTOR_RANGE1*>(
                dest.pParameters[i].DescriptorTable.pDescriptorRanges)[j] =
                src.pParameters[i].DescriptorTable.pDescriptorRanges[j];
          }
        }
      }
    }
    if (dest.NumStaticSamplers) {
      dest.pStaticSamplers = new D3D12_STATIC_SAMPLER_DESC1[dest.NumStaticSamplers];
      for (unsigned i = 0; i < dest.NumStaticSamplers; ++i) {
        const_cast<D3D12_STATIC_SAMPLER_DESC1*>(dest.pStaticSamplers)[i] = src.pStaticSamplers[i];
      }
    }
  } break;
  }
  Copy = true;
}

PointerArgument<D3D12_VERSIONED_ROOT_SIGNATURE_DESC>::~PointerArgument() {
  if (Copy) {
    switch (Value->Version) {
    case D3D_ROOT_SIGNATURE_VERSION_1_0: {
      D3D12_ROOT_SIGNATURE_DESC& desc = Value->Desc_1_0;
      if (desc.NumParameters) {
        for (unsigned i = 0; i < desc.NumParameters; ++i) {
          if (desc.pParameters[i].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
            delete[] desc.pParameters[i].DescriptorTable.pDescriptorRanges;
          }
        }
        delete[] desc.pParameters;
      }
      if (desc.NumStaticSamplers) {
        delete[] desc.pStaticSamplers;
      }
    } break;
    case D3D_ROOT_SIGNATURE_VERSION_1_1: {
      D3D12_ROOT_SIGNATURE_DESC1& desc = Value->Desc_1_1;
      if (desc.NumParameters) {
        for (unsigned i = 0; i < desc.NumParameters; ++i) {
          if (desc.pParameters[i].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
            delete[] desc.pParameters[i].DescriptorTable.pDescriptorRanges;
          }
        }
        delete[] desc.pParameters;
      }
      if (desc.NumStaticSamplers) {
        delete[] desc.pStaticSamplers;
      }
    } break;
    case D3D_ROOT_SIGNATURE_VERSION_1_2: {
      D3D12_ROOT_SIGNATURE_DESC2& desc = Value->Desc_1_2;
      if (desc.NumParameters) {
        for (unsigned i = 0; i < desc.NumParameters; ++i) {
          if (desc.pParameters[i].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
            delete[] desc.pParameters[i].DescriptorTable.pDescriptorRanges;
          }
        }
        delete[] desc.pParameters;
      }
      if (desc.NumStaticSamplers) {
        delete[] desc.pStaticSamplers;
      }
    } break;
    }
    delete Value;
  }
}

D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument::D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument(
    const D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument& arg) {
  RootSignatureKey = arg.RootSignatureKey;
  if (!arg.Value) {
    return;
  }
  Value = new D3D12_GRAPHICS_PIPELINE_STATE_DESC();
  *Value = *arg.Value;
  if (Value->VS.pShaderBytecode) {
    Value->VS.pShaderBytecode = new uint8_t[Value->VS.BytecodeLength];
    memcpy(const_cast<void*>(Value->VS.pShaderBytecode), arg.Value->VS.pShaderBytecode,
           Value->VS.BytecodeLength);
  }
  if (Value->PS.pShaderBytecode) {
    Value->PS.pShaderBytecode = new uint8_t[Value->PS.BytecodeLength];
    memcpy(const_cast<void*>(Value->PS.pShaderBytecode), arg.Value->PS.pShaderBytecode,
           Value->PS.BytecodeLength);
  }
  if (Value->DS.pShaderBytecode) {
    Value->DS.pShaderBytecode = new uint8_t[Value->DS.BytecodeLength];
    memcpy(const_cast<void*>(Value->DS.pShaderBytecode), arg.Value->DS.pShaderBytecode,
           Value->DS.BytecodeLength);
  }
  if (Value->HS.pShaderBytecode) {
    Value->HS.pShaderBytecode = new uint8_t[Value->HS.BytecodeLength];
    memcpy(const_cast<void*>(Value->HS.pShaderBytecode), arg.Value->HS.pShaderBytecode,
           Value->HS.BytecodeLength);
  }
  if (Value->GS.pShaderBytecode) {
    Value->GS.pShaderBytecode = new uint8_t[Value->GS.BytecodeLength];
    memcpy(const_cast<void*>(Value->GS.pShaderBytecode), arg.Value->GS.pShaderBytecode,
           Value->GS.BytecodeLength);
  }
  if (Value->StreamOutput.pSODeclaration) {
    Value->StreamOutput.pSODeclaration =
        new D3D12_SO_DECLARATION_ENTRY[Value->StreamOutput.NumEntries];
    for (unsigned i = 0; i < arg.Value->StreamOutput.NumEntries; ++i) {
      const_cast<D3D12_SO_DECLARATION_ENTRY*>(Value->StreamOutput.pSODeclaration)[i] =
          arg.Value->StreamOutput.pSODeclaration[i];
      unsigned len = strlen(Value->StreamOutput.pSODeclaration[i].SemanticName);
      const_cast<D3D12_SO_DECLARATION_ENTRY*>(Value->StreamOutput.pSODeclaration)[i].SemanticName =
          new char[len + 1];
      memcpy(const_cast<char*>(Value->StreamOutput.pSODeclaration[i].SemanticName),
             arg.Value->StreamOutput.pSODeclaration[i].SemanticName, len + 1);
    }
  }
  if (Value->StreamOutput.pBufferStrides) {
    Value->StreamOutput.pBufferStrides = new UINT[Value->StreamOutput.NumEntries];
    memcpy(const_cast<UINT*>(Value->StreamOutput.pBufferStrides),
           arg.Value->StreamOutput.pBufferStrides, Value->StreamOutput.NumEntries * sizeof(UINT));
  }
  Value->InputLayout.pInputElementDescs =
      new D3D12_INPUT_ELEMENT_DESC[arg.Value->InputLayout.NumElements];
  for (unsigned i = 0; i < Value->InputLayout.NumElements; ++i) {
    const_cast<D3D12_INPUT_ELEMENT_DESC*>(Value->InputLayout.pInputElementDescs)[i] =
        arg.Value->InputLayout.pInputElementDescs[i];
    unsigned len = strlen(Value->InputLayout.pInputElementDescs[i].SemanticName);
    const_cast<D3D12_INPUT_ELEMENT_DESC*>(Value->InputLayout.pInputElementDescs)[i].SemanticName =
        new char[len + 1];
    memcpy(const_cast<char*>(Value->InputLayout.pInputElementDescs[i].SemanticName),
           arg.Value->InputLayout.pInputElementDescs[i].SemanticName, len + 1);
  }
  if (Value->CachedPSO.pCachedBlob) {
    Value->CachedPSO.pCachedBlob = new uint8_t[Value->CachedPSO.CachedBlobSizeInBytes];
    memcpy(const_cast<void*>(Value->CachedPSO.pCachedBlob), arg.Value->CachedPSO.pCachedBlob,
           Value->CachedPSO.CachedBlobSizeInBytes);
  }
  Copy = true;
}

D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument::~D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument() {
  if (Copy) {
    delete[] static_cast<const uint8_t*>(Value->VS.pShaderBytecode);
    delete[] static_cast<const uint8_t*>(Value->PS.pShaderBytecode);
    delete[] static_cast<const uint8_t*>(Value->DS.pShaderBytecode);
    delete[] static_cast<const uint8_t*>(Value->HS.pShaderBytecode);
    delete[] static_cast<const uint8_t*>(Value->GS.pShaderBytecode);
    for (unsigned i = 0; i < Value->StreamOutput.NumEntries; ++i) {
      delete[] Value->StreamOutput.pSODeclaration[i].SemanticName;
    }
    delete[] Value->StreamOutput.pSODeclaration;
    delete[] Value->StreamOutput.pBufferStrides;
    for (unsigned i = 0; i < Value->InputLayout.NumElements; ++i) {
      delete[] Value->InputLayout.pInputElementDescs[i].SemanticName;
    }
    delete[] Value->InputLayout.pInputElementDescs;
    delete[] static_cast<const uint8_t*>(Value->CachedPSO.pCachedBlob);
    delete Value;
  }
}

D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument::D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument(
    const D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument& arg) {
  RootSignatureKey = arg.RootSignatureKey;
  if (!arg.Value) {
    return;
  }
  Value = new D3D12_COMPUTE_PIPELINE_STATE_DESC();
  *Value = *arg.Value;
  if (Value->CS.pShaderBytecode) {
    Value->CS.pShaderBytecode = new uint8_t[Value->CS.BytecodeLength];
    memcpy(const_cast<void*>(Value->CS.pShaderBytecode), arg.Value->CS.pShaderBytecode,
           Value->CS.BytecodeLength);
  }
  if (Value->CachedPSO.pCachedBlob) {
    Value->CachedPSO.pCachedBlob = new uint8_t[Value->CachedPSO.CachedBlobSizeInBytes];
    memcpy(const_cast<void*>(Value->CachedPSO.pCachedBlob), arg.Value->CachedPSO.pCachedBlob,
           Value->CachedPSO.CachedBlobSizeInBytes);
  }
  Copy = true;
}

D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument::~D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument() {
  if (Copy) {
    delete[] static_cast<const uint8_t*>(Value->CS.pShaderBytecode);
    delete[] static_cast<const uint8_t*>(Value->CachedPSO.pCachedBlob);
    delete Value;
  }
}

D3D12_TEXTURE_COPY_LOCATION_Argument::D3D12_TEXTURE_COPY_LOCATION_Argument(
    const D3D12_TEXTURE_COPY_LOCATION_Argument& arg) {
  if (arg.Value) {
    Value = new D3D12_TEXTURE_COPY_LOCATION();
    *Value = *arg.Value;
  }
  ResourceKey = arg.ResourceKey;
  Copy = true;
}

D3D12_TEXTURE_COPY_LOCATION_Argument::~D3D12_TEXTURE_COPY_LOCATION_Argument() {
  if (Copy) {
    delete Value;
  }
}

D3D12_RESOURCE_BARRIERs_Argument::D3D12_RESOURCE_BARRIERs_Argument(
    const D3D12_RESOURCE_BARRIERs_Argument& arg) {
  Size = arg.Size;
  if (arg.Value) {
    Value = new D3D12_RESOURCE_BARRIER[Size];
    memcpy(Value, arg.Value, Size * sizeof(D3D12_RESOURCE_BARRIER));
  }
  ResourceKeys = arg.ResourceKeys;
  ResourceAfterKeys = arg.ResourceAfterKeys;
  Copy = true;
}

D3D12_RESOURCE_BARRIERs_Argument::~D3D12_RESOURCE_BARRIERs_Argument() {
  if (Copy) {
    delete[] Value;
  }
}

D3D12_SHADER_RESOURCE_VIEW_DESC_Argument::D3D12_SHADER_RESOURCE_VIEW_DESC_Argument(
    const D3D12_SHADER_RESOURCE_VIEW_DESC_Argument& arg) {
  if (arg.Value) {
    Value = new D3D12_SHADER_RESOURCE_VIEW_DESC();
    *Value = *arg.Value;
  }
  RaytracingLocationKey = arg.RaytracingLocationKey;
  RaytracingLocationOffset = arg.RaytracingLocationOffset;
  Copy = true;
}

D3D12_SHADER_RESOURCE_VIEW_DESC_Argument::~D3D12_SHADER_RESOURCE_VIEW_DESC_Argument() {
  if (Copy) {
    delete Value;
  }
}

D3D12_INDEX_BUFFER_VIEW_Argument::D3D12_INDEX_BUFFER_VIEW_Argument(
    const D3D12_INDEX_BUFFER_VIEW_Argument& arg) {
  if (arg.Value) {
    Value = new D3D12_INDEX_BUFFER_VIEW();
    *Value = *arg.Value;
  }
  BufferLocationKey = arg.BufferLocationKey;
  BufferLocationOffset = arg.BufferLocationOffset;
  Copy = true;
}

D3D12_INDEX_BUFFER_VIEW_Argument::~D3D12_INDEX_BUFFER_VIEW_Argument() {
  if (Copy) {
    delete Value;
  }
}

D3D12_CONSTANT_BUFFER_VIEW_DESC_Argument::D3D12_CONSTANT_BUFFER_VIEW_DESC_Argument(
    const D3D12_CONSTANT_BUFFER_VIEW_DESC_Argument& arg) {
  if (arg.Value) {
    Value = new D3D12_CONSTANT_BUFFER_VIEW_DESC();
    *Value = *arg.Value;
  }
  BufferLocationKey = arg.BufferLocationKey;
  BufferLocationOffset = arg.BufferLocationOffset;
  Copy = true;
}

D3D12_CONSTANT_BUFFER_VIEW_DESC_Argument::~D3D12_CONSTANT_BUFFER_VIEW_DESC_Argument() {
  if (Copy) {
    delete Value;
  }
}

D3D12_VERTEX_BUFFER_VIEWs_Argument::D3D12_VERTEX_BUFFER_VIEWs_Argument(
    const D3D12_VERTEX_BUFFER_VIEWs_Argument& arg) {
  Size = arg.Size;
  if (arg.Value) {
    Value = new D3D12_VERTEX_BUFFER_VIEW[Size];
    memcpy(Value, arg.Value, Size * sizeof(D3D12_VERTEX_BUFFER_VIEW));
  }
  BufferLocationKeys = arg.BufferLocationKeys;
  BufferLocationOffsets = arg.BufferLocationOffsets;
  Copy = true;
}

D3D12_VERTEX_BUFFER_VIEWs_Argument::~D3D12_VERTEX_BUFFER_VIEWs_Argument() {
  if (Copy) {
    delete[] Value;
  }
}

D3D12_STREAM_OUTPUT_BUFFER_VIEWs_Argument::D3D12_STREAM_OUTPUT_BUFFER_VIEWs_Argument(
    const D3D12_STREAM_OUTPUT_BUFFER_VIEWs_Argument& arg) {
  Size = arg.Size;
  if (arg.Value) {
    Value = new D3D12_STREAM_OUTPUT_BUFFER_VIEW[Size];
    memcpy(Value, arg.Value, Size * sizeof(D3D12_STREAM_OUTPUT_BUFFER_VIEW));
  }
  BufferLocationKeys = arg.BufferLocationKeys;
  BufferLocationOffsets = arg.BufferLocationOffsets;
  BufferFilledSizeLocationKeys = arg.BufferFilledSizeLocationKeys;
  BufferFilledSizeLocationOffsets = arg.BufferFilledSizeLocationOffsets;
  Copy = true;
}

D3D12_STREAM_OUTPUT_BUFFER_VIEWs_Argument::~D3D12_STREAM_OUTPUT_BUFFER_VIEWs_Argument() {
  if (Copy) {
    delete[] Value;
  }
}

D3D12_WRITEBUFFERIMMEDIATE_PARAMETERs_Argument::D3D12_WRITEBUFFERIMMEDIATE_PARAMETERs_Argument(
    const D3D12_WRITEBUFFERIMMEDIATE_PARAMETERs_Argument& arg) {
  Size = arg.Size;
  if (arg.Value) {
    Value = new D3D12_WRITEBUFFERIMMEDIATE_PARAMETER[Size];
    memcpy(Value, arg.Value, Size * sizeof(D3D12_WRITEBUFFERIMMEDIATE_PARAMETER));
  }
  DestKeys = arg.DestKeys;
  DestOffsets = arg.DestOffsets;
  Copy = true;
}

D3D12_WRITEBUFFERIMMEDIATE_PARAMETERs_Argument::~D3D12_WRITEBUFFERIMMEDIATE_PARAMETERs_Argument() {
  if (Copy) {
    delete[] Value;
  }
}

D3D12_PIPELINE_STATE_STREAM_DESC_Argument::D3D12_PIPELINE_STATE_STREAM_DESC_Argument(
    const D3D12_PIPELINE_STATE_STREAM_DESC_Argument& arg) {
  if (!arg.Value) {
    return;
  }
  Value = new D3D12_PIPELINE_STATE_STREAM_DESC();
  *Value = *arg.Value;
  Value->pPipelineStateSubobjectStream = new uint8_t[Value->SizeInBytes];
  memcpy(Value->pPipelineStateSubobjectStream, arg.Value->pPipelineStateSubobjectStream,
         Value->SizeInBytes);
  size_t Offset{};
  while (Offset < arg.Value->SizeInBytes) {
    void* destData = static_cast<char*>(Value->pPipelineStateSubobjectStream) + Offset;
    void* srcData = static_cast<char*>(arg.Value->pPipelineStateSubobjectStream) + Offset;
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE subobjectType =
        *reinterpret_cast<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE*>(srcData);
    switch (subobjectType) {
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE:
      Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS: {
      D3D12_SHADER_BYTECODE& dest = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_VS*>(destData);
      D3D12_SHADER_BYTECODE& src = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_VS*>(srcData);
      if (src.pShaderBytecode) {
        dest.pShaderBytecode = new uint8_t[src.BytecodeLength];
        memcpy(const_cast<void*>(dest.pShaderBytecode), src.pShaderBytecode, src.BytecodeLength);
      }
      Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_VS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS: {
      D3D12_SHADER_BYTECODE& dest = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_PS*>(destData);
      D3D12_SHADER_BYTECODE& src = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_PS*>(srcData);
      if (src.pShaderBytecode) {
        dest.pShaderBytecode = new uint8_t[src.BytecodeLength];
        memcpy(const_cast<void*>(dest.pShaderBytecode), src.pShaderBytecode, src.BytecodeLength);
      }
      Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_PS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DS: {
      D3D12_SHADER_BYTECODE& dest = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_DS*>(destData);
      D3D12_SHADER_BYTECODE& src = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_DS*>(srcData);
      if (src.pShaderBytecode) {
        dest.pShaderBytecode = new uint8_t[src.BytecodeLength];
        memcpy(const_cast<void*>(dest.pShaderBytecode), src.pShaderBytecode, src.BytecodeLength);
      }
      Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_DS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_HS: {
      D3D12_SHADER_BYTECODE& dest = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_HS*>(destData);
      D3D12_SHADER_BYTECODE& src = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_HS*>(srcData);
      if (src.pShaderBytecode) {
        dest.pShaderBytecode = new uint8_t[src.BytecodeLength];
        memcpy(const_cast<void*>(dest.pShaderBytecode), src.pShaderBytecode, src.BytecodeLength);
      }
      Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_HS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_GS: {
      D3D12_SHADER_BYTECODE& dest = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_GS*>(destData);
      D3D12_SHADER_BYTECODE& src = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_GS*>(srcData);
      if (src.pShaderBytecode) {
        dest.pShaderBytecode = new uint8_t[src.BytecodeLength];
        memcpy(const_cast<void*>(dest.pShaderBytecode), src.pShaderBytecode, src.BytecodeLength);
      }
      Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_GS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS: {
      D3D12_SHADER_BYTECODE& dest = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_CS*>(destData);
      D3D12_SHADER_BYTECODE& src = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_CS*>(srcData);
      if (src.pShaderBytecode) {
        dest.pShaderBytecode = new uint8_t[src.BytecodeLength];
        memcpy(const_cast<void*>(dest.pShaderBytecode), src.pShaderBytecode, src.BytecodeLength);
      }
      Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_CS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS: {
      D3D12_SHADER_BYTECODE& dest = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_AS*>(destData);
      D3D12_SHADER_BYTECODE& src = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_AS*>(srcData);
      if (src.pShaderBytecode) {
        dest.pShaderBytecode = new uint8_t[src.BytecodeLength];
        memcpy(const_cast<void*>(dest.pShaderBytecode), src.pShaderBytecode, src.BytecodeLength);
      }
      Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_AS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS: {
      D3D12_SHADER_BYTECODE& dest = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_MS*>(destData);
      D3D12_SHADER_BYTECODE& src = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_MS*>(srcData);
      if (src.pShaderBytecode) {
        dest.pShaderBytecode = new uint8_t[src.BytecodeLength];
        memcpy(const_cast<void*>(dest.pShaderBytecode), src.pShaderBytecode, src.BytecodeLength);
      }
      Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_MS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_STREAM_OUTPUT: {
      D3D12_STREAM_OUTPUT_DESC& dest =
          *static_cast<CD3DX12_PIPELINE_STATE_STREAM_STREAM_OUTPUT*>(destData);
      D3D12_STREAM_OUTPUT_DESC& src =
          *static_cast<CD3DX12_PIPELINE_STATE_STREAM_STREAM_OUTPUT*>(srcData);
      if (src.pSODeclaration) {
        dest.pSODeclaration = new D3D12_SO_DECLARATION_ENTRY[src.NumEntries];
        memcpy(const_cast<D3D12_SO_DECLARATION_ENTRY*>(dest.pSODeclaration), src.pSODeclaration,
               src.NumEntries * sizeof(D3D12_SO_DECLARATION_ENTRY));
        for (unsigned i = 0; i < src.NumEntries; ++i) {
          unsigned len = strlen(src.pSODeclaration[i].SemanticName);
          const_cast<D3D12_SO_DECLARATION_ENTRY*>(dest.pSODeclaration)[i].SemanticName =
              new char[len + 1];
          memcpy(const_cast<char*>(dest.pSODeclaration[i].SemanticName),
                 src.pSODeclaration[i].SemanticName, len + 1);
        }
      }
      if (src.pBufferStrides) {
        dest.pBufferStrides = new UINT[src.NumStrides];
        memcpy(const_cast<UINT*>(dest.pBufferStrides), src.pBufferStrides,
               src.NumStrides * sizeof(UINT));
      }
      Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_STREAM_OUTPUT);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND:
      Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_MASK:
      Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_MASK);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_NODE_MASK:
      Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_NODE_MASK);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER:
      Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER1:
      Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER1);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER2:
      Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER2);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL:
      Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL1:
      Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL1);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL2:
      Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL2);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT: {
      D3D12_INPUT_LAYOUT_DESC& dest =
          *static_cast<CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT*>(destData);
      D3D12_INPUT_LAYOUT_DESC& src =
          *static_cast<CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT*>(srcData);
      if (src.pInputElementDescs) {
        dest.pInputElementDescs = new D3D12_INPUT_ELEMENT_DESC[src.NumElements];
        memcpy(const_cast<D3D12_INPUT_ELEMENT_DESC*>(dest.pInputElementDescs),
               src.pInputElementDescs, src.NumElements * sizeof(D3D12_INPUT_ELEMENT_DESC));
        for (unsigned i = 0; i < src.NumElements; ++i) {
          unsigned len = strlen(src.pInputElementDescs[i].SemanticName);
          const_cast<D3D12_INPUT_ELEMENT_DESC*>(dest.pInputElementDescs)[i].SemanticName =
              new char[len + 1];
          memcpy(const_cast<char*>(dest.pInputElementDescs[i].SemanticName),
                 src.pInputElementDescs[i].SemanticName, len + 1);
        }
      }
      Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_IB_STRIP_CUT_VALUE:
      Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_IB_STRIP_CUT_VALUE);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY:
      Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS:
      Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT:
      Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC:
      Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CACHED_PSO: {
      D3D12_CACHED_PIPELINE_STATE& dest =
          *static_cast<CD3DX12_PIPELINE_STATE_STREAM_CACHED_PSO*>(destData);
      D3D12_CACHED_PIPELINE_STATE& src =
          *static_cast<CD3DX12_PIPELINE_STATE_STREAM_CACHED_PSO*>(srcData);
      if (src.CachedBlobSizeInBytes) {
        dest.pCachedBlob = new uint8_t[src.CachedBlobSizeInBytes];
        memcpy(const_cast<void*>(dest.pCachedBlob), src.pCachedBlob, src.CachedBlobSizeInBytes);
      }
      Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_CACHED_PSO);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_FLAGS:
      Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_FLAGS);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VIEW_INSTANCING: {
      D3D12_VIEW_INSTANCING_DESC& dest =
          *static_cast<CD3DX12_PIPELINE_STATE_STREAM_VIEW_INSTANCING*>(destData);
      D3D12_VIEW_INSTANCING_DESC& src =
          *static_cast<CD3DX12_PIPELINE_STATE_STREAM_VIEW_INSTANCING*>(srcData);
      if (src.pViewInstanceLocations) {
        dest.pViewInstanceLocations = new D3D12_VIEW_INSTANCE_LOCATION[src.ViewInstanceCount];
        memcpy(const_cast<D3D12_VIEW_INSTANCE_LOCATION*>(dest.pViewInstanceLocations),
               src.pViewInstanceLocations,
               src.ViewInstanceCount * sizeof(D3D12_VIEW_INSTANCE_LOCATION));
      }
      Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_VIEW_INSTANCING);
    } break;
    }
  }
  RootSignatureKey = arg.RootSignatureKey;
  Copy = true;
}

D3D12_PIPELINE_STATE_STREAM_DESC_Argument::~D3D12_PIPELINE_STATE_STREAM_DESC_Argument() {
  if (Copy) {
    size_t Offset{};
    while (Offset < Value->SizeInBytes) {
      void* Data = static_cast<char*>(Value->pPipelineStateSubobjectStream) + Offset;
      D3D12_PIPELINE_STATE_SUBOBJECT_TYPE subobjectType =
          *reinterpret_cast<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE*>(Data);
      switch (subobjectType) {
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE:
        Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE);
        break;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS: {
        D3D12_SHADER_BYTECODE& desc = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_VS*>(Data);
        delete[] static_cast<const uint8_t*>(desc.pShaderBytecode);
        Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_VS);
      } break;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS: {
        D3D12_SHADER_BYTECODE& desc = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_PS*>(Data);
        delete[] static_cast<const uint8_t*>(desc.pShaderBytecode);
        Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_PS);
      } break;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DS: {
        D3D12_SHADER_BYTECODE& desc = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_DS*>(Data);
        delete[] static_cast<const uint8_t*>(desc.pShaderBytecode);
        Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_DS);
      } break;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_HS: {
        D3D12_SHADER_BYTECODE& desc = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_HS*>(Data);
        delete[] static_cast<const uint8_t*>(desc.pShaderBytecode);
        Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_HS);
      } break;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_GS: {
        D3D12_SHADER_BYTECODE& desc = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_GS*>(Data);
        delete[] static_cast<const uint8_t*>(desc.pShaderBytecode);
        Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_GS);
      } break;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS: {
        D3D12_SHADER_BYTECODE& desc = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_CS*>(Data);
        delete[] static_cast<const uint8_t*>(desc.pShaderBytecode);
        Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_CS);
      } break;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS: {
        D3D12_SHADER_BYTECODE& desc = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_AS*>(Data);
        delete[] static_cast<const uint8_t*>(desc.pShaderBytecode);
        Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_AS);
      } break;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS: {
        D3D12_SHADER_BYTECODE& desc = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_MS*>(Data);
        delete[] static_cast<const uint8_t*>(desc.pShaderBytecode);
        Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_MS);
      } break;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_STREAM_OUTPUT: {
        D3D12_STREAM_OUTPUT_DESC& desc =
            *static_cast<CD3DX12_PIPELINE_STATE_STREAM_STREAM_OUTPUT*>(Data);
        if (desc.pSODeclaration) {
          for (unsigned i = 0; i < desc.NumEntries; ++i) {
            delete[] desc.pSODeclaration[i].SemanticName;
          }
          delete[] desc.pSODeclaration;
        }
        delete[] desc.pBufferStrides;
        Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_STREAM_OUTPUT);
      } break;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND:
        Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC);
        break;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_MASK:
        Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_MASK);
        break;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_NODE_MASK:
        Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_NODE_MASK);
        break;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER:
        Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER);
        break;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER1:
        Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER1);
        break;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER2:
        Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER2);
        break;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL:
        Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL);
        break;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL1:
        Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL1);
        break;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL2:
        Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL2);
        break;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT: {
        D3D12_INPUT_LAYOUT_DESC& desc =
            *static_cast<CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT*>(Data);
        if (desc.pInputElementDescs) {
          for (unsigned i = 0; i < desc.NumElements; ++i) {
            delete[] desc.pInputElementDescs[i].SemanticName;
          }
          delete[] desc.pInputElementDescs;
        }
        Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT);
      } break;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_IB_STRIP_CUT_VALUE:
        Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_IB_STRIP_CUT_VALUE);
        break;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY:
        Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY);
        break;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS:
        Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS);
        break;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT:
        Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT);
        break;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC:
        Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC);
        break;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CACHED_PSO: {
        D3D12_CACHED_PIPELINE_STATE& desc =
            *static_cast<CD3DX12_PIPELINE_STATE_STREAM_CACHED_PSO*>(Data);
        delete[] static_cast<const uint8_t*>(desc.pCachedBlob);
        Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_CACHED_PSO);
      } break;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_FLAGS:
        Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_FLAGS);
        break;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VIEW_INSTANCING: {
        D3D12_VIEW_INSTANCING_DESC& desc =
            *static_cast<CD3DX12_PIPELINE_STATE_STREAM_VIEW_INSTANCING*>(Data);
        delete[] desc.pViewInstanceLocations;
        Offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_VIEW_INSTANCING);
      } break;
      }
    }
    delete[] static_cast<const uint8_t*>(Value->pPipelineStateSubobjectStream);
    delete Value;
  }
}

D3D12_STATE_OBJECT_DESC_Argument::D3D12_STATE_OBJECT_DESC_Argument(
    const D3D12_STATE_OBJECT_DESC_Argument& arg) {
  if (!arg.Value) {
    return;
  }
  Value = new D3D12_STATE_OBJECT_DESC();
  *Value = *arg.Value;
  Value->pSubobjects = new D3D12_STATE_SUBOBJECT[Value->NumSubobjects];
  std::map<const D3D12_STATE_SUBOBJECT*, unsigned> srcSubobjectIndexes;
  for (unsigned i = 0; i < Value->NumSubobjects; ++i) {
    D3D12_STATE_SUBOBJECT& destSub = const_cast<D3D12_STATE_SUBOBJECT*>(Value->pSubobjects)[i];
    const D3D12_STATE_SUBOBJECT& srcSub = arg.Value->pSubobjects[i];
    srcSubobjectIndexes[&srcSub] = i;
    destSub = srcSub;
    switch (srcSub.Type) {
    case D3D12_STATE_SUBOBJECT_TYPE_STATE_OBJECT_CONFIG: {
      destSub.pDesc = new D3D12_STATE_OBJECT_CONFIG();
      *static_cast<D3D12_STATE_OBJECT_CONFIG*>(const_cast<void*>(destSub.pDesc)) =
          *static_cast<const D3D12_STATE_OBJECT_CONFIG*>(srcSub.pDesc);
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE: {
      destSub.pDesc = new D3D12_GLOBAL_ROOT_SIGNATURE();
      *static_cast<D3D12_GLOBAL_ROOT_SIGNATURE*>(const_cast<void*>(destSub.pDesc)) =
          *static_cast<const D3D12_GLOBAL_ROOT_SIGNATURE*>(srcSub.pDesc);
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE: {
      destSub.pDesc = new D3D12_LOCAL_ROOT_SIGNATURE();
      *static_cast<D3D12_LOCAL_ROOT_SIGNATURE*>(const_cast<void*>(destSub.pDesc)) =
          *static_cast<const D3D12_LOCAL_ROOT_SIGNATURE*>(srcSub.pDesc);
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_NODE_MASK: {
      destSub.pDesc = new D3D12_NODE_MASK();
      *static_cast<D3D12_NODE_MASK*>(const_cast<void*>(destSub.pDesc)) =
          *static_cast<const D3D12_NODE_MASK*>(srcSub.pDesc);
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY: {
      destSub.pDesc = new D3D12_DXIL_LIBRARY_DESC();
      D3D12_DXIL_LIBRARY_DESC& dest =
          *static_cast<D3D12_DXIL_LIBRARY_DESC*>(const_cast<void*>(destSub.pDesc));
      const D3D12_DXIL_LIBRARY_DESC& src =
          *static_cast<const D3D12_DXIL_LIBRARY_DESC*>(srcSub.pDesc);
      dest = src;
      dest.DXILLibrary.pShaderBytecode = new uint8_t[src.DXILLibrary.BytecodeLength];
      memcpy(const_cast<void*>(dest.DXILLibrary.pShaderBytecode), src.DXILLibrary.pShaderBytecode,
             src.DXILLibrary.BytecodeLength);
      dest.pExports = new D3D12_EXPORT_DESC[src.NumExports];
      for (unsigned j = 0; j < src.NumExports; ++j) {
        const_cast<D3D12_EXPORT_DESC*>(dest.pExports)[j] = src.pExports[j];
        unsigned len = wcslen(src.pExports[j].Name);
        const_cast<D3D12_EXPORT_DESC*>(dest.pExports)[j].Name = new wchar_t[len + 1];
        memcpy(const_cast<wchar_t*>(dest.pExports[j].Name), src.pExports[j].Name, len * 2 + 2);
        if (src.pExports[j].ExportToRename) {
          len = wcslen(src.pExports[j].ExportToRename);
          const_cast<D3D12_EXPORT_DESC*>(dest.pExports)[j].ExportToRename = new wchar_t[len + 1];
          memcpy(const_cast<wchar_t*>(dest.pExports[j].ExportToRename),
                 src.pExports[j].ExportToRename, len * 2 + 2);
        }
      }
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION: {
      destSub.pDesc = new D3D12_EXISTING_COLLECTION_DESC();
      D3D12_EXISTING_COLLECTION_DESC& dest =
          *static_cast<D3D12_EXISTING_COLLECTION_DESC*>(const_cast<void*>(destSub.pDesc));
      const D3D12_EXISTING_COLLECTION_DESC& src =
          *static_cast<const D3D12_EXISTING_COLLECTION_DESC*>(srcSub.pDesc);
      dest = src;
      dest.pExports = new D3D12_EXPORT_DESC[src.NumExports];
      for (unsigned j = 0; j < src.NumExports; ++j) {
        const_cast<D3D12_EXPORT_DESC*>(dest.pExports)[j] = src.pExports[j];
        unsigned len = wcslen(src.pExports[j].Name);
        const_cast<D3D12_EXPORT_DESC*>(dest.pExports)[j].Name = new wchar_t[len + 1];
        memcpy(const_cast<wchar_t*>(dest.pExports[j].Name), src.pExports[j].Name, len * 2 + 2);
        if (src.pExports[j].ExportToRename) {
          len = wcslen(src.pExports[j].ExportToRename);
          const_cast<D3D12_EXPORT_DESC*>(dest.pExports)[j].ExportToRename = new wchar_t[len + 1];
          memcpy(const_cast<wchar_t*>(dest.pExports[j].ExportToRename),
                 src.pExports[j].ExportToRename, len * 2 + 2);
        }
      }
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION: {
      destSub.pDesc = new D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION();
      D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION& dest =
          *static_cast<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(const_cast<void*>(destSub.pDesc));
      const D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION& src =
          *static_cast<const D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(srcSub.pDesc);
      dest = src;
      dest.pExports = new LPCWSTR[src.NumExports];
      for (unsigned j = 0; j < src.NumExports; ++j) {
        unsigned len = wcslen(src.pExports[j]);
        dest.pExports[j] = new wchar_t[len + 1];
        memcpy(const_cast<wchar_t*>(dest.pExports[j]), src.pExports[j], len * 2 + 2);
      }
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION: {
      destSub.pDesc = new D3D12_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION();
      D3D12_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION& dest =
          *static_cast<D3D12_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(
              const_cast<void*>(destSub.pDesc));
      const D3D12_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION& src =
          *static_cast<const D3D12_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(srcSub.pDesc);
      dest = src;
      unsigned len = wcslen(src.SubobjectToAssociate);
      dest.SubobjectToAssociate = new wchar_t[len + 1];
      memcpy(const_cast<wchar_t*>(dest.SubobjectToAssociate), src.SubobjectToAssociate,
             len * 2 + 2);
      dest.pExports = new LPCWSTR[src.NumExports];
      for (unsigned j = 0; j < src.NumExports; ++j) {
        unsigned len = wcslen(src.pExports[j]);
        dest.pExports[j] = new wchar_t[len + 1];
        memcpy(const_cast<wchar_t*>(dest.pExports[j]), src.pExports[j], len * 2 + 2);
      }
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG: {
      destSub.pDesc = new D3D12_RAYTRACING_SHADER_CONFIG();
      *static_cast<D3D12_RAYTRACING_SHADER_CONFIG*>(const_cast<void*>(destSub.pDesc)) =
          *static_cast<const D3D12_RAYTRACING_SHADER_CONFIG*>(srcSub.pDesc);
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG: {
      destSub.pDesc = new D3D12_RAYTRACING_PIPELINE_CONFIG();
      *static_cast<D3D12_RAYTRACING_PIPELINE_CONFIG*>(const_cast<void*>(destSub.pDesc)) =
          *static_cast<const D3D12_RAYTRACING_PIPELINE_CONFIG*>(srcSub.pDesc);
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP: {
      destSub.pDesc = new D3D12_HIT_GROUP_DESC();
      D3D12_HIT_GROUP_DESC& dest =
          *static_cast<D3D12_HIT_GROUP_DESC*>(const_cast<void*>(destSub.pDesc));
      const D3D12_HIT_GROUP_DESC& src = *static_cast<const D3D12_HIT_GROUP_DESC*>(srcSub.pDesc);
      dest = src;
      unsigned len = wcslen(src.HitGroupExport);
      dest.HitGroupExport = new wchar_t[len + 1];
      memcpy(const_cast<wchar_t*>(dest.HitGroupExport), src.HitGroupExport, len * 2 + 2);
      if (src.AnyHitShaderImport) {
        unsigned len = wcslen(src.AnyHitShaderImport);
        dest.AnyHitShaderImport = new wchar_t[len + 1];
        memcpy(const_cast<wchar_t*>(dest.AnyHitShaderImport), src.AnyHitShaderImport, len * 2 + 2);
      }
      if (src.ClosestHitShaderImport) {
        unsigned len = wcslen(src.ClosestHitShaderImport);
        dest.ClosestHitShaderImport = new wchar_t[len + 1];
        memcpy(const_cast<wchar_t*>(dest.ClosestHitShaderImport), src.ClosestHitShaderImport,
               len * 2 + 2);
      }
      if (src.IntersectionShaderImport) {
        unsigned len = wcslen(src.IntersectionShaderImport);
        dest.IntersectionShaderImport = new wchar_t[len + 1];
        memcpy(const_cast<wchar_t*>(dest.IntersectionShaderImport), src.IntersectionShaderImport,
               len * 2 + 2);
      }
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG1: {
      destSub.pDesc = new D3D12_RAYTRACING_PIPELINE_CONFIG1();
      *static_cast<D3D12_RAYTRACING_PIPELINE_CONFIG1*>(const_cast<void*>(destSub.pDesc)) =
          *static_cast<const D3D12_RAYTRACING_PIPELINE_CONFIG1*>(srcSub.pDesc);
    } break;
    }
  }

  for (unsigned Index = 0; Index < arg.Value->NumSubobjects; ++Index) {
    if (arg.Value->pSubobjects[Index].Type ==
        D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION) {
      const D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION* src =
          static_cast<const D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(
              arg.Value->pSubobjects[Index].pDesc);
      auto it = srcSubobjectIndexes.find(src->pSubobjectToAssociate);
      if (it != srcSubobjectIndexes.end()) {
        unsigned associatedIndex = it->second;
        D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION* dest =
            static_cast<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(
                const_cast<void*>(Value->pSubobjects[Index].pDesc));
        dest->pSubobjectToAssociate = &Value->pSubobjects[associatedIndex];
      }
    }
  }
  InterfaceKeysBySubobject = arg.InterfaceKeysBySubobject;
  Copy = true;
}

D3D12_STATE_OBJECT_DESC_Argument::~D3D12_STATE_OBJECT_DESC_Argument() {
  if (Copy) {
    for (unsigned i = 0; i < Value->NumSubobjects; ++i) {
      D3D12_STATE_SUBOBJECT& sub = const_cast<D3D12_STATE_SUBOBJECT*>(Value->pSubobjects)[i];
      switch (sub.Type) {
      case D3D12_STATE_SUBOBJECT_TYPE_STATE_OBJECT_CONFIG: {
        delete static_cast<const D3D12_STATE_OBJECT_CONFIG*>(sub.pDesc);
      } break;
      case D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE: {
        delete static_cast<const D3D12_GLOBAL_ROOT_SIGNATURE*>(sub.pDesc);
      } break;
      case D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE: {
        delete static_cast<const D3D12_LOCAL_ROOT_SIGNATURE*>(sub.pDesc);
      } break;
      case D3D12_STATE_SUBOBJECT_TYPE_NODE_MASK: {
        delete static_cast<const D3D12_NODE_MASK*>(sub.pDesc);
      } break;
      case D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY: {
        D3D12_DXIL_LIBRARY_DESC& desc =
            *static_cast<D3D12_DXIL_LIBRARY_DESC*>(const_cast<void*>(sub.pDesc));
        delete[] static_cast<const uint8_t*>(desc.DXILLibrary.pShaderBytecode);
        for (unsigned j = 0; j < desc.NumExports; ++j) {
          delete[] desc.pExports[j].Name;
          delete[] desc.pExports[j].ExportToRename;
        }
        delete[] desc.pExports;
        delete static_cast<const D3D12_DXIL_LIBRARY_DESC*>(sub.pDesc);
      } break;
      case D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION: {
        D3D12_EXISTING_COLLECTION_DESC& desc =
            *static_cast<D3D12_EXISTING_COLLECTION_DESC*>(const_cast<void*>(sub.pDesc));
        for (unsigned j = 0; j < desc.NumExports; ++j) {
          delete[] desc.pExports[j].Name;
          delete[] desc.pExports[j].ExportToRename;
        }
        delete[] desc.pExports;
        delete static_cast<const D3D12_EXISTING_COLLECTION_DESC*>(sub.pDesc);
      } break;
      case D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION: {
        D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION& desc =
            *static_cast<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(const_cast<void*>(sub.pDesc));
        for (unsigned j = 0; j < desc.NumExports; ++j) {
          delete[] desc.pExports[j];
        }
        delete[] desc.pExports;
        delete static_cast<const D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(sub.pDesc);
      } break;
      case D3D12_STATE_SUBOBJECT_TYPE_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION: {
        D3D12_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION& desc =
            *static_cast<D3D12_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(
                const_cast<void*>(sub.pDesc));
        delete[] desc.SubobjectToAssociate;
        for (unsigned j = 0; j < desc.NumExports; ++j) {
          delete[] desc.pExports[j];
        }
        delete[] desc.pExports;
        delete static_cast<const D3D12_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(sub.pDesc);
      } break;
      case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG: {
        delete static_cast<const D3D12_RAYTRACING_SHADER_CONFIG*>(sub.pDesc);
      } break;
      case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG: {
        delete static_cast<const D3D12_RAYTRACING_PIPELINE_CONFIG*>(sub.pDesc);
      } break;

      case D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP: {
        D3D12_HIT_GROUP_DESC& desc =
            *static_cast<D3D12_HIT_GROUP_DESC*>(const_cast<void*>(sub.pDesc));
        delete[] desc.HitGroupExport;
        delete[] desc.AnyHitShaderImport;
        delete[] desc.ClosestHitShaderImport;
        delete[] desc.IntersectionShaderImport;
        delete static_cast<const D3D12_HIT_GROUP_DESC*>(sub.pDesc);
      } break;
      case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG1: {
        delete static_cast<const D3D12_RAYTRACING_PIPELINE_CONFIG1*>(sub.pDesc);
      } break;
      }
    }
    delete[] Value->pSubobjects;
    delete Value;
  }
}

D3D12_EXTENSION_ARGUMENTS_Argument::D3D12_EXTENSION_ARGUMENTS_Argument(
    const D3D12_EXTENSION_ARGUMENTS_Argument& arg) {
  if (!arg.Value) {
    return;
  }

  GITS_ASSERT(false, "Copy constructor not implemented for D3D12_EXTENSION_ARGUMENTS_Argument");
}

D3D12_EXTENSION_ARGUMENTS_Argument::~D3D12_EXTENSION_ARGUMENTS_Argument() {
  if (Copy) {
    GITS_ASSERT(false, "Destructor not implemented for D3D12_EXTENSION_ARGUMENTS_Argument");
  }
}

D3D12_EXTENDED_OPERATION_DATA_Argument::D3D12_EXTENDED_OPERATION_DATA_Argument(
    const D3D12_EXTENDED_OPERATION_DATA_Argument& arg) {
  if (!arg.Value) {
    return;
  }

  GITS_ASSERT(false, "Copy constructor not implemented for D3D12_EXTENDED_OPERATION_DATA_Argument");
}

D3D12_EXTENDED_OPERATION_DATA_Argument::~D3D12_EXTENDED_OPERATION_DATA_Argument() {
  if (Copy) {
    GITS_ASSERT(false, "Destructor not implemented for D3D12_EXTENDED_OPERATION_DATA_Argument");
  }
}

PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS>::PointerArgument(
    const PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS>& arg) {
  if (!arg.Value) {
    return;
  }
  Value = new D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS();
  *Value = *arg.Value;
  if (Value->Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
    if (Value->DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY) {
      if (Value->pGeometryDescs) {
        Value->pGeometryDescs = new D3D12_RAYTRACING_GEOMETRY_DESC[Value->NumDescs];
        memcpy(const_cast<D3D12_RAYTRACING_GEOMETRY_DESC*>(Value->pGeometryDescs),
               arg.Value->pGeometryDescs, Value->NumDescs * sizeof(D3D12_RAYTRACING_GEOMETRY_DESC));
        for (unsigned i = 0; i < Value->NumDescs; ++i) {
          auto& desc = const_cast<D3D12_RAYTRACING_GEOMETRY_DESC*>(Value->pGeometryDescs)[i];
          const auto& argDesc = arg.Value->pGeometryDescs[i];
          if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES) {
            if (desc.OmmTriangles.pTriangles) {
              desc.OmmTriangles.pTriangles = new D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC();
              *const_cast<D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC*>(desc.OmmTriangles.pTriangles) =
                  *argDesc.OmmTriangles.pTriangles;
            }
            if (argDesc.OmmTriangles.pOmmLinkage) {
              desc.OmmTriangles.pOmmLinkage = new D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC();
              *const_cast<D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC*>(
                  desc.OmmTriangles.pOmmLinkage) = *argDesc.OmmTriangles.pOmmLinkage;
            }
          }
        }
      }
    } else if (Value->DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS) {
      if (Value->ppGeometryDescs) {
        Value->ppGeometryDescs = new D3D12_RAYTRACING_GEOMETRY_DESC*[Value->NumDescs];
        for (unsigned i = 0; i < Value->NumDescs; ++i) {
          const_cast<D3D12_RAYTRACING_GEOMETRY_DESC**>(Value->ppGeometryDescs)[i] =
              new D3D12_RAYTRACING_GEOMETRY_DESC();

          auto& desc = *const_cast<D3D12_RAYTRACING_GEOMETRY_DESC**>(Value->ppGeometryDescs)[i];
          const auto& argDesc = *arg.Value->ppGeometryDescs[i];
          desc = argDesc;

          if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES) {
            if (desc.OmmTriangles.pTriangles) {
              desc.OmmTriangles.pTriangles = new D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC();
              *const_cast<D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC*>(desc.OmmTriangles.pTriangles) =
                  *argDesc.OmmTriangles.pTriangles;
            }
            if (desc.OmmTriangles.pOmmLinkage) {
              desc.OmmTriangles.pOmmLinkage = new D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC();
              *const_cast<D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC*>(
                  desc.OmmTriangles.pOmmLinkage) = *argDesc.OmmTriangles.pOmmLinkage;
            }
          }
        }
      }
    }
  } else if (Value->Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_OPACITY_MICROMAP_ARRAY) {
    if (Value->pOpacityMicromapArrayDesc) {
      Value->pOpacityMicromapArrayDesc = new D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC();
      auto desc = const_cast<D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC*>(
          Value->pOpacityMicromapArrayDesc);
      memcpy(desc, arg.Value->pOpacityMicromapArrayDesc,
             sizeof(D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC));
      desc->pOmmHistogram =
          new D3D12_RAYTRACING_OPACITY_MICROMAP_HISTOGRAM_ENTRY[Value->pOpacityMicromapArrayDesc
                                                                    ->NumOmmHistogramEntries];
      memcpy(const_cast<D3D12_RAYTRACING_OPACITY_MICROMAP_HISTOGRAM_ENTRY*>(desc->pOmmHistogram),
             arg.Value->pOpacityMicromapArrayDesc->pOmmHistogram,
             sizeof(D3D12_RAYTRACING_OPACITY_MICROMAP_HISTOGRAM_ENTRY) *
                 Value->pOpacityMicromapArrayDesc->NumOmmHistogramEntries);
    }
  }
  InputKeys = arg.InputKeys;
  InputOffsets = arg.InputOffsets;
  Copy = true;
}

PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS>::~PointerArgument() {
  if (Copy) {
    if (Value->Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
      if (Value->DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY) {
        for (unsigned i = 0; i < Value->NumDescs; ++i) {
          if (Value->pGeometryDescs[i].Type == D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES) {
            delete Value->pGeometryDescs[i].OmmTriangles.pTriangles;
            delete Value->pGeometryDescs[i].OmmTriangles.pOmmLinkage;
          }
        }
        delete[] Value->pGeometryDescs;
      } else if (Value->DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS) {
        for (unsigned i = 0; i < Value->NumDescs; ++i) {
          if (Value->ppGeometryDescs[i]->Type == D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES) {
            delete Value->ppGeometryDescs[i]->OmmTriangles.pTriangles;
            delete Value->ppGeometryDescs[i]->OmmTriangles.pOmmLinkage;
          }
          delete Value->ppGeometryDescs[i];
        }
        delete[] Value->ppGeometryDescs;
      }
    } else if (Value->Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_OPACITY_MICROMAP_ARRAY) {
      if (Value->pOpacityMicromapArrayDesc) {
        delete[] Value->pOpacityMicromapArrayDesc->pOmmHistogram;
        delete Value->pOpacityMicromapArrayDesc;
      }
    }
    delete Value;
  }
}

PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC>::PointerArgument(
    const PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC>& arg) {
  if (!arg.Value) {
    return;
  }
  Value = new D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC();
  *Value = *arg.Value;
  if (Value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
    if (Value->Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY) {
      if (Value->Inputs.pGeometryDescs) {
        Value->Inputs.pGeometryDescs = new D3D12_RAYTRACING_GEOMETRY_DESC[Value->Inputs.NumDescs];
        memcpy(const_cast<D3D12_RAYTRACING_GEOMETRY_DESC*>(Value->Inputs.pGeometryDescs),
               arg.Value->Inputs.pGeometryDescs,
               Value->Inputs.NumDescs * sizeof(D3D12_RAYTRACING_GEOMETRY_DESC));
        for (unsigned i = 0; i < Value->Inputs.NumDescs; ++i) {
          auto& desc = const_cast<D3D12_RAYTRACING_GEOMETRY_DESC*>(Value->Inputs.pGeometryDescs)[i];
          const auto& argDesc = arg.Value->Inputs.pGeometryDescs[i];
          if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES) {
            if (desc.OmmTriangles.pTriangles) {
              desc.OmmTriangles.pTriangles = new D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC();
              *const_cast<D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC*>(desc.OmmTriangles.pTriangles) =
                  *argDesc.OmmTriangles.pTriangles;
            }
            if (desc.OmmTriangles.pOmmLinkage) {
              desc.OmmTriangles.pOmmLinkage = new D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC();
              *const_cast<D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC*>(
                  desc.OmmTriangles.pOmmLinkage) = *argDesc.OmmTriangles.pOmmLinkage;
            }
          }
        }
      }
    } else if (Value->Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS) {
      if (Value->Inputs.ppGeometryDescs) {
        Value->Inputs.ppGeometryDescs = new D3D12_RAYTRACING_GEOMETRY_DESC*[Value->Inputs.NumDescs];
        for (unsigned i = 0; i < Value->Inputs.NumDescs; ++i) {
          const_cast<D3D12_RAYTRACING_GEOMETRY_DESC**>(Value->Inputs.ppGeometryDescs)[i] =
              new D3D12_RAYTRACING_GEOMETRY_DESC();

          auto& desc =
              *const_cast<D3D12_RAYTRACING_GEOMETRY_DESC**>(Value->Inputs.ppGeometryDescs)[i];
          const auto& argDesc = *arg.Value->Inputs.ppGeometryDescs[i];
          desc = argDesc;

          if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES) {
            if (desc.OmmTriangles.pTriangles) {
              desc.OmmTriangles.pTriangles = new D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC();
              *const_cast<D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC*>(desc.OmmTriangles.pTriangles) =
                  *argDesc.OmmTriangles.pTriangles;
            }
            if (desc.OmmTriangles.pOmmLinkage) {
              desc.OmmTriangles.pOmmLinkage = new D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC();
              *const_cast<D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC*>(
                  desc.OmmTriangles.pOmmLinkage) = *argDesc.OmmTriangles.pOmmLinkage;
            }
          }
        }
      }
    }
  } else if (Value->Inputs.Type ==
             D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_OPACITY_MICROMAP_ARRAY) {
    if (Value->Inputs.pOpacityMicromapArrayDesc) {
      Value->Inputs.pOpacityMicromapArrayDesc = new D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC();
      auto desc = const_cast<D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC*>(
          Value->Inputs.pOpacityMicromapArrayDesc);
      memcpy(desc, arg.Value->Inputs.pOpacityMicromapArrayDesc,
             sizeof(D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC));
      desc->pOmmHistogram = new D3D12_RAYTRACING_OPACITY_MICROMAP_HISTOGRAM_ENTRY
          [Value->Inputs.pOpacityMicromapArrayDesc->NumOmmHistogramEntries];
      memcpy(const_cast<D3D12_RAYTRACING_OPACITY_MICROMAP_HISTOGRAM_ENTRY*>(desc->pOmmHistogram),
             arg.Value->Inputs.pOpacityMicromapArrayDesc->pOmmHistogram,
             sizeof(D3D12_RAYTRACING_OPACITY_MICROMAP_HISTOGRAM_ENTRY) *
                 Value->Inputs.pOpacityMicromapArrayDesc->NumOmmHistogramEntries);
    }
  }
  DestAccelerationStructureKey = arg.DestAccelerationStructureKey;
  DestAccelerationStructureOffset = arg.DestAccelerationStructureOffset;
  SourceAccelerationStructureKey = arg.SourceAccelerationStructureKey;
  SourceAccelerationStructureOffset = arg.SourceAccelerationStructureOffset;
  ScratchAccelerationStructureKey = arg.ScratchAccelerationStructureKey;
  ScratchAccelerationStructureOffset = arg.ScratchAccelerationStructureOffset;
  InputKeys = arg.InputKeys;
  InputOffsets = arg.InputOffsets;
  Copy = true;
}

PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC>::~PointerArgument() {
  if (Copy) {
    if (Value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
      if (Value->Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY) {
        for (unsigned i = 0; i < Value->Inputs.NumDescs; ++i) {
          if (Value->Inputs.pGeometryDescs[i].Type ==
              D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES) {
            delete Value->Inputs.pGeometryDescs[i].OmmTriangles.pTriangles;
            delete Value->Inputs.pGeometryDescs[i].OmmTriangles.pOmmLinkage;
          }
        }
        delete[] Value->Inputs.pGeometryDescs;
      } else if (Value->Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS) {
        for (unsigned i = 0; i < Value->Inputs.NumDescs; ++i) {
          if (Value->Inputs.ppGeometryDescs[i]->Type ==
              D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES) {
            delete Value->Inputs.ppGeometryDescs[i]->OmmTriangles.pTriangles;
            delete Value->Inputs.ppGeometryDescs[i]->OmmTriangles.pOmmLinkage;
          }
          delete Value->Inputs.ppGeometryDescs[i];
        }
        delete[] Value->Inputs.ppGeometryDescs;
      }
    } else if (Value->Inputs.Type ==
               D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_OPACITY_MICROMAP_ARRAY) {
      if (Value->Inputs.pOpacityMicromapArrayDesc) {
        delete[] Value->Inputs.pOpacityMicromapArrayDesc->pOmmHistogram;
        delete Value->Inputs.pOpacityMicromapArrayDesc;
      }
    }
    delete Value;
  }
}

ArrayArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>::ArrayArgument(
    const ArrayArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>& arg) {
  Size = arg.Size;
  if (arg.Value) {
    Value = new D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC[Size];
    memcpy(Value, arg.Value,
           Size * sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC));
  }
  DestBufferKeys = arg.DestBufferKeys;
  DestBufferOffsets = arg.DestBufferOffsets;
  Copy = true;
}

ArrayArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>::~ArrayArgument() {
  if (Copy) {
    delete[] Value;
  }
}

PointerArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>::PointerArgument(
    const PointerArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>& arg) {
  if (arg.Value) {
    Value = new D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC();
    *Value = *arg.Value;
  }
  destBufferKey = arg.destBufferKey;
  destBufferOffset = arg.destBufferOffset;
  Copy = true;
}

PointerArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>::~PointerArgument() {
  if (Copy) {
    delete Value;
  }
}

PointerArgument<D3D12_DISPATCH_RAYS_DESC>::PointerArgument(
    const PointerArgument<D3D12_DISPATCH_RAYS_DESC>& arg) {
  if (arg.Value) {
    Value = new D3D12_DISPATCH_RAYS_DESC();
    *Value = *arg.Value;
  }
  RayGenerationShaderRecordKey = arg.RayGenerationShaderRecordKey;
  RayGenerationShaderRecordOffset = arg.RayGenerationShaderRecordOffset;
  MissShaderTableKey = arg.MissShaderTableKey;
  MissShaderTableOffset = arg.MissShaderTableOffset;
  HitGroupTableKey = arg.HitGroupTableKey;
  HitGroupTableOffset = arg.HitGroupTableOffset;
  CallableShaderTableKey = arg.CallableShaderTableKey;
  CallableShaderTableOffset = arg.CallableShaderTableOffset;
  Copy = true;
}

PointerArgument<D3D12_DISPATCH_RAYS_DESC>::~PointerArgument() {
  if (Copy) {
    delete Value;
  }
}

D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument::D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument(
    const D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument& arg) {
  if (!arg.Value) {
    return;
  }
  Size = arg.Size;
  Value = new D3D12_RENDER_PASS_RENDER_TARGET_DESC[Size];
  memcpy(Value, arg.Value, Size * sizeof(D3D12_RENDER_PASS_RENDER_TARGET_DESC));
  for (unsigned i = 0; i < Size; ++i) {
    if (Value[i].EndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE) {
      Value[i].EndingAccess.Resolve.pSubresourceParameters =
          new D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS
              [Value[i].EndingAccess.Resolve.SubresourceCount];
      *const_cast<D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS*>(
          Value[i].EndingAccess.Resolve.pSubresourceParameters) =
          *arg.Value[i].EndingAccess.Resolve.pSubresourceParameters;
    }
  }
  DescriptorKeys = arg.DescriptorKeys;
  DescriptorIndexes = arg.DescriptorIndexes;
  ResolveSrcResourceKeys = arg.ResolveSrcResourceKeys;
  ResolveDstResourceKeys = arg.ResolveDstResourceKeys;
  Copy = true;
}

D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument::~D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument() {
  if (Copy) {
    for (unsigned i = 0; i < Size; ++i) {
      if (Value[i].EndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE) {
        delete Value[i].EndingAccess.Resolve.pSubresourceParameters;
      }
    }
    delete[] Value;
  }
}

D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument::D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument(
    const D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument& arg) {
  if (!arg.Value) {
    return;
  }
  Value = new D3D12_RENDER_PASS_DEPTH_STENCIL_DESC();
  *Value = *arg.Value;
  if (Value->DepthEndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE) {
    Value->DepthEndingAccess.Resolve.pSubresourceParameters =
        new D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS();
    *const_cast<D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS*>(
        Value->DepthEndingAccess.Resolve.pSubresourceParameters) =
        *arg.Value->DepthEndingAccess.Resolve.pSubresourceParameters;
  }
  if (Value->StencilEndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE) {
    Value->StencilEndingAccess.Resolve.pSubresourceParameters =
        new D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS();
    *const_cast<D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS*>(
        Value->StencilEndingAccess.Resolve.pSubresourceParameters) =
        *arg.Value->StencilEndingAccess.Resolve.pSubresourceParameters;
  }
  DescriptorKey = arg.DescriptorKey;
  DescriptorIndex = arg.DescriptorIndex;
  ResolveSrcDepthKey = arg.ResolveSrcDepthKey;
  ResolveDstDepthKey = arg.ResolveDstDepthKey;
  ResolveSrcStencilKey = arg.ResolveSrcStencilKey;
  ResolveDstStencilKey = arg.ResolveDstStencilKey;
  Copy = true;
}

D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument::~D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument() {
  if (Copy) {
    if (Value->DepthEndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE) {
      delete Value->DepthEndingAccess.Resolve.pSubresourceParameters;
    }
    if (Value->StencilEndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE) {
      delete Value->StencilEndingAccess.Resolve.pSubresourceParameters;
    }
    delete Value;
  }
}

PointerArgument<D3D12_COMMAND_SIGNATURE_DESC>::PointerArgument(
    const PointerArgument<D3D12_COMMAND_SIGNATURE_DESC>& arg) {
  if (!arg.Value) {
    return;
  }
  Value = new D3D12_COMMAND_SIGNATURE_DESC();
  *Value = *arg.Value;
  if (Value->pArgumentDescs) {
    Value->pArgumentDescs = new D3D12_INDIRECT_ARGUMENT_DESC[Value->NumArgumentDescs];
    memcpy(const_cast<D3D12_INDIRECT_ARGUMENT_DESC*>(Value->pArgumentDescs),
           arg.Value->pArgumentDescs,
           Value->NumArgumentDescs * sizeof(D3D12_INDIRECT_ARGUMENT_DESC));
  }
  Copy = true;
}

PointerArgument<D3D12_COMMAND_SIGNATURE_DESC>::~PointerArgument() {
  if (Copy) {
    delete[] Value->pArgumentDescs;
    delete Value;
  }
}

ArrayArgument<D3D12_META_COMMAND_DESC>::ArrayArgument(
    const ArrayArgument<D3D12_META_COMMAND_DESC>& arg) {
  if (!arg.Value) {
    return;
  }
  Size = arg.Size;
  Value = new D3D12_META_COMMAND_DESC[Size];
  memcpy(Value, arg.Value, Size * sizeof(D3D12_META_COMMAND_DESC));
  for (unsigned i = 0; i < Size; ++i) {
    if (Value[i].Name) {
      unsigned len = wcslen(Value[i].Name);
      Value->Name = new wchar_t[len + 1];
      memcpy(const_cast<wchar_t*>(Value[i].Name), arg.Value[i].Name, len * 2 + 2);
    }
    Copy = true;
  }
}
ArrayArgument<D3D12_META_COMMAND_DESC>::~ArrayArgument() {
  if (Copy) {
    for (unsigned i = 0; i < Size; ++i) {
      if (Value[i].Name) {
        delete[] Value[i].Name;
      }
    }
    delete[] Value;
  }
}

D3D12_BARRIER_GROUPs_Argument::D3D12_BARRIER_GROUPs_Argument(
    const D3D12_BARRIER_GROUPs_Argument& arg) {
  Size = arg.Size;
  if (arg.Value) {
    Value = new D3D12_BARRIER_GROUP[Size];
    memcpy(Value, arg.Value, Size * sizeof(D3D12_BARRIER_GROUP));

    for (unsigned i = 0; i < Size; ++i) {
      if (Value[i].Type == D3D12_BARRIER_TYPE_GLOBAL) {
        Value[i].pGlobalBarriers = new D3D12_GLOBAL_BARRIER[Value[i].NumBarriers];
        memcpy(const_cast<D3D12_GLOBAL_BARRIER*>(Value[i].pGlobalBarriers),
               arg.Value[i].pGlobalBarriers, sizeof(D3D12_GLOBAL_BARRIER) * Value[i].NumBarriers);
      } else if (Value[i].Type == D3D12_BARRIER_TYPE_TEXTURE) {
        Value[i].pTextureBarriers = new D3D12_TEXTURE_BARRIER[Value[i].NumBarriers];
        memcpy(const_cast<D3D12_TEXTURE_BARRIER*>(Value[i].pTextureBarriers),
               arg.Value[i].pTextureBarriers, sizeof(D3D12_TEXTURE_BARRIER) * Value[i].NumBarriers);
      } else if (Value[i].Type == D3D12_BARRIER_TYPE_BUFFER) {
        Value[i].pBufferBarriers = new D3D12_BUFFER_BARRIER[Value[i].NumBarriers];
        memcpy(const_cast<D3D12_BUFFER_BARRIER*>(Value[i].pBufferBarriers),
               arg.Value[i].pBufferBarriers, sizeof(D3D12_BUFFER_BARRIER) * Value[i].NumBarriers);
      }
    }
  }
  ResourceKeys = arg.ResourceKeys;
  Copy = true;
}

D3D12_BARRIER_GROUPs_Argument::~D3D12_BARRIER_GROUPs_Argument() {
  if (Copy) {
    for (unsigned i = 0; i < Size; ++i) {
      if (Value[i].Type == D3D12_BARRIER_TYPE_GLOBAL) {
        delete[] Value[i].pGlobalBarriers;
      } else if (Value[i].Type == D3D12_BARRIER_TYPE_TEXTURE) {
        delete[] Value[i].pTextureBarriers;
      } else if (Value[i].Type == D3D12_BARRIER_TYPE_BUFFER) {
        delete[] Value[i].pBufferBarriers;
      }
    }

    delete[] Value;
  }
}

ArrayArgument<D3D12_LINEAR_ALGEBRA_MATRIX_CONVERSION_INFO>::ArrayArgument(
    const ArrayArgument<D3D12_LINEAR_ALGEBRA_MATRIX_CONVERSION_INFO>& arg) {
  Size = arg.Size;
  if (arg.Value) {
    Value = new D3D12_LINEAR_ALGEBRA_MATRIX_CONVERSION_INFO[arg.Size];
    memcpy(Value, arg.Value, Size * sizeof(D3D12_LINEAR_ALGEBRA_MATRIX_CONVERSION_INFO));
  }
  DestKey = arg.DestKey;
  DestOffset = arg.DestOffset;
  SourceKey = arg.SourceKey;
  SourceOffset = arg.SourceOffset;
  Copy = true;
}

ArrayArgument<D3D12_LINEAR_ALGEBRA_MATRIX_CONVERSION_INFO>::~ArrayArgument() {
  if (Copy) {
    delete[] Value;
  }
}

#pragma endregion

#pragma region INTC

PointerArgument<INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC>::PointerArgument(
    const PointerArgument<INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC>& arg) {
  if (!arg.Value) {
    return;
  }
  Value = new INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC();
  *Value = *arg.Value;
  Value->pD3D12Desc = new D3D12_COMPUTE_PIPELINE_STATE_DESC();
  *Value->pD3D12Desc = *arg.Value->pD3D12Desc;
  {
    Cs = new uint8_t[Value->CS.BytecodeLength];
    memcpy(const_cast<void*>(Cs), arg.Cs ? arg.Cs : arg.Value->CS.pShaderBytecode,
           Value->CS.BytecodeLength);
    Value->CS.pShaderBytecode = Cs;
  }
  if (arg.Value->CompileOptions) {
    const void* str = arg.CompileOptions ? arg.CompileOptions : arg.Value->CompileOptions;
    unsigned len = strlen(static_cast<const char*>(str));
    CompileOptions = new char[len + 1];
    memcpy(const_cast<void*>(CompileOptions), str, len + 1);
    Value->CompileOptions = const_cast<void*>(CompileOptions);
  }
  if (arg.Value->InternalOptions) {
    const void* str = arg.InternalOptions ? arg.InternalOptions : arg.Value->InternalOptions;
    unsigned len = strlen(static_cast<const char*>(str));
    InternalOptions = new char[len + 1];
    memcpy(const_cast<void*>(InternalOptions), str, len + 1);
    Value->InternalOptions = const_cast<void*>(InternalOptions);
  }
  RootSignatureKey = arg.RootSignatureKey;
  Copy = true;
}

PointerArgument<INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC>::~PointerArgument() {
  if (Copy) {
    delete Value->pD3D12Desc;
    delete[] static_cast<const uint8_t*>(Cs);
    delete[] static_cast<const char*>(CompileOptions);
    delete[] static_cast<const char*>(InternalOptions);
    delete Value;
  }
}

PointerArgument<INTCExtensionAppInfo>::PointerArgument(
    const PointerArgument<INTCExtensionAppInfo>& arg) {
  if (!arg.Value) {
    return;
  }
  Value = new INTCExtensionAppInfo();
  *Value = *arg.Value;
  if (arg.Value->pApplicationName) {
    const auto* str = arg.ApplicationName ? arg.ApplicationName : arg.Value->pApplicationName;
    unsigned len = wcslen(str);
    ApplicationName = new wchar_t[len + 1];
    memcpy(const_cast<wchar_t*>(ApplicationName), str, len * 2 + 2);
    Value->pApplicationName = const_cast<wchar_t*>(ApplicationName);
  }
  if (arg.Value->pEngineName) {
    const auto* str = arg.EngineName ? arg.EngineName : arg.Value->pEngineName;
    unsigned len = wcslen(str);
    EngineName = new wchar_t[len + 1];
    memcpy(const_cast<wchar_t*>(EngineName), str, len * 2 + 2);
    Value->pEngineName = const_cast<wchar_t*>(EngineName);
  }
  Copy = true;
}

PointerArgument<INTCExtensionAppInfo>::~PointerArgument() {
  if (Copy) {
    delete[] ApplicationName;
    delete[] EngineName;
    delete Value;
  }
}

PointerArgument<INTCExtensionAppInfo1>::PointerArgument(
    const PointerArgument<INTCExtensionAppInfo1>& arg) {
  if (!arg.Value) {
    return;
  }
  Value = new INTCExtensionAppInfo1();
  *Value = *arg.Value;
  if (arg.Value->pApplicationName) {
    const auto* str = arg.ApplicationName ? arg.ApplicationName : arg.Value->pApplicationName;
    unsigned len = wcslen(str);
    ApplicationName = new wchar_t[len + 1];
    memcpy(const_cast<wchar_t*>(ApplicationName), str, len * 2 + 2);
    Value->pApplicationName = const_cast<wchar_t*>(ApplicationName);
  }
  if (arg.Value->pEngineName) {
    const auto* str = arg.EngineName ? arg.EngineName : arg.Value->pEngineName;
    unsigned len = wcslen(str);
    EngineName = new wchar_t[len + 1];
    memcpy(const_cast<wchar_t*>(EngineName), str, len * 2 + 2);
    Value->pEngineName = const_cast<wchar_t*>(EngineName);
  }
  Copy = true;
}

PointerArgument<INTCExtensionAppInfo1>::~PointerArgument() {
  if (Copy) {
    delete[] ApplicationName;
    delete[] EngineName;
    delete Value;
  }
}

PointerArgument<INTC_D3D12_HEAP_DESC>::PointerArgument(
    const PointerArgument<INTC_D3D12_HEAP_DESC>& arg) {
  if (!arg.Value) {
    return;
  }
  Value = new INTC_D3D12_HEAP_DESC();
  *Value = *arg.Value;
  Value->pD3D12Desc = new D3D12_HEAP_DESC();
  *Value->pD3D12Desc = *arg.Value->pD3D12Desc;
  Copy = true;
}

PointerArgument<INTC_D3D12_HEAP_DESC>::~PointerArgument() {
  if (Copy) {
    delete Value->pD3D12Desc;
    delete Value;
  }
}

PointerArgument<INTC_D3D12_RESOURCE_DESC_0001>::PointerArgument(
    const PointerArgument<INTC_D3D12_RESOURCE_DESC_0001>& arg) {
  if (!arg.Value) {
    return;
  }
  Value = new INTC_D3D12_RESOURCE_DESC_0001();
  *Value = *arg.Value;
  Value->pD3D12Desc = new D3D12_RESOURCE_DESC();
  *Value->pD3D12Desc = *arg.Value->pD3D12Desc;
  Copy = true;
}

PointerArgument<INTC_D3D12_RESOURCE_DESC_0001>::~PointerArgument() {
  if (Copy) {
    delete Value->pD3D12Desc;
    delete Value;
  }
}

PointerArgument<INTC_D3D12_FEATURE>::PointerArgument(
    const PointerArgument<INTC_D3D12_FEATURE>& arg) {
  if (!arg.Value) {
    return;
  }
  Value = new INTC_D3D12_FEATURE();
  *Value = *arg.Value;
  Copy = true;
}

PointerArgument<INTC_D3D12_FEATURE>::~PointerArgument() {
  if (Copy) {
    delete Value;
  }
}

PointerArgument<INTC_D3D12_RESOURCE_DESC>::PointerArgument(
    const PointerArgument<INTC_D3D12_RESOURCE_DESC>& arg) {
  if (!arg.Value) {
    return;
  }
  Value = new INTC_D3D12_RESOURCE_DESC();
  *Value = *arg.Value;
  Value->pD3D12Desc = new D3D12_RESOURCE_DESC();
  *Value->pD3D12Desc = *arg.Value->pD3D12Desc;
  Copy = true;
}

PointerArgument<INTC_D3D12_RESOURCE_DESC>::~PointerArgument() {
  if (Copy) {
    delete Value->pD3D12Desc;
    delete Value;
  }
}

PointerArgument<INTC_D3D12_COMMAND_QUEUE_DESC_0001>::PointerArgument(
    const PointerArgument<INTC_D3D12_COMMAND_QUEUE_DESC_0001>& arg) {
  if (!arg.Value) {
    return;
  }
  Value = new INTC_D3D12_COMMAND_QUEUE_DESC_0001();
  *Value = *arg.Value;
  Value->pD3D12Desc = new D3D12_COMMAND_QUEUE_DESC();
  *Value->pD3D12Desc = *arg.Value->pD3D12Desc;
  Copy = true;
}

PointerArgument<INTC_D3D12_COMMAND_QUEUE_DESC_0001>::~PointerArgument() {
  if (Copy) {
    delete Value->pD3D12Desc;
    delete Value;
  }
}

PointerArgument<INTCExtensionInfo>::PointerArgument(const PointerArgument<INTCExtensionInfo>& arg) {
  if (!arg.Value) {
    return;
  }
  Value = new INTCExtensionInfo();
  *Value = *arg.Value;
  if (Value->pDeviceDriverDesc) {
    unsigned len = wcslen(Value->pDeviceDriverDesc);
    Value->pDeviceDriverDesc = new wchar_t[len + 1];
    memcpy(const_cast<wchar_t*>(Value->pDeviceDriverDesc), arg.Value->pDeviceDriverDesc,
           len * 2 + 2);
    CopyDeviceDriverDesc = true;
  }
  if (Value->pDeviceDriverVersion) {
    unsigned len = wcslen(Value->pDeviceDriverVersion);
    Value->pDeviceDriverVersion = new wchar_t[len + 1];
    memcpy(const_cast<wchar_t*>(Value->pDeviceDriverVersion), arg.Value->pDeviceDriverVersion,
           len * 2 + 2);
    CopyDeviceDriverVersion = true;
  }
  Copy = true;
}

PointerArgument<INTCExtensionInfo>::~PointerArgument() {
  if (Copy) {
    if (CopyDeviceDriverDesc) {
      delete[] Value->pDeviceDriverDesc;
    }
    if (CopyDeviceDriverVersion) {
      delete[] Value->pDeviceDriverVersion;
    }
    delete Value;
  }
}

ArrayArgument<INTCExtensionVersion>::ArrayArgument(const ArrayArgument<INTCExtensionVersion>& arg) {
  if (!arg.Value) {
    return;
  }
  Size = arg.Size;
  Value = new INTCExtensionVersion[Size];
  memcpy(Value, arg.Value, Size * sizeof(INTCExtensionVersion));
  Copy = true;
}

ArrayArgument<INTCExtensionVersion>::~ArrayArgument() {
  if (Copy) {
    delete[] Value;
  }
}

#pragma endregion

#pragma region DML

DML_BINDING_TABLE_DESC_Argument::DML_BINDING_TABLE_DESC_Argument(
    const DML_BINDING_TABLE_DESC_Argument& arg) {
  if (arg.Value) {
    Value = new DML_BINDING_TABLE_DESC();
    *Value = *arg.Value;
  }
  TableFields = arg.TableFields;
  Copy = true;
}

DML_BINDING_TABLE_DESC_Argument::~DML_BINDING_TABLE_DESC_Argument() {
  if (Copy) {
    delete Value;
  }
}

DML_BINDING_DESC_Argument::DML_BINDING_DESC_Argument(const DML_BINDING_DESC_Argument& arg) {
  if (arg.Value) {
    Value = new DML_BINDING_DESC();
    *Value = *arg.Value;
  }
  ResourceKeysSize = arg.ResourceKeysSize;
  ResourceKeys = arg.ResourceKeys;
  Copy = true;
}

DML_BINDING_DESC_Argument::~DML_BINDING_DESC_Argument() {
  if (Copy) {
    delete Value;
  }
}

DML_BINDING_DESCs_Argument::DML_BINDING_DESCs_Argument(const DML_BINDING_DESCs_Argument& arg) {
  if (arg.Value) {
    Size = arg.Size;
    Value = new DML_BINDING_DESC[Size];
    memcpy(Value, arg.Value, Size * sizeof(DML_BINDING_DESC));
  }
  Copy = true;
}

DML_BINDING_DESCs_Argument::~DML_BINDING_DESCs_Argument() {
  if (Copy) {
    delete[] Value;
  }
}

DML_GRAPH_DESC_Argument::DML_GRAPH_DESC_Argument(const DML_GRAPH_DESC_Argument& arg) {
  if (arg.Value) {
    Value = new DML_GRAPH_DESC();
    *Value = *arg.Value;
  }
  OperatorKeysSize = arg.OperatorKeysSize;
  OperatorKeys = arg.OperatorKeys;
  Copy = true;
}

DML_GRAPH_DESC_Argument::~DML_GRAPH_DESC_Argument() {
  if (Copy) {
    delete Value;
  }
}

DML_OPERATOR_DESC_Argument::DML_OPERATOR_DESC_Argument(const DML_OPERATOR_DESC_Argument& arg) {
  if (arg.Value) {
    Value = new DML_OPERATOR_DESC();
    *Value = *arg.Value;
  }
  Copy = true;
}

DML_OPERATOR_DESC_Argument::~DML_OPERATOR_DESC_Argument() {
  if (Copy) {
    delete Value;
  }
}

DML_CheckFeatureSupport_BufferArgument::DML_CheckFeatureSupport_BufferArgument(
    const DML_CheckFeatureSupport_BufferArgument& arg) {
  if (arg.Value) {
    Size = arg.Size;
    Value = new uint8_t[Size];
    memcpy(Value, arg.Value, Size);
  }
  feature = arg.feature;
  Copy = true;
}

DML_CheckFeatureSupport_BufferArgument::~DML_CheckFeatureSupport_BufferArgument() {
  if (Copy) {
    delete[] static_cast<const uint8_t*>(Value);
  }
}

#pragma endregion

#pragma region DStorage

DSTORAGE_QUEUE_DESC_Argument::DSTORAGE_QUEUE_DESC_Argument(
    const DSTORAGE_QUEUE_DESC_Argument& arg) {
  if (!arg.Value) {
    return;
  }
  Value = new DSTORAGE_QUEUE_DESC();
  *Value = *arg.Value;
  if (Value->Name) {
    unsigned len = strlen(Value->Name);
    Value->Name = new char[len + 1];
    memcpy(const_cast<char*>(Value->Name), arg.Value->Name, len + 1);
  }
  DeviceKey = arg.DeviceKey;
  Copy = true;
}

DSTORAGE_QUEUE_DESC_Argument::~DSTORAGE_QUEUE_DESC_Argument() {
  if (Copy) {
    delete[] Value->Name;
    delete Value;
  }
}

DSTORAGE_REQUEST_Argument::DSTORAGE_REQUEST_Argument(const DSTORAGE_REQUEST_Argument& arg) {
  if (!arg.Value) {
    return;
  }
  Value = new DSTORAGE_REQUEST();
  *Value = *arg.Value;
  if (Value->Name) {
    unsigned len = strlen(Value->Name);
    Value->Name = new char[len + 1];
    memcpy(const_cast<char*>(Value->Name), arg.Value->Name, len + 1);
  }
  FileKey = arg.FileKey;
  ResourceKey = arg.ResourceKey;
  NewOffset = arg.NewOffset;
  Copy = true;
}

DSTORAGE_REQUEST_Argument::~DSTORAGE_REQUEST_Argument() {
  if (Copy) {
    delete[] Value->Name;
    delete Value;
  }
}

#pragma endregion

#pragma region XESS

xess_d3d12_init_params_t_Argument::xess_d3d12_init_params_t_Argument(
    const xess_d3d12_init_params_t_Argument& arg) {
  if (arg.Value) {
    Value = new xess_d3d12_init_params_t();
    *Value = *arg.Value;
  }
  Key = arg.Key;
  TempBufferHeapKey = arg.TempBufferHeapKey;
  TempTextureHeapKey = arg.TempTextureHeapKey;
  PipelineLibraryKey = arg.PipelineLibraryKey;
  Copy = true;
}

xess_d3d12_init_params_t_Argument::~xess_d3d12_init_params_t_Argument() {
  if (Copy) {
    delete Value;
  }
}

xess_d3d12_execute_params_t_Argument::xess_d3d12_execute_params_t_Argument(
    const xess_d3d12_execute_params_t_Argument& arg) {
  if (arg.Value) {
    Value = new xess_d3d12_execute_params_t();
    *Value = *arg.Value;
  }
  ColorTextureKey = arg.ColorTextureKey;
  VelocityTextureKey = arg.VelocityTextureKey;
  DepthTextureKey = arg.DepthTextureKey;
  ExposureScaleTextureKey = arg.ExposureScaleTextureKey;
  ResponsivePixelMaskTextureKey = arg.ResponsivePixelMaskTextureKey;
  OutputTextureKey = arg.OutputTextureKey;
  DescriptorHeapKey = arg.DescriptorHeapKey;
  Copy = true;
}

xess_d3d12_execute_params_t_Argument::~xess_d3d12_execute_params_t_Argument() {
  if (Copy) {
    delete Value;
  }
}

#pragma endregion

#pragma region NVAPI

PointerArgument<NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS>::PointerArgument(
    const PointerArgument<NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS>& arg) {
  if (!arg.Value) {
    return;
  }
  Value = new NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS();
  *Value = *arg.Value;

  Value->pDesc = new NVAPI_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC_EX();
  auto pDescMod =
      const_cast<NVAPI_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC_EX*>(Value->pDesc);
  *pDescMod = *arg.Value->pDesc;

  if (Value->pDesc->inputs.type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
    if (Value->pDesc->inputs.descsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY) {
      if (Value->pDesc->inputs.pGeometryDescs) {
        pDescMod->inputs.pGeometryDescs =
            reinterpret_cast<NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX*>(
                new char[Value->pDesc->inputs.geometryDescStrideInBytes *
                         Value->pDesc->inputs.numDescs]);
        memcpy(const_cast<NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX*>(
                   Value->pDesc->inputs.pGeometryDescs),
               arg.Value->pDesc->inputs.pGeometryDescs,
               Value->pDesc->inputs.geometryDescStrideInBytes * Value->pDesc->inputs.numDescs);

        for (unsigned i = 0; i < Value->pDesc->inputs.numDescs; ++i) {
          NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX& desc =
              *(NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX*)((char*)(Value->pDesc->inputs
                                                                      .pGeometryDescs) +
                                                          Value->pDesc->inputs
                                                                  .geometryDescStrideInBytes *
                                                              i);
          NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX& argDesc =
              *(NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX*)((char*)(arg.Value->pDesc->inputs
                                                                      .pGeometryDescs) +
                                                          arg.Value->pDesc->inputs
                                                                  .geometryDescStrideInBytes *
                                                              i);
          if (desc.type == NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES_EX) {
            desc.ommTriangles.ommAttachment.pOMMUsageCounts =
                new NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_USAGE_COUNT
                    [desc.ommTriangles.ommAttachment.numOMMUsageCounts];
            memcpy(const_cast<NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_USAGE_COUNT*>(
                       desc.ommTriangles.ommAttachment.pOMMUsageCounts),
                   argDesc.ommTriangles.ommAttachment.pOMMUsageCounts,
                   sizeof(NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_USAGE_COUNT) *
                       desc.ommTriangles.ommAttachment.numOMMUsageCounts);
          } else if (desc.type == NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_DMM_TRIANGLES_EX) {
            desc.dmmTriangles.dmmAttachment.pDMMUsageCounts =
                new NVAPI_D3D12_RAYTRACING_DISPLACEMENT_MICROMAP_USAGE_COUNT
                    [desc.dmmTriangles.dmmAttachment.numDMMUsageCounts];
            memcpy(const_cast<NVAPI_D3D12_RAYTRACING_DISPLACEMENT_MICROMAP_USAGE_COUNT*>(
                       desc.dmmTriangles.dmmAttachment.pDMMUsageCounts),
                   argDesc.dmmTriangles.dmmAttachment.pDMMUsageCounts,
                   sizeof(NVAPI_D3D12_RAYTRACING_DISPLACEMENT_MICROMAP_USAGE_COUNT) *
                       desc.dmmTriangles.dmmAttachment.numDMMUsageCounts);
          }
        }
      }
    } else if (Value->pDesc->inputs.descsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS) {
      if (Value->pDesc->inputs.ppGeometryDescs) {
        pDescMod->inputs.ppGeometryDescs =
            new NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX*[Value->pDesc->inputs.numDescs];
        for (unsigned i = 0; i < Value->pDesc->inputs.numDescs; ++i) {
          const_cast<NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX**>(
              Value->pDesc->inputs.ppGeometryDescs)[i] =
              new NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX();

          NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX& desc =
              *const_cast<NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX**>(
                  Value->pDesc->inputs.ppGeometryDescs)[i];
          NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX& argDesc =
              *const_cast<NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX**>(
                  arg.Value->pDesc->inputs.ppGeometryDescs)[i];
          desc = argDesc;

          if (desc.type == NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES_EX) {
            desc.ommTriangles.ommAttachment.pOMMUsageCounts =
                new NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_USAGE_COUNT
                    [desc.ommTriangles.ommAttachment.numOMMUsageCounts];
            memcpy(const_cast<NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_USAGE_COUNT*>(
                       desc.ommTriangles.ommAttachment.pOMMUsageCounts),
                   argDesc.ommTriangles.ommAttachment.pOMMUsageCounts,
                   sizeof(NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_USAGE_COUNT) *
                       desc.ommTriangles.ommAttachment.numOMMUsageCounts);
          } else if (desc.type == NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_DMM_TRIANGLES_EX) {
            desc.dmmTriangles.dmmAttachment.pDMMUsageCounts =
                new NVAPI_D3D12_RAYTRACING_DISPLACEMENT_MICROMAP_USAGE_COUNT
                    [desc.dmmTriangles.dmmAttachment.numDMMUsageCounts];
            memcpy(const_cast<NVAPI_D3D12_RAYTRACING_DISPLACEMENT_MICROMAP_USAGE_COUNT*>(
                       desc.dmmTriangles.dmmAttachment.pDMMUsageCounts),
                   argDesc.dmmTriangles.dmmAttachment.pDMMUsageCounts,
                   sizeof(NVAPI_D3D12_RAYTRACING_DISPLACEMENT_MICROMAP_USAGE_COUNT) *
                       desc.dmmTriangles.dmmAttachment.numDMMUsageCounts);
          }
        }
      }
    }
  }
  DestAccelerationStructureKey = arg.DestAccelerationStructureKey;
  DestAccelerationStructureOffset = arg.DestAccelerationStructureOffset;
  SourceAccelerationStructureKey = arg.SourceAccelerationStructureKey;
  SourceAccelerationStructureOffset = arg.SourceAccelerationStructureOffset;
  ScratchAccelerationStructureKey = arg.ScratchAccelerationStructureKey;
  ScratchAccelerationStructureOffset = arg.ScratchAccelerationStructureOffset;
  InputKeys = arg.InputKeys;
  InputOffsets = arg.InputOffsets;
  DestPostBuildBufferKeys = arg.DestPostBuildBufferKeys;
  DestPostBuildBufferOffsets = arg.DestPostBuildBufferOffsets;

  if (Value->pPostbuildInfoDescs) {
    Value->pPostbuildInfoDescs = new D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC
        [Value->numPostbuildInfoDescs];
    memcpy(const_cast<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC*>(
               Value->pPostbuildInfoDescs),
           arg.Value->pPostbuildInfoDescs,
           sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC) *
               Value->numPostbuildInfoDescs);
  }

  Copy = true;
}

PointerArgument<NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS>::~PointerArgument() {
  if (Copy) {
    if (Value->pDesc->inputs.type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
      if (Value->pDesc->inputs.descsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY) {
        for (unsigned i = 0; i < Value->pDesc->inputs.numDescs; ++i) {
          NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX& desc =
              *(NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX*)((char*)(Value->pDesc->inputs
                                                                      .pGeometryDescs) +
                                                          Value->pDesc->inputs
                                                                  .geometryDescStrideInBytes *
                                                              i);
          if (desc.type == NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES_EX) {
            delete[] desc.ommTriangles.ommAttachment.pOMMUsageCounts;
          } else if (desc.type == NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_DMM_TRIANGLES_EX) {
            delete[] desc.dmmTriangles.dmmAttachment.pDMMUsageCounts;
          }
        }
        delete[] Value->pDesc->inputs.pGeometryDescs;
      } else if (Value->pDesc->inputs.descsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS) {
        for (unsigned i = 0; i < Value->pDesc->inputs.numDescs; ++i) {
          const NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX* desc =
              Value->pDesc->inputs.ppGeometryDescs[i];
          if (desc->type == NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES_EX) {
            delete[] desc->ommTriangles.ommAttachment.pOMMUsageCounts;
          } else if (desc->type == NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_DMM_TRIANGLES_EX) {
            delete[] desc->dmmTriangles.dmmAttachment.pDMMUsageCounts;
          }
          delete Value->pDesc->inputs.ppGeometryDescs[i];
        }
        delete[] Value->pDesc->inputs.ppGeometryDescs;
      }
    }

    if (Value->pPostbuildInfoDescs) {
      delete[] Value->pPostbuildInfoDescs;
    }

    delete Value->pDesc;
    delete Value;
  }
}

PointerArgument<NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS>::PointerArgument(
    const PointerArgument<NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS>& arg) {
  if (!arg.Value) {
    return;
  }
  Value = new NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS();
  *Value = *arg.Value;

  Value->pDesc = new NVAPI_D3D12_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC();
  auto* pDescMod =
      const_cast<NVAPI_D3D12_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC*>(Value->pDesc);
  *pDescMod = *arg.Value->pDesc;

  if (Value->pDesc->inputs.pOMMUsageCounts) {
    pDescMod->inputs.pOMMUsageCounts =
        new NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_USAGE_COUNT[Value->pDesc->inputs
                                                                    .numOMMUsageCounts];
    memcpy(const_cast<NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_USAGE_COUNT*>(
               pDescMod->inputs.pOMMUsageCounts),
           arg.Value->pDesc->inputs.pOMMUsageCounts,
           Value->pDesc->inputs.numOMMUsageCounts *
               sizeof(NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_USAGE_COUNT));
  }

  DestOpacityMicromapArrayDataKey = arg.DestOpacityMicromapArrayDataKey;
  DestOpacityMicromapArrayDataOffset = arg.DestOpacityMicromapArrayDataOffset;
  InputBufferKey = arg.InputBufferKey;
  InputBufferOffset = arg.InputBufferOffset;
  PerOMMDescsKey = arg.PerOMMDescsKey;
  PerOMMDescsOffset = arg.PerOMMDescsOffset;
  ScratchOpacityMicromapArrayDataKey = arg.ScratchOpacityMicromapArrayDataKey;
  ScratchOpacityMicromapArrayDataOffset = arg.ScratchOpacityMicromapArrayDataOffset;
  DestPostBuildBufferKeys = arg.DestPostBuildBufferKeys;
  DestPostBuildBufferOffsets = arg.DestPostBuildBufferOffsets;

  if (Value->pPostbuildInfoDescs) {
    Value->pPostbuildInfoDescs =
        new NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_POSTBUILD_INFO_DESC
            [Value->numPostbuildInfoDescs];
    memcpy(const_cast<NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_POSTBUILD_INFO_DESC*>(
               Value->pPostbuildInfoDescs),
           arg.Value->pPostbuildInfoDescs,
           sizeof(NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_POSTBUILD_INFO_DESC) *
               Value->numPostbuildInfoDescs);
  }

  Copy = true;
}

PointerArgument<NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS>::~PointerArgument() {
  if (Copy) {
    if (Value->pDesc->inputs.pOMMUsageCounts) {
      delete[] Value->pDesc->inputs.pOMMUsageCounts;
    }
    if (Value->pPostbuildInfoDescs) {
      delete[] Value->pPostbuildInfoDescs;
    }
    delete Value->pDesc;
    delete Value;
  }
}

PointerArgument<NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS>::PointerArgument(
    const PointerArgument<NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS>& arg) {
  if (!arg.Value) {
    return;
  }
  Value = new NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS();
  *Value = *arg.Value;

  Value->pDesc = new NVAPI_D3D12_RAYTRACING_MULTI_INDIRECT_CLUSTER_OPERATION_DESC();
  auto* pDescMod =
      const_cast<NVAPI_D3D12_RAYTRACING_MULTI_INDIRECT_CLUSTER_OPERATION_DESC*>(Value->pDesc);
  *pDescMod = *arg.Value->pDesc;

  BatchResultDataKey = arg.BatchResultDataKey;
  BatchResultDataOffset = arg.BatchResultDataOffset;
  BatchScratchDataKey = arg.BatchScratchDataKey;
  BatchScratchDataOffset = arg.BatchScratchDataOffset;
  DestinationAddressArrayKey = arg.DestinationAddressArrayKey;
  DestinationAddressArrayOffset = arg.DestinationAddressArrayOffset;
  ResultSizeArrayKey = arg.ResultSizeArrayKey;
  ResultSizeArrayOffset = arg.ResultSizeArrayOffset;
  IndirectArgArrayKey = arg.IndirectArgArrayKey;
  IndirectArgArrayOffset = arg.IndirectArgArrayOffset;
  IndirectArgCountKey = arg.IndirectArgCountKey;
  IndirectArgCountOffset = arg.IndirectArgCountOffset;

  Copy = true;
}

PointerArgument<
    NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS>::~PointerArgument() {
  if (Copy) {
    delete Value->pDesc;
    delete Value;
  }
}

#pragma endregion

#pragma region XELL

xell_frame_report_t_Argument::xell_frame_report_t_Argument(
    const xell_frame_report_t_Argument& arg) {
  if (!arg.Value) {
    return;
  }
  Value = new xell_frame_report_t[FRAME_REPORTS_COUNT];
  memcpy(Value, arg.Value, FRAME_REPORTS_COUNT * sizeof(xell_frame_report_t));
  Copy = true;
}

xell_frame_report_t_Argument::~xell_frame_report_t_Argument() {
  if (Copy) {
    delete[] Value;
  }
}

#pragma endregion

#pragma region XEFG

xefg_swapchain_d3d12_init_params_t_Argument::xefg_swapchain_d3d12_init_params_t_Argument(
    const xefg_swapchain_d3d12_init_params_t_Argument& arg) {
  if (!arg.Value) {
    return;
  }
  Value = new xefg_swapchain_d3d12_init_params_t();
  *Value = *arg.Value;
  Key = arg.Key;
  ApplicationSwapChainKey = arg.ApplicationSwapChainKey;
  TempBufferHeapKey = arg.TempBufferHeapKey;
  TempTextureHeapKey = arg.TempTextureHeapKey;
  PipelineLibraryKey = arg.PipelineLibraryKey;
  Copy = true;
}

xefg_swapchain_d3d12_init_params_t_Argument::~xefg_swapchain_d3d12_init_params_t_Argument() {
  if (Copy) {
    delete Value;
  }
}

xefg_swapchain_d3d12_resource_data_t_Argument::xefg_swapchain_d3d12_resource_data_t_Argument(
    const xefg_swapchain_d3d12_resource_data_t_Argument& arg) {
  if (!arg.Value) {
    return;
  }
  Value = new xefg_swapchain_d3d12_resource_data_t();
  *Value = *arg.Value;
  ResourceKey = arg.ResourceKey;
  Copy = true;
}

xefg_swapchain_d3d12_resource_data_t_Argument::~xefg_swapchain_d3d12_resource_data_t_Argument() {
  if (Copy) {
    delete Value;
  }
}

#pragma endregion

} // namespace DirectX
} // namespace gits
