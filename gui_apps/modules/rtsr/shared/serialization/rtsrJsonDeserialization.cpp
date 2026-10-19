// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "rtsrJsonDeserialization.h"

#include <stdexcept>
#include <string>

namespace gits::rtsr::detail {

namespace {

// A null semantic name is a gap in a stream output declaration, see
// SemanticNameToJson() in rtsrJsonSerialization.cpp.
const char* SemanticNameFromJson(const nlohmann::json& serialized, PipelineStateStorage& storage) {
  if (serialized.is_null()) {
    return nullptr;
  }
  return storage.CopyString(serialized.get_ref<const nlohmann::json::string_t&>());
}

const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* RootSignatureFromJson(
    const nlohmann::json& serializedPipelineStateDesc, PipelineStateStorage& storage) {
  auto* rootSignature = storage.AllocateObject<D3D12_VERSIONED_ROOT_SIGNATURE_DESC>();
  FromJson(serializedPipelineStateDesc.at("RootSignature"), *rootSignature, storage);
  return rootSignature;
}

} // namespace

void FromJson(const nlohmann::json& serialized, D3D12_SHADER_BYTECODE& bytecode) {
  // The document carries the pointer as a number (always 0); whoever recreates
  // the pipeline state attaches the actual bytecode afterwards.
  bytecode.pShaderBytecode =
      reinterpret_cast<const void*>(serialized.at("pShaderBytecode").get<size_t>());
  bytecode.BytecodeLength = serialized.at("BytecodeLength").get<SIZE_T>();
}

void FromJson(const nlohmann::json& serialized,
              D3D12_SO_DECLARATION_ENTRY& entry,
              PipelineStateStorage& storage) {
  entry.Stream = serialized.at("Stream").get<UINT>();
  entry.SemanticName = SemanticNameFromJson(serialized.at("SemanticName"), storage);
  entry.SemanticIndex = serialized.at("SemanticIndex").get<UINT>();
  entry.StartComponent = serialized.at("StartComponent").get<BYTE>();
  entry.ComponentCount = serialized.at("ComponentCount").get<BYTE>();
  entry.OutputSlot = serialized.at("OutputSlot").get<BYTE>();
}

void FromJson(const nlohmann::json& serialized,
              D3D12_STREAM_OUTPUT_DESC& desc,
              PipelineStateStorage& storage) {
  desc.NumEntries = serialized.at("NumEntries").get<UINT>();
  desc.NumStrides = serialized.at("NumStrides").get<UINT>();
  desc.RasterizedStream = serialized.at("RasterizedStream").get<UINT>();

  auto* declarations = storage.AllocateArray<D3D12_SO_DECLARATION_ENTRY>(desc.NumEntries);
  if (desc.NumEntries != 0) {
    const auto& serializedDeclarations = serialized.at("SODeclaration");
    for (UINT i = 0; i < desc.NumEntries; ++i) {
      FromJson(serializedDeclarations.at(i), declarations[i], storage);
    }
  }
  desc.pSODeclaration = declarations;

  auto* strides = storage.AllocateArray<UINT>(desc.NumStrides);
  if (desc.NumStrides != 0) {
    const auto& serializedStrides = serialized.at("BufferStrides");
    for (UINT i = 0; i < desc.NumStrides; ++i) {
      strides[i] = serializedStrides.at(i).get<UINT>();
    }
  }
  desc.pBufferStrides = strides;
}

void FromJson(const nlohmann::json& serialized, D3D12_RENDER_TARGET_BLEND_DESC& desc) {
  desc.BlendEnable = serialized.at("BlendEnable").get<BOOL>();
  desc.LogicOpEnable = serialized.at("LogicOpEnable").get<BOOL>();
  desc.SrcBlend = serialized.at("SrcBlend").get<D3D12_BLEND>();
  desc.DestBlend = serialized.at("DestBlend").get<D3D12_BLEND>();
  desc.BlendOp = serialized.at("BlendOp").get<D3D12_BLEND_OP>();
  desc.SrcBlendAlpha = serialized.at("SrcBlendAlpha").get<D3D12_BLEND>();
  desc.DestBlendAlpha = serialized.at("DestBlendAlpha").get<D3D12_BLEND>();
  desc.BlendOpAlpha = serialized.at("BlendOpAlpha").get<D3D12_BLEND_OP>();
  desc.LogicOp = serialized.at("LogicOp").get<D3D12_LOGIC_OP>();
  desc.RenderTargetWriteMask = serialized.at("RenderTargetWriteMask").get<UINT8>();
}

void FromJson(const nlohmann::json& serialized, D3D12_BLEND_DESC& desc) {
  desc.AlphaToCoverageEnable = serialized.at("AlphaToCoverageEnable").get<BOOL>();
  desc.IndependentBlendEnable = serialized.at("IndependentBlendEnable").get<BOOL>();

  const auto& serializedRenderTargets = serialized.at("RenderTarget");
  for (size_t i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i) {
    FromJson(serializedRenderTargets.at(i), desc.RenderTarget[i]);
  }
}

void FromJson(const nlohmann::json& serialized, D3D12_RASTERIZER_DESC& desc) {
  desc.FillMode = serialized.at("FillMode").get<D3D12_FILL_MODE>();
  desc.CullMode = serialized.at("CullMode").get<D3D12_CULL_MODE>();
  desc.FrontCounterClockwise = serialized.at("FrontCounterClockwise").get<BOOL>();
  desc.DepthBias = serialized.at("DepthBias").get<INT>();
  desc.DepthBiasClamp = serialized.at("DepthBiasClamp").get<FLOAT>();
  desc.SlopeScaledDepthBias = serialized.at("SlopeScaledDepthBias").get<FLOAT>();
  desc.DepthClipEnable = serialized.at("DepthClipEnable").get<BOOL>();
  desc.MultisampleEnable = serialized.at("MultisampleEnable").get<BOOL>();
  desc.AntialiasedLineEnable = serialized.at("AntialiasedLineEnable").get<BOOL>();
  desc.ForcedSampleCount = serialized.at("ForcedSampleCount").get<UINT>();
  desc.ConservativeRaster =
      serialized.at("ConservativeRaster").get<D3D12_CONSERVATIVE_RASTERIZATION_MODE>();
}

void FromJson(const nlohmann::json& serialized, D3D12_DEPTH_STENCILOP_DESC& desc) {
  desc.StencilFailOp = serialized.at("StencilFailOp").get<D3D12_STENCIL_OP>();
  desc.StencilDepthFailOp = serialized.at("StencilDepthFailOp").get<D3D12_STENCIL_OP>();
  desc.StencilPassOp = serialized.at("StencilPassOp").get<D3D12_STENCIL_OP>();
  desc.StencilFunc = serialized.at("StencilFunc").get<D3D12_COMPARISON_FUNC>();
}

void FromJson(const nlohmann::json& serialized, D3D12_DEPTH_STENCIL_DESC& desc) {
  desc.DepthEnable = serialized.at("DepthEnable").get<BOOL>();
  desc.DepthWriteMask = serialized.at("DepthWriteMask").get<D3D12_DEPTH_WRITE_MASK>();
  desc.DepthFunc = serialized.at("DepthFunc").get<D3D12_COMPARISON_FUNC>();
  desc.StencilEnable = serialized.at("StencilEnable").get<BOOL>();
  desc.StencilReadMask = serialized.at("StencilReadMask").get<UINT8>();
  desc.StencilWriteMask = serialized.at("StencilWriteMask").get<UINT8>();
  FromJson(serialized.at("FrontFace"), desc.FrontFace);
  FromJson(serialized.at("BackFace"), desc.BackFace);
}

void FromJson(const nlohmann::json& serialized,
              D3D12_INPUT_ELEMENT_DESC& element,
              PipelineStateStorage& storage) {
  element.SemanticName = SemanticNameFromJson(serialized.at("SemanticName"), storage);
  element.SemanticIndex = serialized.at("SemanticIndex").get<UINT>();
  element.Format = serialized.at("Format").get<DXGI_FORMAT>();
  element.InputSlot = serialized.at("InputSlot").get<UINT>();
  element.AlignedByteOffset = serialized.at("AlignedByteOffset").get<UINT>();
  element.InputSlotClass = serialized.at("InputSlotClass").get<D3D12_INPUT_CLASSIFICATION>();
  element.InstanceDataStepRate = serialized.at("InstanceDataStepRate").get<UINT>();
}

void FromJson(const nlohmann::json& serialized,
              D3D12_INPUT_LAYOUT_DESC& desc,
              PipelineStateStorage& storage) {
  desc.NumElements = serialized.at("NumElements").get<UINT>();

  auto* elements = storage.AllocateArray<D3D12_INPUT_ELEMENT_DESC>(desc.NumElements);
  if (desc.NumElements != 0) {
    const auto& serializedElements = serialized.at("InputElementDescs");
    for (UINT i = 0; i < desc.NumElements; ++i) {
      FromJson(serializedElements.at(i), elements[i], storage);
    }
  }
  desc.pInputElementDescs = elements;
}

void FromJson(const nlohmann::json& serialized, DXGI_SAMPLE_DESC& desc) {
  desc.Count = serialized.at("Count").get<UINT>();
  desc.Quality = serialized.at("Quality").get<UINT>();
}

void FromJson(const nlohmann::json& serialized, D3D12_DESCRIPTOR_RANGE& range) {
  range.RangeType = serialized.at("RangeType").get<D3D12_DESCRIPTOR_RANGE_TYPE>();
  range.NumDescriptors = serialized.at("NumDescriptors").get<UINT>();
  range.BaseShaderRegister = serialized.at("BaseShaderRegister").get<UINT>();
  range.RegisterSpace = serialized.at("RegisterSpace").get<UINT>();
  range.OffsetInDescriptorsFromTableStart =
      serialized.at("OffsetInDescriptorsFromTableStart").get<UINT>();
}

void FromJson(const nlohmann::json& serialized, D3D12_DESCRIPTOR_RANGE1& range) {
  range.RangeType = serialized.at("RangeType").get<D3D12_DESCRIPTOR_RANGE_TYPE>();
  range.NumDescriptors = serialized.at("NumDescriptors").get<UINT>();
  range.BaseShaderRegister = serialized.at("BaseShaderRegister").get<UINT>();
  range.RegisterSpace = serialized.at("RegisterSpace").get<UINT>();
  range.Flags = serialized.at("Flags").get<D3D12_DESCRIPTOR_RANGE_FLAGS>();
  range.OffsetInDescriptorsFromTableStart =
      serialized.at("OffsetInDescriptorsFromTableStart").get<UINT>();
}

void FromJson(const nlohmann::json& serialized,
              D3D12_ROOT_DESCRIPTOR_TABLE& table,
              PipelineStateStorage& storage) {
  table.NumDescriptorRanges = serialized.at("NumDescriptorRanges").get<UINT>();

  auto* ranges = storage.AllocateArray<D3D12_DESCRIPTOR_RANGE>(table.NumDescriptorRanges);
  if (table.NumDescriptorRanges != 0) {
    const auto& serializedRanges = serialized.at("DescriptorRanges");
    for (UINT i = 0; i < table.NumDescriptorRanges; ++i) {
      FromJson(serializedRanges.at(i), ranges[i]);
    }
  }
  table.pDescriptorRanges = ranges;
}

void FromJson(const nlohmann::json& serialized,
              D3D12_ROOT_DESCRIPTOR_TABLE1& table,
              PipelineStateStorage& storage) {
  table.NumDescriptorRanges = serialized.at("NumDescriptorRanges").get<UINT>();

  auto* ranges = storage.AllocateArray<D3D12_DESCRIPTOR_RANGE1>(table.NumDescriptorRanges);
  if (table.NumDescriptorRanges != 0) {
    const auto& serializedRanges = serialized.at("DescriptorRanges");
    for (UINT i = 0; i < table.NumDescriptorRanges; ++i) {
      FromJson(serializedRanges.at(i), ranges[i]);
    }
  }
  table.pDescriptorRanges = ranges;
}

void FromJson(const nlohmann::json& serialized, D3D12_ROOT_CONSTANTS& constants) {
  constants.ShaderRegister = serialized.at("ShaderRegister").get<UINT>();
  constants.RegisterSpace = serialized.at("RegisterSpace").get<UINT>();
  constants.Num32BitValues = serialized.at("Num32BitValues").get<UINT>();
}

void FromJson(const nlohmann::json& serialized, D3D12_ROOT_DESCRIPTOR& descriptor) {
  descriptor.ShaderRegister = serialized.at("ShaderRegister").get<UINT>();
  descriptor.RegisterSpace = serialized.at("RegisterSpace").get<UINT>();
}

void FromJson(const nlohmann::json& serialized, D3D12_ROOT_DESCRIPTOR1& descriptor) {
  descriptor.ShaderRegister = serialized.at("ShaderRegister").get<UINT>();
  descriptor.RegisterSpace = serialized.at("RegisterSpace").get<UINT>();
  descriptor.Flags = serialized.at("Flags").get<D3D12_ROOT_DESCRIPTOR_FLAGS>();
}

void FromJson(const nlohmann::json& serialized,
              D3D12_ROOT_PARAMETER& parameter,
              PipelineStateStorage& storage) {
  parameter.ParameterType = serialized.at("ParameterType").get<D3D12_ROOT_PARAMETER_TYPE>();
  parameter.ShaderVisibility = serialized.at("ShaderVisibility").get<D3D12_SHADER_VISIBILITY>();

  switch (parameter.ParameterType) {
  case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
    FromJson(serialized.at("DescriptorTable"), parameter.DescriptorTable, storage);
    break;
  case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
    FromJson(serialized.at("Constants"), parameter.Constants);
    break;
  case D3D12_ROOT_PARAMETER_TYPE_CBV:
  case D3D12_ROOT_PARAMETER_TYPE_SRV:
  case D3D12_ROOT_PARAMETER_TYPE_UAV:
    FromJson(serialized.at("Descriptor"), parameter.Descriptor);
    break;
  default:
    throw std::invalid_argument("Unknown root parameter type " +
                                std::to_string(static_cast<int>(parameter.ParameterType)));
  }
}

void FromJson(const nlohmann::json& serialized,
              D3D12_ROOT_PARAMETER1& parameter,
              PipelineStateStorage& storage) {
  parameter.ParameterType = serialized.at("ParameterType").get<D3D12_ROOT_PARAMETER_TYPE>();
  parameter.ShaderVisibility = serialized.at("ShaderVisibility").get<D3D12_SHADER_VISIBILITY>();

  switch (parameter.ParameterType) {
  case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
    FromJson(serialized.at("DescriptorTable"), parameter.DescriptorTable, storage);
    break;
  case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
    FromJson(serialized.at("Constants"), parameter.Constants);
    break;
  case D3D12_ROOT_PARAMETER_TYPE_CBV:
  case D3D12_ROOT_PARAMETER_TYPE_SRV:
  case D3D12_ROOT_PARAMETER_TYPE_UAV:
    FromJson(serialized.at("Descriptor"), parameter.Descriptor);
    break;
  default:
    throw std::invalid_argument("Unknown root parameter type " +
                                std::to_string(static_cast<int>(parameter.ParameterType)));
  }
}

void FromJson(const nlohmann::json& serialized, D3D12_STATIC_SAMPLER_DESC& sampler) {
  sampler.Filter = serialized.at("Filter").get<D3D12_FILTER>();
  sampler.AddressU = serialized.at("AddressU").get<D3D12_TEXTURE_ADDRESS_MODE>();
  sampler.AddressV = serialized.at("AddressV").get<D3D12_TEXTURE_ADDRESS_MODE>();
  sampler.AddressW = serialized.at("AddressW").get<D3D12_TEXTURE_ADDRESS_MODE>();
  sampler.MipLODBias = serialized.at("MipLODBias").get<FLOAT>();
  sampler.MaxAnisotropy = serialized.at("MaxAnisotropy").get<UINT>();
  sampler.ComparisonFunc = serialized.at("ComparisonFunc").get<D3D12_COMPARISON_FUNC>();
  sampler.BorderColor = serialized.at("BorderColor").get<D3D12_STATIC_BORDER_COLOR>();
  sampler.MinLOD = serialized.at("MinLOD").get<FLOAT>();
  sampler.MaxLOD = serialized.at("MaxLOD").get<FLOAT>();
  sampler.ShaderRegister = serialized.at("ShaderRegister").get<UINT>();
  sampler.RegisterSpace = serialized.at("RegisterSpace").get<UINT>();
  sampler.ShaderVisibility = serialized.at("ShaderVisibility").get<D3D12_SHADER_VISIBILITY>();
}

void FromJson(const nlohmann::json& serialized, D3D12_STATIC_SAMPLER_DESC1& sampler) {
  sampler.Filter = serialized.at("Filter").get<D3D12_FILTER>();
  sampler.AddressU = serialized.at("AddressU").get<D3D12_TEXTURE_ADDRESS_MODE>();
  sampler.AddressV = serialized.at("AddressV").get<D3D12_TEXTURE_ADDRESS_MODE>();
  sampler.AddressW = serialized.at("AddressW").get<D3D12_TEXTURE_ADDRESS_MODE>();
  sampler.MipLODBias = serialized.at("MipLODBias").get<FLOAT>();
  sampler.MaxAnisotropy = serialized.at("MaxAnisotropy").get<UINT>();
  sampler.ComparisonFunc = serialized.at("ComparisonFunc").get<D3D12_COMPARISON_FUNC>();
  sampler.BorderColor = serialized.at("BorderColor").get<D3D12_STATIC_BORDER_COLOR>();
  sampler.MinLOD = serialized.at("MinLOD").get<FLOAT>();
  sampler.MaxLOD = serialized.at("MaxLOD").get<FLOAT>();
  sampler.ShaderRegister = serialized.at("ShaderRegister").get<UINT>();
  sampler.RegisterSpace = serialized.at("RegisterSpace").get<UINT>();
  sampler.ShaderVisibility = serialized.at("ShaderVisibility").get<D3D12_SHADER_VISIBILITY>();
  sampler.Flags = serialized.at("Flags").get<D3D12_SAMPLER_FLAGS>();
}

void FromJson(const nlohmann::json& serialized,
              D3D12_ROOT_SIGNATURE_DESC& desc,
              PipelineStateStorage& storage) {
  desc.NumParameters = serialized.at("NumParameters").get<UINT>();

  auto* parameters = storage.AllocateArray<D3D12_ROOT_PARAMETER>(desc.NumParameters);
  if (desc.NumParameters != 0) {
    const auto& serializedParameters = serialized.at("Parameters");
    for (UINT i = 0; i < desc.NumParameters; ++i) {
      FromJson(serializedParameters.at(i), parameters[i], storage);
    }
  }
  desc.pParameters = parameters;

  desc.NumStaticSamplers = serialized.at("NumStaticSamplers").get<UINT>();

  auto* staticSamplers = storage.AllocateArray<D3D12_STATIC_SAMPLER_DESC>(desc.NumStaticSamplers);
  if (desc.NumStaticSamplers != 0) {
    const auto& serializedSamplers = serialized.at("StaticSamplers");
    for (UINT i = 0; i < desc.NumStaticSamplers; ++i) {
      FromJson(serializedSamplers.at(i), staticSamplers[i]);
    }
  }
  desc.pStaticSamplers = staticSamplers;

  desc.Flags = serialized.at("Flags").get<D3D12_ROOT_SIGNATURE_FLAGS>();
}

void FromJson(const nlohmann::json& serialized,
              D3D12_ROOT_SIGNATURE_DESC1& desc,
              PipelineStateStorage& storage) {
  desc.NumParameters = serialized.at("NumParameters").get<UINT>();

  auto* parameters = storage.AllocateArray<D3D12_ROOT_PARAMETER1>(desc.NumParameters);
  if (desc.NumParameters != 0) {
    const auto& serializedParameters = serialized.at("Parameters");
    for (UINT i = 0; i < desc.NumParameters; ++i) {
      FromJson(serializedParameters.at(i), parameters[i], storage);
    }
  }
  desc.pParameters = parameters;

  desc.NumStaticSamplers = serialized.at("NumStaticSamplers").get<UINT>();

  auto* staticSamplers = storage.AllocateArray<D3D12_STATIC_SAMPLER_DESC>(desc.NumStaticSamplers);
  if (desc.NumStaticSamplers != 0) {
    const auto& serializedSamplers = serialized.at("StaticSamplers");
    for (UINT i = 0; i < desc.NumStaticSamplers; ++i) {
      FromJson(serializedSamplers.at(i), staticSamplers[i]);
    }
  }
  desc.pStaticSamplers = staticSamplers;

  desc.Flags = serialized.at("Flags").get<D3D12_ROOT_SIGNATURE_FLAGS>();
}

void FromJson(const nlohmann::json& serialized,
              D3D12_ROOT_SIGNATURE_DESC2& desc,
              PipelineStateStorage& storage) {
  desc.NumParameters = serialized.at("NumParameters").get<UINT>();

  auto* parameters = storage.AllocateArray<D3D12_ROOT_PARAMETER1>(desc.NumParameters);
  if (desc.NumParameters != 0) {
    const auto& serializedParameters = serialized.at("Parameters");
    for (UINT i = 0; i < desc.NumParameters; ++i) {
      FromJson(serializedParameters.at(i), parameters[i], storage);
    }
  }
  desc.pParameters = parameters;

  desc.NumStaticSamplers = serialized.at("NumStaticSamplers").get<UINT>();

  auto* staticSamplers = storage.AllocateArray<D3D12_STATIC_SAMPLER_DESC1>(desc.NumStaticSamplers);
  if (desc.NumStaticSamplers != 0) {
    const auto& serializedSamplers = serialized.at("StaticSamplers");
    for (UINT i = 0; i < desc.NumStaticSamplers; ++i) {
      FromJson(serializedSamplers.at(i), staticSamplers[i]);
    }
  }
  desc.pStaticSamplers = staticSamplers;

  desc.Flags = serialized.at("Flags").get<D3D12_ROOT_SIGNATURE_FLAGS>();
}

void FromJson(const nlohmann::json& serialized,
              D3D12_VERSIONED_ROOT_SIGNATURE_DESC& desc,
              PipelineStateStorage& storage) {
  desc.Version = serialized.at("Version").get<D3D_ROOT_SIGNATURE_VERSION>();

  switch (desc.Version) {
  case D3D_ROOT_SIGNATURE_VERSION_1_0:
    FromJson(serialized.at("Desc_1_0"), desc.Desc_1_0, storage);
    break;
  case D3D_ROOT_SIGNATURE_VERSION_1_1:
    FromJson(serialized.at("Desc_1_1"), desc.Desc_1_1, storage);
    break;
  case D3D_ROOT_SIGNATURE_VERSION_1_2:
    FromJson(serialized.at("Desc_1_2"), desc.Desc_1_2, storage);
    break;
  default:
    throw std::invalid_argument("Unknown root signature version " +
                                std::to_string(static_cast<int>(desc.Version)));
  }
}

void FromJson(const nlohmann::json& serialized, D3D12_CACHED_PIPELINE_STATE& desc) {
  desc.pCachedBlob = reinterpret_cast<const void*>(serialized.at("pCachedBlob").get<size_t>());
  desc.CachedBlobSizeInBytes = serialized.at("CachedBlobSizeInBytes").get<SIZE_T>();
}

void FromJson(const nlohmann::json& serialized,
              D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc,
              PipelineStateStorage& storage) {
  FromJson(serialized.at("VS"), desc.VS);
  FromJson(serialized.at("PS"), desc.PS);
  FromJson(serialized.at("DS"), desc.DS);
  FromJson(serialized.at("HS"), desc.HS);
  FromJson(serialized.at("GS"), desc.GS);
  FromJson(serialized.at("StreamOutput"), desc.StreamOutput, storage);
  FromJson(serialized.at("BlendState"), desc.BlendState);
  desc.SampleMask = serialized.at("SampleMask").get<UINT>();
  FromJson(serialized.at("RasterizerState"), desc.RasterizerState);
  FromJson(serialized.at("DepthStencilState"), desc.DepthStencilState);
  FromJson(serialized.at("InputLayout"), desc.InputLayout, storage);
  desc.IBStripCutValue = serialized.at("IBStripCutValue").get<D3D12_INDEX_BUFFER_STRIP_CUT_VALUE>();
  desc.PrimitiveTopologyType =
      serialized.at("PrimitiveTopologyType").get<D3D12_PRIMITIVE_TOPOLOGY_TYPE>();
  desc.NumRenderTargets = serialized.at("NumRenderTargets").get<UINT>();

  const auto& serializedFormats = serialized.at("RTVFormats");
  for (size_t i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i) {
    desc.RTVFormats[i] = serializedFormats.at(i).get<DXGI_FORMAT>();
  }

  desc.DSVFormat = serialized.at("DSVFormat").get<DXGI_FORMAT>();
  FromJson(serialized.at("SampleDesc"), desc.SampleDesc);
  desc.NodeMask = serialized.at("NodeMask").get<UINT>();
  FromJson(serialized.at("CachedPSO"), desc.CachedPSO);
  desc.Flags = serialized.at("Flags").get<D3D12_PIPELINE_STATE_FLAGS>();
}

void FromJson(const nlohmann::json& serialized, D3D12_COMPUTE_PIPELINE_STATE_DESC& desc) {
  FromJson(serialized.at("CS"), desc.CS);
  desc.NodeMask = serialized.at("NodeMask").get<UINT>();
  FromJson(serialized.at("CachedPSO"), desc.CachedPSO);
  desc.Flags = serialized.at("Flags").get<D3D12_PIPELINE_STATE_FLAGS>();
}

void FromJson(const nlohmann::json& serialized,
              PipelineStateInfo& info,
              PipelineStateStorage& storage) {
  info.Type = serialized.at("Type").get<PipelineStateType>();

  switch (info.Type) {
  case PipelineStateType::Graphics: {
    const auto& serializedDesc = serialized.at("GraphicsPipelineStateDesc");
    auto* desc = storage.AllocateObject<D3D12_GRAPHICS_PIPELINE_STATE_DESC>();
    FromJson(serializedDesc, *desc, storage);
    info.pGraphicsPipelineStateDesc = desc;
    info.pRootSignatureDesc = RootSignatureFromJson(serializedDesc, storage);
    break;
  }
  case PipelineStateType::Compute: {
    const auto& serializedDesc = serialized.at("ComputePipelineStateDesc");
    auto* desc = storage.AllocateObject<D3D12_COMPUTE_PIPELINE_STATE_DESC>();
    FromJson(serializedDesc, *desc);
    info.pComputePipelineStateDesc = desc;
    info.pRootSignatureDesc = RootSignatureFromJson(serializedDesc, storage);
    break;
  }
  default:
    throw std::invalid_argument("Pipeline state type " +
                                std::to_string(static_cast<int>(info.Type)) +
                                " cannot be deserialized");
  }
}

} // namespace gits::rtsr::detail
