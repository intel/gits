// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "ccodeArguments.h"
#include "gits.h"
#include "nvapi.h"

#include <iomanip>
#include <d3dx12/d3dx12_pipeline_state_stream.h>

namespace gits {
namespace DirectX {
namespace ccode {

// directx::DescriptorHeapService::Get().GetHandle
static std::string descriptorHeapHandleStr(unsigned key,
                                           unsigned index,
                                           const std::string& typeStr) {
  GITS_ASSERT(typeStr == "CpuHandle" || typeStr == "GpuHandle");

  auto keyStr = key ? objKeyToStr(key) : "0";
  std::ostringstream ss;
  ss << "directx::DescriptorHeapService::Get().GetHandle(" << keyStr
     << ", directx::DescriptorHeapService::HandleType::" << typeStr << ", " << index << ")";
  return ss.str();
}

// directx::GpuAddressService::Get().GetGpuAddress
static std::string gpuAddressStr(unsigned key, unsigned offset) {
  auto keyStr = key ? objKeyToStr(key) : "0";
  std::ostringstream ss;
  ss << "directx::GpuAddressService::Get().GetGpuAddress(" << keyStr << ", " << offset << ")";
  return ss.str();
}

void argumentToCpp(Argument<IID>& arg, CppParameterInfo& info, CppParameterOutput& out) {
  out.initialization = "";
  out.value = toStr(arg.value);
  out.decorator = "";
}

void argumentToCpp(BufferArgument& arg, CppParameterInfo& info, CppParameterOutput& out) {
  info.size = arg.size;
  toCpp(arg.value, info, out);
}
void argumentToCpp(OutputBufferArgument& arg, CppParameterInfo& info, CppParameterOutput& out) {
  std::ostringstream ss;
  ss << "void *" << info.name << " = nullptr;" << std::endl;
  out.initialization = ss.str();
  out.value = info.name;
  out.decorator = "&";
}
void argumentToCpp(ShaderIdentifierArgument& arg, CppParameterInfo& info, CppParameterOutput& out) {
}
void argumentToCpp(DescriptorHandleArrayArgument<D3D12_CPU_DESCRIPTOR_HANDLE>& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  if (arg.size == 0) {
    out.initialization = "";
    out.value = "nullptr";
    out.decorator = "";
    return;
  }

  std::ostringstream ss;

  ss << info.type << " " << info.name << "[" << arg.size << "] = {};" << std::endl;
  for (size_t i = 0; i < arg.size; ++i) {
    ss << info.name << "[" << i
       << "].ptr = " << descriptorHeapHandleStr(arg.interfaceKeys[i], arg.indexes[i], "CpuHandle")
       << ";" << std::endl;
  }

  out.initialization = ss.str();
  out.value = info.name;
  out.decorator = "";
}

void argumentToCpp(DescriptorHandleArgument<D3D12_CPU_DESCRIPTOR_HANDLE>& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  std::ostringstream ss;
  ss << info.type << " " << info.name << " = {};" << std::endl;
  ss << info.name << ".ptr = " << descriptorHeapHandleStr(arg.interfaceKey, arg.index, "CpuHandle")
     << ";" << std::endl;

  out.initialization = ss.str();
  out.value = info.name;
  out.decorator = "";
}
void argumentToCpp(DescriptorHandleArgument<D3D12_GPU_DESCRIPTOR_HANDLE>& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  std::ostringstream ss;
  ss << info.type << " " << info.name << " = {};" << std::endl;
  ss << info.name << ".ptr = " << descriptorHeapHandleStr(arg.interfaceKey, arg.index, "GpuHandle")
     << ";" << std::endl;

  out.initialization = ss.str();
  out.value = info.name;
  out.decorator = "";
}

void argumentToCpp(LPCWSTR_Argument& arg, CppParameterInfo& info, CppParameterOutput& out) {
  toCpp(arg.value, info, out);
}

void argumentToCpp(LPCSTR_Argument& arg, CppParameterInfo& info, CppParameterOutput& out) {
  toCpp(arg.value, info, out);
}

void argumentToCpp(D3D12_GPU_VIRTUAL_ADDRESS_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  auto keyStr = arg.interfaceKey ? objKeyToStr(arg.interfaceKey) : "0";

  std::ostringstream ss;
  ss << info.type << " " << info.name << " = " << gpuAddressStr(arg.interfaceKey, arg.offset) << ";"
     << std::endl;

  out.initialization = ss.str();
  out.value = info.name;
  out.decorator = "";
}

void argumentToCpp(D3D12_GPU_VIRTUAL_ADDRESSs_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  std::ostringstream ss;
  ss << info.type << " " << info.name << "[" << arg.size << "] = {};" << std::endl;
  for (size_t i = 0; i < arg.size; ++i) {
    ss << info.name << "[" << i << "] = " << gpuAddressStr(arg.interfaceKeys[i], arg.offsets[i])
       << ";" << std::endl;
  }

  out.initialization = ss.str();
  out.value = info.name;
  out.decorator = "";
}

void argumentToCpp(D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  GITS_ASSERT(arg.value != nullptr);
  auto& value = *arg.value;

  std::ostringstream ss;

  CppParameterInfo vsInfo("D3D12_SHADER_BYTECODE", "vs");
  CppParameterOutput vsOut;
  toCpp(value.VS, vsInfo, vsOut);
  ss << vsOut.initialization;

  CppParameterInfo psInfo("D3D12_SHADER_BYTECODE", "ps");
  CppParameterOutput psOut;
  toCpp(value.PS, psInfo, psOut);
  ss << psOut.initialization;

  CppParameterInfo dsInfo("D3D12_SHADER_BYTECODE", "ds");
  CppParameterOutput dsOut;
  toCpp(value.DS, dsInfo, dsOut);
  ss << dsOut.initialization;

  CppParameterInfo hsInfo("D3D12_SHADER_BYTECODE", "hs");
  CppParameterOutput hsOut;
  toCpp(value.HS, hsInfo, hsOut);
  ss << hsOut.initialization;

  CppParameterInfo gsInfo("D3D12_SHADER_BYTECODE", "gs");
  CppParameterOutput gsOut;
  toCpp(value.GS, gsInfo, gsOut);
  ss << gsOut.initialization;

  CppParameterInfo streamOutputDescInfo("D3D12_STREAM_OUTPUT_DESC", "streamOutput");
  CppParameterOutput streamOutputDescOut;
  toCpp(value.StreamOutput, streamOutputDescInfo, streamOutputDescOut);
  ss << streamOutputDescOut.initialization;

  CppParameterInfo blendDescInfo("D3D12_BLEND_DESC", "blendDesc");
  CppParameterOutput blendDescOut;
  toCpp(value.BlendState, blendDescInfo, blendDescOut);
  ss << blendDescOut.initialization;

  CppParameterInfo rasterizerDescInfo("D3D12_RASTERIZER_DESC", "rasterizerDesc");
  CppParameterOutput rasterizerDescOut;
  toCpp(value.RasterizerState, rasterizerDescInfo, rasterizerDescOut);
  ss << rasterizerDescOut.initialization;

  CppParameterInfo depthStencilDescInfo("D3D12_DEPTH_STENCIL_DESC", "depthStencilDesc");
  CppParameterOutput depthStencilDescOut;
  toCpp(value.DepthStencilState, depthStencilDescInfo, depthStencilDescOut);
  ss << depthStencilDescOut.initialization;

  CppParameterInfo inputLayoutInfo("D3D12_INPUT_LAYOUT_DESC", "inputLayout");
  CppParameterOutput inputLayoutOut;
  toCpp(value.InputLayout, inputLayoutInfo, inputLayoutOut);
  ss << inputLayoutOut.initialization;

  ss << info.type << " " << info.name << " = {};" << std::endl;
  ss << info.name << ".pRootSignature = " << objKeyToPtrStr(arg.rootSignatureKey) << ";"
     << std::endl;

  ss << info.name << ".VS = " << vsOut.value << ";" << std::endl;
  ss << info.name << ".PS = " << psOut.value << ";" << std::endl;
  ss << info.name << ".DS = " << dsOut.value << ";" << std::endl;
  ss << info.name << ".HS = " << hsOut.value << ";" << std::endl;
  ss << info.name << ".GS = " << gsOut.value << ";" << std::endl;

  ss << info.name << ".StreamOutput = " << streamOutputDescOut.value << ";" << std::endl;
  ss << info.name << ".BlendState = " << blendDescOut.value << ";" << std::endl;
  ss << info.name << ".SampleMask = " << value.SampleMask << ";" << std::endl;
  ss << info.name << ".RasterizerState = " << rasterizerDescOut.value << ";" << std::endl;
  ss << info.name << ".DepthStencilState = " << depthStencilDescOut.value << ";" << std::endl;
  ss << info.name << ".InputLayout = " << inputLayoutOut.value << ";" << std::endl;
  ss << info.name << ".IBStripCutValue = " << toStr(value.IBStripCutValue) << ";" << std::endl;
  ss << info.name << ".PrimitiveTopologyType = " << toStr(value.PrimitiveTopologyType) << ";"
     << std::endl;
  ss << info.name << ".NumRenderTargets = " << value.NumRenderTargets << ";" << std::endl;
  for (UINT i = 0; i < 8; ++i) {
    ss << info.name << ".RTVFormats[" << i << "] = " << toStr(value.RTVFormats[i]) << ";"
       << std::endl;
  }
  ss << info.name << ".DSVFormat = " << toStr(value.DSVFormat) << ";" << std::endl;
  ss << info.name << ".SampleDesc.Count = " << value.SampleDesc.Count << ";" << std::endl;
  ss << info.name << ".SampleDesc.Quality = " << value.SampleDesc.Quality << ";" << std::endl;
  ss << info.name << ".NodeMask = " << value.NodeMask << ";" << std::endl;
  ss << info.name << ".Flags = " << toStr(value.Flags) << ";" << std::endl;

  ss << "// CachedPSO is always set to null" << std::endl;
  ss << info.name << ".CachedPSO.CachedBlobSizeInBytes = 0;" << std::endl;
  ss << info.name << ".CachedPSO.pCachedBlob = nullptr;" << std::endl;

  out.initialization = ss.str();
  out.value = info.name;
  out.decorator = "&";
}
void argumentToCpp(D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  GITS_ASSERT(arg.value != nullptr);
  auto& value = *arg.value;

  std::ostringstream ss;

  CppParameterInfo csInfo("D3D12_SHADER_BYTECODE", "cs");
  CppParameterOutput csOut;
  toCpp(value.CS, csInfo, csOut);
  ss << csOut.initialization;

  ss << info.type << " " << info.name << " = {};" << std::endl;
  ss << info.name << ".pRootSignature = " << objKeyToPtrStr(arg.rootSignatureKey) << ";"
     << std::endl;
  ss << info.name << ".CS = " << csOut.value << ";" << std::endl;
  ss << info.name << ".NodeMask = " << value.NodeMask << ";" << std::endl;

  ss << "// CachedPSO is always set to null" << std::endl;
  ss << info.name << ".CachedPSO.CachedBlobSizeInBytes = 0;" << std::endl;
  ss << info.name << ".CachedPSO.pCachedBlob = nullptr;" << std::endl;

  ss << info.name << ".Flags = " << toStr(value.Flags) << ";" << std::endl;

  out.initialization = ss.str();
  out.value = info.name;
  out.decorator = "&";
}

void argumentToCpp(D3D12_TEXTURE_COPY_LOCATION_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  GITS_ASSERT(arg.value != nullptr);
  auto& value = *arg.value;

  std::ostringstream ss;
  std::ostringstream ssUnion;

  if (value.Type == D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT) {
    CppParameterInfo footprintInfo("D3D12_PLACED_SUBRESOURCE_FOOTPRINT", "PlacedFootprint");
    CppParameterOutput footprintOut;
    toCpp(value.PlacedFootprint, footprintInfo, footprintOut);
    ss << footprintOut.initialization;
    ssUnion << info.name << ".PlacedFootprint = " << footprintOut.value << ";" << std::endl;
  } else if (value.Type == D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX) {
    ssUnion << info.name << ".SubresourceIndex = " << value.SubresourceIndex << ";" << std::endl;
  }

  ss << info.type << " " << info.name << " = {};" << std::endl;
  ss << info.name << ".pResource = " << objKeyToPtrStr(arg.resourceKey) << ";" << std::endl;
  ss << info.name << ".Type = " << toStr(value.Type) << ";" << std::endl;
  ss << ssUnion.str();
  out.initialization = ss.str();
  out.value = info.name;
  out.decorator = "&";
}

void argumentToCpp(D3D12_RESOURCE_BARRIERs_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  auto* value = arg.value;

  std::ostringstream ss;
  ss << info.type << " " << info.name << "[" << arg.size << "] = {};" << std::endl;
  for (unsigned i = 0; i < arg.size; ++i) {
    std::ostringstream ssUnion;
    // Handle union members
    switch (value[i].Type) {
    case D3D12_RESOURCE_BARRIER_TYPE_TRANSITION: {
      ssUnion << info.name << "[" << i
              << "].Transition.pResource = " << objKeyToPtrStr(arg.resourceKeys[i]) << ";"
              << std::endl;
      ssUnion << info.name << "[" << i
              << "].Transition.Subresource = " << toHex(value[i].Transition.Subresource) << ";"
              << std::endl;
      ssUnion << info.name << "[" << i
              << "].Transition.StateBefore = " << toStr(value[i].Transition.StateBefore) << ";"
              << std::endl;
      ssUnion << info.name << "[" << i
              << "].Transition.StateAfter = " << toStr(value[i].Transition.StateAfter) << ";"
              << std::endl;
    } break;
    case D3D12_RESOURCE_BARRIER_TYPE_ALIASING: {
      ssUnion << info.name << "[" << i
              << "].Aliasing.pResourceBefore = " << objKeyToPtrStr(arg.resourceKeys[i]) << ";"
              << std::endl;
      ssUnion << info.name << "[" << i
              << "].Aliasing.pResourceAfter = " << objKeyToPtrStr(arg.resourceAfterKeys[i]) << ";"
              << std::endl;
    } break;
    case D3D12_RESOURCE_BARRIER_TYPE_UAV:
      ssUnion << info.name << "[" << i
              << "].UAV.pResource = " << objKeyToPtrStr(arg.resourceKeys[i]) << ";" << std::endl;
      break;
    default:
      GITS_ASSERT("Unknown D3D12_RESOURCE_BARRIER type");
    }

    ss << info.name << "[" << i << "].Type = " << toStr(value[i].Type) << ";" << std::endl;
    ss << info.name << "[" << i << "].Flags = " << toStr(value[i].Flags) << ";" << std::endl;
    ss << ssUnion.str();
  }

  out.initialization = ss.str();
  out.value = info.name;
  out.decorator = "";
}

void argumentToCpp(D3D12_INDEX_BUFFER_VIEW_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  if (!arg.value) {
    out.initialization = "";
    out.value = "nullptr";
    out.decorator = "";
    return;
  }

  auto& value = *arg.value;

  std::ostringstream ss;
  ss << info.type << " " << info.name << " = {};" << std::endl;
  ss << info.name
     << ".BufferLocation = " << gpuAddressStr(arg.bufferLocationKey, arg.bufferLocationOffset)
     << ";" << std::endl;
  ss << info.name << ".SizeInBytes = " << value.SizeInBytes << ";" << std::endl;
  ss << info.name << ".Format = " << toStr(value.Format) << ";" << std::endl;

  out.initialization = ss.str();
  out.value = info.name;
  out.decorator = "&";
}

void argumentToCpp(D3D12_CONSTANT_BUFFER_VIEW_DESC_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  if (!arg.value) {
    out.initialization = "";
    out.value = "nullptr";
    out.decorator = "";
    return;
  }

  auto& value = *arg.value;
  std::ostringstream ss;
  ss << info.type << " " << info.name << " = {};" << std::endl;
  ss << info.name
     << ".BufferLocation = " << gpuAddressStr(arg.bufferLocationKey, arg.bufferLocationOffset)
     << ";" << std::endl;
  ss << info.name << ".SizeInBytes = " << value.SizeInBytes << ";" << std::endl;

  out.initialization = ss.str();
  out.value = info.name;
  out.decorator = "&";
}

void argumentToCpp(D3D12_VERTEX_BUFFER_VIEWs_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  if (arg.value == nullptr || arg.size == 0) {
    out.initialization = "";
    out.value = "nullptr";
    out.decorator = "";
    return;
  }

  std::ostringstream ss;
  ss << info.type << " " << info.name << "[" << arg.size << "] = {};" << std::endl;

  for (unsigned i = 0; i < arg.size; ++i) {
    ss << info.name << "[" << i << "].BufferLocation = "
       << gpuAddressStr(arg.bufferLocationKeys[i], arg.bufferLocationOffsets[i]) << ";"
       << std::endl;
    ss << info.name << "[" << i << "].SizeInBytes = " << arg.value[i].SizeInBytes << ";"
       << std::endl;
    ss << info.name << "[" << i << "].StrideInBytes = " << arg.value[i].StrideInBytes << ";"
       << std::endl;
  }

  out.initialization = ss.str();
  out.value = info.name;
  out.decorator = "";
}

void argumentToCpp(D3D12_STREAM_OUTPUT_BUFFER_VIEWs_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  GITS_ASSERT(false, "argumentToCpp not implemented for D3D12_STREAM_OUTPUT_BUFFER_VIEWs_Argument");
}

void argumentToCpp(D3D12_WRITEBUFFERIMMEDIATE_PARAMETERs_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  if (!arg.value || arg.size == 0) {
    out.initialization = "";
    out.value = "nullptr";
    out.decorator = "";
    return;
  }

  std::ostringstream ss;
  ss << info.type << " " << info.name << "[" << arg.size << "] = {};" << std::endl;

  for (unsigned i = 0; i < arg.size; ++i) {
    ss << info.name << "[" << i << "].Dest = " << gpuAddressStr(arg.destKeys[i], arg.destOffsets[i])
       << ";" << std::endl;
    ss << info.name << "[" << i << "].Value = " << arg.value[i].Value << ";" << std::endl;
  }

  out.initialization = ss.str();
  out.value = info.name;
  out.decorator = "";
}

void argumentToCpp(D3D12_PIPELINE_STATE_STREAM_DESC_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  if (!arg.value) {
    out.initialization = "";
    out.value = "nullptr";
    out.decorator = "";
    return;
  }

  toCpp(arg.value, info, out);

  std::ostringstream ss;
  ss << out.initialization;
  ss << "directx::PatchPipelineState(" << out.value << "," << objKeyToPtrStr(arg.rootSignatureKey)
     << ", subobjectData.data(), subobjectData.size());" << std::endl;

  out.initialization = ss.str();
}

void argumentToCpp(D3D12_STATE_OBJECT_DESC_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  GITS_ASSERT(false, "argumentToCpp not implemented for D3D12_STATE_OBJECT_DESC_Argument");
}

void argumentToCpp(D3D12_SHADER_RESOURCE_VIEW_DESC_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  if (!arg.value) {
    out.initialization = "";
    out.value = "nullptr";
    out.decorator = "";
    return;
  }

  auto& value = *arg.value;
  std::string name = info.getIndexedName();
  std::ostringstream ss;
  std::ostringstream ssUnion;

  CppParameterInfo paramInfo("", "");
  CppParameterOutput paramOut;
  switch (value.ViewDimension) {
  case D3D12_SRV_DIMENSION_BUFFER:
    paramInfo.type = "D3D12_BUFFER_SRV";
    paramInfo.name = "Buffer";
    toCpp(value.Buffer, paramInfo, paramOut);
    break;
  case D3D12_SRV_DIMENSION_TEXTURE1D:
    paramInfo.type = "D3D12_TEX1D_SRV";
    paramInfo.name = "Texture1D";
    toCpp(value.Texture1D, paramInfo, paramOut);
    break;
  case D3D12_SRV_DIMENSION_TEXTURE1DARRAY:
    paramInfo.type = "D3D12_TEX1D_ARRAY_SRV";
    paramInfo.name = "Texture1DArray";
    toCpp(value.Texture1DArray, paramInfo, paramOut);
    break;
  case D3D12_SRV_DIMENSION_TEXTURE2D:
    paramInfo.type = "D3D12_TEX2D_SRV";
    paramInfo.name = "Texture2D";
    toCpp(value.Texture2D, paramInfo, paramOut);
    break;
  case D3D12_SRV_DIMENSION_TEXTURE2DARRAY:
    paramInfo.type = "D3D12_TEX2D_ARRAY_SRV";
    paramInfo.name = "Texture2DArray";
    toCpp(value.Texture2DArray, paramInfo, paramOut);
    break;
  case D3D12_SRV_DIMENSION_TEXTURE2DMS:
    paramInfo.type = "D3D12_TEX2DMS_ARRAY_SRV";
    paramInfo.name = "Texture2DMS";
    toCpp(value.Texture2DMS, paramInfo, paramOut);
    break;
  case D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY:
    paramInfo.type = "D3D12_TEX2DMS_ARRAY_SRV";
    paramInfo.name = "Texture2DMSArray";
    toCpp(value.Texture2DMSArray, paramInfo, paramOut);
    break;
  case D3D12_SRV_DIMENSION_TEXTURE3D:
    paramInfo.type = "D3D12_TEX3D_SRV";
    paramInfo.name = "Texture3D";
    toCpp(value.Texture3D, paramInfo, paramOut);
    break;
  case D3D12_SRV_DIMENSION_TEXTURECUBE:
    paramInfo.type = "D3D12_TEXCUBE_SRV";
    paramInfo.name = "TextureCube";
    toCpp(value.TextureCube, paramInfo, paramOut);
    break;
  case D3D12_SRV_DIMENSION_TEXTURECUBEARRAY:
    paramInfo.type = "D3D12_TEXCUBE_ARRAY_SRV";
    paramInfo.name = "TextureCubeArray";
    toCpp(value.TextureCubeArray, paramInfo, paramOut);
    break;
  case D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE:
    // Write directly to "ss" since this member needs a helper function
    paramInfo.type = "D3D12_RAYTRACING_ACCELERATION_STRUCTURE_SRV";
    paramInfo.name = "RaytracingAccelerationStructure";
    paramOut.initialization = "";
    ss << paramInfo.type << " " << paramInfo.name << " = {};" << std::endl;
    ss << paramInfo.name
       << ".Location = " << gpuAddressStr(arg.raytracingLocationKey, arg.raytracingLocationOffset)
       << ";" << std::endl;
    break;
  default:
    GITS_ASSERT(false, "Unknown D3D12_SHADER_RESOURCE_VIEW_DESC ViewDimension");
  }
  ss << paramOut.initialization;
  ssUnion << name << "." << paramInfo.name << " = " << paramOut.value << ";" << std::endl;

  ss << info.type << " " << name << " = {};" << std::endl;
  ss << name << ".Format = " << toStr(value.Format) << ";" << std::endl;
  ss << name << ".ViewDimension = " << toStr(value.ViewDimension) << ";" << std::endl;
  ss << name << ".Shader4ComponentMapping = " << toHex(value.Shader4ComponentMapping) << ";"
     << std::endl;
  ss << ssUnion.str();

  out.initialization = ss.str();
  out.value = name;
  out.decorator = "&";
}
void argumentToCpp(ArrayArgument<D3D12_RESIDENCY_PRIORITY>& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  if (!arg.value) {
    out.initialization = "";
    out.value = "nullptr";
    out.decorator = "";
    return;
  }

  std::ostringstream ss;
  ss << info.type << " " << info.name << "[" << arg.size << "] = {};" << std::endl;
  for (size_t i = 0; i < arg.size; ++i) {
    // D3D12_RESIDENCY_PRIORITY is an enum but the values can be arbitrary
    ss << info.name << "[" << i << "] = D3D12_RESIDENCY_PRIORITY(" << std::to_string(arg.value[i])
       << ");" << std::endl;
  }

  out.initialization = ss.str();
  out.value = info.name;
  out.decorator = "";
}
void argumentToCpp(D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  GITS_ASSERT(false,
              "argumentToCpp not implemented for D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument");
}
void argumentToCpp(D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  GITS_ASSERT(false,
              "argumentToCpp not implemented for D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument");
}
void argumentToCpp(PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC>& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  GITS_ASSERT(false, "argumentToCpp not implemented for "
                     "PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC>");
}
void argumentToCpp(ArrayArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  GITS_ASSERT(false, "argumentToCpp not implemented for "
                     "ArrayArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>");
}
void printArgument(
    CCodeStream& ccodeStream,
    PointerArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>& arg,
    CppParameterInfo& info,
    CppParameterOutput& out) {
  GITS_ASSERT(false,
              "argumentToCpp not implemented for "
              "PointerArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>");
}
void argumentToCpp(D3D12_BARRIER_GROUPs_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  GITS_ASSERT(false, "argumentToCpp not implemented for D3D12_BARRIER_GROUPs_Argument");
}
void argumentToCpp(DML_BINDING_TABLE_DESC_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  GITS_ASSERT(false, "argumentToCpp not implemented for DML_BINDING_TABLE_DESC_Argument");
}
void argumentToCpp(DML_GRAPH_DESC_Argument& arg, CppParameterInfo& info, CppParameterOutput& out) {
  GITS_ASSERT(false, "argumentToCpp not implemented for DML_GRAPH_DESC_Argument");
}
void argumentToCpp(DML_BINDING_DESC_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  GITS_ASSERT(false, "argumentToCpp not implemented for DML_BINDING_DESC_Argument");
}
void argumentToCpp(DML_BINDING_DESCs_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  GITS_ASSERT(false, "argumentToCpp not implemented for DML_BINDING_DESCs_Argument");
}
void argumentToCpp(DML_OPERATOR_DESC_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  GITS_ASSERT(false, "argumentToCpp not implemented for DML_OPERATOR_DESC_Argument");
}
void argumentToCpp(xess_d3d12_init_params_t_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  GITS_ASSERT(false, "argumentToCpp not implemented for xess_d3d12_init_params_t_Argument");
}
void argumentToCpp(xess_d3d12_execute_params_t_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  GITS_ASSERT(false, "argumentToCpp not implemented for xess_d3d12_execute_params_t_Argument");
}
void argumentToCpp(DML_CheckFeatureSupport_BufferArgument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  GITS_ASSERT(false, "argumentToCpp not implemented for DML_CheckFeatureSupport_BufferArgument");
}
void argumentToCpp(DSTORAGE_QUEUE_DESC_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  GITS_ASSERT(false, "argumentToCpp not implemented for DSTORAGE_QUEUE_DESC_Argument");
}
void argumentToCpp(DSTORAGE_REQUEST_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  GITS_ASSERT(false, "argumentToCpp not implemented for DSTORAGE_REQUEST_Argument");
}
void argumentToCpp(PointerArgument<NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS>& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  GITS_ASSERT(false, "argumentToCpp not implemented for "
                     "PointerArgument<NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS>");
}
void argumentToCpp(PointerArgument<NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS>& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  GITS_ASSERT(false, "argumentToCpp not implemented for "
                     "PointerArgument<NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS>");
}
void argumentToCpp(
    PointerArgument<NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS>& arg,
    CppParameterInfo& info,
    CppParameterOutput& out) {
  GITS_ASSERT(false,
              "argumentToCpp not implemented for "
              "PointerArgument<NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS>");
}

void argumentToCpp(PointerArgument<INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC>& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {

  GITS_ASSERT(arg.value != nullptr);
  auto& value = *arg.value;

  std::ostringstream ss;

  // Initialize the pD3D12Desc member first by creating a local argument
  CppParameterInfo pD3D12DescInfo("D3D12_COMPUTE_PIPELINE_STATE_DESC", "pD3D12Desc");
  pD3D12DescInfo.isPtr = true;
  CppParameterOutput pD3D12DescOut;
  {
    D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument pD3D12DescArg(value.pD3D12Desc);
    pD3D12DescArg.rootSignatureKey = arg.rootSignatureKey;
    argumentToCpp(pD3D12DescArg, pD3D12DescInfo, pD3D12DescOut);
    ss << pD3D12DescOut.initialization;
  }

  CppParameterInfo csInfo("D3D12_SHADER_BYTECODE", "cs");
  CppParameterOutput csOut;
  toCpp(value.CS, csInfo, csOut);
  ss << csOut.initialization;

  ss << info.type << " " << info.name << " = {};" << std::endl;
  ss << info.name << ".CS = " << csOut.value << ";" << std::endl;
  ss << info.name << ".ShaderInputType = " << value.ShaderInputType << ";" << std::endl;
  ss << info.name << ".CompileOptions = " << value.CompileOptions << ";" << std::endl;
  ss << info.name << ".InternalOptions = " << value.InternalOptions << ";" << std::endl;

  out.initialization = ss.str();
  out.value = info.name;
  out.decorator = "&";
}

} // namespace ccode
} // namespace DirectX
} // namespace gits
