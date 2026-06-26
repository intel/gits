// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "ccodeArguments.h"
#include "nvapi.h"

#include <vector>

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
  const auto& keyStr = key ? objKeyToStr(key) : "0";
  std::ostringstream ss;
  ss << "directx::GpuAddressService::Get().GetGpuAddress(" << keyStr << ", " << offset << ")";
  return ss.str();
}

static void appendGpuVirtualAddress(std::ostringstream& ss,
                                    const std::string& field,
                                    unsigned key,
                                    unsigned offset) {
  ss << field << " = " << gpuAddressStr(key, offset) << ";" << std::endl;
}

static void appendRenderPassEndingAccess(std::ostringstream& ss,
                                         const D3D12_RENDER_PASS_ENDING_ACCESS& value,
                                         const std::string& name,
                                         unsigned srcResourceKey,
                                         unsigned dstResourceKey) {
  CppParameterInfo parentInfo("D3D12_RENDER_PASS_ENDING_ACCESS", name);
  std::ostringstream ssUnion;

  CppParameterInfo paramInfo("", "", parentInfo);
  CppParameterOutput paramOut;
  switch (value.Type) {
  case D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE: {
    const auto& resolve = value.Resolve;
    const UINT subresourceCount = resolve.SubresourceCount;
    const std::string paramsVar = name + "_resolveParams";
    if (subresourceCount > 0 && resolve.pSubresourceParameters) {
      ss << "D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS " << paramsVar << "["
         << subresourceCount << "] = {};" << std::endl;
      for (UINT k = 0; k < subresourceCount; ++k) {
        CppParameterInfo subresourceInfo(
            "D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS", paramsVar,
            parentInfo);
        subresourceInfo.isArrayElement = true;
        subresourceInfo.index = k;
        CppParameterOutput subresourceOut;
        toCpp(resolve.pSubresourceParameters[k], subresourceInfo, subresourceOut);
        ss << subresourceOut.initialization;
      }
      ssUnion << name << ".Resolve.pSubresourceParameters = " << paramsVar << ";" << std::endl;
    } else {
      ssUnion << name << ".Resolve.pSubresourceParameters = nullptr;" << std::endl;
    }
    ssUnion << name << ".Resolve.pSrcResource = " << objKeyToPtrStr(srcResourceKey) << ";"
            << std::endl;
    ssUnion << name << ".Resolve.pDstResource = " << objKeyToPtrStr(dstResourceKey) << ";"
            << std::endl;
    ssUnion << name << ".Resolve.SubresourceCount = " << subresourceCount << ";" << std::endl;
    ssUnion << name << ".Resolve.Format = " << toStr(resolve.Format) << ";" << std::endl;
    ssUnion << name << ".Resolve.ResolveMode = " << toStr(resolve.ResolveMode) << ";" << std::endl;
    ssUnion << name << ".Resolve.PreserveResolveSource = " << toStr(resolve.PreserveResolveSource)
            << ";" << std::endl;
  } break;
  case D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE_LOCAL_RENDER:
  case D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE_LOCAL_SRV:
  case D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE_LOCAL_UAV:
    paramInfo.type = "D3D12_RENDER_PASS_ENDING_ACCESS_PRESERVE_LOCAL_PARAMETERS";
    paramInfo.name = "PreserveLocal";
    toCpp(value.PreserveLocal, paramInfo, paramOut);
    ss << paramOut.initialization;
    ssUnion << name << ".PreserveLocal = " << paramOut.value << ";" << std::endl;
    break;
  case D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD:
  case D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE:
    break;
  default:
    GITS_ASSERT(false, "Unknown D3D12_RENDER_PASS_ENDING_ACCESS Type");
    break;
  }

  ss << "D3D12_RENDER_PASS_ENDING_ACCESS " << name << " = {};" << std::endl;
  ss << name << ".Type = " << toStr(value.Type) << ";" << std::endl;
  ss << ssUnion.str();
}

static void appendPostbuildInfoDescElement(
    std::ostringstream& ss,
    const std::string& field,
    unsigned destBufferKey,
    unsigned destBufferOffset,
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_TYPE infoType) {
  ss << "D3D12_GPU_VIRTUAL_ADDRESS " << field
     << "_destBuffer = " << gpuAddressStr(destBufferKey, destBufferOffset) << ";" << std::endl;
  ss << field << ".DestBuffer = " << field << "_destBuffer;" << std::endl;
  ss << field << ".InfoType = " << toStr(infoType) << ";" << std::endl;
}

static void appendD3D12BuildRaytracingInputs(
    std::ostringstream& ss,
    const std::string& inputsVar,
    const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs,
    const std::vector<unsigned>& inputKeys,
    const std::vector<unsigned>& inputOffsets) {
  auto appendD3D12RaytracingGeometryDesc =
      [&](const std::string& prefix, const D3D12_RAYTRACING_GEOMETRY_DESC& desc,
          const std::vector<unsigned>& geomInputKeys, const std::vector<unsigned>& geomInputOffsets,
          unsigned geomInputIndex) -> unsigned {
    ss << prefix << ".Type = " << toStr(desc.Type) << ";" << std::endl;
    ss << prefix << ".Flags = " << toStr(desc.Flags) << ";" << std::endl;

    if (geomInputKeys.empty()) {
      return geomInputIndex;
    }

    if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES) {
      const auto& tri = desc.Triangles;
      ss << prefix << ".Triangles.IndexFormat = " << toStr(tri.IndexFormat) << ";" << std::endl;
      ss << prefix << ".Triangles.VertexFormat = " << toStr(tri.VertexFormat) << ";" << std::endl;
      ss << prefix << ".Triangles.IndexCount = " << tri.IndexCount << ";" << std::endl;
      ss << prefix << ".Triangles.VertexCount = " << tri.VertexCount << ";" << std::endl;
      appendGpuVirtualAddress(ss, prefix + ".Triangles.Transform3x4", geomInputKeys[geomInputIndex],
                              geomInputOffsets[geomInputIndex]);
      ++geomInputIndex;
      appendGpuVirtualAddress(ss, prefix + ".Triangles.IndexBuffer", geomInputKeys[geomInputIndex],
                              geomInputOffsets[geomInputIndex]);
      ++geomInputIndex;
      appendGpuVirtualAddress(ss, prefix + ".Triangles.VertexBuffer.StartAddress",
                              geomInputKeys[geomInputIndex], geomInputOffsets[geomInputIndex]);
      ++geomInputIndex;
      ss << prefix << ".Triangles.VertexBuffer.StrideInBytes = " << tri.VertexBuffer.StrideInBytes
         << ";" << std::endl;
    } else if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS) {
      const auto& aabbs = desc.AABBs;
      ss << prefix << ".AABBs.AABBCount = " << aabbs.AABBCount << ";" << std::endl;
      appendGpuVirtualAddress(ss, prefix + ".AABBs.AABBs.StartAddress",
                              geomInputKeys[geomInputIndex], geomInputOffsets[geomInputIndex]);
      ++geomInputIndex;
      ss << prefix << ".AABBs.AABBs.StrideInBytes = " << aabbs.AABBs.StrideInBytes << ";"
         << std::endl;
    } else if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES) {
      // Field paths such as "inputs_geometryDescs[0]" are not valid C identifiers.
      std::string localPrefix;
      localPrefix.reserve(prefix.size());
      for (char c : prefix) {
        if (c == '[') {
          localPrefix.push_back('_');
        } else if (c != ']') {
          localPrefix.push_back(c);
        }
      }
      const std::string triStruct = localPrefix + "_ommTriangles";
      if (desc.OmmTriangles.pTriangles) {
        const auto& triangles = *desc.OmmTriangles.pTriangles;
        ss << "D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC " << triStruct << " = {};" << std::endl;
        ss << triStruct << ".IndexFormat = " << toStr(triangles.IndexFormat) << ";" << std::endl;
        ss << triStruct << ".VertexFormat = " << toStr(triangles.VertexFormat) << ";" << std::endl;
        ss << triStruct << ".IndexCount = " << triangles.IndexCount << ";" << std::endl;
        ss << triStruct << ".VertexCount = " << triangles.VertexCount << ";" << std::endl;
        appendGpuVirtualAddress(ss, triStruct + ".Transform3x4", geomInputKeys[geomInputIndex],
                                geomInputOffsets[geomInputIndex]);
        ++geomInputIndex;
        appendGpuVirtualAddress(ss, triStruct + ".IndexBuffer", geomInputKeys[geomInputIndex],
                                geomInputOffsets[geomInputIndex]);
        ++geomInputIndex;
        appendGpuVirtualAddress(ss, triStruct + ".VertexBuffer.StartAddress",
                                geomInputKeys[geomInputIndex], geomInputOffsets[geomInputIndex]);
        ++geomInputIndex;
        ss << triStruct << ".VertexBuffer.StrideInBytes = " << triangles.VertexBuffer.StrideInBytes
           << ";" << std::endl;
        ss << prefix << ".OmmTriangles.pTriangles = &" << triStruct << ";" << std::endl;
      }
      if (desc.OmmTriangles.pOmmLinkage) {
        const auto& ommLinkage = *desc.OmmTriangles.pOmmLinkage;
        const std::string linkageStruct = localPrefix + "_ommLinkage";
        ss << "D3D12_RAYTRACING_GEOMETRY_OMM_LINKAGE_DESC " << linkageStruct << " = {};"
           << std::endl;
        appendGpuVirtualAddress(ss, linkageStruct + ".OpacityMicromapIndexBuffer.StartAddress",
                                geomInputKeys[geomInputIndex], geomInputOffsets[geomInputIndex]);
        ++geomInputIndex;
        ss << linkageStruct << ".OpacityMicromapIndexBuffer.StrideInBytes = "
           << ommLinkage.OpacityMicromapIndexBuffer.StrideInBytes << ";" << std::endl;
        ss << linkageStruct
           << ".OpacityMicromapIndexFormat = " << toStr(ommLinkage.OpacityMicromapIndexFormat)
           << ";" << std::endl;
        ss << linkageStruct
           << ".OpacityMicromapBaseLocation = " << ommLinkage.OpacityMicromapBaseLocation << ";"
           << std::endl;
        appendGpuVirtualAddress(ss, linkageStruct + ".OpacityMicromapArray",
                                geomInputKeys[geomInputIndex], geomInputOffsets[geomInputIndex]);
        ++geomInputIndex;
        ss << prefix << ".OmmTriangles.pOmmLinkage = &" << linkageStruct << ";" << std::endl;
      }
    }
    return geomInputIndex;
  };

  ss << "D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS " << inputsVar << " = {};"
     << std::endl;
  ss << inputsVar << ".Type = " << toStr(inputs.Type) << ";" << std::endl;
  ss << inputsVar << ".Flags = " << toStr(inputs.Flags) << ";" << std::endl;
  ss << inputsVar << ".NumDescs = " << inputs.NumDescs << ";" << std::endl;
  ss << inputsVar << ".DescsLayout = " << toStr(inputs.DescsLayout) << ";" << std::endl;

  if (!inputKeys.empty() && inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
    ss << inputsVar << ".InstanceDescs = " << gpuAddressStr(inputKeys[0], inputOffsets[0]) << ";"
       << std::endl;
  } else if (inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL &&
             inputs.NumDescs > 0) {
    const std::string geoArray = inputsVar + "_geometryDescs";
    ss << "D3D12_RAYTRACING_GEOMETRY_DESC " << geoArray << "[" << inputs.NumDescs << "] = {};"
       << std::endl;
    unsigned inputIndex = 0;
    for (unsigned i = 0; i < inputs.NumDescs; ++i) {
      const D3D12_RAYTRACING_GEOMETRY_DESC& desc = inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY
                                                       ? inputs.pGeometryDescs[i]
                                                       : *inputs.ppGeometryDescs[i];
      const std::string elem = geoArray + "[" + std::to_string(i) + "]";
      inputIndex =
          appendD3D12RaytracingGeometryDesc(elem, desc, inputKeys, inputOffsets, inputIndex);
    }
    if (inputs.DescsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY) {
      ss << inputsVar << ".pGeometryDescs = " << geoArray << ";" << std::endl;
    } else {
      const std::string ptrArray = inputsVar + "_geometryDescPtrs";
      ss << "D3D12_RAYTRACING_GEOMETRY_DESC* " << ptrArray << "[" << inputs.NumDescs << "];"
         << std::endl;
      for (unsigned i = 0; i < inputs.NumDescs; ++i) {
        ss << ptrArray << "[" << i << "] = &" << geoArray << "[" << i << "];" << std::endl;
      }
      ss << inputsVar << ".ppGeometryDescs = " << ptrArray << ";" << std::endl;
    }
  } else if (!inputKeys.empty() &&
             inputs.Type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_OPACITY_MICROMAP_ARRAY &&
             inputs.pOpacityMicromapArrayDesc) {
    const auto* ommDesc = inputs.pOpacityMicromapArrayDesc;
    const std::string ommVar = inputsVar + "_opacityMicromapArrayDesc";
    ss << "D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC " << ommVar << " = {};" << std::endl;
    ss << ommVar << ".NumOmmHistogramEntries = " << ommDesc->NumOmmHistogramEntries << ";"
       << std::endl;
    if (ommDesc->NumOmmHistogramEntries > 0 && ommDesc->pOmmHistogram) {
      const std::string histArray = ommVar + "_histogram";
      ss << "D3D12_RAYTRACING_OPACITY_MICROMAP_HISTOGRAM_ENTRY " << histArray << "["
         << ommDesc->NumOmmHistogramEntries << "] = {};" << std::endl;
      for (UINT i = 0; i < ommDesc->NumOmmHistogramEntries; ++i) {
        ss << histArray << "[" << i << "].Count = " << ommDesc->pOmmHistogram[i].Count << ";"
           << std::endl;
        ss << histArray << "[" << i
           << "].SubdivisionLevel = " << ommDesc->pOmmHistogram[i].SubdivisionLevel << ";"
           << std::endl;
        ss << histArray << "[" << i << "].Format = " << toStr(ommDesc->pOmmHistogram[i].Format)
           << ";" << std::endl;
      }
      ss << ommVar << ".pOmmHistogram = " << histArray << ";" << std::endl;
    }
    unsigned inputIndex = 0;
    appendGpuVirtualAddress(ss, ommVar + ".InputBuffer", inputKeys[inputIndex],
                            inputOffsets[inputIndex]);
    ++inputIndex;
    appendGpuVirtualAddress(ss, ommVar + ".PerOmmDescs.StartAddress", inputKeys[inputIndex],
                            inputOffsets[inputIndex]);
    ++inputIndex;
    ss << ommVar << ".PerOmmDescs.StrideInBytes = " << ommDesc->PerOmmDescs.StrideInBytes << ";"
       << std::endl;
    ss << inputsVar << ".pOpacityMicromapArrayDesc = &" << ommVar << ";" << std::endl;
  }
}

