// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "rtsrJsonSerialization.h"

#include <stdexcept>
#include <string>
#include <utility>

namespace gits::rtsr::detail {

namespace {

// RTSR dereferences the array pointers of a descriptor without checking them.
// An inconsistent descriptor is reported instead of crashing the application
// this code is linked into.
void RequireArray(const void* elements, size_t count, const char* name) {
  if (count != 0 && elements == nullptr) {
    throw std::invalid_argument(std::string(name) + " is null while its element count is " +
                                std::to_string(count));
  }
}

// A null semantic name marks a gap in the stream output declaration. RTSR
// builds a std::string from the pointer unconditionally, which crashes on such
// gaps, so they are written as a JSON null instead.
nlohmann::json SemanticNameToJson(const char* name) {
  if (name == nullptr) {
    return nlohmann::json();
  }
  return nlohmann::json(name);
}

} // namespace

nlohmann::json ToJson(const D3D12_SHADER_BYTECODE& bytecode) {
  // The bytecode itself is not part of the document, the pointer is written as
  // a literal 0 and only the length is kept.
  return {
      {"pShaderBytecode", 0},
      {"BytecodeLength", bytecode.BytecodeLength},
  };
}

nlohmann::json ToJson(const D3D12_SO_DECLARATION_ENTRY& entry) {
  return {
      {"Stream", entry.Stream},
      {"SemanticName", SemanticNameToJson(entry.SemanticName)},
      {"SemanticIndex", entry.SemanticIndex},
      {"StartComponent", entry.StartComponent},
      {"ComponentCount", entry.ComponentCount},
      {"OutputSlot", entry.OutputSlot},
  };
}

nlohmann::json ToJson(const D3D12_STREAM_OUTPUT_DESC& desc) {
  RequireArray(desc.pSODeclaration, desc.NumEntries, "D3D12_STREAM_OUTPUT_DESC::pSODeclaration");
  RequireArray(desc.pBufferStrides, desc.NumStrides, "D3D12_STREAM_OUTPUT_DESC::pBufferStrides");

  nlohmann::json declarations = nlohmann::json::array();
  for (UINT i = 0; i < desc.NumEntries; ++i) {
    declarations.push_back(ToJson(desc.pSODeclaration[i]));
  }

  nlohmann::json strides = nlohmann::json::array();
  for (UINT i = 0; i < desc.NumStrides; ++i) {
    strides.push_back(desc.pBufferStrides[i]);
  }

  return {
      {"SODeclaration", std::move(declarations)},  {"NumEntries", desc.NumEntries},
      {"BufferStrides", std::move(strides)},       {"NumStrides", desc.NumStrides},
      {"RasterizedStream", desc.RasterizedStream},
  };
}

nlohmann::json ToJson(const D3D12_RENDER_TARGET_BLEND_DESC& desc) {
  return {
      {"BlendEnable", desc.BlendEnable},
      {"LogicOpEnable", desc.LogicOpEnable},
      {"SrcBlend", desc.SrcBlend},
      {"DestBlend", desc.DestBlend},
      {"BlendOp", desc.BlendOp},
      {"SrcBlendAlpha", desc.SrcBlendAlpha},
      {"DestBlendAlpha", desc.DestBlendAlpha},
      {"BlendOpAlpha", desc.BlendOpAlpha},
      {"LogicOp", desc.LogicOp},
      {"RenderTargetWriteMask", desc.RenderTargetWriteMask},
  };
}

nlohmann::json ToJson(const D3D12_BLEND_DESC& desc) {
  nlohmann::json renderTargets = nlohmann::json::array();
  for (const auto& renderTarget : desc.RenderTarget) {
    renderTargets.push_back(ToJson(renderTarget));
  }

  return {
      {"AlphaToCoverageEnable", desc.AlphaToCoverageEnable},
      {"IndependentBlendEnable", desc.IndependentBlendEnable},
      {"RenderTarget", std::move(renderTargets)},
  };
}

nlohmann::json ToJson(const D3D12_RASTERIZER_DESC& desc) {
  return {
      {"FillMode", desc.FillMode},
      {"CullMode", desc.CullMode},
      {"FrontCounterClockwise", desc.FrontCounterClockwise},
      {"DepthBias", desc.DepthBias},
      {"DepthBiasClamp", desc.DepthBiasClamp},
      {"SlopeScaledDepthBias", desc.SlopeScaledDepthBias},
      {"DepthClipEnable", desc.DepthClipEnable},
      {"MultisampleEnable", desc.MultisampleEnable},
      {"AntialiasedLineEnable", desc.AntialiasedLineEnable},
      {"ForcedSampleCount", desc.ForcedSampleCount},
      {"ConservativeRaster", desc.ConservativeRaster},
  };
}

nlohmann::json ToJson(const D3D12_DEPTH_STENCILOP_DESC& desc) {
  return {
      {"StencilFailOp", desc.StencilFailOp},
      {"StencilDepthFailOp", desc.StencilDepthFailOp},
      {"StencilPassOp", desc.StencilPassOp},
      {"StencilFunc", desc.StencilFunc},
  };
}

nlohmann::json ToJson(const D3D12_DEPTH_STENCIL_DESC& desc) {
  return {
      {"DepthEnable", desc.DepthEnable},
      {"DepthWriteMask", desc.DepthWriteMask},
      {"DepthFunc", desc.DepthFunc},
      {"StencilEnable", desc.StencilEnable},
      {"StencilReadMask", desc.StencilReadMask},
      {"StencilWriteMask", desc.StencilWriteMask},
      {"FrontFace", ToJson(desc.FrontFace)},
      {"BackFace", ToJson(desc.BackFace)},
  };
}

nlohmann::json ToJson(const D3D12_INPUT_ELEMENT_DESC& element) {
  return {
      {"SemanticName", SemanticNameToJson(element.SemanticName)},
      {"SemanticIndex", element.SemanticIndex},
      {"Format", element.Format},
      {"InputSlot", element.InputSlot},
      {"AlignedByteOffset", element.AlignedByteOffset},
      {"InputSlotClass", element.InputSlotClass},
      {"InstanceDataStepRate", element.InstanceDataStepRate},
  };
}

nlohmann::json ToJson(const D3D12_INPUT_LAYOUT_DESC& desc) {
  RequireArray(desc.pInputElementDescs, desc.NumElements,
               "D3D12_INPUT_LAYOUT_DESC::pInputElementDescs");

  nlohmann::json elements = nlohmann::json::array();
  for (UINT i = 0; i < desc.NumElements; ++i) {
    elements.push_back(ToJson(desc.pInputElementDescs[i]));
  }

  return {
      {"InputElementDescs", std::move(elements)},
      {"NumElements", desc.NumElements},
  };
}

nlohmann::json ToJson(const DXGI_SAMPLE_DESC& desc) {
  return {
      {"Count", desc.Count},
      {"Quality", desc.Quality},
  };
}

nlohmann::json ToJson(const D3D12_DESCRIPTOR_RANGE& range) {
  return {
      {"RangeType", range.RangeType},
      {"NumDescriptors", range.NumDescriptors},
      {"BaseShaderRegister", range.BaseShaderRegister},
      {"RegisterSpace", range.RegisterSpace},
      {"OffsetInDescriptorsFromTableStart", range.OffsetInDescriptorsFromTableStart},
  };
}

nlohmann::json ToJson(const D3D12_DESCRIPTOR_RANGE1& range) {
  return {
      {"RangeType", range.RangeType},
      {"NumDescriptors", range.NumDescriptors},
      {"BaseShaderRegister", range.BaseShaderRegister},
      {"RegisterSpace", range.RegisterSpace},
      {"Flags", range.Flags},
      {"OffsetInDescriptorsFromTableStart", range.OffsetInDescriptorsFromTableStart},
  };
}

nlohmann::json ToJson(const D3D12_ROOT_DESCRIPTOR_TABLE& table) {
  RequireArray(table.pDescriptorRanges, table.NumDescriptorRanges,
               "D3D12_ROOT_DESCRIPTOR_TABLE::pDescriptorRanges");

  nlohmann::json ranges = nlohmann::json::array();
  for (UINT i = 0; i < table.NumDescriptorRanges; ++i) {
    ranges.push_back(ToJson(table.pDescriptorRanges[i]));
  }

  return {
      {"NumDescriptorRanges", table.NumDescriptorRanges},
      {"DescriptorRanges", std::move(ranges)},
  };
}

nlohmann::json ToJson(const D3D12_ROOT_DESCRIPTOR_TABLE1& table) {
  RequireArray(table.pDescriptorRanges, table.NumDescriptorRanges,
               "D3D12_ROOT_DESCRIPTOR_TABLE1::pDescriptorRanges");

  nlohmann::json ranges = nlohmann::json::array();
  for (UINT i = 0; i < table.NumDescriptorRanges; ++i) {
    ranges.push_back(ToJson(table.pDescriptorRanges[i]));
  }

  return {
      {"NumDescriptorRanges", table.NumDescriptorRanges},
      {"DescriptorRanges", std::move(ranges)},
  };
}

nlohmann::json ToJson(const D3D12_ROOT_CONSTANTS& constants) {
  return {
      {"ShaderRegister", constants.ShaderRegister},
      {"RegisterSpace", constants.RegisterSpace},
      {"Num32BitValues", constants.Num32BitValues},
  };
}

nlohmann::json ToJson(const D3D12_ROOT_DESCRIPTOR& descriptor) {
  return {
      {"ShaderRegister", descriptor.ShaderRegister},
      {"RegisterSpace", descriptor.RegisterSpace},
  };
}

nlohmann::json ToJson(const D3D12_ROOT_DESCRIPTOR1& descriptor) {
  return {
      {"ShaderRegister", descriptor.ShaderRegister},
      {"RegisterSpace", descriptor.RegisterSpace},
      {"Flags", descriptor.Flags},
  };
}

nlohmann::json ToJson(const D3D12_ROOT_PARAMETER& parameter) {
  nlohmann::json result = {
      {"ParameterType", parameter.ParameterType},
      {"ShaderVisibility", parameter.ShaderVisibility},
  };

  switch (parameter.ParameterType) {
  case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
    result["DescriptorTable"] = ToJson(parameter.DescriptorTable);
    break;
  case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
    result["Constants"] = ToJson(parameter.Constants);
    break;
  case D3D12_ROOT_PARAMETER_TYPE_CBV:
  case D3D12_ROOT_PARAMETER_TYPE_SRV:
  case D3D12_ROOT_PARAMETER_TYPE_UAV:
    result["Descriptor"] = ToJson(parameter.Descriptor);
    break;
  default:
    throw std::invalid_argument("Unknown root parameter type " +
                                std::to_string(static_cast<int>(parameter.ParameterType)));
  }

  return result;
}

nlohmann::json ToJson(const D3D12_ROOT_PARAMETER1& parameter) {
  nlohmann::json result = {
      {"ParameterType", parameter.ParameterType},
      {"ShaderVisibility", parameter.ShaderVisibility},
  };

  switch (parameter.ParameterType) {
  case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
    result["DescriptorTable"] = ToJson(parameter.DescriptorTable);
    break;
  case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
    result["Constants"] = ToJson(parameter.Constants);
    break;
  case D3D12_ROOT_PARAMETER_TYPE_CBV:
  case D3D12_ROOT_PARAMETER_TYPE_SRV:
  case D3D12_ROOT_PARAMETER_TYPE_UAV:
    result["Descriptor"] = ToJson(parameter.Descriptor);
    break;
  default:
    throw std::invalid_argument("Unknown root parameter type " +
                                std::to_string(static_cast<int>(parameter.ParameterType)));
  }

  return result;
}

nlohmann::json ToJson(const D3D12_STATIC_SAMPLER_DESC& sampler) {
  return {
      {"Filter", sampler.Filter},
      {"AddressU", sampler.AddressU},
      {"AddressV", sampler.AddressV},
      {"AddressW", sampler.AddressW},
      {"MipLODBias", sampler.MipLODBias},
      {"MaxAnisotropy", sampler.MaxAnisotropy},
      {"ComparisonFunc", sampler.ComparisonFunc},
      {"BorderColor", sampler.BorderColor},
      {"MinLOD", sampler.MinLOD},
      {"MaxLOD", sampler.MaxLOD},
      {"ShaderRegister", sampler.ShaderRegister},
      {"RegisterSpace", sampler.RegisterSpace},
      {"ShaderVisibility", sampler.ShaderVisibility},
  };
}

nlohmann::json ToJson(const D3D12_STATIC_SAMPLER_DESC1& sampler) {
  return {
      {"Filter", sampler.Filter},
      {"AddressU", sampler.AddressU},
      {"AddressV", sampler.AddressV},
      {"AddressW", sampler.AddressW},
      {"MipLODBias", sampler.MipLODBias},
      {"MaxAnisotropy", sampler.MaxAnisotropy},
      {"ComparisonFunc", sampler.ComparisonFunc},
      {"BorderColor", sampler.BorderColor},
      {"MinLOD", sampler.MinLOD},
      {"MaxLOD", sampler.MaxLOD},
      {"ShaderRegister", sampler.ShaderRegister},
      {"RegisterSpace", sampler.RegisterSpace},
      {"ShaderVisibility", sampler.ShaderVisibility},
      {"Flags", sampler.Flags},
  };
}

nlohmann::json ToJson(const D3D12_ROOT_SIGNATURE_DESC& desc) {
  RequireArray(desc.pParameters, desc.NumParameters, "D3D12_ROOT_SIGNATURE_DESC::pParameters");
  RequireArray(desc.pStaticSamplers, desc.NumStaticSamplers,
               "D3D12_ROOT_SIGNATURE_DESC::pStaticSamplers");

  nlohmann::json parameters = nlohmann::json::array();
  for (UINT i = 0; i < desc.NumParameters; ++i) {
    parameters.push_back(ToJson(desc.pParameters[i]));
  }

  nlohmann::json staticSamplers = nlohmann::json::array();
  for (UINT i = 0; i < desc.NumStaticSamplers; ++i) {
    staticSamplers.push_back(ToJson(desc.pStaticSamplers[i]));
  }

  return {
      {"NumParameters", desc.NumParameters},
      {"Parameters", std::move(parameters)},
      {"NumStaticSamplers", desc.NumStaticSamplers},
      {"StaticSamplers", std::move(staticSamplers)},
      {"Flags", desc.Flags},
  };
}

nlohmann::json ToJson(const D3D12_ROOT_SIGNATURE_DESC1& desc) {
  RequireArray(desc.pParameters, desc.NumParameters, "D3D12_ROOT_SIGNATURE_DESC1::pParameters");
  RequireArray(desc.pStaticSamplers, desc.NumStaticSamplers,
               "D3D12_ROOT_SIGNATURE_DESC1::pStaticSamplers");

  nlohmann::json parameters = nlohmann::json::array();
  for (UINT i = 0; i < desc.NumParameters; ++i) {
    parameters.push_back(ToJson(desc.pParameters[i]));
  }

  nlohmann::json staticSamplers = nlohmann::json::array();
  for (UINT i = 0; i < desc.NumStaticSamplers; ++i) {
    staticSamplers.push_back(ToJson(desc.pStaticSamplers[i]));
  }

  return {
      {"NumParameters", desc.NumParameters},
      {"Parameters", std::move(parameters)},
      {"NumStaticSamplers", desc.NumStaticSamplers},
      {"StaticSamplers", std::move(staticSamplers)},
      {"Flags", desc.Flags},
  };
}

nlohmann::json ToJson(const D3D12_ROOT_SIGNATURE_DESC2& desc) {
  RequireArray(desc.pParameters, desc.NumParameters, "D3D12_ROOT_SIGNATURE_DESC2::pParameters");
  RequireArray(desc.pStaticSamplers, desc.NumStaticSamplers,
               "D3D12_ROOT_SIGNATURE_DESC2::pStaticSamplers");

  nlohmann::json parameters = nlohmann::json::array();
  for (UINT i = 0; i < desc.NumParameters; ++i) {
    parameters.push_back(ToJson(desc.pParameters[i]));
  }

  nlohmann::json staticSamplers = nlohmann::json::array();
  for (UINT i = 0; i < desc.NumStaticSamplers; ++i) {
    staticSamplers.push_back(ToJson(desc.pStaticSamplers[i]));
  }

  return {
      {"NumParameters", desc.NumParameters},
      {"Parameters", std::move(parameters)},
      {"NumStaticSamplers", desc.NumStaticSamplers},
      {"StaticSamplers", std::move(staticSamplers)},
      {"Flags", desc.Flags},
  };
}

nlohmann::json ToJson(const D3D12_VERSIONED_ROOT_SIGNATURE_DESC& desc) {
  nlohmann::json result = {
      {"Version", desc.Version},
  };

  switch (desc.Version) {
  case D3D_ROOT_SIGNATURE_VERSION_1_0:
    result["Desc_1_0"] = ToJson(desc.Desc_1_0);
    break;
  case D3D_ROOT_SIGNATURE_VERSION_1_1:
    result["Desc_1_1"] = ToJson(desc.Desc_1_1);
    break;
  case D3D_ROOT_SIGNATURE_VERSION_1_2:
    result["Desc_1_2"] = ToJson(desc.Desc_1_2);
    break;
  default:
    throw std::invalid_argument("Unknown root signature version " +
                                std::to_string(static_cast<int>(desc.Version)));
  }

  return result;
}

nlohmann::json ToJson(const D3D12_CACHED_PIPELINE_STATE& desc) {
  // Like the shader bytecode, the cached blob is not part of the document.
  return {
      {"pCachedBlob", 0},
      {"CachedBlobSizeInBytes", desc.CachedBlobSizeInBytes},
  };
}

nlohmann::json ToJson(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc) {
  nlohmann::json renderTargetFormats = nlohmann::json::array();
  for (const auto& format : desc.RTVFormats) {
    renderTargetFormats.push_back(format);
  }

  return {
      {"VS", ToJson(desc.VS)},
      {"PS", ToJson(desc.PS)},
      {"DS", ToJson(desc.DS)},
      {"HS", ToJson(desc.HS)},
      {"GS", ToJson(desc.GS)},
      {"StreamOutput", ToJson(desc.StreamOutput)},
      {"BlendState", ToJson(desc.BlendState)},
      {"SampleMask", desc.SampleMask},
      {"RasterizerState", ToJson(desc.RasterizerState)},
      {"DepthStencilState", ToJson(desc.DepthStencilState)},
      {"InputLayout", ToJson(desc.InputLayout)},
      {"IBStripCutValue", desc.IBStripCutValue},
      {"PrimitiveTopologyType", desc.PrimitiveTopologyType},
      {"NumRenderTargets", desc.NumRenderTargets},
      {"RTVFormats", std::move(renderTargetFormats)},
      {"DSVFormat", desc.DSVFormat},
      {"SampleDesc", ToJson(desc.SampleDesc)},
      {"NodeMask", desc.NodeMask},
      {"CachedPSO", ToJson(desc.CachedPSO)},
      {"Flags", desc.Flags},
  };
}

nlohmann::json ToJson(const D3D12_COMPUTE_PIPELINE_STATE_DESC& desc) {
  return {
      {"CS", ToJson(desc.CS)},
      {"NodeMask", desc.NodeMask},
      {"CachedPSO", ToJson(desc.CachedPSO)},
      {"Flags", desc.Flags},
  };
}

nlohmann::json ToJson(const PipelineStateInfo& info) {
  if (info.pRootSignatureDesc == nullptr) {
    throw std::invalid_argument("PipelineStateInfo::pRootSignatureDesc is null");
  }

  nlohmann::json result;
  result["Type"] = info.Type;

  // The root signature description is nested into the pipeline state
  // description, where D3D12 keeps the root signature object itself.
  switch (info.Type) {
  case PipelineStateType::Graphics: {
    if (info.pGraphicsPipelineStateDesc == nullptr) {
      throw std::invalid_argument("PipelineStateInfo::pGraphicsPipelineStateDesc is null");
    }
    nlohmann::json desc = ToJson(*info.pGraphicsPipelineStateDesc);
    desc["RootSignature"] = ToJson(*info.pRootSignatureDesc);
    result["GraphicsPipelineStateDesc"] = std::move(desc);
    break;
  }
  case PipelineStateType::Compute: {
    if (info.pComputePipelineStateDesc == nullptr) {
      throw std::invalid_argument("PipelineStateInfo::pComputePipelineStateDesc is null");
    }
    nlohmann::json desc = ToJson(*info.pComputePipelineStateDesc);
    desc["RootSignature"] = ToJson(*info.pRootSignatureDesc);
    result["ComputePipelineStateDesc"] = std::move(desc);
    break;
  }
  default:
    throw std::invalid_argument("Pipeline state type " +
                                std::to_string(static_cast<int>(info.Type)) +
                                " cannot be serialized");
  }

  return result;
}

} // namespace gits::rtsr::detail
