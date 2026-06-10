// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "graphicsPipelineState.h"
#include "pipelineStateStreamDescDump.h"
#include "to_string/toStr.h"
#include "to_string/enumToStrAuto.h"

namespace gits {
namespace DirectX {

void GraphicsPipelineState::Reset() {
  m_StateDesc = nullptr;
  m_StateStreamDesc = nullptr;
  m_RootSignatureKey = 0;
  m_BindingState.Reset();
  m_IndexBufferKey = 0;
  m_IndexBufferOffset = 0;
  m_VertexBuffers.fill(VertexBuffer{});
}

void GraphicsPipelineState::SetRootSignature(unsigned rootSignatureKey,
                                             D3D12_ROOT_SIGNATURE_DESC2* desc) {
  m_RootSignatureKey = rootSignatureKey;
  m_BindingState.SetRootSignature(desc);
}

void GraphicsPipelineState::IASetIndexBuffer(ID3D12GraphicsCommandListIASetIndexBufferCommand& c) {
  m_IndexBufferKey = c.m_pView.BufferLocationKey;
  m_IndexBufferOffset = c.m_pView.BufferLocationOffset;
}

void GraphicsPipelineState::IASetVertexBuffers(
    ID3D12GraphicsCommandListIASetVertexBuffersCommand& c) {
  for (unsigned i = 0; i < c.m_NumViews.Value; ++i) {
    VertexBuffer& vertexBuffer = m_VertexBuffers[c.m_StartSlot.Value + i];
    vertexBuffer.Key = c.m_pViews.BufferLocationKeys[i];
    vertexBuffer.Offset = c.m_pViews.BufferLocationOffsets[i];
    vertexBuffer.Size = c.m_pViews.Value[i].SizeInBytes;
    vertexBuffer.Stride = c.m_pViews.Value[i].StrideInBytes;
  }
}

void GraphicsPipelineState::IASetPrimitiveTopology(
    ID3D12GraphicsCommandListIASetPrimitiveTopologyCommand& c) {}

void GraphicsPipelineState::OMSetBlendFactor(ID3D12GraphicsCommandListOMSetBlendFactorCommand& c) {}

void GraphicsPipelineState::OMSetRenderTargets(
    ID3D12GraphicsCommandListOMSetRenderTargetsCommand& c) {
  for (unsigned i = 0; i < m_RenderTargets.size(); ++i) {
    if (i < c.m_NumRenderTargetDescriptors.Value) {
      if (c.m_RTsSingleHandleToDescriptorRange.Value) {
        DescriptorHeapTracker::Descriptor* descriptor = m_DescriptorService.GetDescriptor(
            c.m_pRenderTargetDescriptors.InterfaceKeys[0], c.m_pRenderTargetDescriptors.Indexes[0]);
        m_RenderTargets[i] = *descriptor;
      } else {
        DescriptorHeapTracker::Descriptor* descriptor = m_DescriptorService.GetDescriptor(
            c.m_pRenderTargetDescriptors.InterfaceKeys[i], c.m_pRenderTargetDescriptors.Indexes[i]);
        m_RenderTargets[i] = *descriptor;
      }
    } else {
      m_RenderTargets[i].reset();
    }
  }
  if (c.m_pDepthStencilDescriptor.Value) {
    DescriptorHeapTracker::Descriptor* descriptor = m_DescriptorService.GetDescriptor(
        c.m_pDepthStencilDescriptor.InterfaceKeys[0], c.m_pDepthStencilDescriptor.Indexes[0]);
    m_DepthStencil = *descriptor;
  } else {
    m_DepthStencil.reset();
  }
}

void GraphicsPipelineState::OMSetStencilRef(ID3D12GraphicsCommandListOMSetStencilRefCommand& c) {}

void GraphicsPipelineState::RSSetScissorRects(
    ID3D12GraphicsCommandListRSSetScissorRectsCommand& c) {}

void GraphicsPipelineState::RSSetViewports(ID3D12GraphicsCommandListRSSetViewportsCommand& c) {}

void GraphicsPipelineState::SOSetTargets(ID3D12GraphicsCommandListSOSetTargetsCommand& c) {}

void GraphicsPipelineState::SetGraphicsRoot32BitConstant(
    ID3D12GraphicsCommandListSetGraphicsRoot32BitConstantCommand& c) {
  m_BindingState.SetConstant(c.m_RootParameterIndex.Value, c.m_SrcData.Value,
                             c.m_DestOffsetIn32BitValues.Value);
}

void GraphicsPipelineState::SetGraphicsRoot32BitConstants(
    ID3D12GraphicsCommandListSetGraphicsRoot32BitConstantsCommand& c) {
  m_BindingState.SetConstants(c.m_RootParameterIndex.Value,
                              static_cast<unsigned*>(c.m_pSrcData.Value),
                              c.m_DestOffsetIn32BitValues.Value, c.m_Num32BitValuesToSet.Value);
}

void GraphicsPipelineState::SetGraphicsRootConstantBufferView(
    ID3D12GraphicsCommandListSetGraphicsRootConstantBufferViewCommand& c) {
  m_BindingState.SetConstantBufferView(c.m_RootParameterIndex.Value,
                                       c.m_BufferLocation.InterfaceKey, c.m_BufferLocation.Offset);
}

void GraphicsPipelineState::SetGraphicsRootUnorderedAccessView(
    ID3D12GraphicsCommandListSetGraphicsRootUnorderedAccessViewCommand& c) {
  m_BindingState.SetUnorderedAccessView(c.m_RootParameterIndex.Value,
                                        c.m_BufferLocation.InterfaceKey, c.m_BufferLocation.Offset);
}

void GraphicsPipelineState::SetGraphicsRootShaderResourceView(
    ID3D12GraphicsCommandListSetGraphicsRootShaderResourceViewCommand& c) {
  m_BindingState.SetShaderResourceView(c.m_RootParameterIndex.Value,
                                       c.m_BufferLocation.InterfaceKey, c.m_BufferLocation.Offset);
}

void GraphicsPipelineState::SetGraphicsRootDescriptorTable(
    ID3D12GraphicsCommandListSetGraphicsRootDescriptorTableCommand& c) {
  m_BindingState.SetDescriptorTable(c.m_RootParameterIndex.Value, c.m_BaseDescriptor.InterfaceKey,
                                    c.m_BaseDescriptor.Index);
}

void GraphicsPipelineState::DumpState(const std::wstring& dumpDir,
                                      ID3D12GraphicsCommandListDrawInstancedCommand& c) {
  std::wstring dumpName = dumpDir + L"/draw_" + keyToWStr(c.Key) + L".txt";

  std::ofstream stream(dumpName);
  stream << c.Key << " DrawInstanced(" << c.m_VertexCountPerInstance.Value << ", "
         << c.m_InstanceCount.Value << ", " << c.m_StartVertexLocation.Value << ", "
         << c.m_StartInstanceLocation.Value << ")\n";
  DumpState(stream);
}

void GraphicsPipelineState::DumpState(const std::wstring& dumpDir,
                                      ID3D12GraphicsCommandListDrawIndexedInstancedCommand& c) {
  std::wstring dumpName = dumpDir + L"/draw_" + keyToWStr(c.Key) + L".txt";
  std::ofstream stream(dumpName);
  stream << c.Key << " DrawInstanced(" << c.m_IndexCountPerInstance.Value << ", "
         << c.m_InstanceCount.Value << ", " << c.m_StartIndexLocation.Value << ", "
         << c.m_BaseVertexLocation.Value << ", " << c.m_StartInstanceLocation.Value << ")\n";
  DumpState(stream);
}

void GraphicsPipelineState::DumpState(std::ofstream& stream) {
  stream << "\n";
  if (m_StateDesc) {
    DumpStateDesc(stream);
  } else if (m_StateStreamDesc) {
    DumpPipelineStateStreamDesc(*m_StateStreamDesc, stream);
  }
  stream << "\n";
  if (m_IndexBufferKey) {
    stream << "Index buffer O" << m_IndexBufferKey << " offset " << m_IndexBufferOffset << "\n";
  }
  for (unsigned i = 0; i < m_VertexBuffers.size(); ++i) {
    if (m_VertexBuffers[i].Key) {
      stream << "Vertex buffer slot " << i << " O" << m_VertexBuffers[i].Key << " offset "
             << m_VertexBuffers[i].Offset << " size " << m_VertexBuffers[i].Size << " stride "
             << m_VertexBuffers[i].Stride << "\n";
    }
  }
  stream << "\n";
  stream << "Root signature O" << m_RootSignatureKey << "\n";
  stream << "\n";
  m_BindingState.DumpState(stream);
  stream << "\n";
  for (unsigned i = 0; i < m_RenderTargets.size(); ++i) {
    if (m_RenderTargets[i].has_value()) {
      stream << "RenderTarget[" << i << "] resource O" << m_RenderTargets[i]->ResourceKey
             << " descriptor O" << m_RenderTargets[i]->HeapKey << " "
             << m_RenderTargets[i]->DescriptorIndex << "\n";
    }
  }
  if (m_DepthStencil.has_value()) {
    stream << "DepthStencil resource O" << m_DepthStencil->ResourceKey << " descriptor O"
           << m_DepthStencil->HeapKey << " " << m_DepthStencil->DescriptorIndex << "\n";
  }
}

void GraphicsPipelineState::DumpStateDesc(std::ofstream& stream) {
  if (!m_StateDesc || !m_StateDesc->Value) {
    return;
  }
  const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc = *m_StateDesc->Value;

  stream << "D3D12_GRAPHICS_PIPELINE_STATE_DESC\n";
  stream << "\tRootSignature O" << m_StateDesc->RootSignatureKey << "\n";

  stream << "\tVS BytecodeLength " << desc.VS.BytecodeLength << "\n";
  stream << "\tPS BytecodeLength " << desc.PS.BytecodeLength << "\n";
  stream << "\tDS BytecodeLength " << desc.DS.BytecodeLength << "\n";
  stream << "\tHS BytecodeLength " << desc.HS.BytecodeLength << "\n";
  stream << "\tGS BytecodeLength " << desc.GS.BytecodeLength << "\n";

  stream << "\tStreamOutput\n";
  for (unsigned i = 0; i < desc.StreamOutput.NumEntries; ++i) {
    const D3D12_SO_DECLARATION_ENTRY& entry = desc.StreamOutput.pSODeclaration[i];
    stream << "\t\tEntry[" << i << "]\n";
    stream << "\t\t\tStream " << entry.Stream << "\n";
    stream << "\t\t\tSemanticName " << entry.SemanticName << "\n";
    stream << "\t\t\tSemanticIndex " << entry.SemanticIndex << "\n";
    stream << "\t\t\tStartComponent " << static_cast<unsigned>(entry.StartComponent) << "\n";
    stream << "\t\t\tComponentCount " << static_cast<unsigned>(entry.ComponentCount) << "\n";
    stream << "\t\t\tOutputSlot " << static_cast<unsigned>(entry.OutputSlot) << "\n";
  }
  stream << "\t\BufferStrides " << desc.StreamOutput.NumStrides;
  for (unsigned i = 0; i < desc.StreamOutput.NumStrides; ++i) {
    stream << " " << desc.StreamOutput.pBufferStrides[i];
  }
  stream << "\n";
  stream << "\t\tRasterizedStream " << desc.StreamOutput.RasterizedStream << "\n";

  stream << "\tBlendState\n";
  stream << "\t\tAlphaToCoverageEnable "
         << (desc.BlendState.AlphaToCoverageEnable ? "TRUE" : "FALSE") << "\n";
  stream << "\t\tIndependentBlendEnable "
         << (desc.BlendState.IndependentBlendEnable ? "TRUE" : "FALSE") << "\n";
  for (unsigned i = 0; i < desc.NumRenderTargets; ++i) {
    const D3D12_RENDER_TARGET_BLEND_DESC& rt = desc.BlendState.RenderTarget[i];
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

  stream << "\tSampleMask 0x" << std::hex << desc.SampleMask << std::dec << "\n";

  stream << "\tRasterizerState\n";
  stream << "\t\tFillMode " << toStr(desc.RasterizerState.FillMode) << "\n";
  stream << "\t\tCullMode " << toStr(desc.RasterizerState.CullMode) << "\n";
  stream << "\t\tFrontCounterClockwise "
         << (desc.RasterizerState.FrontCounterClockwise ? "TRUE" : "FALSE") << "\n";
  stream << "\t\tDepthBias " << desc.RasterizerState.DepthBias << "\n";
  stream << "\t\tDepthBiasClamp " << desc.RasterizerState.DepthBiasClamp << "\n";
  stream << "\t\tSlopeScaledDepthBias " << desc.RasterizerState.SlopeScaledDepthBias << "\n";
  stream << "\t\tDepthClipEnable " << (desc.RasterizerState.DepthClipEnable ? "TRUE" : "FALSE")
         << "\n";
  stream << "\t\tMultisampleEnable " << (desc.RasterizerState.MultisampleEnable ? "TRUE" : "FALSE")
         << "\n";
  stream << "\t\tAntialiasedLineEnable "
         << (desc.RasterizerState.AntialiasedLineEnable ? "TRUE" : "FALSE") << "\n";
  stream << "\t\tForcedSampleCount " << desc.RasterizerState.ForcedSampleCount << "\n";
  stream << "\t\tConservativeRaster " << toStr(desc.RasterizerState.ConservativeRaster) << "\n";

  stream << "\tDepthStencilState\n";
  stream << "\t\tDepthEnable " << (desc.DepthStencilState.DepthEnable ? "TRUE" : "FALSE") << "\n";
  stream << "\t\tDepthWriteMask " << toStr(desc.DepthStencilState.DepthWriteMask) << "\n";
  stream << "\t\tDepthFunc " << toStr(desc.DepthStencilState.DepthFunc) << "\n";
  stream << "\t\tStencilEnable " << (desc.DepthStencilState.StencilEnable ? "TRUE" : "FALSE")
         << "\n";
  stream << "\t\tStencilReadMask 0x" << std::hex
         << static_cast<unsigned>(desc.DepthStencilState.StencilReadMask) << std::dec << "\n";
  stream << "\t\tStencilWriteMask 0x" << std::hex
         << static_cast<unsigned>(desc.DepthStencilState.StencilWriteMask) << std::dec << "\n";
  stream << "\t\tFrontFace.StencilFailOp " << toStr(desc.DepthStencilState.FrontFace.StencilFailOp)
         << "\n";
  stream << "\t\tFrontFace.StencilDepthFailOp "
         << toStr(desc.DepthStencilState.FrontFace.StencilDepthFailOp) << "\n";
  stream << "\t\tFrontFace.StencilPassOp " << toStr(desc.DepthStencilState.FrontFace.StencilPassOp)
         << "\n";
  stream << "\t\tFrontFace.StencilFunc " << toStr(desc.DepthStencilState.FrontFace.StencilFunc)
         << "\n";
  stream << "\t\tBackFace.StencilFailOp " << toStr(desc.DepthStencilState.BackFace.StencilFailOp)
         << "\n";
  stream << "\t\tBackFace.StencilDepthFailOp "
         << toStr(desc.DepthStencilState.BackFace.StencilDepthFailOp) << "\n";
  stream << "\t\tBackFace.StencilPassOp " << toStr(desc.DepthStencilState.BackFace.StencilPassOp)
         << "\n";
  stream << "\t\tBackFace.StencilFunc " << toStr(desc.DepthStencilState.BackFace.StencilFunc)
         << "\n";

  stream << "\tInputLayout\n";
  stream << "\t\tNumElements " << desc.InputLayout.NumElements << "\n";
  for (unsigned i = 0; i < desc.InputLayout.NumElements; ++i) {
    const D3D12_INPUT_ELEMENT_DESC& el = desc.InputLayout.pInputElementDescs[i];
    stream << "\t\tElement[" << i << "]\n";
    stream << "\t\t\tSemanticName " << (el.SemanticName ? el.SemanticName : "") << "\n";
    stream << "\t\t\tSemanticIndex " << el.SemanticIndex << "\n";
    stream << "\t\t\tFormat " << toStr(el.Format) << "\n";
    stream << "\t\t\tInputSlot " << el.InputSlot << "\n";
    stream << "\t\t\tAlignedByteOffset " << el.AlignedByteOffset << "\n";
    stream << "\t\t\tInputSlotClass " << toStr(el.InputSlotClass) << "\n";
    stream << "\t\t\tInstanceDataStepRate " << el.InstanceDataStepRate << "\n";
  }

  stream << "\tIBStripCutValue " << toStr(desc.IBStripCutValue) << "\n";
  stream << "\tPrimitiveTopologyType " << toStr(desc.PrimitiveTopologyType) << "\n";

  stream << "\tNumRenderTargets " << desc.NumRenderTargets << "\n";
  for (unsigned i = 0; i < desc.NumRenderTargets; ++i) {
    stream << "\tRTVFormats[" << i << "] " << toStr(desc.RTVFormats[i]) << "\n";
  }
  stream << "\tDSVFormat " << toStr(desc.DSVFormat) << "\n";

  stream << "\tSampleDesc.Count " << desc.SampleDesc.Count << "\n";
  stream << "\tSampleDesc.Quality " << desc.SampleDesc.Quality << "\n";

  stream << "\tNodeMask 0x" << std::hex << desc.NodeMask << std::dec << "\n";
  stream << "\tCachedPSO.CachedBlobSizeInBytes " << desc.CachedPSO.CachedBlobSizeInBytes << "\n";

  stream << "\tFlags " << toStr(desc.Flags) << "\n";
}

} // namespace DirectX
} // namespace gits