void declareObject(const std::string& type, unsigned key) {
  static std::unordered_set<unsigned> s_declaredKeys;
  if (key == 0) {
    return;
  }

  auto& stream = ccode::CCodeStream::getInstance();
  auto objectName = objKeyToStr(key);
  auto objectType = stream.getInterfaceName(key);
  if (objectType.empty()) {
    // Type is not an interface (i.e. XeSS or Intel Extensions context)
    objectType = type;
  } else if (objectType == "ID3D12CommandList") {
    // Do not use ID3D12CommandList as it is an abstract interface
    objectType = "ID3D12GraphicsCommandList7";
  }

  // SDK typedefs that are already pointers. Storage must be a handle value, not T*.
  static const std::unordered_set<std::string> opaqueHandleTypes = {
      "xess_context_handle_t",
      "xell_context_handle_t",
      "xefg_swapchain_handle_t",
  };

  // Write out the declaration (only for new interfaces)
  auto result = s_declaredKeys.insert(key);
  if (result.second) {
    auto& ss = stream.getObjectsHeader();
    ss << std::endl;
    ss << "// " << objectName << std::endl;
    ss << "inline constexpr unsigned " << objectName << " = " << key << ";" << std::endl;
    if (opaqueHandleTypes.contains(objectType)) {
      ss << "inline " << objectType << " g_" << objectName << " = nullptr;" << std::endl;
    } else {
      ss << "inline " << objectType << "* g_" << objectName << " = nullptr;" << std::endl;
    }
  }
}

void argumentToCpp(Argument<IID>& arg, CppParameterInfo& info, CppParameterOutput& out) {
  out.initialization = "";
  out.value = toStr(arg.Value);
  out.decorator = "";
}

void argumentToCpp(Argument<xess_app_log_callback_t>& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  out.initialization = "";
  out.value = "nullptr";
  out.decorator = "";
}

void argumentToCpp(Argument<xell_app_log_callback_t>& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  out.initialization = "";
  out.value = "nullptr";
  out.decorator = "";
}

void argumentToCpp(Argument<xell_latency_marker_type_t>& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  out.initialization = "";
  out.value = toStr(arg.Value);
  out.decorator = "";
}

void argumentToCpp(Argument<xefg_swapchain_app_log_callback_t>& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  out.initialization = "";
  out.value = "nullptr";
  out.decorator = "";
}

