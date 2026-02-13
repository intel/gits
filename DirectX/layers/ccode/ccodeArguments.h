// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "arguments.h"
#include "directx.h"
#include "ccodeStructsAuto.h"
#include "ccodeStream.h"
#include "ccodeTypes.h"
#include "keyUtils.h"

#include <ostream>
#include <sstream>
#include <string>

namespace gits {
namespace DirectX {
namespace ccode {

template <template <typename> typename Arg, typename T>
void argumentToCpp(Arg<T>& arg, CppParameterInfo& info, CppParameterOutput& out) {
  toCpp(arg.value, info, out);
}

template <typename T>
void argumentToCpp(PointerArgument<T>& arg, CppParameterInfo& info, CppParameterOutput& out) {
  toCpp(arg.value, info, out);
}

template <typename T, int N>
void argumentToCpp(StaticArrayArgument<T, N>& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  GITS_ASSERT(N == info.size.value());
  toCpp(arg.value, info, out);
}

template <typename T>
void argumentToCpp(ArrayArgument<T>& arg, CppParameterInfo& info, CppParameterOutput& out) {
  toCpp(arg.value, info, out);
}

// Template for interface arguments
template <typename T>
void argumentToCpp(InterfaceArgument<T>& arg, CppParameterInfo& info, CppParameterOutput& out) {
  out.initialization = "";
  out.decorator = "";
  if (arg.value) {
    out.value = "g_" + objKeyToStr(arg.key) + ".Get()";
  } else {
    out.value = "nullptr";
  }
}

template <typename T>
void argumentToCpp(InterfaceOutputArgument<T>& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  static std::unordered_set<unsigned> s_declaredKeys;
  auto& stream = ccode::CCodeStream::getInstance();

  out.initialization = "";
  out.decorator = "";

  std::string objName = "";
  if (arg.key != 0) {
    objName = "g_" + objKeyToStr(arg.key);
    auto type = stream.getInterfaceName(arg.key);
    if ((info.type == type) || (info.type == "void")) {
      out.value = objName;
      out.decorator = "&";
    } else {
      // Needed for interface output arguments with specific types (i.e. CreateSwapChainForHwnd)
      out.value = objName + ".GetAddressOf()";
      out.decorator = "(" + info.type + "**)";
    }
  } else {
    out.value = "nullptr";
    return;
  }

  // Write out the declaration (only for new interfaces)
  auto result = s_declaredKeys.insert(arg.key);
  if (result.second) {
    GITS_ASSERT(!objName.empty());
    auto iidStr = stream.getInterfaceName(arg.key);
    // Do not use ID3D12CommandList as it is an abstract interface
    if (iidStr == "ID3D12CommandList") {
      iidStr = "ID3D12GraphicsCommandList7";
    }

    auto& ss = stream.getObjectsHeader();
    ss << std::endl;
    ss << "// " << objKeyToStr(arg.key) << std::endl;
    ss << "inline constexpr unsigned " << objKeyToStr(arg.key) << " = " << arg.key << ";"
       << std::endl;
    ss << "inline ComPtr<" << iidStr << "> " << objName << " = nullptr;" << std::endl;
  }
}

template <typename T>
void argumentToCpp(InterfaceArrayArgument<T>& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out) {
  std::ostringstream ss;
  ss << info.type << "* " << info.name << "[" << arg.size << "];" << std::endl;
  for (unsigned i = 0; i < arg.size; ++i) {
    if (arg.value[i]) {
      ss << info.name << "[" << i << "] = " << objKeyToPtrStr(arg.keys[i]) << ";" << std::endl;
    } else {
      ss << info.name << "[" << i << "] = nullptr;" << std::endl;
    }
  }
  out.initialization = ss.str();
  out.value = info.name;
  out.decorator = "";
}

template <typename T>
void argumentToCpp(ContextArgument<T>& arg, CppParameterInfo& info, CppParameterOutput& out) {
  // empty for now
}

template <typename T>
void argumentToCpp(ContextOutputArgument<T>& arg, CppParameterInfo& info, CppParameterOutput& out) {
  // empty for now
}

// Overloads for specific argument types
void argumentToCpp(Argument<IID>& arg, CppParameterInfo& info, CppParameterOutput& out);
void argumentToCpp(BufferArgument& arg, CppParameterInfo& info, CppParameterOutput& out);
void argumentToCpp(OutputBufferArgument& arg, CppParameterInfo& info, CppParameterOutput& out);
void argumentToCpp(ShaderIdentifierArgument& arg, CppParameterInfo& info, CppParameterOutput& out);
void argumentToCpp(DescriptorHandleArrayArgument<D3D12_CPU_DESCRIPTOR_HANDLE>& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out);
void argumentToCpp(DescriptorHandleArgument<D3D12_CPU_DESCRIPTOR_HANDLE>& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out);
void argumentToCpp(DescriptorHandleArgument<D3D12_GPU_DESCRIPTOR_HANDLE>& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out);
void argumentToCpp(LPCWSTR_Argument& arg, CppParameterInfo& info, CppParameterOutput& out);
void argumentToCpp(LPCSTR_Argument& arg, CppParameterInfo& info, CppParameterOutput& out);
void argumentToCpp(D3D12_GPU_VIRTUAL_ADDRESS_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out);
void argumentToCpp(D3D12_GPU_VIRTUAL_ADDRESSs_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out);
void argumentToCpp(D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out);
void argumentToCpp(D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out);
void argumentToCpp(D3D12_TEXTURE_COPY_LOCATION_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out);
void argumentToCpp(D3D12_RESOURCE_BARRIERs_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out);
void argumentToCpp(D3D12_INDEX_BUFFER_VIEW_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out);
void argumentToCpp(D3D12_CONSTANT_BUFFER_VIEW_DESC_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out);
void argumentToCpp(D3D12_VERTEX_BUFFER_VIEWs_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out);
void argumentToCpp(D3D12_STREAM_OUTPUT_BUFFER_VIEWs_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out);
void argumentToCpp(D3D12_WRITEBUFFERIMMEDIATE_PARAMETERs_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out);
void argumentToCpp(D3D12_PIPELINE_STATE_STREAM_DESC_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out);
void argumentToCpp(D3D12_STATE_OBJECT_DESC_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out);
void argumentToCpp(D3D12_SHADER_RESOURCE_VIEW_DESC_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out);
void argumentToCpp(ArrayArgument<D3D12_RESIDENCY_PRIORITY>& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out);
void argumentToCpp(D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out);
void argumentToCpp(D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out);
void argumentToCpp(PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC>& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out);
void argumentToCpp(ArrayArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out);
void printArgument(
    CCodeStream& ccodeStream,
    PointerArgument<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC>& arg,
    CppParameterInfo& info,
    CppParameterOutput& out);
void argumentToCpp(D3D12_BARRIER_GROUPs_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out);
void argumentToCpp(DML_BINDING_TABLE_DESC_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out);
void argumentToCpp(DML_GRAPH_DESC_Argument& arg, CppParameterInfo& info, CppParameterOutput& out);
void argumentToCpp(DML_BINDING_DESC_Argument& arg, CppParameterInfo& info, CppParameterOutput& out);
void argumentToCpp(DML_BINDING_DESCs_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out);
void argumentToCpp(DML_OPERATOR_DESC_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out);
void argumentToCpp(xess_d3d12_init_params_t_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out);
void argumentToCpp(xess_d3d12_execute_params_t_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out);
void argumentToCpp(DML_CheckFeatureSupport_BufferArgument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out);
void argumentToCpp(DSTORAGE_QUEUE_DESC_Argument& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out);
void argumentToCpp(DSTORAGE_REQUEST_Argument& arg, CppParameterInfo& info, CppParameterOutput& out);
void argumentToCpp(PointerArgument<NVAPI_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_EX_PARAMS>& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out);
void argumentToCpp(PointerArgument<NVAPI_BUILD_RAYTRACING_OPACITY_MICROMAP_ARRAY_PARAMS>& arg,
                   CppParameterInfo& info,
                   CppParameterOutput& out);
void argumentToCpp(
    PointerArgument<NVAPI_RAYTRACING_EXECUTE_MULTI_INDIRECT_CLUSTER_OPERATION_PARAMS>& arg,
    CppParameterInfo& info,
    CppParameterOutput& out);

} // namespace ccode
} // namespace DirectX
} // namespace gits
