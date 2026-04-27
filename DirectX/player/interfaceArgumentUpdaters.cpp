// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "interfaceArgumentUpdaters.h"

#include <d3dx12/d3dx12_pipeline_state_stream.h>

namespace gits {
namespace DirectX {

void UpdateInterface(PlayerManager& manager, D3D12_TEXTURE_COPY_LOCATION_Argument& arg) {
  if (!manager.ExecuteCommands()) {
    return;
  }
  if (arg.ResourceKey) {
    arg.Value->pResource = static_cast<ID3D12Resource*>(manager.FindObject(arg.ResourceKey));
  }
}

void UpdateInterface(PlayerManager& manager, D3D12_RESOURCE_BARRIERs_Argument& arg) {
  if (!manager.ExecuteCommands()) {
    return;
  }

  for (unsigned i = 0; i < arg.Size; ++i) {
    D3D12_RESOURCE_BARRIER& barrier = arg.Value[i];
    switch (barrier.Type) {
    case D3D12_RESOURCE_BARRIER_TYPE_ALIASING:
      if (arg.ResourceKeys[i]) {
        barrier.Aliasing.pResourceBefore =
            static_cast<ID3D12Resource*>(manager.FindObject(arg.ResourceKeys[i]));
      }
      if (arg.ResourceAfterKeys[i]) {
        barrier.Aliasing.pResourceAfter =
            static_cast<ID3D12Resource*>(manager.FindObject(arg.ResourceAfterKeys[i]));
      }
      break;

    case D3D12_RESOURCE_BARRIER_TYPE_TRANSITION:
      if (arg.ResourceKeys[i]) {
        barrier.Transition.pResource =
            static_cast<ID3D12Resource*>(manager.FindObject(arg.ResourceKeys[i]));
      }
      break;

    case D3D12_RESOURCE_BARRIER_TYPE_UAV:
      if (arg.ResourceKeys[i]) {
        barrier.UAV.pResource =
            static_cast<ID3D12Resource*>(manager.FindObject(arg.ResourceKeys[i]));
      }
      break;
    }
  }
}

void UpdateInterface(PlayerManager& manager, D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument& arg) {
  if (!manager.ExecuteCommands()) {
    return;
  }

  if (arg.RootSignatureKey) {
    arg.Value->pRootSignature =
        static_cast<ID3D12RootSignature*>(manager.FindObject(arg.RootSignatureKey));
  }
}

void UpdateInterface(PlayerManager& manager, D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument& arg) {
  if (!manager.ExecuteCommands()) {
    return;
  }

  if (arg.RootSignatureKey) {
    arg.Value->pRootSignature =
        static_cast<ID3D12RootSignature*>(manager.FindObject(arg.RootSignatureKey));
  }
}

void UpdateInterface(PlayerManager& manager, D3D12_PIPELINE_STATE_STREAM_DESC_Argument& arg) {
  if (!manager.ExecuteCommands()) {
    return;
  }

  ID3D12RootSignature* rootSignature = nullptr;
  if (arg.RootSignatureKey) {
    rootSignature = static_cast<ID3D12RootSignature*>(manager.FindObject(arg.RootSignatureKey));

    size_t offset = 0;
    while (offset < arg.Value->SizeInBytes) {
      void* subobjectData = static_cast<char*>(arg.Value->pPipelineStateSubobjectStream) + offset;
      D3D12_PIPELINE_STATE_SUBOBJECT_TYPE subobjectType =
          *reinterpret_cast<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE*>(subobjectData);

      if (subobjectType == D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE) {
        auto* rootSignatureSubobject =
            reinterpret_cast<CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE*>(subobjectData);
        *rootSignatureSubobject = rootSignature;
        offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE);
        break;
      }

      switch (subobjectType) {
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS:
        offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_VS);
        break;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS:
        offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_PS);
        break;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DS:
        offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_DS);
        break;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_HS:
        offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_HS);
        break;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_GS:
        offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_GS);
        break;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS:
        offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_CS);
        break;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS:
        offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_AS);
        break;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS:
        offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_MS);
        break;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_STREAM_OUTPUT:
        offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_STREAM_OUTPUT);
        break;
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
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT:
        offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT);
        break;
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
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CACHED_PSO:
        offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_CACHED_PSO);
        break;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_FLAGS:
        offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_FLAGS);
        break;
      case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VIEW_INSTANCING:
        offset += sizeof(CD3DX12_PIPELINE_STATE_STREAM_VIEW_INSTANCING);
        break;
      default:
        GITS_ASSERT(0 && "Unexpected subobject type");
        break;
      }
    }
  }
}