void argumentToCpp(BufferArgument& arg, CppParameterInfo& info, CppParameterOutput& out) {
  info.size = arg.Size;
  toCpp(arg.Value, info, out);
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
  if (arg.Size == 0) {
    out.initialization = "";
    out.value = "nullptr";
    out.decorator = "";
    return;
  }

  std::ostringstream ss;

  ss << info.type << " " << info.name << "[" << arg.Size << "] = {};" << std::endl;
  for (size_t i = 0; i < arg.Size; ++i) {
    ss << info.name << "[" << i
       << "].ptr = " << descriptorHeapHandleStr(arg.InterfaceKeys[i], arg.Indexes[i], "CpuHandle")
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
  ss << info.name << ".ptr = " << descriptorHeapHandleStr(arg.InterfaceKey, arg.Index, "CpuHandle")
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
  ss << info.name << ".ptr = " << descriptorHeapHandleStr(arg.InterfaceKey, arg.Index, "GpuHandle")
     << ";" << std::endl;

  out.initialization = ss.str();
  out.value = info.name;
  out.decorator = "";
}

void argumentToCpp(LPCWSTR_Argument& arg, CppParameterInfo& info, CppParameterOutput& out) {
  toCpp(arg.Value, info, out);
}

void argumentToCpp(LPCSTR_Argument& arg, CppParameterInfo& info, CppParameterOutput& out) {
  toCpp(arg.Value, info, out);
}

void argumentToCpp(D3D12_GPU_VIRTUAL_ADDRESS_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  std::ostringstream ss;
  ss << info.type << " " << info.name << " = " << gpuAddressStr(arg.InterfaceKey, arg.Offset) << ";"
     << std::endl;

  out.initialization = ss.str();
  out.value = info.name;
  out.decorator = "";
}

void argumentToCpp(D3D12_GPU_VIRTUAL_ADDRESSs_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  std::ostringstream ss;
  ss << info.type << " " << info.name << "[" << arg.Size << "] = {};" << std::endl;
  for (size_t i = 0; i < arg.Size; ++i) {
    ss << info.name << "[" << i << "] = " << gpuAddressStr(arg.InterfaceKeys[i], arg.Offsets[i])
       << ";" << std::endl;
  }

  out.initialization = ss.str();
  out.value = info.name;
  out.decorator = "";
}

void argumentToCpp(D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  GITS_ASSERT(arg.Value != nullptr);
  auto& value = *arg.Value;

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
  ss << info.name << ".pRootSignature = " << objKeyToPtrStr(arg.RootSignatureKey) << ";"
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
  GITS_ASSERT(arg.Value != nullptr);
  auto& value = *arg.Value;

  std::ostringstream ss;

  CppParameterInfo csInfo("D3D12_SHADER_BYTECODE", "cs");
  CppParameterOutput csOut;
  toCpp(value.CS, csInfo, csOut);
  ss << csOut.initialization;

  ss << info.type << " " << info.name << " = {};" << std::endl;
  ss << info.name << ".pRootSignature = " << objKeyToPtrStr(arg.RootSignatureKey) << ";"
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
  GITS_ASSERT(arg.Value != nullptr);
  auto& value = *arg.Value;

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
  ss << info.name << ".pResource = " << objKeyToPtrStr(arg.ResourceKey) << ";" << std::endl;
  ss << info.name << ".Type = " << toStr(value.Type) << ";" << std::endl;
  ss << ssUnion.str();
  out.initialization = ss.str();
  out.value = info.name;
  out.decorator = "&";
}

void argumentToCpp(D3D12_RESOURCE_BARRIERs_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  auto* value = arg.Value;

  std::ostringstream ss;
  ss << info.type << " " << info.name << "[" << arg.Size << "] = {};" << std::endl;
  for (unsigned i = 0; i < arg.Size; ++i) {
    std::ostringstream ssUnion;
    // Handle union members
    switch (value[i].Type) {
    case D3D12_RESOURCE_BARRIER_TYPE_TRANSITION: {
      ssUnion << info.name << "[" << i
              << "].Transition.pResource = " << objKeyToPtrStr(arg.ResourceKeys[i]) << ";"
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
              << "].Aliasing.pResourceBefore = " << objKeyToPtrStr(arg.ResourceKeys[i]) << ";"
              << std::endl;
      ssUnion << info.name << "[" << i
              << "].Aliasing.pResourceAfter = " << objKeyToPtrStr(arg.ResourceAfterKeys[i]) << ";"
              << std::endl;
    } break;
    case D3D12_RESOURCE_BARRIER_TYPE_UAV:
      ssUnion << info.name << "[" << i
              << "].UAV.pResource = " << objKeyToPtrStr(arg.ResourceKeys[i]) << ";" << std::endl;
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
  if (!arg.Value) {
    out.initialization = "";
    out.value = "nullptr";
    out.decorator = "";
    return;
  }

  auto& value = *arg.Value;

  std::ostringstream ss;
  ss << info.type << " " << info.name << " = {};" << std::endl;
  ss << info.name
     << ".BufferLocation = " << gpuAddressStr(arg.BufferLocationKey, arg.BufferLocationOffset)
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
  if (!arg.Value) {
    out.initialization = "";
    out.value = "nullptr";
    out.decorator = "";
    return;
  }

  auto& value = *arg.Value;
  std::ostringstream ss;
  ss << info.type << " " << info.name << " = {};" << std::endl;
  ss << info.name
     << ".BufferLocation = " << gpuAddressStr(arg.BufferLocationKey, arg.BufferLocationOffset)
     << ";" << std::endl;
  ss << info.name << ".SizeInBytes = " << value.SizeInBytes << ";" << std::endl;

  out.initialization = ss.str();
  out.value = info.name;
  out.decorator = "&";
}

void argumentToCpp(D3D12_VERTEX_BUFFER_VIEWs_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  if (arg.Value == nullptr || arg.Size == 0) {
    out.initialization = "";
    out.value = "nullptr";
    out.decorator = "";
    return;
  }

  std::ostringstream ss;
  ss << info.type << " " << info.name << "[" << arg.Size << "] = {};" << std::endl;

  for (unsigned i = 0; i < arg.Size; ++i) {
    ss << info.name << "[" << i << "].BufferLocation = "
       << gpuAddressStr(arg.BufferLocationKeys[i], arg.BufferLocationOffsets[i]) << ";"
       << std::endl;
    ss << info.name << "[" << i << "].SizeInBytes = " << arg.Value[i].SizeInBytes << ";"
       << std::endl;
    ss << info.name << "[" << i << "].StrideInBytes = " << arg.Value[i].StrideInBytes << ";"
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
  if (!arg.Value || arg.Size == 0) {
    out.initialization = "";
    out.value = "nullptr";
    out.decorator = "";
    return;
  }

  std::ostringstream ss;
  ss << info.type << " " << info.name << "[" << arg.Size << "] = {};" << std::endl;

  for (unsigned i = 0; i < arg.Size; ++i) {
    ss << info.name << "[" << i << "].Dest = " << gpuAddressStr(arg.DestKeys[i], arg.DestOffsets[i])
       << ";" << std::endl;
    ss << info.name << "[" << i << "].Value = " << arg.Value[i].Value << ";" << std::endl;
  }

  out.initialization = ss.str();
  out.value = info.name;
  out.decorator = "";
}

void argumentToCpp(D3D12_PIPELINE_STATE_STREAM_DESC_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  if (!arg.Value) {
    out.initialization = "";
    out.value = "nullptr";
    out.decorator = "";
    return;
  }

  toCpp(arg.Value, info, out);

  std::ostringstream ss;
  ss << out.initialization;
  ss << "directx::PatchPipelineState(" << out.value << "," << objKeyToPtrStr(arg.RootSignatureKey)
     << ", subobjectData.data(), subobjectData.size());" << std::endl;

  out.initialization = ss.str();
}

void argumentToCpp(D3D12_STATE_OBJECT_DESC_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  if (!arg.Value) {
    out.initialization = "";
    out.value = "nullptr";
    out.decorator = "";
    return;
  }

  auto& value = *arg.Value;
  std::ostringstream ss;

  auto appendExportDescFields = [&](std::ostringstream& ss, const std::string& field,
                                    const D3D12_EXPORT_DESC& exportDesc) {
    ss << field << ".Name = " << charPtrToCpp(exportDesc.Name) << ";" << std::endl;
    ss << field << ".ExportToRename = " << charPtrToCpp(exportDesc.ExportToRename) << ";"
       << std::endl;
    ss << field << ".Flags = " << toStr(exportDesc.Flags) << ";" << std::endl;
  };

  auto appendExportDescArray = [&](std::ostringstream& ss, const std::string& arrayName,
                                   const D3D12_EXPORT_DESC* exports,
                                   UINT numExports) -> std::string {
    if (!exports || numExports == 0) {
      return "nullptr";
    }
    ss << "D3D12_EXPORT_DESC " << arrayName << "[" << numExports << "] = {};" << std::endl;
    for (UINT i = 0; i < numExports; ++i) {
      appendExportDescFields(ss, arrayName + "[" + std::to_string(i) + "]", exports[i]);
    }
    return arrayName;
  };

  auto appendStateSubobjectDesc = [&](std::ostringstream& ss, const std::string& varPrefix,
                                      const D3D12_STATE_SUBOBJECT& subobject,
                                      unsigned subobjectIndex) -> std::string {
    auto interfaceKeyForSubobject = [&](unsigned index) -> unsigned {
      auto it = arg.InterfaceKeysBySubobject.find(index);
      return it != arg.InterfaceKeysBySubobject.end() ? it->second : 0;
    };

    switch (subobject.Type) {
    case D3D12_STATE_SUBOBJECT_TYPE_STATE_OBJECT_CONFIG: {
      const auto* desc = static_cast<const D3D12_STATE_OBJECT_CONFIG*>(subobject.pDesc);
      const std::string name = varPrefix + "_stateObjectConfig";
      ss << "D3D12_STATE_OBJECT_CONFIG " << name << " = {};" << std::endl;
      ss << name << ".Flags = " << toStr(desc->Flags) << ";" << std::endl;
      return name;
    }
    case D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE: {
      const std::string name = varPrefix + "_globalRootSignature";
      ss << "D3D12_GLOBAL_ROOT_SIGNATURE " << name << " = {};" << std::endl;
      ss << name
         << ".pGlobalRootSignature = " << objKeyToPtrStr(interfaceKeyForSubobject(subobjectIndex))
         << ";" << std::endl;
      return name;
    }
    case D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE: {
      const std::string name = varPrefix + "_localRootSignature";
      ss << "D3D12_LOCAL_ROOT_SIGNATURE " << name << " = {};" << std::endl;
      ss << name
         << ".pLocalRootSignature = " << objKeyToPtrStr(interfaceKeyForSubobject(subobjectIndex))
         << ";" << std::endl;
      return name;
    }
    case D3D12_STATE_SUBOBJECT_TYPE_NODE_MASK: {
      const auto* desc = static_cast<const D3D12_NODE_MASK*>(subobject.pDesc);
      const std::string name = varPrefix + "_nodeMask";
      ss << "D3D12_NODE_MASK " << name << " = {};" << std::endl;
      ss << name << ".NodeMask = " << desc->NodeMask << ";" << std::endl;
      return name;
    }
    case D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY: {
      const auto* desc = static_cast<const D3D12_DXIL_LIBRARY_DESC*>(subobject.pDesc);
      const std::string bytecodeVar = varPrefix + "_dxilBytecode";
      CppParameterInfo bytecodeInfo("void", bytecodeVar);
      bytecodeInfo.size = desc->DXILLibrary.BytecodeLength;
      CppParameterOutput bytecodeOut;
      toCpp(desc->DXILLibrary.pShaderBytecode, bytecodeInfo, bytecodeOut);
      ss << bytecodeOut.initialization;

      const std::string shaderBytecodeVar = varPrefix + "_shaderBytecode";
      ss << "D3D12_SHADER_BYTECODE " << shaderBytecodeVar << " = {};" << std::endl;
      ss << shaderBytecodeVar << ".pShaderBytecode = " << bytecodeOut.decorator << bytecodeOut.value
         << ";" << std::endl;
      ss << shaderBytecodeVar << ".BytecodeLength = " << desc->DXILLibrary.BytecodeLength << ";"
         << std::endl;

      const std::string exportsArray = varPrefix + "_dxilExports";
      const std::string exportsPtr =
          appendExportDescArray(ss, exportsArray, desc->pExports, desc->NumExports);

      const std::string name = varPrefix + "_dxilLibrary";
      ss << "D3D12_DXIL_LIBRARY_DESC " << name << " = {};" << std::endl;
      ss << name << ".DXILLibrary = " << shaderBytecodeVar << ";" << std::endl;
      ss << name << ".NumExports = " << desc->NumExports << ";" << std::endl;
      ss << name << ".pExports = " << exportsPtr << ";" << std::endl;
      return name;
    }
    case D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION: {
      const auto* desc = static_cast<const D3D12_EXISTING_COLLECTION_DESC*>(subobject.pDesc);
      const std::string exportsArray = varPrefix + "_collectionExports";
      const std::string exportsPtr =
          appendExportDescArray(ss, exportsArray, desc->pExports, desc->NumExports);

      const std::string name = varPrefix + "_existingCollection";
      ss << "D3D12_EXISTING_COLLECTION_DESC " << name << " = {};" << std::endl;
      ss << name
         << ".pExistingCollection = " << objKeyToPtrStr(interfaceKeyForSubobject(subobjectIndex))
         << ";" << std::endl;
      ss << name << ".NumExports = " << desc->NumExports << ";" << std::endl;
      ss << name << ".pExports = " << exportsPtr << ";" << std::endl;
      return name;
    }
    case D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION: {
      const auto* desc =
          static_cast<const D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(subobject.pDesc);
      const std::string exportsArray = varPrefix + "_associationExports";
      if (desc->NumExports > 0) {
        ss << "LPCWSTR " << exportsArray << "[" << desc->NumExports << "] = {";
        for (UINT i = 0; i < desc->NumExports; ++i) {
          if (i > 0) {
            ss << ", ";
          }
          ss << charPtrToCpp(desc->pExports[i]);
        }
        ss << "};" << std::endl;
      }

      const std::string name = varPrefix + "_subobjectToExports";
      ss << "D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION " << name << " = {};" << std::endl;
      ss << name << ".NumExports = " << desc->NumExports << ";" << std::endl;
      if (desc->NumExports > 0) {
        ss << name << ".pExports = " << exportsArray << ";" << std::endl;
      } else {
        ss << name << ".pExports = nullptr;" << std::endl;
      }
      return name;
    }
    case D3D12_STATE_SUBOBJECT_TYPE_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION: {
      const auto* desc =
          static_cast<const D3D12_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(subobject.pDesc);
      const std::string exportsArray = varPrefix + "_dxilAssociationExports";
      if (desc->NumExports > 0) {
        ss << "LPCWSTR " << exportsArray << "[" << desc->NumExports << "] = {";
        for (UINT i = 0; i < desc->NumExports; ++i) {
          if (i > 0) {
            ss << ", ";
          }
          ss << charPtrToCpp(desc->pExports[i]);
        }
        ss << "};" << std::endl;
      }

      const std::string name = varPrefix + "_dxilSubobjectToExports";
      ss << "D3D12_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION " << name << " = {};" << std::endl;
      ss << name << ".SubobjectToAssociate = " << charPtrToCpp(desc->SubobjectToAssociate) << ";"
         << std::endl;
      ss << name << ".NumExports = " << desc->NumExports << ";" << std::endl;
      if (desc->NumExports > 0) {
        ss << name << ".pExports = " << exportsArray << ";" << std::endl;
      } else {
        ss << name << ".pExports = nullptr;" << std::endl;
      }
      return name;
    }
    case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG: {
      const auto* desc = static_cast<const D3D12_RAYTRACING_SHADER_CONFIG*>(subobject.pDesc);
      const std::string name = varPrefix + "_raytracingShaderConfig";
      ss << "D3D12_RAYTRACING_SHADER_CONFIG " << name << " = {};" << std::endl;
      ss << name << ".MaxPayloadSizeInBytes = " << desc->MaxPayloadSizeInBytes << ";" << std::endl;
      ss << name << ".MaxAttributeSizeInBytes = " << desc->MaxAttributeSizeInBytes << ";"
         << std::endl;
      return name;
    }
    case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG: {
      const auto* desc = static_cast<const D3D12_RAYTRACING_PIPELINE_CONFIG*>(subobject.pDesc);
      const std::string name = varPrefix + "_raytracingPipelineConfig";
      ss << "D3D12_RAYTRACING_PIPELINE_CONFIG " << name << " = {};" << std::endl;
      ss << name << ".MaxTraceRecursionDepth = " << desc->MaxTraceRecursionDepth << ";"
         << std::endl;
      return name;
    }
    case D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP: {
      const auto* desc = static_cast<const D3D12_HIT_GROUP_DESC*>(subobject.pDesc);
      const std::string name = varPrefix + "_hitGroup";
      ss << "D3D12_HIT_GROUP_DESC " << name << " = {};" << std::endl;
      ss << name << ".HitGroupExport = " << charPtrToCpp(desc->HitGroupExport) << ";" << std::endl;
      ss << name << ".Type = " << toStr(desc->Type) << ";" << std::endl;
      ss << name << ".AnyHitShaderImport = " << charPtrToCpp(desc->AnyHitShaderImport) << ";"
         << std::endl;
      ss << name << ".ClosestHitShaderImport = " << charPtrToCpp(desc->ClosestHitShaderImport)
         << ";" << std::endl;
      ss << name << ".IntersectionShaderImport = " << charPtrToCpp(desc->IntersectionShaderImport)
         << ";" << std::endl;
      return name;
    }
    case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG1: {
      const auto* desc = static_cast<const D3D12_RAYTRACING_PIPELINE_CONFIG1*>(subobject.pDesc);
      const std::string name = varPrefix + "_raytracingPipelineConfig1";
      ss << "D3D12_RAYTRACING_PIPELINE_CONFIG1 " << name << " = {};" << std::endl;
      ss << name << ".MaxTraceRecursionDepth = " << desc->MaxTraceRecursionDepth << ";"
         << std::endl;
      ss << name << ".Flags = " << toStr(desc->Flags) << ";" << std::endl;
      return name;
    }
    default:
      GITS_ASSERT(false, "Unhandled D3D12 state subobject type for CCode");
      return varPrefix + "_unsupported";
    }
  };

  const std::string subobjectsName = "subobjects_" + info.name;
  std::vector<std::string> subobjectDescVars;
  subobjectDescVars.reserve(value.NumSubobjects);

  if (value.NumSubobjects > 0 && value.pSubobjects != nullptr) {
    for (UINT i = 0; i < value.NumSubobjects; ++i) {
      const std::string varPrefix = info.name + "_sub" + std::to_string(i);
      subobjectDescVars.push_back(appendStateSubobjectDesc(ss, varPrefix, value.pSubobjects[i], i));
    }

    ss << "D3D12_STATE_SUBOBJECT " << subobjectsName << "[" << value.NumSubobjects << "];"
       << std::endl;
    for (UINT i = 0; i < value.NumSubobjects; ++i) {
      ss << subobjectsName << "[" << i << "].Type = " << toStr(value.pSubobjects[i].Type) << ";"
         << std::endl;
      ss << subobjectsName << "[" << i << "].pDesc = &" << subobjectDescVars[i] << ";" << std::endl;
    }

    for (UINT i = 0; i < value.NumSubobjects; ++i) {
      if (value.pSubobjects[i].Type !=
          D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION) {
        continue;
      }
      const auto* desc =
          static_cast<const D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(value.pSubobjects[i].pDesc);
      for (UINT j = 0; j < value.NumSubobjects; ++j) {
        if (desc->pSubobjectToAssociate == &value.pSubobjects[j]) {
          ss << subobjectDescVars[i] << ".pSubobjectToAssociate = &" << subobjectsName << "[" << j
             << "];" << std::endl;
          break;
        }
      }
    }
  }

  ss << info.type << " " << info.name << " = {};" << std::endl;
  ss << info.name << ".Type = " << toStr(value.Type) << ";" << std::endl;

  if (value.NumSubobjects > 0) {
    ss << info.name << ".NumSubobjects = " << value.NumSubobjects << ";" << std::endl;
    ss << info.name << ".pSubobjects = " << subobjectsName << ";" << std::endl;
  } else {
    ss << info.name << ".NumSubobjects = 0;" << std::endl;
    ss << info.name << ".pSubobjects = nullptr;" << std::endl;
  }

  out.initialization = ss.str();
  out.value = info.name;
  out.decorator = "&";
}

void argumentToCpp(PointerArgument<D3D12_DISPATCH_RAYS_DESC>& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  if (!info.isPtr && isZeroInitialized(arg.Value)) {
    out.initialization = "";
    out.value = "{}";
    out.decorator = "";
    return;
  }

  const D3D12_DISPATCH_RAYS_DESC& value = *arg.Value;

  std::ostringstream ss;

  CppParameterOutput rayGenerationShaderRecordOut;
  rayGenerationShaderRecordOut.value = "rayGenerationShaderRecord";
  {
    const std::string& field = rayGenerationShaderRecordOut.value;
    ss << "D3D12_GPU_VIRTUAL_ADDRESS_RANGE " << field << " = {};" << std::endl;
    ss << field << ".StartAddress = "
       << gpuAddressStr(arg.RayGenerationShaderRecordKey, arg.RayGenerationShaderRecordOffset)
       << ";" << std::endl;
    ss << field << ".SizeInBytes = " << value.RayGenerationShaderRecord.SizeInBytes << ";"
       << std::endl;
  }

  auto appendGpuVirtualAddressRangeAndStride = [&](const std::string& field, unsigned key,
                                                   unsigned offset, UINT64 sizeInBytes,
                                                   UINT strideInBytes) {
    ss << "D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE " << field << " = {};" << std::endl;
    ss << field << ".StartAddress = " << gpuAddressStr(key, offset) << ";" << std::endl;
    ss << field << ".SizeInBytes = " << sizeInBytes << ";" << std::endl;
    ss << field << ".StrideInBytes = " << strideInBytes << ";" << std::endl;
  };

  CppParameterOutput missShaderTableOut;
  missShaderTableOut.value = "missShaderTable";
  appendGpuVirtualAddressRangeAndStride(
      missShaderTableOut.value, arg.MissShaderTableKey, arg.MissShaderTableOffset,
      value.MissShaderTable.SizeInBytes, value.MissShaderTable.StrideInBytes);

  CppParameterOutput hitGroupTableOut;
  hitGroupTableOut.value = "hitGroupTable";
  appendGpuVirtualAddressRangeAndStride(hitGroupTableOut.value, arg.HitGroupTableKey,
                                        arg.HitGroupTableOffset, value.HitGroupTable.SizeInBytes,
                                        value.HitGroupTable.StrideInBytes);

  CppParameterOutput callableShaderTableOut;
  callableShaderTableOut.value = "callableShaderTable";
  appendGpuVirtualAddressRangeAndStride(
      callableShaderTableOut.value, arg.CallableShaderTableKey, arg.CallableShaderTableOffset,
      value.CallableShaderTable.SizeInBytes, value.CallableShaderTable.StrideInBytes);

  std::string name = info.getIndexedName();

  if (!info.isArrayElement) {
    ss << info.type << " " << name << " = {};" << std::endl;
  }
  ss << name << ".RayGenerationShaderRecord = " << rayGenerationShaderRecordOut.decorator
     << rayGenerationShaderRecordOut.value << ";" << std::endl;
  ss << name << ".MissShaderTable = " << missShaderTableOut.decorator << missShaderTableOut.value
     << ";" << std::endl;
  ss << name << ".HitGroupTable = " << hitGroupTableOut.decorator << hitGroupTableOut.value << ";"
     << std::endl;
  ss << name << ".CallableShaderTable = " << callableShaderTableOut.decorator
     << callableShaderTableOut.value << ";" << std::endl;
  ss << name << ".Width = " << value.Width << ";" << std::endl;
  ss << name << ".Height = " << value.Height << ";" << std::endl;
  ss << name << ".Depth = " << value.Depth << ";" << std::endl;

  out.initialization = ss.str();
  out.value = std::move(name);
  out.decorator = "&";
}

void argumentToCpp(D3D12_SHADER_RESOURCE_VIEW_DESC_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  if (!arg.Value) {
    out.initialization = "";
    out.value = "nullptr";
    out.decorator = "";
    return;
  }

  auto& value = *arg.Value;
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
    paramInfo.type = "D3D12_RAYTRACING_ACCELERATION_STRUCTURE_SRV";
    paramInfo.name = "RaytracingAccelerationStructure";
    paramOut.initialization = "";
    paramOut.value = paramInfo.name;
    ss << paramInfo.type << " " << paramInfo.name << " = {};" << std::endl;
    ss << paramInfo.name
       << ".Location = " << gpuAddressStr(arg.RaytracingLocationKey, arg.RaytracingLocationOffset)
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
  if (!arg.Value || arg.Size == 0) {
    out.initialization = "";
    out.value = "nullptr";
    out.decorator = "";
    return;
  }

  std::ostringstream ss;
  ss << info.type << " " << info.name << "[" << arg.Size << "] = {};" << std::endl;
  for (size_t i = 0; i < arg.Size; ++i) {
    // D3D12_RESIDENCY_PRIORITY is an enum but the values can be arbitrary
    ss << info.name << "[" << i << "] = D3D12_RESIDENCY_PRIORITY(" << std::to_string(arg.Value[i])
       << ");" << std::endl;
  }

  out.initialization = ss.str();
  out.value = info.name;
  out.decorator = "";
}
void argumentToCpp(D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  if (!arg.Value || arg.Size == 0) {
    out.initialization = "";
    out.value = "nullptr";
    out.decorator = "";
    return;
  }

  std::ostringstream ss;
  ss << info.type << " " << info.name << "[" << arg.Size << "] = {};" << std::endl;

  for (unsigned i = 0, resolveKeyIndex = 0; i < arg.Size; ++i) {
    ss << info.name << "[" << i << "].cpuDescriptor.ptr = "
       << descriptorHeapHandleStr(arg.DescriptorKeys[i], arg.DescriptorIndexes[i], "CpuHandle")
       << ";" << std::endl;

    CppParameterInfo beginningAccessInfo("D3D12_RENDER_PASS_BEGINNING_ACCESS",
                                         info.name + "_beginningAccess" + std::to_string(i));
    CppParameterOutput beginningAccessOut;
    toCpp(arg.Value[i].BeginningAccess, beginningAccessInfo, beginningAccessOut);
    ss << beginningAccessOut.initialization;
    ss << info.name << "[" << i << "].BeginningAccess = " << beginningAccessOut.value << ";"
       << std::endl;

    const std::string endingAccessVar = info.name + "_endingAccess" + std::to_string(i);
    unsigned srcResourceKey = 0;
    unsigned dstResourceKey = 0;
    if (arg.Value[i].EndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE) {
      GITS_ASSERT(resolveKeyIndex < arg.ResolveSrcResourceKeys.size() &&
                  resolveKeyIndex < arg.ResolveDstResourceKeys.size());
      srcResourceKey = arg.ResolveSrcResourceKeys[resolveKeyIndex];
      dstResourceKey = arg.ResolveDstResourceKeys[resolveKeyIndex];
      ++resolveKeyIndex;
    }
    appendRenderPassEndingAccess(ss, arg.Value[i].EndingAccess, endingAccessVar, srcResourceKey,
                                 dstResourceKey);
    ss << info.name << "[" << i << "].EndingAccess = " << endingAccessVar << ";" << std::endl;
  }

  out.initialization = ss.str();
  out.value = info.name;
  out.decorator = "";
}
void argumentToCpp(D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  if (!arg.Value) {
    out.initialization = "";
    out.value = "nullptr";
    out.decorator = "";
    return;
  }

  std::ostringstream ss;
  ss << info.type << " " << info.name << " = {};" << std::endl;
  ss << info.name << ".cpuDescriptor.ptr = "
     << descriptorHeapHandleStr(arg.DescriptorKey, arg.DescriptorIndex, "CpuHandle") << ";"
     << std::endl;

  CppParameterInfo depthBeginningAccessInfo("D3D12_RENDER_PASS_BEGINNING_ACCESS",
                                            info.name + "_depthBeginningAccess");
  CppParameterOutput depthBeginningAccessOut;
  toCpp(arg.Value->DepthBeginningAccess, depthBeginningAccessInfo, depthBeginningAccessOut);
  ss << depthBeginningAccessOut.initialization;
  ss << info.name << ".DepthBeginningAccess = " << depthBeginningAccessOut.value << ";"
     << std::endl;

  CppParameterInfo stencilBeginningAccessInfo("D3D12_RENDER_PASS_BEGINNING_ACCESS",
                                              info.name + "_stencilBeginningAccess");
  CppParameterOutput stencilBeginningAccessOut;
  toCpp(arg.Value->StencilBeginningAccess, stencilBeginningAccessInfo, stencilBeginningAccessOut);
  ss << stencilBeginningAccessOut.initialization;
  ss << info.name << ".StencilBeginningAccess = " << stencilBeginningAccessOut.value << ";"
     << std::endl;

  const std::string depthEndingAccessVar = info.name + "_depthEndingAccess";
  const unsigned depthSrcKey =
      arg.Value->DepthEndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE
          ? arg.ResolveSrcDepthKey
          : 0u;
  const unsigned depthDstKey =
      arg.Value->DepthEndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE
          ? arg.ResolveDstDepthKey
          : 0u;
  appendRenderPassEndingAccess(ss, arg.Value->DepthEndingAccess, depthEndingAccessVar, depthSrcKey,
                               depthDstKey);
  ss << info.name << ".DepthEndingAccess = " << depthEndingAccessVar << ";" << std::endl;

  const std::string stencilEndingAccessVar = info.name + "_stencilEndingAccess";
  const unsigned stencilSrcKey =
      arg.Value->StencilEndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE
          ? arg.ResolveSrcStencilKey
          : 0u;
  const unsigned stencilDstKey =
      arg.Value->StencilEndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE
          ? arg.ResolveDstStencilKey
          : 0u;
  appendRenderPassEndingAccess(ss, arg.Value->StencilEndingAccess, stencilEndingAccessVar,
                               stencilSrcKey, stencilDstKey);
  ss << info.name << ".StencilEndingAccess = " << stencilEndingAccessVar << ";" << std::endl;

  out.initialization = ss.str();
  out.value = info.name;
  out.decorator = "&";
}
void argumentToCpp(D3D12_EXTENSION_ARGUMENTS_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  GITS_ASSERT(false, "argumentToCpp not implemented for D3D12_EXTENSION_ARGUMENTS_Argument");
}
void argumentToCpp(D3D12_EXTENDED_OPERATION_DATA_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  GITS_ASSERT(false, "argumentToCpp not implemented for D3D12_EXTENDED_OPERATION_DATA_Argument");
}
void argumentToCpp(PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS>& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  if (!arg.Value) {
    out.initialization = "";
    out.value = "nullptr";
    out.decorator = "";
    return;
  }

  std::ostringstream ss;
  const std::string name = info.getIndexedName();
  appendD3D12BuildRaytracingInputs(ss, name, *arg.Value, arg.InputKeys, arg.InputOffsets);

  out.initialization = ss.str();
  out.value = name;
  out.decorator = info.isPtr ? "&" : "";
}

void argumentToCpp(PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC>& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  if (!info.isPtr && isZeroInitialized(arg.Value)) {
    out.initialization = "";
    out.value = "{}";
    out.decorator = "";
    return;
  }

  const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC& value = *arg.Value;
  std::ostringstream ss;

  CppParameterOutput destAccelerationStructureDataOut;
  destAccelerationStructureDataOut.value = "destAccelerationStructureData";
  ss << "D3D12_GPU_VIRTUAL_ADDRESS " << destAccelerationStructureDataOut.value << " = "
     << gpuAddressStr(arg.DestAccelerationStructureKey, arg.DestAccelerationStructureOffset) << ";"
     << std::endl;

  CppParameterOutput inputsOut;
  inputsOut.value = "inputs";
  appendD3D12BuildRaytracingInputs(ss, inputsOut.value, value.Inputs, arg.InputKeys,
                                   arg.InputOffsets);

  CppParameterOutput sourceAccelerationStructureDataOut;
  sourceAccelerationStructureDataOut.value = "sourceAccelerationStructureData";
  ss << "D3D12_GPU_VIRTUAL_ADDRESS " << sourceAccelerationStructureDataOut.value << " = "
     << gpuAddressStr(arg.SourceAccelerationStructureKey, arg.SourceAccelerationStructureOffset)
     << ";" << std::endl;

  CppParameterOutput scratchAccelerationStructureDataOut;
  scratchAccelerationStructureDataOut.value = "scratchAccelerationStructureData";
  ss << "D3D12_GPU_VIRTUAL_ADDRESS " << scratchAccelerationStructureDataOut.value << " = "
     << gpuAddressStr(arg.ScratchAccelerationStructureKey, arg.ScratchAccelerationStructureOffset)
     << ";" << std::endl;

  std::string name = info.getIndexedName();

  if (!info.isArrayElement) {
    ss << info.type << " " << name << " = {};" << std::endl;
  }
  ss << name << ".DestAccelerationStructureData = " << destAccelerationStructureDataOut.decorator
     << destAccelerationStructureDataOut.value << ";" << std::endl;
  ss << name << ".Inputs = " << inputsOut.decorator << inputsOut.value << ";" << std::endl;
  ss << name
     << ".SourceAccelerationStructureData = " << sourceAccelerationStructureDataOut.decorator
     << sourceAccelerationStructureDataOut.value << ";" << std::endl;
  ss << name
     << ".ScratchAccelerationStructureData = " << scratchAccelerationStructureDataOut.decorator
     << scratchAccelerationStructureDataOut.value << ";" << std::endl;

  out.initialization = ss.str();
  out.value = std::move(name);
  out.decorator = "&";
}
void argumentToCpp(ArrayArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  if (!arg.Value || arg.Size == 0) {
    out.initialization = "";
    out.value = "nullptr";
    out.decorator = "";
    return;
  }

  std::ostringstream ss;
  ss << info.type << " " << info.name << "[" << arg.Size << "] = {};" << std::endl;
  for (size_t i = 0; i < arg.Size; ++i) {
    const std::string elem = info.name + "_" + std::to_string(i);
    ss << info.type << " " << elem << " = {};" << std::endl;
    appendPostbuildInfoDescElement(ss, elem, arg.DestBufferKeys[i], arg.DestBufferOffsets[i],
                                   arg.Value[i].InfoType);
    ss << info.name << "[" << i << "] = " << elem << ";" << std::endl;
  }

  out.initialization = ss.str();
  out.value = info.name;
  out.decorator = "";
}

void argumentToCpp(
    PointerArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>& arg,
    CppParameterInfo& info,
    CppParameterOutput& out) {
  if (!arg.Value) {
    out.initialization = "";
    out.value = "nullptr";
    out.decorator = "";
    return;
  }

  std::ostringstream ss;
  const std::string name = info.getIndexedName();
  if (!info.isArrayElement) {
    ss << info.type << " " << name << " = {};" << std::endl;
  }
  appendPostbuildInfoDescElement(ss, name, arg.destBufferKey, arg.destBufferOffset,
                                 arg.Value->InfoType);

  out.initialization = ss.str();
  out.value = name;
  out.decorator = info.isPtr ? "&" : "";
}
void argumentToCpp(D3D12_BARRIER_GROUPs_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  if (!arg.Value || arg.Size == 0) {
    out.initialization = "";
    out.value = "nullptr";
    out.decorator = "";
    return;
  }

  std::ostringstream ss;
  ss << info.type << " " << info.name << "[" << arg.Size << "] = {};" << std::endl;

  unsigned resourceKeyIndex = 0;
  for (unsigned i = 0; i < arg.Size; ++i) {
    const D3D12_BARRIER_GROUP& group = arg.Value[i];
    const std::string groupIndex = std::to_string(i);

    ss << info.name << "[" << i << "].Type = " << toStr(group.Type) << ";" << std::endl;
    ss << info.name << "[" << i << "].NumBarriers = " << group.NumBarriers << ";" << std::endl;

    if (group.NumBarriers == 0) {
      if (group.Type == D3D12_BARRIER_TYPE_GLOBAL) {
        ss << info.name << "[" << i << "].pGlobalBarriers = nullptr;" << std::endl;
      } else if (group.Type == D3D12_BARRIER_TYPE_TEXTURE) {
        ss << info.name << "[" << i << "].pTextureBarriers = nullptr;" << std::endl;
      } else if (group.Type == D3D12_BARRIER_TYPE_BUFFER) {
        ss << info.name << "[" << i << "].pBufferBarriers = nullptr;" << std::endl;
      }
      continue;
    }

    if (group.Type == D3D12_BARRIER_TYPE_GLOBAL) {
      const std::string barriersName = info.name + "_globalBarriers" + groupIndex;
      ss << "D3D12_GLOBAL_BARRIER " << barriersName << "[" << group.NumBarriers << "] = {};"
         << std::endl;
      for (unsigned j = 0; j < group.NumBarriers; ++j) {
        const D3D12_GLOBAL_BARRIER& barrier = group.pGlobalBarriers[j];
        ss << barriersName << "[" << j << "].SyncBefore = " << toStr(barrier.SyncBefore) << ";"
           << std::endl;
        ss << barriersName << "[" << j << "].SyncAfter = " << toStr(barrier.SyncAfter) << ";"
           << std::endl;
        ss << barriersName << "[" << j << "].AccessBefore = " << toStr(barrier.AccessBefore) << ";"
           << std::endl;
        ss << barriersName << "[" << j << "].AccessAfter = " << toStr(barrier.AccessAfter) << ";"
           << std::endl;
      }
      ss << info.name << "[" << i << "].pGlobalBarriers = " << barriersName << ";" << std::endl;
    } else if (group.Type == D3D12_BARRIER_TYPE_TEXTURE) {
      const std::string barriersName = info.name + "_textureBarriers" + groupIndex;
      ss << "D3D12_TEXTURE_BARRIER " << barriersName << "[" << group.NumBarriers << "] = {};"
         << std::endl;
      for (unsigned j = 0; j < group.NumBarriers; ++j) {
        const D3D12_TEXTURE_BARRIER& barrier = group.pTextureBarriers[j];
        ss << barriersName << "[" << j << "].SyncBefore = " << toStr(barrier.SyncBefore) << ";"
           << std::endl;
        ss << barriersName << "[" << j << "].SyncAfter = " << toStr(barrier.SyncAfter) << ";"
           << std::endl;
        ss << barriersName << "[" << j << "].AccessBefore = " << toStr(barrier.AccessBefore) << ";"
           << std::endl;
        ss << barriersName << "[" << j << "].AccessAfter = " << toStr(barrier.AccessAfter) << ";"
           << std::endl;
        ss << barriersName << "[" << j << "].LayoutBefore = " << toStr(barrier.LayoutBefore) << ";"
           << std::endl;
        ss << barriersName << "[" << j << "].LayoutAfter = " << toStr(barrier.LayoutAfter) << ";"
           << std::endl;
        ss << barriersName << "[" << j
           << "].pResource = " << objKeyToPtrStr(arg.ResourceKeys[resourceKeyIndex++]) << ";"
           << std::endl;
        {
          CppParameterInfo groupElementInfo(info.type, info.name);
          groupElementInfo.isArrayElement = true;
          groupElementInfo.index = i;
          CppParameterInfo textureBarrierInfo("D3D12_TEXTURE_BARRIER", "textureBarrier",
                                              groupElementInfo);
          textureBarrierInfo.isArrayElement = true;
          textureBarrierInfo.index = j;
          CppParameterInfo subresourcesInfo("D3D12_BARRIER_SUBRESOURCE_RANGE", "Subresources",
                                            textureBarrierInfo);
          CppParameterOutput subresourcesOut;
          toCpp(barrier.Subresources, subresourcesInfo, subresourcesOut);
          ss << subresourcesOut.initialization;
          ss << barriersName << "[" << j << "].Subresources = " << subresourcesOut.decorator
             << subresourcesOut.value << ";" << std::endl;
        }
        ss << barriersName << "[" << j << "].Flags = " << toStr(barrier.Flags) << ";" << std::endl;
      }
      ss << info.name << "[" << i << "].pTextureBarriers = " << barriersName << ";" << std::endl;
    } else if (group.Type == D3D12_BARRIER_TYPE_BUFFER) {
      const std::string barriersName = info.name + "_bufferBarriers" + groupIndex;
      ss << "D3D12_BUFFER_BARRIER " << barriersName << "[" << group.NumBarriers << "] = {};"
         << std::endl;
      for (unsigned j = 0; j < group.NumBarriers; ++j) {
        const D3D12_BUFFER_BARRIER& barrier = group.pBufferBarriers[j];
        ss << barriersName << "[" << j << "].SyncBefore = " << toStr(barrier.SyncBefore) << ";"
           << std::endl;
        ss << barriersName << "[" << j << "].SyncAfter = " << toStr(barrier.SyncAfter) << ";"
           << std::endl;
        ss << barriersName << "[" << j << "].AccessBefore = " << toStr(barrier.AccessBefore) << ";"
           << std::endl;
        ss << barriersName << "[" << j << "].AccessAfter = " << toStr(barrier.AccessAfter) << ";"
           << std::endl;
        ss << barriersName << "[" << j
           << "].pResource = " << objKeyToPtrStr(arg.ResourceKeys[resourceKeyIndex++]) << ";"
           << std::endl;
        ss << barriersName << "[" << j << "].Offset = " << barrier.Offset << ";" << std::endl;
        ss << barriersName << "[" << j << "].Size = " << barrier.Size << ";" << std::endl;
      }
      ss << info.name << "[" << i << "].pBufferBarriers = " << barriersName << ";" << std::endl;
    } else {
      GITS_ASSERT(false, "Unknown D3D12_BARRIER_GROUP type");
    }
  }

  out.initialization = ss.str();
  out.value = info.name;
  out.decorator = "";
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
  if (!arg.Value) {
    out.initialization = "";
    out.value = "nullptr";
    out.decorator = "";
    return;
  }
  const auto& value = *arg.Value;

  std::ostringstream ss;
  ss << info.type << " " << info.name << " = {};" << std::endl;
  ss << info.name << ".outputResolution.x = " << value.outputResolution.x << ";" << std::endl;
  ss << info.name << ".outputResolution.y = " << value.outputResolution.y << ";" << std::endl;
  ss << info.name << ".qualitySetting = " << toStr(value.qualitySetting) << ";" << std::endl;
  ss << info.name << ".initFlags = " << value.initFlags << ";" << std::endl;
  ss << info.name << ".creationNodeMask = " << value.creationNodeMask << ";" << std::endl;
  ss << info.name << ".visibleNodeMask = " << value.visibleNodeMask << ";" << std::endl;
  if (arg.TempBufferHeapKey) {
    ss << info.name << ".pTempBufferHeap = " << objKeyToPtrStr(arg.TempBufferHeapKey) << ";"
       << std::endl;
  }
  ss << info.name << ".bufferHeapOffset = " << value.bufferHeapOffset << ";" << std::endl;
  if (arg.TempTextureHeapKey) {
    ss << info.name << ".pTempTextureHeap = " << objKeyToPtrStr(arg.TempTextureHeapKey) << ";"
       << std::endl;
  }
  ss << info.name << ".textureHeapOffset = " << value.textureHeapOffset << ";" << std::endl;
  if (arg.PipelineLibraryKey) {
    ss << info.name << ".pPipelineLibrary = " << objKeyToPtrStr(arg.PipelineLibraryKey) << ";"
       << std::endl;
  }

  out.initialization = ss.str();
  out.value = info.name;
  out.decorator = "&";
}

void argumentToCpp(xess_d3d12_execute_params_t_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  if (!arg.Value) {
    out.initialization = "";
    out.value = "nullptr";
    out.decorator = "";
    return;
  }
  const auto& value = *arg.Value;

  std::ostringstream ss;
  ss << info.type << " " << info.name << " = {};" << std::endl;
  if (arg.ColorTextureKey) {
    ss << info.name << ".pColorTexture = " << objKeyToPtrStr(arg.ColorTextureKey) << ";"
       << std::endl;
  }
  if (arg.VelocityTextureKey) {
    ss << info.name << ".pVelocityTexture = " << objKeyToPtrStr(arg.VelocityTextureKey) << ";"
       << std::endl;
  }
  if (arg.DepthTextureKey) {
    ss << info.name << ".pDepthTexture = " << objKeyToPtrStr(arg.DepthTextureKey) << ";"
       << std::endl;
  }
  if (arg.ExposureScaleTextureKey) {
    ss << info.name << ".pExposureScaleTexture = " << objKeyToPtrStr(arg.ExposureScaleTextureKey)
       << ";" << std::endl;
  }
  if (arg.ResponsivePixelMaskTextureKey) {
    ss << info.name
       << ".pResponsivePixelMaskTexture = " << objKeyToPtrStr(arg.ResponsivePixelMaskTextureKey)
       << ";" << std::endl;
  }
  if (arg.OutputTextureKey) {
    ss << info.name << ".pOutputTexture = " << objKeyToPtrStr(arg.OutputTextureKey) << ";"
       << std::endl;
  }
  ss << info.name << ".jitterOffsetX = " << toStr(value.jitterOffsetX) << ";" << std::endl;
  ss << info.name << ".jitterOffsetY = " << toStr(value.jitterOffsetY) << ";" << std::endl;
  ss << info.name << ".exposureScale = " << toStr(value.exposureScale) << ";" << std::endl;
  ss << info.name << ".resetHistory = " << (value.resetHistory ? "true" : "false") << ";"
     << std::endl;
  ss << info.name << ".inputWidth = " << value.inputWidth << ";" << std::endl;
  ss << info.name << ".inputHeight = " << value.inputHeight << ";" << std::endl;
  ss << info.name << ".inputColorBase.x = " << value.inputColorBase.x << ";" << std::endl;
  ss << info.name << ".inputColorBase.y = " << value.inputColorBase.y << ";" << std::endl;
  ss << info.name << ".inputMotionVectorBase.x = " << value.inputMotionVectorBase.x << ";"
     << std::endl;
  ss << info.name << ".inputMotionVectorBase.y = " << value.inputMotionVectorBase.y << ";"
     << std::endl;
  ss << info.name << ".inputDepthBase.x = " << value.inputDepthBase.x << ";" << std::endl;
  ss << info.name << ".inputDepthBase.y = " << value.inputDepthBase.y << ";" << std::endl;
  ss << info.name << ".inputResponsiveMaskBase.x = " << value.inputResponsiveMaskBase.x << ";"
     << std::endl;
  ss << info.name << ".inputResponsiveMaskBase.y = " << value.inputResponsiveMaskBase.y << ";"
     << std::endl;
  ss << info.name << ".reserved0.x = " << value.reserved0.x << ";" << std::endl;
  ss << info.name << ".reserved0.y = " << value.reserved0.y << ";" << std::endl;
  ss << info.name << ".outputColorBase.x = " << value.outputColorBase.x << ";" << std::endl;
  ss << info.name << ".outputColorBase.y = " << value.outputColorBase.y << ";" << std::endl;
  if (arg.DescriptorHeapKey) {
    ss << info.name << ".pDescriptorHeap = " << objKeyToPtrStr(arg.DescriptorHeapKey) << ";"
       << std::endl;
  }
  ss << info.name << ".descriptorHeapOffset = " << value.descriptorHeapOffset << ";" << std::endl;

  out.initialization = ss.str();
  out.value = info.name;
  out.decorator = "&";
}

void argumentToCpp(DML_CheckFeatureSupport_BufferArgument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  GITS_ASSERT(false, "argumentToCpp not implemented for DML_CheckFeatureSupport_BufferArgument");
}

void argumentToCpp(xell_frame_report_t_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  if (!arg.Value) {
    out.initialization = "";
    out.value = "nullptr";
    out.decorator = "";
    return;
  }

  std::ostringstream ss;
  ss << info.type << " " << info.name << "[" << arg.FRAME_REPORTS_COUNT << "] = {};" << std::endl;
  for (size_t i = 0; i < arg.FRAME_REPORTS_COUNT; ++i) {
    ss << info.name << "[" << i << "].m_frame_id = " << arg.Value[i].m_frame_id << ";" << std::endl;
    ss << info.name << "[" << i << "].m_sim_start_ts = " << arg.Value[i].m_sim_start_ts << ";"
       << std::endl;
    ss << info.name << "[" << i << "].m_sim_end_ts = " << arg.Value[i].m_sim_end_ts << ";"
       << std::endl;
    ss << info.name << "[" << i
       << "].m_render_submit_start_ts = " << arg.Value[i].m_render_submit_start_ts << ";"
       << std::endl;
    ss << info.name << "[" << i
       << "].m_render_submit_end_ts = " << arg.Value[i].m_render_submit_end_ts << ";" << std::endl;
    ss << info.name << "[" << i << "].m_present_start_ts = " << arg.Value[i].m_present_start_ts
       << ";" << std::endl;
    ss << info.name << "[" << i << "].m_present_end_ts = " << arg.Value[i].m_present_end_ts << ";"
       << std::endl;
  }

  out.initialization = ss.str();
  out.value = info.name;
  out.decorator = "";
}

void argumentToCpp(xefg_swapchain_d3d12_init_params_t_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  if (!arg.Value) {
    out.initialization = "";
    out.value = "nullptr";
    out.decorator = "";
    return;
  }
  const auto& value = *arg.Value;

  std::ostringstream ss;
  ss << info.type << " " << info.name << " = {};" << std::endl;
  if (arg.ApplicationSwapChainKey) {
    ss << info.name << ".pApplicationSwapChain = " << objKeyToPtrStr(arg.ApplicationSwapChainKey)
       << ";" << std::endl;
  }
  ss << info.name << ".initFlags = " << value.initFlags << ";" << std::endl;
  ss << info.name << ".maxInterpolatedFrames = " << value.maxInterpolatedFrames << ";" << std::endl;
  ss << info.name << ".creationNodeMask = " << value.creationNodeMask << ";" << std::endl;
  ss << info.name << ".visibleNodeMask = " << value.visibleNodeMask << ";" << std::endl;
  if (arg.TempBufferHeapKey) {
    ss << info.name << ".pTempBufferHeap = " << objKeyToPtrStr(arg.TempBufferHeapKey) << ";"
       << std::endl;
  }
  ss << info.name << ".bufferHeapOffset = " << value.bufferHeapOffset << ";" << std::endl;
  if (arg.TempTextureHeapKey) {
    ss << info.name << ".pTempTextureHeap = " << objKeyToPtrStr(arg.TempTextureHeapKey) << ";"
       << std::endl;
  }
  ss << info.name << ".textureHeapOffset = " << value.textureHeapOffset << ";" << std::endl;
  if (arg.PipelineLibraryKey) {
    ss << info.name << ".pPipelineLibrary = " << objKeyToPtrStr(arg.PipelineLibraryKey) << ";"
       << std::endl;
  }
  ss << info.name << ".uiMode = " << toStr(value.uiMode) << ";" << std::endl;

  out.initialization = ss.str();
  out.value = info.name;
  out.decorator = "&";
}

void argumentToCpp(xefg_swapchain_d3d12_resource_data_t_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  if (!arg.Value) {
    out.initialization = "";
    out.value = "nullptr";
    out.decorator = "";
    return;
  }
  const auto& value = *arg.Value;

  std::ostringstream ss;
  ss << info.type << " " << info.name << " = {};" << std::endl;
  ss << info.name << ".type = " << toStr(value.type) << ";" << std::endl;
  ss << info.name << ".validity = " << toStr(value.validity) << ";" << std::endl;
  ss << info.name << ".resourceBase.x = " << value.resourceBase.x << ";" << std::endl;
  ss << info.name << ".resourceBase.y = " << value.resourceBase.y << ";" << std::endl;
  ss << info.name << ".resourceSize.x = " << value.resourceSize.x << ";" << std::endl;
  ss << info.name << ".resourceSize.y = " << value.resourceSize.y << ";" << std::endl;
  if (arg.ResourceKey) {
    ss << info.name << ".pResource = " << objKeyToPtrStr(arg.ResourceKey) << ";" << std::endl;
  }
  ss << info.name << ".incomingState = " << toStr(value.incomingState) << ";" << std::endl;

  out.initialization = ss.str();
  out.value = info.name;
  out.decorator = "&";
}

void argumentToCpp(DSTORAGE_QUEUE_DESC_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  if (!arg.Value) {
    out.initialization = "";
    out.value = "nullptr";
    out.decorator = "";
    return;
  }
  const auto& value = *arg.Value;

  std::ostringstream ss;
  ss << info.type << " " << info.name << " = {};" << std::endl;
  ss << info.name << ".SourceType = " << toStr(value.SourceType) << ";" << std::endl;
  ss << info.name << ".Capacity = " << value.Capacity << ";" << std::endl;
  ss << info.name << ".Priority = " << toStr(value.Priority) << ";" << std::endl;
  ss << info.name << ".Name = " << charPtrToCpp(value.Name) << ";" << std::endl;
  if (arg.DeviceKey) {
    ss << info.name << ".Device = static_cast<ID3D12Device*>(" << objKeyToPtrStr(arg.DeviceKey)
       << ");" << std::endl;
  }

  out.initialization = ss.str();
  out.value = info.name;
  out.decorator = "&";
}

void argumentToCpp(DSTORAGE_REQUEST_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  if (!arg.Value) {
    out.initialization = "";
    out.value = "nullptr";
    out.decorator = "";
    return;
  }

  const auto& value = *arg.Value;
  const std::string req = info.name;
  std::ostringstream ss;
  ss << info.type << " " << req << " = {};" << std::endl;

  ss << req << ".Options.SourceType = " << toStr(value.Options.SourceType) << ";" << std::endl;
  ss << req << ".Options.DestinationType = " << toStr(value.Options.DestinationType) << ";"
     << std::endl;
  ss << req << ".Options.CompressionFormat = "
     << enumFlagsToCpp(toStr(value.Options.CompressionFormat), "DSTORAGE_COMPRESSION_FORMAT") << ";"
     << std::endl;

  const auto voidPtrFromCapture = [](const void* ptr) {
    return ptr ? "reinterpret_cast<void*>(" + toHex(reinterpret_cast<uintptr_t>(ptr)) + "ULL)"
               : "nullptr";
  };
  const auto constVoidPtrFromCapture = [](const void* ptr) {
    return ptr ? "reinterpret_cast<void const*>(" + toHex(reinterpret_cast<uintptr_t>(ptr)) + "ULL)"
               : "nullptr";
  };

  switch (value.Options.SourceType) {
  case DSTORAGE_REQUEST_SOURCE_MEMORY:
    ss << req << ".Source.Memory.Source = " << constVoidPtrFromCapture(value.Source.Memory.Source)
       << ";" << std::endl;
    ss << req << ".Source.Memory.Size = " << value.Source.Memory.Size << ";" << std::endl;
    break;
  case DSTORAGE_REQUEST_SOURCE_FILE:
    if (arg.FileKey) {
      ss << req << ".Source.File.Source = static_cast<IDStorageFile*>("
         << objKeyToPtrStr(arg.FileKey) << ");" << std::endl;
    }
    ss << req << ".Source.File.Offset = " << arg.NewOffset << "ULL;" << std::endl;
    ss << req << ".Source.File.Size = " << value.Source.File.Size << ";" << std::endl;
    break;
  default:
    break;
  }

  switch (value.Options.DestinationType) {
  case DSTORAGE_REQUEST_DESTINATION_MEMORY:
    ss << req
       << ".Destination.Memory.Buffer = " << voidPtrFromCapture(value.Destination.Memory.Buffer)
       << ";" << std::endl;
    ss << req << ".Destination.Memory.Size = " << value.Destination.Memory.Size << ";" << std::endl;
    break;
  case DSTORAGE_REQUEST_DESTINATION_BUFFER:
    if (arg.ResourceKey) {
      ss << req << ".Destination.Buffer.Resource = static_cast<ID3D12Resource*>("
         << objKeyToPtrStr(arg.ResourceKey) << ");" << std::endl;
    }
    ss << req << ".Destination.Buffer.Offset = " << value.Destination.Buffer.Offset << "ULL;"
       << std::endl;
    ss << req << ".Destination.Buffer.Size = " << value.Destination.Buffer.Size << ";" << std::endl;
    break;
  case DSTORAGE_REQUEST_DESTINATION_TEXTURE_REGION: {
    if (arg.ResourceKey) {
      ss << req << ".Destination.Texture.Resource = static_cast<ID3D12Resource*>("
         << objKeyToPtrStr(arg.ResourceKey) << ");" << std::endl;
    }
    ss << req
       << ".Destination.Texture.SubresourceIndex = " << value.Destination.Texture.SubresourceIndex
       << ";" << std::endl;
    const auto& region = value.Destination.Texture.Region;
    ss << req << ".Destination.Texture.Region.left = " << region.left << ";" << std::endl;
    ss << req << ".Destination.Texture.Region.top = " << region.top << ";" << std::endl;
    ss << req << ".Destination.Texture.Region.front = " << region.front << ";" << std::endl;
    ss << req << ".Destination.Texture.Region.right = " << region.right << ";" << std::endl;
    ss << req << ".Destination.Texture.Region.bottom = " << region.bottom << ";" << std::endl;
    ss << req << ".Destination.Texture.Region.back = " << region.back << ";" << std::endl;
    break;
  }
  case DSTORAGE_REQUEST_DESTINATION_MULTIPLE_SUBRESOURCES:
    if (arg.ResourceKey) {
      ss << req << ".Destination.MultipleSubresources.Resource = static_cast<ID3D12Resource*>("
         << objKeyToPtrStr(arg.ResourceKey) << ");" << std::endl;
    }
    ss << req << ".Destination.MultipleSubresources.FirstSubresource = "
       << value.Destination.MultipleSubresources.FirstSubresource << ";" << std::endl;
    break;
  case DSTORAGE_REQUEST_DESTINATION_TILES: {
    if (arg.ResourceKey) {
      ss << req << ".Destination.Tiles.Resource = static_cast<ID3D12Resource*>("
         << objKeyToPtrStr(arg.ResourceKey) << ");" << std::endl;
    }
    const auto& start = value.Destination.Tiles.TiledRegionStartCoordinate;
    ss << req << ".Destination.Tiles.TiledRegionStartCoordinate.X = " << start.X << ";"
       << std::endl;
    ss << req << ".Destination.Tiles.TiledRegionStartCoordinate.Y = " << start.Y << ";"
       << std::endl;
    ss << req << ".Destination.Tiles.TiledRegionStartCoordinate.Z = " << start.Z << ";"
       << std::endl;
    const auto& tileSize = value.Destination.Tiles.TileRegionSize;
    ss << req << ".Destination.Tiles.TileRegionSize.NumTiles = " << tileSize.NumTiles << ";"
       << std::endl;
    ss << req
       << ".Destination.Tiles.TileRegionSize.UseBox = " << (tileSize.UseBox ? "TRUE" : "FALSE")
       << ";" << std::endl;
    ss << req << ".Destination.Tiles.TileRegionSize.Width = " << tileSize.Width << ";" << std::endl;
    ss << req << ".Destination.Tiles.TileRegionSize.Height = " << tileSize.Height << ";"
       << std::endl;
    ss << req << ".Destination.Tiles.TileRegionSize.Depth = " << tileSize.Depth << ";" << std::endl;
    break;
  }
  default:
    break;
  }

  ss << req << ".UncompressedSize = " << value.UncompressedSize << "ULL;" << std::endl;
  ss << req << ".CancellationTag = " << value.CancellationTag << "ULL;" << std::endl;
  ss << req << ".Name = " << charPtrToCpp(value.Name) << ";" << std::endl;

  out.initialization = ss.str();
  out.value = req;
  out.decorator = "&";
}

void argumentToCpp(PointerArgument<NVAPI_D3D12_SET_CREATE_PIPELINE_STATE_OPTIONS_PARAMS>& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  if (!arg.Value) {
    out.initialization = "";
    out.value = "nullptr";
    out.decorator = "";
    return;
  }

  const std::string name = info.getIndexedName();
  std::ostringstream ss;
  ss << info.type << " " << name << " = {};" << std::endl;
  ss << name << ".version = " << arg.Value->version << ";" << std::endl;
  const auto flags = static_cast<NVAPI_D3D12_PIPELINE_CREATION_STATE_FLAGS>(arg.Value->flags);
  ss << name << ".flags = static_cast<NvU32>("
     << enumFlagsToCpp(toStr(flags), "NVAPI_D3D12_PIPELINE_CREATION_STATE_FLAGS") << ");"
     << std::endl;

  out.initialization = ss.str();
  out.value = name;
  out.decorator = info.isPtr ? "&" : "";
}

void argumentToCpp(PointerArgument<NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS>& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  if (!arg.Value) {
    out.initialization = "";
    out.value = "nullptr";
    out.decorator = "";
    return;
  }

  std::ostringstream ss;

  auto appendNvapiRaytracingGeometryDescEx =
      [&](const std::string& prefix, const NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX& desc,
          const std::vector<unsigned>& inputKeys, const std::vector<unsigned>& inputOffsets,
          unsigned inputIndex) -> unsigned {
    ss << prefix << ".type = " << toStr(desc.type) << ";" << std::endl;
    ss << prefix << ".flags = " << toStr(desc.flags) << ";" << std::endl;

    if (inputKeys.empty()) {
      return inputIndex;
    }

    if (desc.type == D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES) {
      ss << prefix << ".triangles.IndexFormat = " << toStr(desc.triangles.IndexFormat) << ";"
         << std::endl;
      ss << prefix << ".triangles.VertexFormat = " << toStr(desc.triangles.VertexFormat) << ";"
         << std::endl;
      ss << prefix << ".triangles.IndexCount = " << desc.triangles.IndexCount << ";" << std::endl;
      ss << prefix << ".triangles.VertexCount = " << desc.triangles.VertexCount << ";" << std::endl;
      ss << prefix << ".triangles.Transform3x4 = "
         << gpuAddressStr(inputKeys[inputIndex], inputOffsets[inputIndex]) << ";" << std::endl;
      ++inputIndex;
      ss << prefix << ".triangles.IndexBuffer = "
         << gpuAddressStr(inputKeys[inputIndex], inputOffsets[inputIndex]) << ";" << std::endl;
      ++inputIndex;
      appendGpuVirtualAddress(ss, prefix + ".triangles.VertexBuffer.StartAddress",
                              inputKeys[inputIndex], inputOffsets[inputIndex]);
      ++inputIndex;
      ss << prefix
         << ".triangles.VertexBuffer.StrideInBytes = " << desc.triangles.VertexBuffer.StrideInBytes
         << ";" << std::endl;
    } else if (desc.type == D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS) {
      ss << prefix << ".aabbs.AABBCount = " << desc.aabbs.AABBCount << ";" << std::endl;
      appendGpuVirtualAddress(ss, prefix + ".aabbs.AABBs.StartAddress", inputKeys[inputIndex],
                              inputOffsets[inputIndex]);
      ++inputIndex;
      ss << prefix << ".aabbs.AABBs.StrideInBytes = " << desc.aabbs.AABBs.StrideInBytes << ";"
         << std::endl;
    } else if (desc.type == NVAPI_D3D12_RAYTRACING_GEOMETRY_TYPE_OMM_TRIANGLES_EX) {
      const auto& tri = desc.ommTriangles.triangles;
      ss << prefix << ".ommTriangles.triangles.IndexFormat = " << toStr(tri.IndexFormat) << ";"
         << std::endl;
      ss << prefix << ".ommTriangles.triangles.VertexFormat = " << toStr(tri.VertexFormat) << ";"
         << std::endl;
      ss << prefix << ".ommTriangles.triangles.IndexCount = " << tri.IndexCount << ";" << std::endl;
      ss << prefix << ".ommTriangles.triangles.VertexCount = " << tri.VertexCount << ";"
         << std::endl;
      ss << prefix << ".ommTriangles.triangles.Transform3x4 = "
         << gpuAddressStr(inputKeys[inputIndex], inputOffsets[inputIndex]) << ";" << std::endl;
      ++inputIndex;
      ss << prefix << ".ommTriangles.triangles.IndexBuffer = "
         << gpuAddressStr(inputKeys[inputIndex], inputOffsets[inputIndex]) << ";" << std::endl;
      ++inputIndex;
      appendGpuVirtualAddress(ss, prefix + ".ommTriangles.triangles.VertexBuffer.StartAddress",
                              inputKeys[inputIndex], inputOffsets[inputIndex]);
      ++inputIndex;
      ss << prefix << ".ommTriangles.triangles.VertexBuffer.StrideInBytes = "
         << tri.VertexBuffer.StrideInBytes << ";" << std::endl;
      const auto& omm = desc.ommTriangles.ommAttachment;
      appendGpuVirtualAddress(
          ss, prefix + ".ommTriangles.ommAttachment.opacityMicromapIndexBuffer.StartAddress",
          inputKeys[inputIndex], inputOffsets[inputIndex]);
      ++inputIndex;
      ss << prefix << ".ommTriangles.ommAttachment.opacityMicromapIndexBuffer.StrideInBytes = "
         << omm.opacityMicromapIndexBuffer.StrideInBytes << ";" << std::endl;
      ss << prefix << ".ommTriangles.ommAttachment.opacityMicromapIndexFormat = "
         << toStr(omm.opacityMicromapIndexFormat) << ";" << std::endl;
      ss << prefix << ".ommTriangles.ommAttachment.opacityMicromapBaseLocation = "
         << omm.opacityMicromapBaseLocation << ";" << std::endl;
      ss << prefix << ".ommTriangles.ommAttachment.opacityMicromapArray = "
         << gpuAddressStr(inputKeys[inputIndex], inputOffsets[inputIndex]) << ";" << std::endl;
      ++inputIndex;
      ss << prefix << ".ommTriangles.ommAttachment.numOMMUsageCounts = " << omm.numOMMUsageCounts
         << ";" << std::endl;
      if (omm.numOMMUsageCounts > 0 && omm.pOMMUsageCounts) {
        const std::string usageArray = prefix + "_ommUsageCounts";
        ss << "NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_USAGE_COUNT " << usageArray << "["
           << omm.numOMMUsageCounts << "] = {};" << std::endl;
        for (NvU32 j = 0; j < omm.numOMMUsageCounts; ++j) {
          ss << usageArray << "[" << j << "].count = " << omm.pOMMUsageCounts[j].count << ";"
             << std::endl;
          ss << usageArray << "[" << j
             << "].subdivisionLevel = " << omm.pOMMUsageCounts[j].subdivisionLevel << ";"
             << std::endl;
          ss << usageArray << "[" << j << "].format = " << toStr(omm.pOMMUsageCounts[j].format)
             << ";" << std::endl;
        }
        ss << prefix << ".ommTriangles.ommAttachment.pOMMUsageCounts = " << usageArray << ";"
           << std::endl;
      }
    } else {
      GITS_ASSERT(false, "appendNvapiRaytracingGeometryDescEx: unsupported geometry type");
    }
    return inputIndex;
  };

  const std::string buildDescVar = info.name + "_buildDesc";
  ss << "NVAPI_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC_EX " << buildDescVar << " = {};"
     << std::endl;
  ss << buildDescVar << ".destAccelerationStructureData = "
     << gpuAddressStr(arg.DestAccelerationStructureKey, arg.DestAccelerationStructureOffset) << ";"
     << std::endl;

  const std::string inputsVar = buildDescVar + "_inputs";
  {
    const auto& inputs = arg.Value->pDesc->inputs;
    const auto& inputKeys = arg.InputKeys;
    const auto& inputOffsets = arg.InputOffsets;

    ss << "NVAPI_D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS_EX " << inputsVar << " = {};"
       << std::endl;
    ss << inputsVar << ".type = " << toStr(inputs.type) << ";" << std::endl;
    ss << inputsVar << ".flags = "
       << enumFlagsToCpp(toStr(inputs.flags),
                         "NVAPI_D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS_EX")
       << ";" << std::endl;
    ss << inputsVar << ".numDescs = " << inputs.numDescs << ";" << std::endl;
    ss << inputsVar << ".descsLayout = " << toStr(inputs.descsLayout) << ";" << std::endl;
    const bool emitCompactGeometryDescArray =
        inputs.type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL &&
        inputs.numDescs > 0 && inputs.descsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY;
    if (!emitCompactGeometryDescArray) {
      ss << inputsVar << ".geometryDescStrideInBytes = " << inputs.geometryDescStrideInBytes << ";"
         << std::endl;
    }

    if (!inputKeys.empty() &&
        inputs.type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
      ss << inputsVar << ".instanceDescs = " << gpuAddressStr(inputKeys[0], inputOffsets[0]) << ";"
         << std::endl;
    } else if (inputs.type == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL &&
               inputs.numDescs > 0) {
      const std::string geoArray = inputsVar + "_geometryDescs";
      ss << "NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX " << geoArray << "[" << inputs.numDescs
         << "] = {};" << std::endl;
      unsigned inputIndex = 0;
      for (unsigned i = 0; i < inputs.numDescs; ++i) {
        const NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX& desc =
            inputs.descsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY
                ? *(NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX*)((char*)(inputs.pGeometryDescs) +
                                                              inputs.geometryDescStrideInBytes * i)
                : *inputs.ppGeometryDescs[i];
        const std::string elem = geoArray + "_" + std::to_string(i);
        ss << "NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX " << elem << " = {};" << std::endl;
        inputIndex =
            appendNvapiRaytracingGeometryDescEx(elem, desc, inputKeys, inputOffsets, inputIndex);
        ss << geoArray << "[" << i << "] = " << elem << ";" << std::endl;
      }
      if (inputs.descsLayout == D3D12_ELEMENTS_LAYOUT_ARRAY) {
        ss << inputsVar << ".pGeometryDescs = " << geoArray << ";" << std::endl;
        ss << inputsVar
           << ".geometryDescStrideInBytes = sizeof(NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX);"
           << std::endl;
      } else {
        const std::string ptrArray = inputsVar + "_geometryDescPtrs";
        ss << "NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX* " << ptrArray << "[" << inputs.numDescs
           << "];" << std::endl;
        for (unsigned i = 0; i < inputs.numDescs; ++i) {
          ss << ptrArray << "[" << i << "] = &" << geoArray << "[" << i << "];" << std::endl;
        }
        ss << inputsVar << ".ppGeometryDescs = " << ptrArray << ";" << std::endl;
      }
    }
  }
  ss << buildDescVar << ".inputs = " << inputsVar << ";" << std::endl;

  ss << buildDescVar << ".sourceAccelerationStructureData = "
     << gpuAddressStr(arg.SourceAccelerationStructureKey, arg.SourceAccelerationStructureOffset)
     << ";" << std::endl;
  ss << buildDescVar << ".scratchAccelerationStructureData = "
     << gpuAddressStr(arg.ScratchAccelerationStructureKey, arg.ScratchAccelerationStructureOffset)
     << ";" << std::endl;

  const std::string postbuildArray = info.name + "_postbuildInfoDescs";
  if (arg.Value->numPostbuildInfoDescs > 0 && arg.Value->pPostbuildInfoDescs) {
    const NvU32 count = arg.Value->numPostbuildInfoDescs;
    const auto* descs = arg.Value->pPostbuildInfoDescs;
    const auto& destKeys = arg.DestPostBuildBufferKeys;
    const auto& destOffsets = arg.DestPostBuildBufferOffsets;
    const std::string& arrayName = postbuildArray;
    ss << "D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC " << arrayName << "["
       << count << "] = {};" << std::endl;
    for (NvU32 i = 0; i < count; ++i) {
      const std::string elem = arrayName + "_" + std::to_string(i);
      ss << "D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC " << elem << " = {};"
         << std::endl;
      appendPostbuildInfoDescElement(ss, elem, destKeys[i], destOffsets[i], descs[i].InfoType);
      ss << arrayName << "[" << i << "] = " << elem << ";" << std::endl;
    }
  }

  std::string name = info.getIndexedName();
  if (!info.isArrayElement) {
    ss << info.type << " " << name << " = {};" << std::endl;
  }
  ss << name << ".version = " << arg.Value->version << ";" << std::endl;
  ss << name << ".pDesc = &" << buildDescVar << ";" << std::endl;
  ss << name << ".numPostbuildInfoDescs = " << arg.Value->numPostbuildInfoDescs << ";" << std::endl;
  if (arg.Value->numPostbuildInfoDescs > 0 && arg.Value->pPostbuildInfoDescs) {
    ss << name << ".pPostbuildInfoDescs = " << postbuildArray << ";" << std::endl;
  } else {
    ss << name << ".pPostbuildInfoDescs = nullptr;" << std::endl;
  }

  out.initialization = ss.str();
  out.value = std::move(name);
  out.decorator = info.isPtr ? "&" : "";
}

void argumentToCpp(PointerArgument<NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS>& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  if (!arg.Value) {
    out.initialization = "";
    out.value = "nullptr";
    out.decorator = "";
    return;
  }

  std::ostringstream ss;
  const std::string descVar = info.name + "_desc";
  ss << "NVAPI_D3D12_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_DESC " << descVar << " = {};"
     << std::endl;
  ss << descVar << ".destOpacityMicromapArrayData = "
     << gpuAddressStr(arg.DestOpacityMicromapArrayDataKey, arg.DestOpacityMicromapArrayDataOffset)
     << ";" << std::endl;

  const std::string inputsVar = descVar + "_inputs";
  const auto& inputs = arg.Value->pDesc->inputs;
  ss << "NVAPI_D3D12_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_INPUTS " << inputsVar << " = {};"
     << std::endl;
  ss << inputsVar << ".flags = "
     << enumFlagsToCpp(toStr(inputs.flags),
                       "NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_BUILD_FLAGS")
     << ";" << std::endl;
  ss << inputsVar << ".numOMMUsageCounts = " << inputs.numOMMUsageCounts << ";" << std::endl;
  if (inputs.numOMMUsageCounts > 0 && inputs.pOMMUsageCounts) {
    const std::string usageArray = inputsVar + "_usageCounts";
    ss << "NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_USAGE_COUNT " << usageArray << "["
       << inputs.numOMMUsageCounts << "] = {};" << std::endl;
    for (NvU32 i = 0; i < inputs.numOMMUsageCounts; ++i) {
      ss << usageArray << "[" << i << "].count = " << inputs.pOMMUsageCounts[i].count << ";"
         << std::endl;
      ss << usageArray << "[" << i
         << "].subdivisionLevel = " << inputs.pOMMUsageCounts[i].subdivisionLevel << ";"
         << std::endl;
      ss << usageArray << "[" << i << "].format = " << toStr(inputs.pOMMUsageCounts[i].format)
         << ";" << std::endl;
    }
    ss << inputsVar << ".pOMMUsageCounts = " << usageArray << ";" << std::endl;
  }
  appendGpuVirtualAddress(ss, inputsVar + ".inputBuffer", arg.InputBufferKey,
                          arg.InputBufferOffset);
  appendGpuVirtualAddress(ss, inputsVar + ".perOMMDescs.StartAddress", arg.PerOMMDescsKey,
                          arg.PerOMMDescsOffset);
  ss << inputsVar << ".perOMMDescs.StrideInBytes = " << inputs.perOMMDescs.StrideInBytes << ";"
     << std::endl;
  ss << descVar << ".inputs = " << inputsVar << ";" << std::endl;

  ss << descVar << ".scratchOpacityMicromapArrayData = "
     << gpuAddressStr(arg.ScratchOpacityMicromapArrayDataKey,
                      arg.ScratchOpacityMicromapArrayDataOffset)
     << ";" << std::endl;

  const std::string postbuildArray = info.name + "_postbuildInfoDescs";
  if (arg.Value->numPostbuildInfoDescs > 0 && arg.Value->pPostbuildInfoDescs) {
    const NvU32 count = arg.Value->numPostbuildInfoDescs;
    const auto* descs = arg.Value->pPostbuildInfoDescs;
    const auto& destKeys = arg.DestPostBuildBufferKeys;
    const auto& destOffsets = arg.DestPostBuildBufferOffsets;
    ss << "NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_POSTBUILD_INFO_DESC " << postbuildArray
       << "[" << count << "] = {};" << std::endl;
    for (NvU32 i = 0; i < count; ++i) {
      const std::string elem = postbuildArray + "_" + std::to_string(i);
      ss << "NVAPI_D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_POSTBUILD_INFO_DESC " << elem << " = {};"
         << std::endl;
      ss << elem << ".destBuffer = " << gpuAddressStr(destKeys[i], destOffsets[i]) << ";"
         << std::endl;
      ss << elem << ".infoType = " << toStr(descs[i].infoType) << ";" << std::endl;
      ss << postbuildArray << "[" << i << "] = " << elem << ";" << std::endl;
    }
  }

  std::string name = info.getIndexedName();
  if (!info.isArrayElement) {
    ss << info.type << " " << name << " = {};" << std::endl;
  }
  ss << name << ".version = " << arg.Value->version << ";" << std::endl;
  ss << name << ".pDesc = &" << descVar << ";" << std::endl;
  ss << name << ".numPostbuildInfoDescs = " << arg.Value->numPostbuildInfoDescs << ";" << std::endl;
  if (arg.Value->numPostbuildInfoDescs > 0 && arg.Value->pPostbuildInfoDescs) {
    ss << name << ".pPostbuildInfoDescs = " << postbuildArray << ";" << std::endl;
  } else {
    ss << name << ".pPostbuildInfoDescs = nullptr;" << std::endl;
  }

  out.initialization = ss.str();
  out.value = std::move(name);
  out.decorator = info.isPtr ? "&" : "";
}

void argumentToCpp(
    PointerArgument<NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS>& arg,
    CppParameterInfo& info,
    CppParameterOutput& out) {
  if (!arg.Value) {
    out.initialization = "";
    out.value = "nullptr";
    out.decorator = "";
    return;
  }

  const auto& inputs = arg.Value->pDesc->inputs;
  std::ostringstream ss;
  const std::string descVar = info.name + "_desc";
  ss << "NVAPI_D3D12_RAYTRACING_MULTI_INDIRECT_CLUSTER_OPERATION_DESC " << descVar << " = {};"
     << std::endl;

  const std::string inputsVar = descVar + "_inputs";
  ss << "NVAPI_D3D12_RAYTRACING_MULTI_INDIRECT_CLUSTER_OPERATION_INPUTS " << inputsVar << " = {};"
     << std::endl;
  ss << inputsVar << ".maxArgCount = " << inputs.maxArgCount << ";" << std::endl;
  ss << inputsVar << ".flags = "
     << enumFlagsToCpp(toStr(inputs.flags),
                       "NVAPI_D3D12_RAYTRACING_MULTI_INDIRECT_CLUSTER_OPERATION_FLAGS")
     << ";" << std::endl;
  ss << inputsVar << ".type = " << toStr(inputs.type) << ";" << std::endl;
  ss << inputsVar << ".mode = " << toStr(inputs.mode) << ";" << std::endl;
  if (inputs.type ==
      NVAPI_D3D12_RAYTRACING_MULTI_INDIRECT_CLUSTER_OPERATION_TYPE_BUILD_BLAS_FROM_CLAS) {
    ss << inputsVar << ".clasDesc.maxTotalClasCount = " << inputs.clasDesc.maxTotalClasCount << ";"
       << std::endl;
    ss << inputsVar << ".clasDesc.maxClasCountPerArg = " << inputs.clasDesc.maxClasCountPerArg
       << ";" << std::endl;
  } else if (inputs.type ==
             NVAPI_D3D12_RAYTRACING_MULTI_INDIRECT_CLUSTER_OPERATION_TYPE_MOVE_CLUSTER_OBJECT) {
    ss << inputsVar << ".movesDesc.type = " << toStr(inputs.movesDesc.type) << ";" << std::endl;
    ss << inputsVar << ".movesDesc.maxBytesMoved = " << inputs.movesDesc.maxBytesMoved << ";"
       << std::endl;
  } else {
    ss << inputsVar << ".trianglesDesc.vertexFormat = " << toStr(inputs.trianglesDesc.vertexFormat)
       << ";" << std::endl;
    ss << inputsVar
       << ".trianglesDesc.maxGeometryIndexValue = " << inputs.trianglesDesc.maxGeometryIndexValue
       << ";" << std::endl;
    ss << inputsVar << ".trianglesDesc.maxUniqueGeometryCountPerArg = "
       << inputs.trianglesDesc.maxUniqueGeometryCountPerArg << ";" << std::endl;
    ss << inputsVar
       << ".trianglesDesc.maxTriangleCountPerArg = " << inputs.trianglesDesc.maxTriangleCountPerArg
       << ";" << std::endl;
    ss << inputsVar
       << ".trianglesDesc.maxVertexCountPerArg = " << inputs.trianglesDesc.maxVertexCountPerArg
       << ";" << std::endl;
    ss << inputsVar
       << ".trianglesDesc.maxTotalTriangleCount = " << inputs.trianglesDesc.maxTotalTriangleCount
       << ";" << std::endl;
    ss << inputsVar
       << ".trianglesDesc.maxTotalVertexCount = " << inputs.trianglesDesc.maxTotalVertexCount << ";"
       << std::endl;
    ss << inputsVar << ".trianglesDesc.minPositionTruncateBitCount = "
       << inputs.trianglesDesc.minPositionTruncateBitCount << ";" << std::endl;
  }
  ss << descVar << ".inputs = " << inputsVar << ";" << std::endl;
  ss << descVar << ".addressResolutionFlags = "
     << enumFlagsToCpp(
            toStr(arg.Value->pDesc->addressResolutionFlags),
            "NVAPI_D3D12_RAYTRACING_MULTI_INDIRECT_CLUSTER_OPERATION_ADDRESS_RESOLUTION_FLAGS")
     << ";" << std::endl;

  appendGpuVirtualAddress(ss, descVar + ".batchResultData", arg.BatchResultDataKey,
                          arg.BatchResultDataOffset);

  appendGpuVirtualAddress(ss, descVar + ".batchScratchData", arg.BatchScratchDataKey,
                          arg.BatchScratchDataOffset);

  appendGpuVirtualAddress(ss, descVar + ".destinationAddressArray.StartAddress",
                          arg.DestinationAddressArrayKey, arg.DestinationAddressArrayOffset);
  ss << descVar << ".destinationAddressArray.StrideInBytes = "
     << arg.Value->pDesc->destinationAddressArray.StrideInBytes << ";" << std::endl;

  appendGpuVirtualAddress(ss, descVar + ".resultSizeArray.StartAddress", arg.ResultSizeArrayKey,
                          arg.ResultSizeArrayOffset);
  ss << descVar
     << ".resultSizeArray.StrideInBytes = " << arg.Value->pDesc->resultSizeArray.StrideInBytes
     << ";" << std::endl;

  appendGpuVirtualAddress(ss, descVar + ".indirectArgArray.StartAddress", arg.IndirectArgArrayKey,
                          arg.IndirectArgArrayOffset);
  ss << descVar
     << ".indirectArgArray.StrideInBytes = " << arg.Value->pDesc->indirectArgArray.StrideInBytes
     << ";" << std::endl;

  ss << descVar
     << ".indirectArgCount = " << gpuAddressStr(arg.IndirectArgCountKey, arg.IndirectArgCountOffset)
     << ";" << std::endl;

  std::string name = info.getIndexedName();
  if (!info.isArrayElement) {
    ss << info.type << " " << name << " = {};" << std::endl;
  }
  ss << name << ".version = " << arg.Value->version << ";" << std::endl;
  ss << name << ".pDesc = &" << descVar << ";" << std::endl;

  out.initialization = ss.str();
  out.value = std::move(name);
  out.decorator = info.isPtr ? "&" : "";
}

void argumentToCpp(Argument<D3D12StateObjectFunc>& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  GITS_ASSERT(false, "argumentToCpp not implemented for "
                     "Argument<D3D12StateObjectFunc>");
}

void argumentToCpp(Argument<D3D12ApplicationDescFunc>& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  GITS_ASSERT(false, "argumentToCpp not implemented for "
                     "Argument<D3D12ApplicationDescFunc>");
}

void argumentToCpp(Argument<D3D12PipelineStateFunc>& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  GITS_ASSERT(false, "argumentToCpp not implemented for "
                     "Argument<D3D12PipelineStateFunc>");
}

void argumentToCpp(PointerArgument<INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC>& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {

  GITS_ASSERT(arg.Value != nullptr);
  auto& value = *arg.Value;

  std::ostringstream ss;

  // Initialize the pD3D12Desc member first by creating a local argument
  CppParameterInfo pD3D12DescInfo("D3D12_COMPUTE_PIPELINE_STATE_DESC", "pD3D12Desc");
  pD3D12DescInfo.isPtr = true;
  CppParameterOutput pD3D12DescOut;
  {
    D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument pD3D12DescArg(value.pD3D12Desc);
    pD3D12DescArg.RootSignatureKey = arg.RootSignatureKey;
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
