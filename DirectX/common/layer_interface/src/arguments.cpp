// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "arguments.h"
#include "intelExtensions.h"
#include <d3dx12/d3dx12_pipeline_state_stream.h>

namespace gits {
namespace DirectX {

#pragma region Generic

BufferArgument::BufferArgument(const BufferArgument& arg) {
  size = arg.size;
  if (arg.value) {
    value = new uint8_t[size];
    memcpy(value, arg.value, size);
  }
  copy = true;
}

BufferArgument::~BufferArgument() {
  if (copy) {
    delete static_cast<uint8_t*>(value);
  }
}

OutputBufferArgument::OutputBufferArgument(const OutputBufferArgument& arg) {
  data = *arg.value;
  value = &data;
  captureValue = arg.captureValue;
}

LPCWSTR_Argument::LPCWSTR_Argument(const LPCWSTR_Argument& arg) {
  if (arg.value) {
    unsigned len = wcslen(arg.value);
    value = new wchar_t[len + 1];
    memcpy(value, arg.value, len * 2 + 2);
  }
  copy = true;
}

LPCWSTR_Argument::~LPCWSTR_Argument() {
  if (copy) {
    delete[] value;
  }
}

LPCSTR_Argument::LPCSTR_Argument(const LPCSTR_Argument& arg) {
  if (arg.value) {
    unsigned len = strlen(arg.value);
    value = new char[len + 1];
    memcpy(value, arg.value, len + 1);
  }
  copy = true;
}

LPCSTR_Argument::~LPCSTR_Argument() {
  if (copy) {
    delete[] value;
  }
}

#pragma endregion

#pragma region D3D12

D3D12_GPU_VIRTUAL_ADDRESSs_Argument::D3D12_GPU_VIRTUAL_ADDRESSs_Argument(
    const D3D12_GPU_VIRTUAL_ADDRESSs_Argument& arg) {
  size = arg.size;
  if (arg.value) {
    value = new D3D12_GPU_VIRTUAL_ADDRESS[size];
    memcpy(value, arg.value, size * sizeof(D3D12_GPU_VIRTUAL_ADDRESS));
  }
  interfaceKeys = arg.interfaceKeys;
  offsets = arg.offsets;
  copy = true;
}

D3D12_GPU_VIRTUAL_ADDRESSs_Argument::~D3D12_GPU_VIRTUAL_ADDRESSs_Argument() {
  if (copy) {
    delete[] value;
  }
}

ShaderIdentifierArgument::ShaderIdentifierArgument(const ShaderIdentifierArgument& arg) {
  if (arg.value) {
    data.resize(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    memcpy(data.data(), arg.value, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    value = data.data();
  }
}

PointerArgument<D3D12_ROOT_SIGNATURE_DESC>::PointerArgument(
    const PointerArgument<D3D12_ROOT_SIGNATURE_DESC>& arg) {
  if (!arg.value) {
    return;
  }
  value = new D3D12_ROOT_SIGNATURE_DESC;
  *value = *arg.value;
  if (value->NumParameters) {
    value->pParameters = new D3D12_ROOT_PARAMETER[value->NumParameters];
    for (unsigned i = 0; i < value->NumParameters; ++i) {
      const_cast<D3D12_ROOT_PARAMETER*>(value->pParameters)[i] = arg.value->pParameters[i];
      if (value->pParameters[i].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
        const_cast<D3D12_ROOT_PARAMETER*>(value->pParameters)[i].DescriptorTable.pDescriptorRanges =
            new D3D12_DESCRIPTOR_RANGE[value->pParameters[i].DescriptorTable.NumDescriptorRanges];
        for (unsigned j = 0; j < value->pParameters[i].DescriptorTable.NumDescriptorRanges; ++j) {
          const_cast<D3D12_DESCRIPTOR_RANGE*>(
              value->pParameters[i].DescriptorTable.pDescriptorRanges)[j] =
              arg.value->pParameters[i].DescriptorTable.pDescriptorRanges[j];
        }
      }
    }
  }
  if (value->NumStaticSamplers) {
    value->pStaticSamplers = new D3D12_STATIC_SAMPLER_DESC[value->NumStaticSamplers];
    for (unsigned i = 0; i < value->NumStaticSamplers; ++i) {
      const_cast<D3D12_STATIC_SAMPLER_DESC*>(value->pStaticSamplers)[i] =
          arg.value->pStaticSamplers[i];
    }
  }
  copy = true;
}

PointerArgument<D3D12_ROOT_SIGNATURE_DESC>::~PointerArgument() {
  if (copy) {
    if (value->NumParameters) {
      for (unsigned i = 0; i < value->NumParameters; ++i) {
        if (value->pParameters[i].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
          delete[] value->pParameters[i].DescriptorTable.pDescriptorRanges;
        }
      }
      delete[] value->pParameters;
    }
    if (value->NumStaticSamplers) {
      delete[] value->pStaticSamplers;
    }
    delete value;
  }
}

PointerArgument<D3D12_VERSIONED_ROOT_SIGNATURE_DESC>::PointerArgument(
    const PointerArgument<D3D12_VERSIONED_ROOT_SIGNATURE_DESC>& arg) {
  if (!arg.value) {
    return;
  }
  value = new D3D12_VERSIONED_ROOT_SIGNATURE_DESC();
  *value = *arg.value;
  switch (value->Version) {
  case D3D_ROOT_SIGNATURE_VERSION_1_0: {
    D3D12_ROOT_SIGNATURE_DESC& dest = value->Desc_1_0;
    D3D12_ROOT_SIGNATURE_DESC& src = arg.value->Desc_1_0;
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
    D3D12_ROOT_SIGNATURE_DESC1& dest = value->Desc_1_1;
    D3D12_ROOT_SIGNATURE_DESC1& src = arg.value->Desc_1_1;
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
    D3D12_ROOT_SIGNATURE_DESC2& dest = value->Desc_1_2;
    D3D12_ROOT_SIGNATURE_DESC2& src = arg.value->Desc_1_2;
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
  copy = true;
}

PointerArgument<D3D12_VERSIONED_ROOT_SIGNATURE_DESC>::~PointerArgument() {
  if (copy) {
    switch (value->Version) {
    case D3D_ROOT_SIGNATURE_VERSION_1_0: {
      D3D12_ROOT_SIGNATURE_DESC& desc = value->Desc_1_0;
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
      D3D12_ROOT_SIGNATURE_DESC1& desc = value->Desc_1_1;
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
      D3D12_ROOT_SIGNATURE_DESC2& desc = value->Desc_1_2;
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
    delete value;
  }
}

D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument::D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument(
    const D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument& arg) {
  rootSignatureKey = arg.rootSignatureKey;
  if (!arg.value) {
    return;
  }
  value = new D3D12_GRAPHICS_PIPELINE_STATE_DESC();
  *value = *arg.value;
  if (value->VS.pShaderBytecode) {
    value->VS.pShaderBytecode = new uint8_t[value->VS.BytecodeLength];
    memcpy(const_cast<void*>(value->VS.pShaderBytecode), arg.value->VS.pShaderBytecode,
           value->VS.BytecodeLength);
  }
  if (value->PS.pShaderBytecode) {
    value->PS.pShaderBytecode = new uint8_t[value->PS.BytecodeLength];
    memcpy(const_cast<void*>(value->PS.pShaderBytecode), arg.value->PS.pShaderBytecode,
           value->PS.BytecodeLength);
  }
  if (value->DS.pShaderBytecode) {
    value->DS.pShaderBytecode = new uint8_t[value->DS.BytecodeLength];
    memcpy(const_cast<void*>(value->DS.pShaderBytecode), arg.value->DS.pShaderBytecode,
           value->DS.BytecodeLength);
  }
  if (value->HS.pShaderBytecode) {
    value->HS.pShaderBytecode = new uint8_t[value->HS.BytecodeLength];
    memcpy(const_cast<void*>(value->HS.pShaderBytecode), arg.value->HS.pShaderBytecode,
           value->HS.BytecodeLength);
  }
  if (value->GS.pShaderBytecode) {
    value->GS.pShaderBytecode = new uint8_t[value->GS.BytecodeLength];
    memcpy(const_cast<void*>(value->GS.pShaderBytecode), arg.value->GS.pShaderBytecode,
           value->GS.BytecodeLength);
  }
  if (value->StreamOutput.pSODeclaration) {
    value->StreamOutput.pSODeclaration =
        new D3D12_SO_DECLARATION_ENTRY[value->StreamOutput.NumEntries];
    for (unsigned i = 0; i < arg.value->StreamOutput.NumEntries; ++i) {
      const_cast<D3D12_SO_DECLARATION_ENTRY*>(value->StreamOutput.pSODeclaration)[i] =
          arg.value->StreamOutput.pSODeclaration[i];
      unsigned len = strlen(value->StreamOutput.pSODeclaration[i].SemanticName);
      const_cast<D3D12_SO_DECLARATION_ENTRY*>(value->StreamOutput.pSODeclaration)[i].SemanticName =
          new char[len + 1];
      memcpy(const_cast<char*>(value->StreamOutput.pSODeclaration[i].SemanticName),
             arg.value->StreamOutput.pSODeclaration[i].SemanticName, len + 1);
    }
  }
  if (value->StreamOutput.pBufferStrides) {
    value->StreamOutput.pBufferStrides = new UINT[value->StreamOutput.NumEntries];
    memcpy(const_cast<UINT*>(value->StreamOutput.pBufferStrides),
           arg.value->StreamOutput.pBufferStrides, value->StreamOutput.NumEntries * sizeof(UINT));
  }
  value->InputLayout.pInputElementDescs =
      new D3D12_INPUT_ELEMENT_DESC[arg.value->InputLayout.NumElements];
  for (unsigned i = 0; i < value->InputLayout.NumElements; ++i) {
    const_cast<D3D12_INPUT_ELEMENT_DESC*>(value->InputLayout.pInputElementDescs)[i] =
        arg.value->InputLayout.pInputElementDescs[i];
    unsigned len = strlen(value->InputLayout.pInputElementDescs[i].SemanticName);
    const_cast<D3D12_INPUT_ELEMENT_DESC*>(value->InputLayout.pInputElementDescs)[i].SemanticName =
        new char[len + 1];
    memcpy(const_cast<char*>(value->InputLayout.pInputElementDescs[i].SemanticName),
           arg.value->InputLayout.pInputElementDescs[i].SemanticName, len + 1);
  }
  if (value->CachedPSO.pCachedBlob) {
    value->CachedPSO.pCachedBlob = new uint8_t[value->CachedPSO.CachedBlobSizeInBytes];
    memcpy(const_cast<void*>(value->CachedPSO.pCachedBlob), arg.value->CachedPSO.pCachedBlob,
           value->CachedPSO.CachedBlobSizeInBytes);
  }
  copy = true;
}

D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument::~D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument() {
  if (copy) {
    delete[] static_cast<const uint8_t*>(value->VS.pShaderBytecode);
    delete[] static_cast<const uint8_t*>(value->PS.pShaderBytecode);
    delete[] static_cast<const uint8_t*>(value->DS.pShaderBytecode);
    delete[] static_cast<const uint8_t*>(value->HS.pShaderBytecode);
    delete[] static_cast<const uint8_t*>(value->GS.pShaderBytecode);
    for (unsigned i = 0; i < value->StreamOutput.NumEntries; ++i) {
      delete[] value->StreamOutput.pSODeclaration[i].SemanticName;
    }
    delete[] value->StreamOutput.pSODeclaration;
    delete[] value->StreamOutput.pBufferStrides;
    for (unsigned i = 0; i < value->InputLayout.NumElements; ++i) {
      delete[] value->InputLayout.pInputElementDescs[i].SemanticName;
    }
    delete[] value->InputLayout.pInputElementDescs;
    delete[] static_cast<const uint8_t*>(value->CachedPSO.pCachedBlob);
    delete value;
  }
}

D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument::D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument(
    const D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument& arg) {
  rootSignatureKey = arg.rootSignatureKey;
  if (!arg.value) {
    return;
  }
  value = new D3D12_COMPUTE_PIPELINE_STATE_DESC();
  *value = *arg.value;
  if (value->CS.pShaderBytecode) {
    value->CS.pShaderBytecode = new uint8_t[value->CS.BytecodeLength];
    memcpy(const_cast<void*>(value->CS.pShaderBytecode), arg.value->CS.pShaderBytecode,
           value->CS.BytecodeLength);
  }
  if (value->CachedPSO.pCachedBlob) {
    value->CachedPSO.pCachedBlob = new uint8_t[value->CachedPSO.CachedBlobSizeInBytes];
    memcpy(const_cast<void*>(value->CachedPSO.pCachedBlob), arg.value->CachedPSO.pCachedBlob,
           value->CachedPSO.CachedBlobSizeInBytes);
  }
  copy = true;
}

D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument::~D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument() {
  if (copy) {
    delete[] static_cast<const uint8_t*>(value->CS.pShaderBytecode);
    delete[] static_cast<const uint8_t*>(value->CachedPSO.pCachedBlob);
    delete value;
  }
}

D3D12_TEXTURE_COPY_LOCATION_Argument::D3D12_TEXTURE_COPY_LOCATION_Argument(
    const D3D12_TEXTURE_COPY_LOCATION_Argument& arg) {
  if (arg.value) {
    value = new D3D12_TEXTURE_COPY_LOCATION();
    *value = *arg.value;
  }
  resourceKey = arg.resourceKey;
  copy = true;
}

D3D12_TEXTURE_COPY_LOCATION_Argument::~D3D12_TEXTURE_COPY_LOCATION_Argument() {
  if (copy) {
    delete value;
  }
}

D3D12_RESOURCE_BARRIERs_Argument::D3D12_RESOURCE_BARRIERs_Argument(
    const D3D12_RESOURCE_BARRIERs_Argument& arg) {
  size = arg.size;
  if (arg.value) {
    value = new D3D12_RESOURCE_BARRIER[size];
    memcpy(value, arg.value, size * sizeof(D3D12_RESOURCE_BARRIER));
  }
  resourceKeys = arg.resourceKeys;
  resourceAfterKeys = arg.resourceAfterKeys;
  copy = true;
}

D3D12_RESOURCE_BARRIERs_Argument::~D3D12_RESOURCE_BARRIERs_Argument() {
  if (copy) {
    delete[] value;
  }
}

D3D12_SHADER_RESOURCE_VIEW_DESC_Argument::D3D12_SHADER_RESOURCE_VIEW_DESC_Argument(
    const D3D12_SHADER_RESOURCE_VIEW_DESC_Argument& arg) {
  if (arg.value) {
    value = new D3D12_SHADER_RESOURCE_VIEW_DESC();
    *value = *arg.value;
  }
  raytracingLocationKey = arg.raytracingLocationKey;
  raytracingLocationOffset = arg.raytracingLocationOffset;
  copy = true;
}

D3D12_SHADER_RESOURCE_VIEW_DESC_Argument::~D3D12_SHADER_RESOURCE_VIEW_DESC_Argument() {
  if (copy) {
    delete value;
  }
}

D3D12_INDEX_BUFFER_VIEW_Argument::D3D12_INDEX_BUFFER_VIEW_Argument(
    const D3D12_INDEX_BUFFER_VIEW_Argument& arg) {
  if (arg.value) {
    value = new D3D12_INDEX_BUFFER_VIEW();
    *value = *arg.value;
  }
  bufferLocationKey = arg.bufferLocationKey;
  bufferLocationOffset = arg.bufferLocationOffset;
  copy = true;
}

D3D12_INDEX_BUFFER_VIEW_Argument::~D3D12_INDEX_BUFFER_VIEW_Argument() {
  if (copy) {
    delete value;
  }
}

D3D12_CONSTANT_BUFFER_VIEW_DESC_Argument::D3D12_CONSTANT_BUFFER_VIEW_DESC_Argument(
    const D3D12_CONSTANT_BUFFER_VIEW_DESC_Argument& arg) {
  if (arg.value) {
    value = new D3D12_CONSTANT_BUFFER_VIEW_DESC();
    *value = *arg.value;
  }
  bufferLocationKey = arg.bufferLocationKey;
  bufferLocationOffset = arg.bufferLocationOffset;
  copy = true;
}

D3D12_CONSTANT_BUFFER_VIEW_DESC_Argument::~D3D12_CONSTANT_BUFFER_VIEW_DESC_Argument() {
  if (copy) {
    delete value;
  }
}

D3D12_VERTEX_BUFFER_VIEWs_Argument::D3D12_VERTEX_BUFFER_VIEWs_Argument(
    const D3D12_VERTEX_BUFFER_VIEWs_Argument& arg) {
  size = arg.size;
  if (arg.value) {
    value = new D3D12_VERTEX_BUFFER_VIEW[size];
    memcpy(value, arg.value, size * sizeof(D3D12_VERTEX_BUFFER_VIEW));
  }
  bufferLocationKeys = arg.bufferLocationKeys;
  bufferLocationOffsets = arg.bufferLocationOffsets;
  copy = true;
}

D3D12_VERTEX_BUFFER_VIEWs_Argument::~D3D12_VERTEX_BUFFER_VIEWs_Argument() {
  if (copy) {
    delete[] value;
  }
}

D3D12_STREAM_OUTPUT_BUFFER_VIEWs_Argument::D3D12_STREAM_OUTPUT_BUFFER_VIEWs_Argument(
    const D3D12_STREAM_OUTPUT_BUFFER_VIEWs_Argument& arg) {
  size = arg.size;
  if (arg.value) {
    value = new D3D12_STREAM_OUTPUT_BUFFER_VIEW[size];
    memcpy(value, arg.value, size * sizeof(D3D12_STREAM_OUTPUT_BUFFER_VIEW));
  }
  bufferLocationKeys = arg.bufferLocationKeys;
  bufferLocationOffsets = arg.bufferLocationOffsets;
  bufferFilledSizeLocationKeys = arg.bufferFilledSizeLocationKeys;
  bufferFilledSizeLocationOffsets = arg.bufferFilledSizeLocationOffsets;
  copy = true;
}

D3D12_STREAM_OUTPUT_BUFFER_VIEWs_Argument::~D3D12_STREAM_OUTPUT_BUFFER_VIEWs_Argument() {
  if (copy) {
    delete[] value;
  }
}

D3D12_WRITEBUFFERIMMEDIATE_PARAMETERs_Argument::D3D12_WRITEBUFFERIMMEDIATE_PARAMETERs_Argument(
    const D3D12_WRITEBUFFERIMMEDIATE_PARAMETERs_Argument& arg) {
  size = arg.size;
  if (arg.value) {
    value = new D3D12_WRITEBUFFERIMMEDIATE_PARAMETER[size];
    memcpy(value, arg.value, size * sizeof(D3D12_WRITEBUFFERIMMEDIATE_PARAMETER));
  }
  destKeys = arg.destKeys;
  destOffsets = arg.destOffsets;
  copy = true;
}

D3D12_WRITEBUFFERIMMEDIATE_PARAMETERs_Argument::~D3D12_WRITEBUFFERIMMEDIATE_PARAMETERs_Argument() {
  if (copy) {
    delete[] value;
  }
}

D3D12_PIPELINE_STATE_STREAM_DESC_Argument::D3D12_PIPELINE_STATE_STREAM_DESC_Argument(
    const D3D12_PIPELINE_STATE_STREAM_DESC_Argument& arg) {
  if (!arg.value) {
    return;
  }
  value = new D3D12_PIPELINE_STATE_STREAM_DESC();
  *value = *arg.value;
  value->pPipelineStateSubobjectStream = new uint8_t[value->SizeInBytes];
  memcpy(value->pPipelineStateSubobjectStream, arg.value->pPipelineStateSubobjectStream,
         value->SizeInBytes);
  size_t offset{};
  while (offset < arg.value->SizeInBytes) {
    void* destData = static_cast<char*>(value->pPipelineStateSubobjectStream) + offset;
    void* srcData = static_cast<char*>(arg.value->pPipelineStateSubobjectStream) + offset;
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE subobjectType =
        *reinterpret_cast<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE*>(srcData);
    switch (subobjectType) {
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE:
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS: {
      D3D12_SHADER_BYTECODE& dest = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_VS*>(destData);
      D3D12_SHADER_BYTECODE& src = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_VS*>(srcData);
      if (src.pShaderBytecode) {
        dest.pShaderBytecode = new uint8_t[src.BytecodeLength];
        memcpy(const_cast<void*>(dest.pShaderBytecode), src.pShaderBytecode, src.BytecodeLength);
      }
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_VS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS: {
      D3D12_SHADER_BYTECODE& dest = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_PS*>(destData);
      D3D12_SHADER_BYTECODE& src = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_PS*>(srcData);
      if (src.pShaderBytecode) {
        dest.pShaderBytecode = new uint8_t[src.BytecodeLength];
        memcpy(const_cast<void*>(dest.pShaderBytecode), src.pShaderBytecode, src.BytecodeLength);
      }
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_PS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DS: {
      D3D12_SHADER_BYTECODE& dest = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_DS*>(destData);
      D3D12_SHADER_BYTECODE& src = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_DS*>(srcData);
      if (src.pShaderBytecode) {
        dest.pShaderBytecode = new uint8_t[src.BytecodeLength];
        memcpy(const_cast<void*>(dest.pShaderBytecode), src.pShaderBytecode, src.BytecodeLength);
      }
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_DS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_HS: {
      D3D12_SHADER_BYTECODE& dest = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_HS*>(destData);
      D3D12_SHADER_BYTECODE& src = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_HS*>(srcData);
      if (src.pShaderBytecode) {
        dest.pShaderBytecode = new uint8_t[src.BytecodeLength];
        memcpy(const_cast<void*>(dest.pShaderBytecode), src.pShaderBytecode, src.BytecodeLength);
      }
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_HS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_GS: {
      D3D12_SHADER_BYTECODE& dest = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_GS*>(destData);
      D3D12_SHADER_BYTECODE& src = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_GS*>(srcData);
      if (src.pShaderBytecode) {
        dest.pShaderBytecode = new uint8_t[src.BytecodeLength];
        memcpy(const_cast<void*>(dest.pShaderBytecode), src.pShaderBytecode, src.BytecodeLength);
      }
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_GS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS: {
      D3D12_SHADER_BYTECODE& dest = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_CS*>(destData);
      D3D12_SHADER_BYTECODE& src = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_CS*>(srcData);
      if (src.pShaderBytecode) {
        dest.pShaderBytecode = new uint8_t[src.BytecodeLength];
        memcpy(const_cast<void*>(dest.pShaderBytecode), src.pShaderBytecode, src.BytecodeLength);
      }
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_CS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS: {
      D3D12_SHADER_BYTECODE& dest = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_AS*>(destData);
      D3D12_SHADER_BYTECODE& src = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_AS*>(srcData);
      if (src.pShaderBytecode) {
        dest.pShaderBytecode = new uint8_t[src.BytecodeLength];
        memcpy(const_cast<void*>(dest.pShaderBytecode), src.pShaderBytecode, src.BytecodeLength);
      }
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_AS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS: {
      D3D12_SHADER_BYTECODE& dest = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_MS*>(destData);
      D3D12_SHADER_BYTECODE& src = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_MS*>(srcData);
      if (src.pShaderBytecode) {
        dest.pShaderBytecode = new uint8_t[src.BytecodeLength];
        memcpy(const_cast<void*>(dest.pShaderBytecode), src.pShaderBytecode, src.BytecodeLength);
      }
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_MS);
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
      D3D12_CACHED_PIPELINE_STATE& dest =
          *static_cast<CD3DX12_PIPELINE_STATE_STREAM_CACHED_PSO*>(destData);
      D3D12_CACHED_PIPELINE_STATE& src =
          *static_cast<CD3DX12_PIPELINE_STATE_STREAM_CACHED_PSO*>(srcData);
      if (src.CachedBlobSizeInBytes) {
        dest.pCachedBlob = new uint8_t[src.CachedBlobSizeInBytes];
        memcpy(const_cast<void*>(dest.pCachedBlob), src.pCachedBlob, src.CachedBlobSizeInBytes);
      }
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_CACHED_PSO);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_FLAGS:
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_FLAGS);
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
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_VIEW_INSTANCING);
    } break;
    }
  }
  rootSignatureKey = arg.rootSignatureKey;
  copy = true;
}

D3D12_PIPELINE_STATE_STREAM_DESC_Argument::~D3D12_PIPELINE_STATE_STREAM_DESC_Argument() {
  if (copy) {
    size_t offset{};
    while (offset < value->SizeInBytes) {
      void* data = static_cast<char*>(value->pPipelineStateSubobjectStream) + offset;
      D3D12_PIPELINE_STATE_SUBOBJECT_TYPE subobjectType =
          *reinterpret_cast<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE*>(data);
      switch (subobjectType) {
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE:
        offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE);
        break;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS: {
        D3D12_SHADER_BYTECODE& desc = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_VS*>(data);
        delete[] static_cast<const uint8_t*>(desc.pShaderBytecode);
        offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_VS);
      } break;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS: {
        D3D12_SHADER_BYTECODE& desc = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_PS*>(data);
        delete[] static_cast<const uint8_t*>(desc.pShaderBytecode);
        offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_PS);
      } break;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DS: {
        D3D12_SHADER_BYTECODE& desc = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_DS*>(data);
        delete[] static_cast<const uint8_t*>(desc.pShaderBytecode);
        offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_DS);
      } break;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_HS: {
        D3D12_SHADER_BYTECODE& desc = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_HS*>(data);
        delete[] static_cast<const uint8_t*>(desc.pShaderBytecode);
        offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_HS);
      } break;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_GS: {
        D3D12_SHADER_BYTECODE& desc = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_GS*>(data);
        delete[] static_cast<const uint8_t*>(desc.pShaderBytecode);
        offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_GS);
      } break;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS: {
        D3D12_SHADER_BYTECODE& desc = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_CS*>(data);
        delete[] static_cast<const uint8_t*>(desc.pShaderBytecode);
        offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_CS);
      } break;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS: {
        D3D12_SHADER_BYTECODE& desc = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_AS*>(data);
        delete[] static_cast<const uint8_t*>(desc.pShaderBytecode);
        offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_AS);
      } break;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS: {
        D3D12_SHADER_BYTECODE& desc = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_MS*>(data);
        delete[] static_cast<const uint8_t*>(desc.pShaderBytecode);
        offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_MS);
      } break;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_STREAM_OUTPUT: {
        D3D12_STREAM_OUTPUT_DESC& desc =
            *static_cast<CD3DX12_PIPELINE_STATE_STREAM_STREAM_OUTPUT*>(data);
        if (desc.pSODeclaration) {
          for (unsigned i = 0; i < desc.NumEntries; ++i) {
            delete[] desc.pSODeclaration[i].SemanticName;
          }
          delete[] desc.pSODeclaration;
        }
        delete[] desc.pBufferStrides;
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
        D3D12_INPUT_LAYOUT_DESC& desc =
            *static_cast<CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT*>(data);
        if (desc.pInputElementDescs) {
          for (unsigned i = 0; i < desc.NumElements; ++i) {
            delete[] desc.pInputElementDescs[i].SemanticName;
          }
          delete[] desc.pInputElementDescs;
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
        D3D12_CACHED_PIPELINE_STATE& desc =
            *static_cast<CD3DX12_PIPELINE_STATE_STREAM_CACHED_PSO*>(data);
        delete[] static_cast<const uint8_t*>(desc.pCachedBlob);
        offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_CACHED_PSO);
      } break;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_FLAGS:
        offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_FLAGS);
        break;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VIEW_INSTANCING: {
        D3D12_VIEW_INSTANCING_DESC& desc =
            *static_cast<CD3DX12_PIPELINE_STATE_STREAM_VIEW_INSTANCING*>(data);
        delete[] desc.pViewInstanceLocations;
        offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_VIEW_INSTANCING);
      } break;
      }
    }
    delete[] static_cast<const uint8_t*>(value->pPipelineStateSubobjectStream);
    delete value;
  }
}