void UpdateInterface(PlayerManager& manager, D3D12_STATE_OBJECT_DESC_Argument& arg) {
  if (!manager.ExecuteCommands()) {
    return;
  }

  for (unsigned index = 0; index < arg.Value->NumSubobjects; ++index) {
    D3D12_STATE_SUBOBJECT* subobject =
        const_cast<D3D12_STATE_SUBOBJECT*>(&(arg.Value->pSubobjects[index]));
    switch (subobject->Type) {
    case D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE: {
      D3D12_GLOBAL_ROOT_SIGNATURE* globalSignature =
          static_cast<D3D12_GLOBAL_ROOT_SIGNATURE*>(const_cast<void*>(subobject->pDesc));
      globalSignature->pGlobalRootSignature = static_cast<ID3D12RootSignature*>(
          manager.FindObject(arg.InterfaceKeysBySubobject[index]));
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE: {
      D3D12_LOCAL_ROOT_SIGNATURE* localSignature =
          static_cast<D3D12_LOCAL_ROOT_SIGNATURE*>(const_cast<void*>(subobject->pDesc));
      localSignature->pLocalRootSignature = static_cast<ID3D12RootSignature*>(
          manager.FindObject(arg.InterfaceKeysBySubobject[index]));
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION: {
      D3D12_EXISTING_COLLECTION_DESC* desc =
          static_cast<D3D12_EXISTING_COLLECTION_DESC*>(const_cast<void*>(subobject->pDesc));
      desc->pExistingCollection =
          static_cast<ID3D12StateObject*>(manager.FindObject(arg.InterfaceKeysBySubobject[index]));
    } break;
    }
  }
}

void UpdateInterface(PlayerManager& manager, D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument& arg) {
  if (!arg.Value || !manager.ExecuteCommands()) {
    return;
  }

  for (unsigned i = 0, j = 0; i < arg.Size; ++i) {
    if (arg.Value[i].EndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE) {
      arg.Value[i].EndingAccess.Resolve.pSrcResource =
          static_cast<ID3D12Resource*>(manager.FindObject(arg.ResolveSrcResourceKeys[j]));
      arg.Value[i].EndingAccess.Resolve.pDstResource =
          static_cast<ID3D12Resource*>(manager.FindObject(arg.ResolveDstResourceKeys[j]));
      ++j;
    }
  }
}

void UpdateInterface(PlayerManager& manager, D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument& arg) {
  if (!arg.Value || !manager.ExecuteCommands()) {
    return;
  }

  if (arg.Value->DepthEndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE) {
    arg.Value->DepthEndingAccess.Resolve.pSrcResource =
        static_cast<ID3D12Resource*>(manager.FindObject(arg.ResolveSrcDepthKey));
    arg.Value->DepthEndingAccess.Resolve.pDstResource =
        static_cast<ID3D12Resource*>(manager.FindObject(arg.ResolveDstDepthKey));
  }
  if (arg.Value->StencilEndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE) {
    arg.Value->StencilEndingAccess.Resolve.pSrcResource =
        static_cast<ID3D12Resource*>(manager.FindObject(arg.ResolveSrcStencilKey));
    arg.Value->StencilEndingAccess.Resolve.pDstResource =
        static_cast<ID3D12Resource*>(manager.FindObject(arg.ResolveDstStencilKey));
  }
}

void UpdateInterface(PlayerManager& manager,
                     PointerArgument<INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC>& arg) {
  if (!manager.ExecuteCommands()) {
    return;
  }

  if (arg.RootSignatureKey) {
    arg.Value->pD3D12Desc->pRootSignature =
        static_cast<ID3D12RootSignature*>(manager.FindObject(arg.RootSignatureKey));
  }
}

void UpdateInterface(PlayerManager& manager, D3D12_BARRIER_GROUPs_Argument& arg) {
  if (!manager.ExecuteCommands()) {
    return;
  }

  unsigned resourceKeyIndex = 0;

  for (unsigned i = 0; i < arg.Size; ++i) {
    D3D12_BARRIER_GROUP& barrierGroup = arg.Value[i];
    if (barrierGroup.Type == D3D12_BARRIER_TYPE_GLOBAL) {
      continue;
    } else if (barrierGroup.Type == D3D12_BARRIER_TYPE_TEXTURE) {
      D3D12_TEXTURE_BARRIER* barriers =
          const_cast<D3D12_TEXTURE_BARRIER*>(barrierGroup.pTextureBarriers);
      for (unsigned j = 0; j < barrierGroup.NumBarriers; ++j) {
        barriers[j].pResource =
            static_cast<ID3D12Resource*>(manager.FindObject(arg.ResourceKeys[resourceKeyIndex++]));
      }
    } else if (barrierGroup.Type == D3D12_BARRIER_TYPE_BUFFER) {
      D3D12_BUFFER_BARRIER* barriers =
          const_cast<D3D12_BUFFER_BARRIER*>(barrierGroup.pBufferBarriers);
      for (unsigned j = 0; j < barrierGroup.NumBarriers; ++j) {
        barriers[j].pResource =
            static_cast<ID3D12Resource*>(manager.FindObject(arg.ResourceKeys[resourceKeyIndex++]));
      }
    }
  }
}

void UpdateInterface(PlayerManager& manager, D3D12_EXTENSION_ARGUMENTS_Argument& arg) {
  if (!manager.ExecuteCommands()) {
    return;
  }

  GITS_ASSERT(false, "UpdateInterface not implemented for D3D12_EXTENSION_ARGUMENTS_Argument");
}

void UpdateInterface(PlayerManager& manager, D3D12_EXTENDED_OPERATION_DATA_Argument& arg) {
  if (!manager.ExecuteCommands()) {
    return;
  }

  GITS_ASSERT(false, "UpdateInterface not implemented for D3D12_EXTENDED_OPERATION_DATA_Argument");
}

void UpdateInterface(PlayerManager& manager, DML_BINDING_TABLE_DESC_Argument& arg) {
  if (!manager.ExecuteCommands()) {
    return;
  }

  if (arg.TableFields.DispatchableKey) {
    arg.Value->Dispatchable =
        static_cast<IDMLDispatchable*>(manager.FindObject(arg.TableFields.DispatchableKey));
  }
}

static void updateDmlBinding(PlayerManager& manager,
                             unsigned resourceKey,
                             const void* bindingDesc) {
  if (!manager.ExecuteCommands()) {
    return;
  }

  auto* binding = static_cast<DML_BUFFER_BINDING*>(const_cast<void*>(bindingDesc));
  GITS_ASSERT(binding);
  if (binding->Buffer) {
    IUnknown* object = manager.FindObject(resourceKey);
    GITS_ASSERT(object);
    binding->Buffer = static_cast<ID3D12Resource*>(object);
  }
}

void UpdateInterface(PlayerManager& manager, DML_BINDING_DESC_Argument& arg) {
  GITS_ASSERT(arg.ResourceKeys.size() == arg.ResourceKeysSize);
  if (!manager.ExecuteCommands()) {
    return;
  }

  if (arg.Value->Type == DML_BINDING_TYPE_BUFFER) {
    updateDmlBinding(manager, arg.ResourceKeys[0], arg.Value->Desc);
  } else if (arg.Value->Type == DML_BINDING_TYPE_BUFFER_ARRAY) {
    auto* bindingArray = static_cast<DML_BUFFER_ARRAY_BINDING*>(const_cast<void*>(arg.Value->Desc));
    GITS_ASSERT(bindingArray);
    for (unsigned i = 0; i < bindingArray->BindingCount; ++i) {
      updateDmlBinding(manager, arg.ResourceKeys[i], &bindingArray->Bindings[i]);
    }
  }
}

void UpdateInterface(PlayerManager& manager, DML_BINDING_DESCs_Argument& arg) {
  GITS_ASSERT(arg.ResourceKeys.size() == arg.ResourceKeysSize);
  if (!manager.ExecuteCommands()) {
    return;
  }

  unsigned bindingIdx = 0;
  for (unsigned i = 0; i < arg.Size; ++i) {
    auto& binding = arg.Value[i];
    if (binding.Type == DML_BINDING_TYPE_BUFFER) {
      updateDmlBinding(manager, arg.ResourceKeys[bindingIdx], binding.Desc);
      ++bindingIdx;
    } else if (binding.Type == DML_BINDING_TYPE_BUFFER_ARRAY) {
      auto* bindingArray = static_cast<DML_BUFFER_ARRAY_BINDING*>(const_cast<void*>(binding.Desc));
      GITS_ASSERT(bindingArray);
      for (unsigned j = 0; j < bindingArray->BindingCount; ++j) {
        updateDmlBinding(manager, arg.ResourceKeys[bindingIdx], &bindingArray->Bindings[j]);
        ++bindingIdx;
      }
    }
  }
}

void UpdateInterface(PlayerManager& manager, DML_GRAPH_DESC_Argument& arg) {
  GITS_ASSERT(arg.OperatorKeysSize == arg.Value->NodeCount);
  GITS_ASSERT(arg.OperatorKeys.size() == arg.OperatorKeysSize);
  if (!manager.ExecuteCommands()) {
    return;
  }

  for (unsigned i = 0; i < arg.Value->NodeCount; ++i) {
    auto& node = arg.Value->Nodes[i];
    if (node.Type == DML_GRAPH_NODE_TYPE_OPERATOR) {
      auto* opNode = static_cast<DML_OPERATOR_GRAPH_NODE_DESC*>(const_cast<void*>(node.Desc));
      if (opNode->Operator) {
        opNode->Operator = static_cast<IDMLOperator*>(manager.FindObject(arg.OperatorKeys[i]));
      }
    }
  }
}

void UpdateInterface(PlayerManager& manager, xess_d3d12_init_params_t_Argument& arg) {
  if (!manager.ExecuteCommands()) {
    return;
  }

  if (arg.TempBufferHeapKey) {
    arg.Value->pTempBufferHeap =
        static_cast<ID3D12Heap*>(manager.FindObject(arg.TempBufferHeapKey));
  }
  if (arg.TempTextureHeapKey) {
    arg.Value->pTempTextureHeap =
        static_cast<ID3D12Heap*>(manager.FindObject(arg.TempTextureHeapKey));
  }
  if (arg.PipelineLibraryKey) {
    arg.Value->pPipelineLibrary =
        static_cast<ID3D12PipelineLibrary*>(manager.FindObject(arg.PipelineLibraryKey));
  }
}

void UpdateInterface(PlayerManager& manager, xess_d3d12_execute_params_t_Argument& arg) {
  if (!manager.ExecuteCommands()) {
    return;
  }

  if (arg.ColorTextureKey) {
    arg.Value->pColorTexture =
        static_cast<ID3D12Resource*>(manager.FindObject(arg.ColorTextureKey));
  }
  if (arg.VelocityTextureKey) {
    arg.Value->pVelocityTexture =
        static_cast<ID3D12Resource*>(manager.FindObject(arg.VelocityTextureKey));
  }
  if (arg.DepthTextureKey) {
    arg.Value->pDepthTexture =
        static_cast<ID3D12Resource*>(manager.FindObject(arg.DepthTextureKey));
  }
  if (arg.ExposureScaleTextureKey) {
    arg.Value->pExposureScaleTexture =
        static_cast<ID3D12Resource*>(manager.FindObject(arg.ExposureScaleTextureKey));
  }
  if (arg.ResponsivePixelMaskTextureKey) {
    arg.Value->pResponsivePixelMaskTexture =
        static_cast<ID3D12Resource*>(manager.FindObject(arg.ResponsivePixelMaskTextureKey));
  }
  if (arg.OutputTextureKey) {
    arg.Value->pOutputTexture =
        static_cast<ID3D12Resource*>(manager.FindObject(arg.OutputTextureKey));
  }
  if (arg.DescriptorHeapKey) {
    arg.Value->pDescriptorHeap =
        static_cast<ID3D12DescriptorHeap*>(manager.FindObject(arg.DescriptorHeapKey));
  }
}

void UpdateInterface(PlayerManager& manager, DSTORAGE_QUEUE_DESC_Argument& arg) {
  if (!manager.ExecuteCommands()) {
    return;
  }

  if (arg.Value->Device) {
    arg.Value->Device = static_cast<ID3D12Device*>(manager.FindObject(arg.DeviceKey));
  }
}

void UpdateInterface(PlayerManager& manager, DSTORAGE_REQUEST_Argument& arg) {
  if (!manager.ExecuteCommands()) {
    return;
  }

  switch (arg.Value->Options.SourceType) {
  case DSTORAGE_REQUEST_SOURCE_FILE:
    if (arg.Value->Source.File.Source) {
      arg.Value->Source.File.Source = static_cast<IDStorageFile*>(manager.FindObject(arg.FileKey));
    }
    break;
  }
  switch (arg.Value->Options.DestinationType) {
  case DSTORAGE_REQUEST_DESTINATION_BUFFER:
    if (arg.Value->Destination.Buffer.Resource) {
      arg.Value->Destination.Buffer.Resource =
          static_cast<ID3D12Resource*>(manager.FindObject(arg.ResourceKey));
    }
    break;
  case DSTORAGE_REQUEST_DESTINATION_TEXTURE_REGION:
    if (arg.Value->Destination.Texture.Resource) {
      arg.Value->Destination.Texture.Resource =
          static_cast<ID3D12Resource*>(manager.FindObject(arg.ResourceKey));
    }
    break;
  case DSTORAGE_REQUEST_DESTINATION_MULTIPLE_SUBRESOURCES:
    if (arg.Value->Destination.MultipleSubresources.Resource) {
      arg.Value->Destination.MultipleSubresources.Resource =
          static_cast<ID3D12Resource*>(manager.FindObject(arg.ResourceKey));
    }
    break;
  case DSTORAGE_REQUEST_DESTINATION_TILES:
    if (arg.Value->Destination.Tiles.Resource) {
      arg.Value->Destination.Tiles.Resource =
          static_cast<ID3D12Resource*>(manager.FindObject(arg.ResourceKey));
    }
    break;
  }
}

void UpdateInterface(PlayerManager& manager, xefg_swapchain_d3d12_init_params_t_Argument& arg) {
  if (!manager.ExecuteCommands()) {
    return;
  }
  if (arg.ApplicationSwapChainKey) {
    arg.Value->pApplicationSwapChain =
        static_cast<IDXGISwapChain*>(manager.FindObject(arg.ApplicationSwapChainKey));
  }
  if (arg.TempBufferHeapKey) {
    arg.Value->pTempBufferHeap =
        static_cast<ID3D12Heap*>(manager.FindObject(arg.TempBufferHeapKey));
  }
  if (arg.TempTextureHeapKey) {
    arg.Value->pTempTextureHeap =
        static_cast<ID3D12Heap*>(manager.FindObject(arg.TempTextureHeapKey));
  }
  if (arg.PipelineLibraryKey) {
    arg.Value->pPipelineLibrary =
        static_cast<ID3D12PipelineLibrary*>(manager.FindObject(arg.PipelineLibraryKey));
  }
}

void UpdateInterface(PlayerManager& manager, xefg_swapchain_d3d12_resource_data_t_Argument& arg) {
  if (!manager.ExecuteCommands()) {
    return;
  }
  if (arg.ResourceKey) {
    arg.Value->pResource = static_cast<ID3D12Resource*>(manager.FindObject(arg.ResourceKey));
  }
}

void UpdateContext(PlayerManager& manager, XESSContextArgument& arg) {
  if (!manager.ExecuteCommands()) {
    return;
  }
  arg.Value =
      reinterpret_cast<xess_context_handle_t>(manager.GetXessContextMap().GetContext(arg.Key));
}

void UpdateOutputContext(PlayerManager& manager, XESSContextOutputArgument& arg) {
  manager.GetXessContextMap().SetContext(arg.Key, reinterpret_cast<std::uintptr_t>(*arg.Value));
  arg.Data = *arg.Value;
}

void UpdateContext(PlayerManager& manager, XELLContextArgument& arg) {
  if (!manager.ExecuteCommands()) {
    return;
  }
  arg.Value =
      reinterpret_cast<xell_context_handle_t>(manager.GetXellContextMap().GetContext(arg.Key));
}

void UpdateOutputContext(PlayerManager& manager, XELLContextOutputArgument& arg) {
  manager.GetXellContextMap().SetContext(arg.Key, reinterpret_cast<std::uintptr_t>(*arg.Value));
  arg.Data = *arg.Value;
}

void UpdateContext(PlayerManager& manager, XEFGContextArgument& arg) {
  if (!manager.ExecuteCommands()) {
    return;
  }
  arg.Value =
      reinterpret_cast<xefg_swapchain_handle_t>(manager.GetXefgContextMap().GetContext(arg.Key));
}

void UpdateOutputContext(PlayerManager& manager, XEFGContextOutputArgument& arg) {
  manager.GetXefgContextMap().SetContext(arg.Key, reinterpret_cast<std::uintptr_t>(*arg.Value));
  arg.Data = *arg.Value;
}

} // namespace DirectX
} // namespace gits
