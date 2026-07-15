// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "pipelineStateStreamDescDump.h"
#include "to_string/toStr.h"
#include "to_string/enumToStrAuto.h"

#include <d3dx12/d3dx12_pipeline_state_stream.h>

namespace gits {
namespace DirectX {

void DumpPipelineStateStreamDesc(const D3D12_PIPELINE_STATE_STREAM_DESC_Argument& arg,
                                 std::ofstream& stream) {
  if (!arg.Value) {
    return;
  }
  stream << "D3D12_PIPELINE_STATE_STREAM_DESC\n";
  size_t offset{};
  while (offset < arg.Value->SizeInBytes) {
    void* data = static_cast<char*>(arg.Value->pPipelineStateSubobjectStream) + offset;
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE subobjectType =
        *reinterpret_cast<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE*>(data);
    switch (subobjectType) {
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE:
      stream << "\tRootSignature O" << arg.RootSignatureKey << "\n";
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE);
      break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS: {
      D3D12_SHADER_BYTECODE& desc = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_VS*>(data);
      stream << "\tVS BytecodeLength " << desc.BytecodeLength << "\n";
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_VS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS: {
      D3D12_SHADER_BYTECODE& desc = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_PS*>(data);
      stream << "\tPS BytecodeLength " << desc.BytecodeLength << "\n";
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_PS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DS: {
      D3D12_SHADER_BYTECODE& desc = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_DS*>(data);
      stream << "\tDS BytecodeLength " << desc.BytecodeLength << "\n";
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_DS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_HS: {
      D3D12_SHADER_BYTECODE& desc = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_HS*>(data);
      stream << "\tHS BytecodeLength " << desc.BytecodeLength << "\n";
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_HS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_GS: {
      D3D12_SHADER_BYTECODE& desc = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_GS*>(data);
      stream << "\tGS BytecodeLength " << desc.BytecodeLength << "\n";
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_GS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS: {
      D3D12_SHADER_BYTECODE& desc = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_CS*>(data);
      stream << "\tCS BytecodeLength " << desc.BytecodeLength << "\n";
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_CS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS: {
      D3D12_SHADER_BYTECODE& desc = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_AS*>(data);
      stream << "\tAS BytecodeLength " << desc.BytecodeLength << "\n";
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_AS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS: {
      D3D12_SHADER_BYTECODE& desc = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_MS*>(data);
      stream << "\tMS BytecodeLength " << desc.BytecodeLength << "\n";
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_MS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_STREAM_OUTPUT: {
      D3D12_STREAM_OUTPUT_DESC& desc =
          *static_cast<CD3DX12_PIPELINE_STATE_STREAM_STREAM_OUTPUT*>(data);
      stream << "\tStreamOutput\n";
      for (unsigned i = 0; i < desc.NumEntries; ++i) {
        const D3D12_SO_DECLARATION_ENTRY& entry = desc.pSODeclaration[i];
        stream << "\t\tEntry[" << i << "]\n";
        stream << "\t\t\tStream " << entry.Stream << "\n";
        stream << "\t\t\tSemanticName " << entry.SemanticName << "\n";
        stream << "\t\t\tSemanticIndex " << entry.SemanticIndex << "\n";
        stream << "\t\t\tStartComponent " << static_cast<unsigned>(entry.StartComponent) << "\n";
        stream << "\t\t\tComponentCount " << static_cast<unsigned>(entry.ComponentCount) << "\n";
        stream << "\t\t\tOutputSlot " << static_cast<unsigned>(entry.OutputSlot) << "\n";
      }
      stream << "\tBufferStrides " << desc.NumStrides;
      for (unsigned i = 0; i < desc.NumStrides; ++i) {
        stream << " " << desc.pBufferStrides[i];
      }
      stream << "\n";
      stream << "\t\tRasterizedStream " << desc.RasterizedStream << "\n";
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_STREAM_OUTPUT);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND: {
      D3D12_BLEND_DESC& desc = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC*>(data);
      stream << "\tBlendState\n";
      stream << "\t\tAlphaToCoverageEnable " << (desc.AlphaToCoverageEnable ? "TRUE" : "FALSE")
             << "\n";
      stream << "\t\tIndependentBlendEnable " << (desc.IndependentBlendEnable ? "TRUE" : "FALSE")
             << "\n";
      for (unsigned i = 0; i < 8; ++i) {
        const D3D12_RENDER_TARGET_BLEND_DESC& rt = desc.RenderTarget[i];
        stream << "\t\tRenderTarget[" << i << "]\n";
        stream << "\t\t\tBlendEnable " << (rt.BlendEnable ? "TRUE" : "FALSE") << "\n";
        stream << "\t\t\tLogicOpEnable " << (rt.LogicOpEnable ? "TRUE" : "FALSE") << "\n";
        stream << "\t\t\tSrcBlend " << toStr(rt.SrcBlend) << "\n";
        stream << "\t\t\tDestBlend " << toStr(rt.DestBlend) << "\n";
        stream << "\t\t\tBlendOp " << toStr(rt.BlendOp) << "\n";
        stream << "\t\t\tSrcBlendAlpha " << toStr(rt.SrcBlendAlpha) << "\n";
        stream << "\t\t\tDestBlendAlpha " << toStr(rt.DestBlendAlpha) << "\n";
        stream << "\t\t\tBlendOpAlpha " << toStr(rt.BlendOpAlpha) << "\n";
        stream << "\t\t\tLogicOp " << toStr(rt.LogicOp) << "\n";
        stream << "\t\t\tRenderTargetWriteMask 0x" << std::hex
               << static_cast<unsigned>(rt.RenderTargetWriteMask) << std::dec << "\n";
      }
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_MASK: {
      UINT& desc = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_MASK*>(data);
      stream << "\tSampleMask 0x" << std::hex << desc << std::dec << "\n";
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_MASK);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_NODE_MASK: {
      UINT& desc = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_NODE_MASK*>(data);
      stream << "\tNodeMask " << desc << "\n";
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_NODE_MASK);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER: {
      D3D12_RASTERIZER_DESC& desc = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER*>(data);
      stream << "\tRasterizerState\n";
      stream << "\t\tFillMode " << toStr(desc.FillMode) << "\n";
      stream << "\t\tCullMode " << toStr(desc.CullMode) << "\n";
      stream << "\t\tFrontCounterClockwise " << (desc.FrontCounterClockwise ? "TRUE" : "FALSE")
             << "\n";
      stream << "\t\tDepthBias " << desc.DepthBias << "\n";
      stream << "\t\tDepthBiasClamp " << desc.DepthBiasClamp << "\n";
      stream << "\t\tSlopeScaledDepthBias " << desc.SlopeScaledDepthBias << "\n";
      stream << "\t\tDepthClipEnable " << (desc.DepthClipEnable ? "TRUE" : "FALSE") << "\n";
      stream << "\t\tMultisampleEnable " << (desc.MultisampleEnable ? "TRUE" : "FALSE") << "\n";
      stream << "\t\tAntialiasedLineEnable " << (desc.AntialiasedLineEnable ? "TRUE" : "FALSE")
             << "\n";
      stream << "\t\tForcedSampleCount " << desc.ForcedSampleCount << "\n";
      stream << "\t\tConservativeRaster " << toStr(desc.ConservativeRaster) << "\n";
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER1: {
      D3D12_RASTERIZER_DESC1& desc = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER1*>(data);
      stream << "\tRasterizerState\n";
      stream << "\t\tFillMode " << toStr(desc.FillMode) << "\n";
      stream << "\t\tCullMode " << toStr(desc.CullMode) << "\n";
      stream << "\t\tFrontCounterClockwise " << (desc.FrontCounterClockwise ? "TRUE" : "FALSE")
             << "\n";
      stream << "\t\tDepthBias " << desc.DepthBias << "\n";
      stream << "\t\tDepthBiasClamp " << desc.DepthBiasClamp << "\n";
      stream << "\t\tSlopeScaledDepthBias " << desc.SlopeScaledDepthBias << "\n";
      stream << "\t\tDepthClipEnable " << (desc.DepthClipEnable ? "TRUE" : "FALSE") << "\n";
      stream << "\t\tMultisampleEnable " << (desc.MultisampleEnable ? "TRUE" : "FALSE") << "\n";
      stream << "\t\tAntialiasedLineEnable " << (desc.AntialiasedLineEnable ? "TRUE" : "FALSE")
             << "\n";
      stream << "\t\tForcedSampleCount " << desc.ForcedSampleCount << "\n";
      stream << "\t\tConservativeRaster " << toStr(desc.ConservativeRaster) << "\n";
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER1);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER2: {
      D3D12_RASTERIZER_DESC2& desc = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER2*>(data);
      stream << "\tRasterizerState\n";
      stream << "\t\tFillMode " << toStr(desc.FillMode) << "\n";
      stream << "\t\tCullMode " << toStr(desc.CullMode) << "\n";
      stream << "\t\tFrontCounterClockwise " << (desc.FrontCounterClockwise ? "TRUE" : "FALSE")
             << "\n";
      stream << "\t\tDepthBias " << desc.DepthBias << "\n";
      stream << "\t\tDepthBiasClamp " << desc.DepthBiasClamp << "\n";
      stream << "\t\tSlopeScaledDepthBias " << desc.SlopeScaledDepthBias << "\n";
      stream << "\t\tDepthClipEnable " << (desc.DepthClipEnable ? "TRUE" : "FALSE") << "\n";
      stream << "\t\tLineRasterizationMode " << toStr(desc.LineRasterizationMode) << "\n";
      stream << "\t\tForcedSampleCount " << desc.ForcedSampleCount << "\n";
      stream << "\t\tConservativeRaster " << toStr(desc.ConservativeRaster) << "\n";
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER2);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL: {
      D3D12_DEPTH_STENCIL_DESC& desc =
          *static_cast<CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL*>(data);
      stream << "\tDepthStencilState\n";
      stream << "\t\tDepthEnable " << (desc.DepthEnable ? "TRUE" : "FALSE") << "\n";
      stream << "\t\tDepthWriteMask " << toStr(desc.DepthWriteMask) << "\n";
      stream << "\t\tDepthFunc " << toStr(desc.DepthFunc) << "\n";
      stream << "\t\tStencilEnable " << (desc.StencilEnable ? "TRUE" : "FALSE") << "\n";
      stream << "\t\tStencilReadMask 0x" << std::hex << static_cast<unsigned>(desc.StencilReadMask)
             << std::dec << "\n";
      stream << "\t\tStencilWriteMask 0x" << std::hex
             << static_cast<unsigned>(desc.StencilWriteMask) << std::dec << "\n";
      stream << "\t\tFrontFace\n";
      stream << "\t\t\tStencilFailOp " << toStr(desc.FrontFace.StencilFailOp) << "\n";
      stream << "\t\t\tStencilDepthFailOp " << toStr(desc.FrontFace.StencilDepthFailOp) << "\n";
      stream << "\t\t\tStencilPassOp " << toStr(desc.FrontFace.StencilPassOp) << "\n";
      stream << "\t\t\tStencilFunc " << toStr(desc.FrontFace.StencilFunc) << "\n";
      stream << "\t\tBackFace\n";
      stream << "\t\t\tStencilFailOp " << toStr(desc.BackFace.StencilFailOp) << "\n";
      stream << "\t\t\tStencilDepthFailOp " << toStr(desc.BackFace.StencilDepthFailOp) << "\n";
      stream << "\t\t\tStencilPassOp " << toStr(desc.BackFace.StencilPassOp) << "\n";
      stream << "\t\t\tStencilFunc " << toStr(desc.BackFace.StencilFunc) << "\n";
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL1: {
      D3D12_DEPTH_STENCIL_DESC1& desc =
          *static_cast<CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL1*>(data);
      stream << "\tDepthStencilState\n";
      stream << "\t\tDepthEnable " << (desc.DepthEnable ? "TRUE" : "FALSE") << "\n";
      stream << "\t\tDepthWriteMask " << toStr(desc.DepthWriteMask) << "\n";
      stream << "\t\tDepthFunc " << toStr(desc.DepthFunc) << "\n";
      stream << "\t\tStencilEnable " << (desc.StencilEnable ? "TRUE" : "FALSE") << "\n";
      stream << "\t\tStencilReadMask 0x" << std::hex << static_cast<unsigned>(desc.StencilReadMask)
             << std::dec << "\n";
      stream << "\t\tStencilWriteMask 0x" << std::hex
             << static_cast<unsigned>(desc.StencilWriteMask) << std::dec << "\n";
      stream << "\t\tFrontFace\n";
      stream << "\t\t\tStencilFailOp " << toStr(desc.FrontFace.StencilFailOp) << "\n";
      stream << "\t\t\tStencilDepthFailOp " << toStr(desc.FrontFace.StencilDepthFailOp) << "\n";
      stream << "\t\t\tStencilPassOp " << toStr(desc.FrontFace.StencilPassOp) << "\n";
      stream << "\t\t\tStencilFunc " << toStr(desc.FrontFace.StencilFunc) << "\n";
      stream << "\t\tBackFace\n";
      stream << "\t\t\tStencilFailOp " << toStr(desc.BackFace.StencilFailOp) << "\n";
      stream << "\t\t\tStencilDepthFailOp " << toStr(desc.BackFace.StencilDepthFailOp) << "\n";
      stream << "\t\t\tStencilPassOp " << toStr(desc.BackFace.StencilPassOp) << "\n";
      stream << "\t\t\tStencilFunc " << toStr(desc.BackFace.StencilFunc) << "\n";
      stream << "\t\tDepthBoundsTestEnable " << (desc.DepthBoundsTestEnable ? "TRUE" : "FALSE")
             << "\n";
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL1);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL2: {
      D3D12_DEPTH_STENCIL_DESC2& desc =
          *static_cast<CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL2*>(data);
      stream << "\tDepthStencilState\n";
      stream << "\t\tDepthEnable " << (desc.DepthEnable ? "TRUE" : "FALSE") << "\n";
      stream << "\t\tDepthWriteMask " << toStr(desc.DepthWriteMask) << "\n";
      stream << "\t\tDepthFunc " << toStr(desc.DepthFunc) << "\n";
      stream << "\t\tStencilEnable " << (desc.StencilEnable ? "TRUE" : "FALSE") << "\n";
      stream << "\t\tFrontFace\n";
      stream << "\t\t\tStencilFailOp " << toStr(desc.FrontFace.StencilFailOp) << "\n";
      stream << "\t\t\tStencilDepthFailOp " << toStr(desc.FrontFace.StencilDepthFailOp) << "\n";
      stream << "\t\t\tStencilPassOp " << toStr(desc.FrontFace.StencilPassOp) << "\n";
      stream << "\t\t\tStencilFunc " << toStr(desc.FrontFace.StencilFunc) << "\n";
      stream << "\t\t\tStencilReadMask 0x" << std::hex
             << static_cast<unsigned>(desc.FrontFace.StencilReadMask) << std::dec << "\n";
      stream << "\t\t\tStencilWriteMask 0x" << std::hex
             << static_cast<unsigned>(desc.FrontFace.StencilWriteMask) << std::dec << "\n";
      stream << "\t\tBackFace\n";
      stream << "\t\t\tStencilFailOp " << toStr(desc.BackFace.StencilFailOp) << "\n";
      stream << "\t\t\tStencilDepthFailOp " << toStr(desc.BackFace.StencilDepthFailOp) << "\n";
      stream << "\t\t\tStencilPassOp " << toStr(desc.BackFace.StencilPassOp) << "\n";
      stream << "\t\t\tStencilFunc " << toStr(desc.BackFace.StencilFunc) << "\n";
      stream << "\t\t\tStencilReadMask 0x" << std::hex
             << static_cast<unsigned>(desc.BackFace.StencilReadMask) << std::dec << "\n";
      stream << "\t\t\tStencilWriteMask 0x" << std::hex
             << static_cast<unsigned>(desc.BackFace.StencilWriteMask) << std::dec << "\n";
      stream << "\t\tDepthBoundsTestEnable " << (desc.DepthBoundsTestEnable ? "TRUE" : "FALSE")
             << "\n";
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL2);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT: {
      D3D12_INPUT_LAYOUT_DESC& desc =
          *static_cast<CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT*>(data);
      stream << "\tInputLayout\n";
      stream << "\t\tNumElements " << desc.NumElements << "\n";
      for (unsigned i = 0; i < desc.NumElements; ++i) {
        const D3D12_INPUT_ELEMENT_DESC& element = desc.pInputElementDescs[i];
        stream << "\t\tInputElement[" << i << "]\n";
        stream << "\t\t\tSemanticName " << (element.SemanticName ? element.SemanticName : "")
               << "\n";
        stream << "\t\t\tSemanticIndex " << element.SemanticIndex << "\n";
        stream << "\t\t\tFormat " << toStr(element.Format) << "\n";
        stream << "\t\t\tInputSlot " << element.InputSlot << "\n";
        stream << "\t\t\tAlignedByteOffset " << element.AlignedByteOffset << "\n";
        stream << "\t\t\tInputSlotClass " << toStr(element.InputSlotClass) << "\n";
        stream << "\t\t\tInstanceDataStepRate " << element.InstanceDataStepRate << "\n";
      }
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_IB_STRIP_CUT_VALUE: {
      D3D12_INDEX_BUFFER_STRIP_CUT_VALUE& desc =
          *static_cast<CD3DX12_PIPELINE_STATE_STREAM_IB_STRIP_CUT_VALUE*>(data);
      stream << "\tIBStripCutValue " << toStr(desc) << "\n";
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_IB_STRIP_CUT_VALUE);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY: {
      D3D12_PRIMITIVE_TOPOLOGY_TYPE& desc =
          *static_cast<CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY*>(data);
      stream << "\tPrimitiveTopologyType " << toStr(desc) << "\n";
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS: {
      D3D12_RT_FORMAT_ARRAY& desc =
          *static_cast<CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS*>(data);
      stream << "\tRenderTargetFormats\n";
      stream << "\t\tNumRenderTargets " << desc.NumRenderTargets << "\n";
      for (unsigned i = 0; i < desc.NumRenderTargets; ++i) {
        stream << "\t\tRTVFormats[" << i << "] " << toStr(desc.RTFormats[i]) << "\n";
      }
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT: {
      DXGI_FORMAT& desc = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT*>(data);
      stream << "\tDepthStencilFormat " << toStr(desc) << "\n";
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC: {
      DXGI_SAMPLE_DESC& desc = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC*>(data);
      stream << "\tSampleDesc\n";
      stream << "\t\tCount " << desc.Count << "\n";
      stream << "\t\tQuality " << desc.Quality << "\n";
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CACHED_PSO: {
      D3D12_CACHED_PIPELINE_STATE& desc =
          *static_cast<CD3DX12_PIPELINE_STATE_STREAM_CACHED_PSO*>(data);
      stream << "\tCachedPSO\n";
      stream << "\t\tCachedBlobSizeInBytes " << desc.CachedBlobSizeInBytes << "\n";
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_CACHED_PSO);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_FLAGS: {
      D3D12_PIPELINE_STATE_FLAGS& desc = *static_cast<CD3DX12_PIPELINE_STATE_STREAM_FLAGS*>(data);
      stream << "\tFlags " << toStr(desc) << "\n";
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_FLAGS);
    } break;
    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VIEW_INSTANCING: {
      D3D12_VIEW_INSTANCING_DESC& desc =
          *static_cast<CD3DX12_PIPELINE_STATE_STREAM_VIEW_INSTANCING*>(data);
      stream << "\tViewInstancing\n";
      stream << "\t\tViewInstanceCount " << desc.ViewInstanceCount << "\n";
      for (unsigned i = 0; i < desc.ViewInstanceCount; ++i) {
        const D3D12_VIEW_INSTANCE_LOCATION& location = desc.pViewInstanceLocations[i];
        stream << "\t\tViewInstanceLocation[" << i << "]\n";
        stream << "\t\t\tViewportArrayIndex " << location.ViewportArrayIndex << "\n";
        stream << "\t\t\tRenderTargetArrayIndex " << location.RenderTargetArrayIndex << "\n";
      }
      stream << "\t\tFlags " << toStr(desc.Flags) << "\n";
      offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_VIEW_INSTANCING);
    } break;
    default:
      stream << "\tUnknown subobject type " << static_cast<unsigned>(subobjectType) << "\n";
      return;
    }
  }
}

} // namespace DirectX
} // namespace gits