D3D12_STATE_OBJECT_DESC_Argument::D3D12_STATE_OBJECT_DESC_Argument(
    const D3D12_STATE_OBJECT_DESC_Argument& arg) {
  if (!arg.value) {
    return;
  }
  value = new D3D12_STATE_OBJECT_DESC();
  *value = *arg.value;
  value->pSubobjects = new D3D12_STATE_SUBOBJECT[value->NumSubobjects];
  std::map<const D3D12_STATE_SUBOBJECT*, unsigned> srcSubobjectIndexes;
  for (unsigned i = 0; i < value->NumSubobjects; ++i) {
    D3D12_STATE_SUBOBJECT& destSub = const_cast<D3D12_STATE_SUBOBJECT*>(value->pSubobjects)[i];
    const D3D12_STATE_SUBOBJECT& srcSub = arg.value->pSubobjects[i];
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

  for (unsigned index = 0; index < arg.value->NumSubobjects; ++index) {
    if (arg.value->pSubobjects[index].Type ==
        D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION) {
      const D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION* src =
          static_cast<const D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(
              arg.value->pSubobjects[index].pDesc);
      auto it = srcSubobjectIndexes.find(src->pSubobjectToAssociate);
      if (it != srcSubobjectIndexes.end()) {
        unsigned associatedIndex = it->second;
        D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION* dest =
            static_cast<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(
                const_cast<void*>(value->pSubobjects[index].pDesc));
        dest->pSubobjectToAssociate = &value->pSubobjects[associatedIndex];
      }
    }
  }
  interfaceKeysBySubobject = arg.interfaceKeysBySubobject;
  copy = true;
}

D3D12_STATE_OBJECT_DESC_Argument::~D3D12_STATE_OBJECT_DESC_Argument() {
  if (copy) {
    for (unsigned i = 0; i < value->NumSubobjects; ++i) {
      D3D12_STATE_SUBOBJECT& sub = const_cast<D3D12_STATE_SUBOBJECT*>(value->pSubobjects)[i];
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
    delete[] value->pSubobjects;
    delete value;
  }
}

PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS>::PointerArgument(
    const PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS>& arg) {
  if (!arg.value) {
    return;
  }
  value = new D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS();
  *value = *arg.value;
  if (value->Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
    if (value->DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY) {
      if (value->pGeometryDescs) {
        value->pGeometryDescs = new D3D12_RAYTRACING_GEOMETRY_DESC[value->NumDescs];
        memcpy(const_cast<D3D12_RAYTRACING_GEOMETRY_DESC*>(value->pGeometryDescs),
               arg.value->pGeometryDescs, value->NumDescs * sizeof(D3D12_RAYTRACING_GEOMETRY_DESC));
      }
    } else if (value->DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS) {
      if (value->ppGeometryDescs) {
        value->ppGeometryDescs = new D3D12_RAYTRACING_GEOMETRY_DESC*[value->NumDescs];
        for (unsigned i = 0; i < value->NumDescs; ++i) {
          const_cast<D3D12_RAYTRACING_GEOMETRY_DESC**>(value->ppGeometryDescs)[i] =
              new D3D12_RAYTRACING_GEOMETRY_DESC();
          *const_cast<D3D12_RAYTRACING_GEOMETRY_DESC**>(value->ppGeometryDescs)[i] =
              *arg.value->ppGeometryDescs[i];
        }
      }
    }
  }
  inputKeys = arg.inputKeys;
  inputOffsets = arg.inputOffsets;
  copy = true;
}

PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS>::~PointerArgument() {
  if (copy) {
    if (value->Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
      if (value->DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY) {
        delete[] value->pGeometryDescs;
      } else if (value->DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS) {
        for (unsigned i = 0; i < value->NumDescs; ++i) {
          delete value->ppGeometryDescs[i];
        }
        delete[] value->ppGeometryDescs;
      }
    }
    delete value;
  }
}

PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC>::PointerArgument(
    const PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC>& arg) {
  if (!arg.value) {
    return;
  }
  value = new D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC();
  *value = *arg.value;
  if (value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
    if (value->Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY) {
      if (value->Inputs.pGeometryDescs) {
        value->Inputs.pGeometryDescs = new D3D12_RAYTRACING_GEOMETRY_DESC[value->Inputs.NumDescs];
        memcpy(const_cast<D3D12_RAYTRACING_GEOMETRY_DESC*>(value->Inputs.pGeometryDescs),
               arg.value->Inputs.pGeometryDescs,
               value->Inputs.NumDescs * sizeof(D3D12_RAYTRACING_GEOMETRY_DESC));
      }
    } else if (value->Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS) {
      if (value->Inputs.ppGeometryDescs) {
        value->Inputs.ppGeometryDescs = new D3D12_RAYTRACING_GEOMETRY_DESC*[value->Inputs.NumDescs];
        for (unsigned i = 0; i < value->Inputs.NumDescs; ++i) {
          const_cast<D3D12_RAYTRACING_GEOMETRY_DESC**>(value->Inputs.ppGeometryDescs)[i] =
              new D3D12_RAYTRACING_GEOMETRY_DESC();
          *const_cast<D3D12_RAYTRACING_GEOMETRY_DESC**>(value->Inputs.ppGeometryDescs)[i] =
              *arg.value->Inputs.ppGeometryDescs[i];
        }
      }
    }
  }
  destAccelerationStructureKey = arg.destAccelerationStructureKey;
  destAccelerationStructureOffset = arg.destAccelerationStructureOffset;
  sourceAccelerationStructureKey = arg.sourceAccelerationStructureKey;
  sourceAccelerationStructureOffset = arg.sourceAccelerationStructureOffset;
  scratchAccelerationStructureKey = arg.scratchAccelerationStructureKey;
  scratchAccelerationStructureOffset = arg.scratchAccelerationStructureOffset;
  inputKeys = arg.inputKeys;
  inputOffsets = arg.inputOffsets;
  copy = true;
}

PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC>::~PointerArgument() {
  if (copy) {
    if (value->Inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
      if (value->Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY) {
        delete[] value->Inputs.pGeometryDescs;
      } else if (value->Inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS) {
        for (unsigned i = 0; i < value->Inputs.NumDescs; ++i) {
          delete value->Inputs.ppGeometryDescs[i];
        }
        delete[] value->Inputs.ppGeometryDescs;
      }
    }
    delete value;
  }
}

ArrayArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>::ArrayArgument(
    const ArrayArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>& arg) {
  size = arg.size;
  if (arg.value) {
    value = new D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC[size];
    memcpy(value, arg.value,
           size * sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC));
  }
  destBufferKeys = arg.destBufferKeys;
  destBufferOffsets = arg.destBufferOffsets;
  copy = true;
}

ArrayArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>::~ArrayArgument() {
  if (copy) {
    delete[] value;
  }
}

PointerArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>::PointerArgument(
    const PointerArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>& arg) {
  if (arg.value) {
    value = new D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC();
    *value = *arg.value;
  }
  destBufferKey = arg.destBufferKey;
  destBufferOffset = arg.destBufferOffset;
  copy = true;
}

PointerArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>::~PointerArgument() {
  if (copy) {
    delete value;
  }
}

PointerArgument<D3D12_DISPATCH_RAYS_DESC>::PointerArgument(
    const PointerArgument<D3D12_DISPATCH_RAYS_DESC>& arg) {
  if (arg.value) {
    value = new D3D12_DISPATCH_RAYS_DESC();
    *value = *arg.value;
  }
  rayGenerationShaderRecordKey = arg.rayGenerationShaderRecordKey;
  rayGenerationShaderRecordOffset = arg.rayGenerationShaderRecordOffset;
  missShaderTableKey = arg.missShaderTableKey;
  missShaderTableOffset = arg.missShaderTableOffset;
  hitGroupTableKey = arg.hitGroupTableKey;
  hitGroupTableOffset = arg.hitGroupTableOffset;
  callableShaderTableKey = arg.callableShaderTableKey;
  callableShaderTableOffset = arg.callableShaderTableOffset;
  copy = true;
}

PointerArgument<D3D12_DISPATCH_RAYS_DESC>::~PointerArgument() {
  if (copy) {
    delete value;
  }
}

D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument::D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument(
    const D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument& arg) {
  if (!arg.value) {
    return;
  }
  size = arg.size;
  value = new D3D12_RENDER_PASS_RENDER_TARGET_DESC[size];
  memcpy(value, arg.value, size * sizeof(D3D12_RENDER_PASS_RENDER_TARGET_DESC));
  for (unsigned i = 0; i < size; ++i) {
    if (value[i].EndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE) {
      value[i].EndingAccess.Resolve.pSubresourceParameters =
          new D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS
              [value[i].EndingAccess.Resolve.SubresourceCount];
      *const_cast<D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS*>(
          value[i].EndingAccess.Resolve.pSubresourceParameters) =
          *arg.value[i].EndingAccess.Resolve.pSubresourceParameters;
    }
  }
  descriptorKeys = arg.descriptorKeys;
  descriptorIndexes = arg.descriptorIndexes;
  resolveSrcResourceKeys = arg.resolveSrcResourceKeys;
  resolveDstResourceKeys = arg.resolveDstResourceKeys;
  copy = true;
}

D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument::~D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument() {
  if (copy) {
    for (unsigned i = 0; i < size; ++i) {
      if (value[i].EndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE) {
        delete value[i].EndingAccess.Resolve.pSubresourceParameters;
      }
    }
    delete[] value;
  }
}

D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument::D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument(
    const D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument& arg) {
  if (!arg.value) {
    return;
  }
  value = new D3D12_RENDER_PASS_DEPTH_STENCIL_DESC();
  *value = *arg.value;
  if (value->DepthEndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE) {
    value->DepthEndingAccess.Resolve.pSubresourceParameters =
        new D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS();
    *const_cast<D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS*>(
        value->DepthEndingAccess.Resolve.pSubresourceParameters) =
        *arg.value->DepthEndingAccess.Resolve.pSubresourceParameters;
  }
  if (value->StencilEndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE) {
    value->StencilEndingAccess.Resolve.pSubresourceParameters =
        new D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS();
    *const_cast<D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS*>(
        value->StencilEndingAccess.Resolve.pSubresourceParameters) =
        *arg.value->StencilEndingAccess.Resolve.pSubresourceParameters;
  }
  descriptorKey = arg.descriptorKey;
  descriptorIndex = arg.descriptorIndex;
  resolveSrcDepthKey = arg.resolveSrcDepthKey;
  resolveDstDepthKey = arg.resolveDstDepthKey;
  resolveSrcStencilKey = arg.resolveSrcStencilKey;
  resolveDstStencilKey = arg.resolveDstStencilKey;
  copy = true;
}

D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument::~D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument() {
  if (copy) {
    if (value->DepthEndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE) {
      delete value->DepthEndingAccess.Resolve.pSubresourceParameters;
    }
    if (value->StencilEndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE) {
      delete value->StencilEndingAccess.Resolve.pSubresourceParameters;
    }
    delete value;
  }
}

PointerArgument<D3D12_COMMAND_SIGNATURE_DESC>::PointerArgument(
    const PointerArgument<D3D12_COMMAND_SIGNATURE_DESC>& arg) {
  if (!arg.value) {
    return;
  }
  value = new D3D12_COMMAND_SIGNATURE_DESC();
  *value = *arg.value;
  if (value->pArgumentDescs) {
    value->pArgumentDescs = new D3D12_INDIRECT_ARGUMENT_DESC[value->NumArgumentDescs];
    memcpy(const_cast<D3D12_INDIRECT_ARGUMENT_DESC*>(value->pArgumentDescs),
           arg.value->pArgumentDescs,
           value->NumArgumentDescs * sizeof(D3D12_INDIRECT_ARGUMENT_DESC));
  }
  copy = true;
}

PointerArgument<D3D12_COMMAND_SIGNATURE_DESC>::~PointerArgument() {
  if (copy) {
    delete[] value->pArgumentDescs;
    delete value;
  }
}

ArrayArgument<D3D12_META_COMMAND_DESC>::ArrayArgument(
    const ArrayArgument<D3D12_META_COMMAND_DESC>& arg) {
  if (!arg.value) {
    return;
  }
  size = arg.size;
  value = new D3D12_META_COMMAND_DESC[size];
  memcpy(value, arg.value, size * sizeof(D3D12_META_COMMAND_DESC));
  for (unsigned i = 0; i < size; ++i) {
    if (value[i].Name) {
      unsigned len = wcslen(value[i].Name);
      value->Name = new wchar_t[len + 1];
      memcpy(const_cast<wchar_t*>(value[i].Name), arg.value[i].Name, len * 2 + 2);
    }
    copy = true;
  }
}
ArrayArgument<D3D12_META_COMMAND_DESC>::~ArrayArgument() {
  if (copy) {
    for (unsigned i = 0; i < size; ++i) {
      if (value[i].Name) {
        delete[] value[i].Name;
      }
    }
    delete[] value;
  }
}

#pragma endregion

#pragma region INTC

PointerArgument<INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC>::PointerArgument(
    const PointerArgument<INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC>& arg) {
  if (!arg.value) {
    return;
  }
  value = new INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC();
  *value = *arg.value;
  value->pD3D12Desc = new D3D12_COMPUTE_PIPELINE_STATE_DESC();
  *value->pD3D12Desc = *arg.value->pD3D12Desc;
  {
    cs = new uint8_t[value->CS.BytecodeLength];
    memcpy(const_cast<void*>(cs), arg.cs ? arg.cs : arg.value->CS.pShaderBytecode,
           value->CS.BytecodeLength);
    value->CS.pShaderBytecode = cs;
  }
  if (arg.value->CompileOptions) {
    const void* str = arg.compileOptions ? arg.compileOptions : arg.value->CompileOptions;
    unsigned len = strlen(static_cast<const char*>(str));
    compileOptions = new char[len + 1];
    memcpy(const_cast<void*>(compileOptions), str, len + 1);
    value->CompileOptions = const_cast<void*>(compileOptions);
  }
  if (arg.value->InternalOptions) {
    const void* str = arg.internalOptions ? arg.internalOptions : arg.value->InternalOptions;
    unsigned len = strlen(static_cast<const char*>(str));
    internalOptions = new char[len + 1];
    memcpy(const_cast<void*>(internalOptions), str, len + 1);
    value->InternalOptions = const_cast<void*>(internalOptions);
  }
  rootSignatureKey = arg.rootSignatureKey;
  copy = true;
}

PointerArgument<INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC>::~PointerArgument() {
  if (copy) {
    delete value->pD3D12Desc;
    delete[] static_cast<const uint8_t*>(cs);
    delete[] static_cast<const char*>(compileOptions);
    delete[] static_cast<const char*>(internalOptions);
    delete value;
  }
}

PointerArgument<INTCExtensionAppInfo>::PointerArgument(
    const PointerArgument<INTCExtensionAppInfo>& arg) {
  if (!arg.value) {
    return;
  }
  value = new INTCExtensionAppInfo();
  *value = *arg.value;
  if (arg.value->pApplicationName) {
    const auto* str = arg.pApplicationName ? arg.pApplicationName : arg.value->pApplicationName;
    unsigned len = wcslen(str);
    pApplicationName = new wchar_t[len + 1];
    memcpy(const_cast<wchar_t*>(pApplicationName), str, len * 2 + 2);
    value->pApplicationName = const_cast<wchar_t*>(pApplicationName);
  }
  if (arg.value->pEngineName) {
    const auto* str = arg.pEngineName ? arg.pEngineName : arg.value->pEngineName;
    unsigned len = wcslen(str);
    pEngineName = new wchar_t[len + 1];
    memcpy(const_cast<wchar_t*>(pEngineName), str, len * 2 + 2);
    value->pEngineName = const_cast<wchar_t*>(pEngineName);
  }
  copy = true;
}

PointerArgument<INTCExtensionAppInfo>::~PointerArgument() {
  if (copy) {
    delete[] pApplicationName;
    delete[] pEngineName;
    delete value;
  }
}

PointerArgument<INTCExtensionAppInfo1>::PointerArgument(
    const PointerArgument<INTCExtensionAppInfo1>& arg) {
  if (!arg.value) {
    return;
  }
  value = new INTCExtensionAppInfo1();
  *value = *arg.value;
  if (arg.value->pApplicationName) {
    const auto* str = arg.pApplicationName ? arg.pApplicationName : arg.value->pApplicationName;
    unsigned len = wcslen(str);
    pApplicationName = new wchar_t[len + 1];
    memcpy(const_cast<wchar_t*>(pApplicationName), str, len * 2 + 2);
    value->pApplicationName = const_cast<wchar_t*>(pApplicationName);
  }
  if (arg.value->pEngineName) {
    const auto* str = arg.pEngineName ? arg.pEngineName : arg.value->pEngineName;
    unsigned len = wcslen(str);
    pEngineName = new wchar_t[len + 1];
    memcpy(const_cast<wchar_t*>(pEngineName), str, len * 2 + 2);
    value->pEngineName = const_cast<wchar_t*>(pEngineName);
  }
  copy = true;
}

PointerArgument<INTCExtensionAppInfo1>::~PointerArgument() {
  if (copy) {
    delete[] pApplicationName;
    delete[] pEngineName;
    delete value;
  }
}

PointerArgument<INTC_D3D12_HEAP_DESC>::PointerArgument(
    const PointerArgument<INTC_D3D12_HEAP_DESC>& arg) {
  if (!arg.value) {
    return;
  }
  value = new INTC_D3D12_HEAP_DESC();
  *value = *arg.value;
  value->pD3D12Desc = new D3D12_HEAP_DESC();
  *value->pD3D12Desc = *arg.value->pD3D12Desc;
  copy = true;
}

PointerArgument<INTC_D3D12_HEAP_DESC>::~PointerArgument() {
  if (copy) {
    delete value->pD3D12Desc;
    delete value;
  }
}

PointerArgument<INTC_D3D12_RESOURCE_DESC_0001>::PointerArgument(
    const PointerArgument<INTC_D3D12_RESOURCE_DESC_0001>& arg) {
  if (!arg.value) {
    return;
  }
  value = new INTC_D3D12_RESOURCE_DESC_0001();
  *value = *arg.value;
  value->pD3D12Desc = new D3D12_RESOURCE_DESC();
  *value->pD3D12Desc = *arg.value->pD3D12Desc;
  copy = true;
}

PointerArgument<INTC_D3D12_RESOURCE_DESC_0001>::~PointerArgument() {
  if (copy) {
    delete value->pD3D12Desc;
    delete value;
  }
}

PointerArgument<INTC_D3D12_FEATURE>::PointerArgument(
    const PointerArgument<INTC_D3D12_FEATURE>& arg) {
  if (!arg.value) {
    return;
  }
  value = new INTC_D3D12_FEATURE();
  *value = *arg.value;
  copy = true;
}

PointerArgument<INTC_D3D12_FEATURE>::~PointerArgument() {
  if (copy) {
    delete value;
  }
}

PointerArgument<INTC_D3D12_RESOURCE_DESC>::PointerArgument(
    const PointerArgument<INTC_D3D12_RESOURCE_DESC>& arg) {
  if (!arg.value) {
    return;
  }
  value = new INTC_D3D12_RESOURCE_DESC();
  *value = *arg.value;
  value->pD3D12Desc = new D3D12_RESOURCE_DESC();
  *value->pD3D12Desc = *arg.value->pD3D12Desc;
  copy = true;
}

PointerArgument<INTC_D3D12_RESOURCE_DESC>::~PointerArgument() {
  if (copy) {
    delete value->pD3D12Desc;
    delete value;
  }
}

PointerArgument<INTC_D3D12_COMMAND_QUEUE_DESC_0001>::PointerArgument(
    const PointerArgument<INTC_D3D12_COMMAND_QUEUE_DESC_0001>& arg) {
  if (!arg.value) {
    return;
  }
  value = new INTC_D3D12_COMMAND_QUEUE_DESC_0001();
  *value = *arg.value;
  value->pD3D12Desc = new D3D12_COMMAND_QUEUE_DESC();
  *value->pD3D12Desc = *arg.value->pD3D12Desc;
  copy = true;
}

PointerArgument<INTC_D3D12_COMMAND_QUEUE_DESC_0001>::~PointerArgument() {
  if (copy) {
    delete value->pD3D12Desc;
    delete value;
  }
}

PointerArgument<INTCExtensionInfo>::PointerArgument(const PointerArgument<INTCExtensionInfo>& arg) {
  if (!arg.value) {
    return;
  }
  value = new INTCExtensionInfo();
  *value = *arg.value;
  if (value->pDeviceDriverDesc) {
    unsigned len = wcslen(value->pDeviceDriverDesc);
    value->pDeviceDriverDesc = new wchar_t[len + 1];
    memcpy(const_cast<wchar_t*>(value->pDeviceDriverDesc), arg.value->pDeviceDriverDesc,
           len * 2 + 2);
    copyDeviceDriverDesc = true;
  }
  if (value->pDeviceDriverVersion) {
    unsigned len = wcslen(value->pDeviceDriverVersion);
    value->pDeviceDriverVersion = new wchar_t[len + 1];
    memcpy(const_cast<wchar_t*>(value->pDeviceDriverVersion), arg.value->pDeviceDriverVersion,
           len * 2 + 2);
    copyDeviceDriverVersion = true;
  }
  copy = true;
}

PointerArgument<INTCExtensionInfo>::~PointerArgument() {
  if (copy) {
    if (copyDeviceDriverDesc) {
      delete[] value->pDeviceDriverDesc;
    }
    if (copyDeviceDriverVersion) {
      delete[] value->pDeviceDriverVersion;
    }
    delete value;
  }
}

ArrayArgument<INTCExtensionVersion>::ArrayArgument(const ArrayArgument<INTCExtensionVersion>& arg) {
  if (!arg.value) {
    return;
  }
  size = arg.size;
  value = new INTCExtensionVersion[size];
  memcpy(value, arg.value, size * sizeof(INTCExtensionVersion));
  copy = true;
}

ArrayArgument<INTCExtensionVersion>::~ArrayArgument() {
  if (copy) {
    delete[] value;
  }
}

#pragma endregion

#pragma region DML

DML_BINDING_TABLE_DESC_Argument::DML_BINDING_TABLE_DESC_Argument(
    const DML_BINDING_TABLE_DESC_Argument& arg) {
  if (arg.value) {
    value = new DML_BINDING_TABLE_DESC();
    *value = *arg.value;
  }
  data = arg.data;
  copy = true;
}

DML_BINDING_TABLE_DESC_Argument::~DML_BINDING_TABLE_DESC_Argument() {
  if (copy) {
    delete value;
  }
}

DML_BINDING_DESC_Argument::DML_BINDING_DESC_Argument(const DML_BINDING_DESC_Argument& arg) {
  if (arg.value) {
    value = new DML_BINDING_DESC();
    *value = *arg.value;
  }
  resourceKeysSize = arg.resourceKeysSize;
  resourceKeys = arg.resourceKeys;
  copy = true;
}

DML_BINDING_DESC_Argument::~DML_BINDING_DESC_Argument() {
  if (copy) {
    delete value;
  }
}

DML_BINDING_DESCs_Argument::DML_BINDING_DESCs_Argument(const DML_BINDING_DESCs_Argument& arg) {
  if (arg.value) {
    size = arg.size;
    value = new DML_BINDING_DESC[size];
    memcpy(value, arg.value, size * sizeof(DML_BINDING_DESC));
  }
  copy = true;
}

DML_BINDING_DESCs_Argument::~DML_BINDING_DESCs_Argument() {
  if (copy) {
    delete[] value;
  }
}

DML_GRAPH_DESC_Argument::DML_GRAPH_DESC_Argument(const DML_GRAPH_DESC_Argument& arg) {
  if (arg.value) {
    value = new DML_GRAPH_DESC();
    *value = *arg.value;
  }
  operatorKeysSize = arg.operatorKeysSize;
  operatorKeys = arg.operatorKeys;
  copy = true;
}

DML_GRAPH_DESC_Argument::~DML_GRAPH_DESC_Argument() {
  if (copy) {
    delete value;
  }
}

DML_OPERATOR_DESC_Argument::DML_OPERATOR_DESC_Argument(const DML_OPERATOR_DESC_Argument& arg) {
  if (arg.value) {
    value = new DML_OPERATOR_DESC();
    *value = *arg.value;
  }
  copy = true;
}

DML_OPERATOR_DESC_Argument::~DML_OPERATOR_DESC_Argument() {
  if (copy) {
    delete value;
  }
}

DML_CheckFeatureSupport_BufferArgument::DML_CheckFeatureSupport_BufferArgument(
    const DML_CheckFeatureSupport_BufferArgument& arg) {
  if (arg.value) {
    size = arg.size;
    value = new uint8_t[size];
    memcpy(value, arg.value, size);
  }
  feature = arg.feature;
  copy = true;
}

DML_CheckFeatureSupport_BufferArgument::~DML_CheckFeatureSupport_BufferArgument() {
  if (copy) {
    delete[] static_cast<const uint8_t*>(value);
  }
}

#pragma endregion

#pragma region DStorage

DSTORAGE_QUEUE_DESC_Argument::DSTORAGE_QUEUE_DESC_Argument(
    const DSTORAGE_QUEUE_DESC_Argument& arg) {
  if (!arg.value) {
    return;
  }
  value = new DSTORAGE_QUEUE_DESC();
  *value = *arg.value;
  if (value->Name) {
    unsigned len = strlen(value->Name);
    value->Name = new char[len + 1];
    memcpy(const_cast<char*>(value->Name), arg.value->Name, len + 1);
  }
  deviceKey = arg.deviceKey;
  copy = true;
}

DSTORAGE_QUEUE_DESC_Argument::~DSTORAGE_QUEUE_DESC_Argument() {
  if (copy) {
    delete[] value->Name;
    delete value;
  }
}

DSTORAGE_REQUEST_Argument::DSTORAGE_REQUEST_Argument(const DSTORAGE_REQUEST_Argument& arg) {
  if (!arg.value) {
    return;
  }
  value = new DSTORAGE_REQUEST();
  *value = *arg.value;
  if (value->Name) {
    unsigned len = strlen(value->Name);
    value->Name = new char[len + 1];
    memcpy(const_cast<char*>(value->Name), arg.value->Name, len + 1);
  }
  fileKey = arg.fileKey;
  resourceKey = arg.resourceKey;
  newOffset = arg.newOffset;
  copy = true;
}

DSTORAGE_REQUEST_Argument::~DSTORAGE_REQUEST_Argument() {
  if (copy) {
    delete[] value->Name;
    delete value;
  }
}

#pragma endregion

#pragma region XESS

xess_d3d12_init_params_t_Argument::xess_d3d12_init_params_t_Argument(
    const xess_d3d12_init_params_t_Argument& arg) {
  if (arg.value) {
    value = new xess_d3d12_init_params_t();
    *value = *arg.value;
  }
  key = arg.key;
  tempBufferHeapKey = arg.tempBufferHeapKey;
  tempTextureHeapKey = arg.tempTextureHeapKey;
  pipelineLibraryKey = arg.pipelineLibraryKey;
  copy = true;
}

xess_d3d12_init_params_t_Argument::~xess_d3d12_init_params_t_Argument() {
  if (copy) {
    delete value;
  }
}

xess_d3d12_execute_params_t_Argument::xess_d3d12_execute_params_t_Argument(
    const xess_d3d12_execute_params_t_Argument& arg) {
  if (arg.value) {
    value = new xess_d3d12_execute_params_t();
    *value = *arg.value;
  }
  colorTextureKey = arg.colorTextureKey;
  velocityTextureKey = arg.velocityTextureKey;
  depthTextureKey = arg.depthTextureKey;
  exposureScaleTextureKey = arg.exposureScaleTextureKey;
  responsivePixelMaskTextureKey = arg.responsivePixelMaskTextureKey;
  outputTextureKey = arg.outputTextureKey;
  descriptorHeapKey = arg.descriptorHeapKey;
  copy = true;
}

xess_d3d12_execute_params_t_Argument::~xess_d3d12_execute_params_t_Argument() {
  if (copy) {
    delete value;
  }
}

#pragma endregion

#pragma region NVAPI

PointerArgument<NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS>::PointerArgument(
    const PointerArgument<NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS>& arg) {
  if (!arg.value) {
    return;
  }
  value = new NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS();
  *value = *arg.value;

  value->pDesc = new NVAPI_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC_EX();
  auto pDescMod =
      const_cast<NVAPI_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC_EX*>(value->pDesc);
  *pDescMod = *arg.value->pDesc;

  if (value->pDesc->inputs.type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
    if (value->pDesc->inputs.descsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY) {
      if (value->pDesc->inputs.pGeometryDescs) {
        pDescMod->inputs.pGeometryDescs =
            reinterpret_cast<NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX*>(
                new char[value->pDesc->inputs.geometryDescStrideInBytes *
                         value->pDesc->inputs.numDescs]);
        memcpy(const_cast<NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX*>(
                   value->pDesc->inputs.pGeometryDescs),
               arg.value->pDesc->inputs.pGeometryDescs,
               value->pDesc->inputs.geometryDescStrideInBytes * value->pDesc->inputs.numDescs);

        for (unsigned i = 0; i < value->pDesc->inputs.numDescs; ++i) {
          NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX& desc =
              *(NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX*)((char*)(value->pDesc->inputs
                                                                      .pGeometryDescs) +
                                                          value->pDesc->inputs
                                                                  .geometryDescStrideInBytes *
                                                              i);
          NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX& argDesc =
              *(NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX*)((char*)(arg.value->pDesc->inputs
                                                                      .pGeometryDescs) +
                                                          arg.value->pDesc->inputs
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
    } else if (value->pDesc->inputs.descsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS) {
      if (value->pDesc->inputs.ppGeometryDescs) {
        pDescMod->inputs.ppGeometryDescs =
            new NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX*[value->pDesc->inputs.numDescs];
        for (unsigned i = 0; i < value->pDesc->inputs.numDescs; ++i) {
          const_cast<NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX**>(
              value->pDesc->inputs.ppGeometryDescs)[i] =
              new NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX();

          NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX& desc =
              *const_cast<NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX**>(
                  value->pDesc->inputs.ppGeometryDescs)[i];
          NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX& argDesc =
              *const_cast<NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX**>(
                  arg.value->pDesc->inputs.ppGeometryDescs)[i];
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
  destAccelerationStructureKey = arg.destAccelerationStructureKey;
  destAccelerationStructureOffset = arg.destAccelerationStructureOffset;
  sourceAccelerationStructureKey = arg.sourceAccelerationStructureKey;
  sourceAccelerationStructureOffset = arg.sourceAccelerationStructureOffset;
  scratchAccelerationStructureKey = arg.scratchAccelerationStructureKey;
  scratchAccelerationStructureOffset = arg.scratchAccelerationStructureOffset;
  inputKeys = arg.inputKeys;
  inputOffsets = arg.inputOffsets;
  destPostBuildBufferKeys = arg.destPostBuildBufferKeys;
  destPostBuildBufferOffsets = arg.destPostBuildBufferOffsets;

  if (value->pPostbuildInfoDescs) {
    value->pPostbuildInfoDescs = new D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC
        [value->numPostbuildInfoDescs];
    memcpy(const_cast<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC*>(
               value->pPostbuildInfoDescs),
           arg.value->pPostbuildInfoDescs,
           sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC) *
               value->numPostbuildInfoDescs);
  }

  copy = true;
}

PointerArgument<NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS>::~PointerArgument() {
  if (copy) {
    if (value->pDesc->inputs.type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
      if (value->pDesc->inputs.descsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY) {
        for (unsigned i = 0; i < value->pDesc->inputs.numDescs; ++i) {
          NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX& desc =
              *(NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX*)((char*)(value->pDesc->inputs
                                                                      .pGeometryDescs) +
                                                          value->pDesc->inputs
                                                                  .geometryDescStrideInBytes *
                                                              i);
          if (desc.type == NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES_EX) {
            delete[] desc.ommTriangles.ommAttachment.pOMMUsageCounts;
          } else if (desc.type == NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_DMM_TRIANGLES_EX) {
            delete[] desc.dmmTriangles.dmmAttachment.pDMMUsageCounts;
          }
        }
        delete[] value->pDesc->inputs.pGeometryDescs;
      } else if (value->pDesc->inputs.descsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS) {
        for (unsigned i = 0; i < value->pDesc->inputs.numDescs; ++i) {
          const NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX* desc =
              value->pDesc->inputs.ppGeometryDescs[i];
          if (desc->type == NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES_EX) {
            delete[] desc->ommTriangles.ommAttachment.pOMMUsageCounts;
          } else if (desc->type == NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_DMM_TRIANGLES_EX) {
            delete[] desc->dmmTriangles.dmmAttachment.pDMMUsageCounts;
          }
          delete value->pDesc->inputs.ppGeometryDescs[i];
        }
        delete[] value->pDesc->inputs.ppGeometryDescs;
      }
    }

    if (value->pPostbuildInfoDescs) {
      delete[] value->pPostbuildInfoDescs;
    }

    delete value->pDesc;
    delete value;
  }
}

PointerArgument<NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS>::PointerArgument(
    const PointerArgument<NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS>& arg) {
  if (!arg.value) {
    return;
  }
  value = new NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS();
  *value = *arg.value;

  value->pDesc = new NVAPI_D3D12_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC();
  auto* pDescMod =
      const_cast<NVAPI_D3D12_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC*>(value->pDesc);
  *pDescMod = *arg.value->pDesc;

  if (value->pDesc->inputs.pOMMUsageCounts) {
    pDescMod->inputs.pOMMUsageCounts =
        new NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_USAGE_COUNT[value->pDesc->inputs
                                                                    .numOMMUsageCounts];
    memcpy(const_cast<NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_USAGE_COUNT*>(
               pDescMod->inputs.pOMMUsageCounts),
           arg.value->pDesc->inputs.pOMMUsageCounts,
           value->pDesc->inputs.numOMMUsageCounts *
               sizeof(NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_USAGE_COUNT));
  }

  destOpacityMicromapArrayDataKey = arg.destOpacityMicromapArrayDataKey;
  destOpacityMicromapArrayDataOffset = arg.destOpacityMicromapArrayDataOffset;
  inputBufferKey = arg.inputBufferKey;
  inputBufferOffset = arg.inputBufferOffset;
  perOMMDescsKey = arg.perOMMDescsKey;
  perOMMDescsOffset = arg.perOMMDescsOffset;
  scratchOpacityMicromapArrayDataKey = arg.scratchOpacityMicromapArrayDataKey;
  scratchOpacityMicromapArrayDataOffset = arg.scratchOpacityMicromapArrayDataOffset;
  destPostBuildBufferKeys = arg.destPostBuildBufferKeys;
  destPostBuildBufferOffsets = arg.destPostBuildBufferOffsets;

  if (value->pPostbuildInfoDescs) {
    value->pPostbuildInfoDescs =
        new NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_POSTBUILD_INFO_DESC
            [value->numPostbuildInfoDescs];
    memcpy(const_cast<NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_POSTBUILD_INFO_DESC*>(
               value->pPostbuildInfoDescs),
           arg.value->pPostbuildInfoDescs,
           sizeof(NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_POSTBUILD_INFO_DESC) *
               value->numPostbuildInfoDescs);
  }

  copy = true;
}

PointerArgument<NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS>::~PointerArgument() {
  if (copy) {
    if (value->pDesc->inputs.pOMMUsageCounts) {
      delete[] value->pDesc->inputs.pOMMUsageCounts;
    }
    if (value->pPostbuildInfoDescs) {
      delete[] value->pPostbuildInfoDescs;
    }
    delete value->pDesc;
    delete value;
  }
}

PointerArgument<NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS>::PointerArgument(
    const PointerArgument<NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS>& arg) {
  if (!arg.value) {
    return;
  }
  value = new NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS();
  *value = *arg.value;

  value->pDesc = new NVAPI_D3D12_RAYTRACING_MULTI_INDIRECT_CLUSTER_OPERATION_DESC();
  auto* pDescMod =
      const_cast<NVAPI_D3D12_RAYTRACING_MULTI_INDIRECT_CLUSTER_OPERATION_DESC*>(value->pDesc);
  *pDescMod = *arg.value->pDesc;

  batchResultDataKey = arg.batchResultDataKey;
  batchResultDataOffset = arg.batchResultDataOffset;
  batchScratchDataKey = arg.batchScratchDataKey;
  batchScratchDataOffset = arg.batchScratchDataOffset;
  destinationAddressArrayKey = arg.destinationAddressArrayKey;
  destinationAddressArrayOffset = arg.destinationAddressArrayOffset;
  resultSizeArrayKey = arg.resultSizeArrayKey;
  resultSizeArrayOffset = arg.resultSizeArrayOffset;
  indirectArgArrayKey = arg.indirectArgArrayKey;
  indirectArgArrayOffset = arg.indirectArgArrayOffset;
  indirectArgCountKey = arg.indirectArgCountKey;
  indirectArgCountOffset = arg.indirectArgCountOffset;

  copy = true;
}

PointerArgument<
    NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS>::~PointerArgument() {
  if (copy) {
    delete value->pDesc;
    delete value;
  }
}

#pragma endregion

} // namespace DirectX
} // namespace gits
