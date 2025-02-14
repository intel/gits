// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "interfaceArgumentUpdaters.h"

#include <d3dx12/d3dx12_pipeline_state_stream.h>

namespace gits {
namespace DirectX {

static void updateBinding(PlayerManager& manager, unsigned resourceKey, const void* bindingDesc) {
  if (!manager.executeCommands()) {
    return;
  }

  auto* binding = static_cast<DML_BUFFER_BINDING*>(const_cast<void*>(bindingDesc));
  GITS_ASSERT(binding);
  if (binding->Buffer) {
    ObjectInfoPlayer* info = manager.findObject(resourceKey);
    GITS_ASSERT(info);
    binding->Buffer = static_cast<ID3D12Resource*>(info->object);
  }
}

void updateInterface(PlayerManager& manager, D3D12_TEXTURE_COPY_LOCATION_Argument& arg) {
  if (!manager.executeCommands()) {
    return;
  }

  if (arg.resourceKey) {
    ObjectInfoPlayer* info = manager.findObject(arg.resourceKey);
    arg.value->pResource = info ? static_cast<ID3D12Resource*>(info->object) : nullptr;
    arg.resourceInfo = info;
  }
}

void updateInterface(PlayerManager& manager, D3D12_RESOURCE_BARRIERs_Argument& arg) {
  if (!manager.executeCommands()) {
    return;
  }

  for (unsigned i = 0; i < arg.size; ++i) {
    arg.resourceInfos.resize(arg.size);
    arg.resourceAfterInfos.resize(arg.size);

    D3D12_RESOURCE_BARRIER& barrier = arg.value[i];

    switch (barrier.Type) {
    case D3D12_RESOURCE_BARRIER_TYPE_ALIASING:
      if (arg.resourceKeys[i]) {
        ObjectInfoPlayer* info = manager.findObject(arg.resourceKeys[i]);
        barrier.Aliasing.pResourceBefore =
            info ? static_cast<ID3D12Resource*>(info->object) : nullptr;
        arg.resourceInfos[i] = info;
      }
      if (arg.resourceAfterKeys[i]) {
        ObjectInfoPlayer* info = manager.findObject(arg.resourceAfterKeys[i]);
        barrier.Aliasing.pResourceAfter =
            info ? static_cast<ID3D12Resource*>(info->object) : nullptr;
        arg.resourceAfterInfos[i] = info;
      }
      break;

    case D3D12_RESOURCE_BARRIER_TYPE_TRANSITION:
      if (arg.resourceKeys[i]) {
        ObjectInfoPlayer* info = manager.findObject(arg.resourceKeys[i]);
        barrier.Transition.pResource = info ? static_cast<ID3D12Resource*>(info->object) : nullptr;
        arg.resourceInfos[i] = info;
      }
      break;

    case D3D12_RESOURCE_BARRIER_TYPE_UAV:
      if (arg.resourceKeys[i]) {
        ObjectInfoPlayer* info = manager.findObject(arg.resourceKeys[i]);
        barrier.UAV.pResource = info ? static_cast<ID3D12Resource*>(info->object) : nullptr;
        arg.resourceInfos[i] = info;
      }
      break;
    }
  }
}

void updateInterface(PlayerManager& manager, D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument& arg) {
  if (!manager.executeCommands()) {
    return;
  }

  if (arg.rootSignatureKey) {
    ObjectInfoPlayer* info = manager.findObject(arg.rootSignatureKey);
    arg.value->pRootSignature = info ? static_cast<ID3D12RootSignature*>(info->object) : nullptr;
    arg.rootSignatureInfo = info;
  }
}

void updateInterface(PlayerManager& manager, D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument& arg) {
  if (!manager.executeCommands()) {
    return;
  }

  if (arg.rootSignatureKey) {
    ObjectInfoPlayer* info = manager.findObject(arg.rootSignatureKey);
    arg.value->pRootSignature = info ? static_cast<ID3D12RootSignature*>(info->object) : nullptr;
    arg.rootSignatureInfo = info;
  }
}

void updateInterface(PlayerManager& manager, D3D12_PIPELINE_STATE_STREAM_DESC_Argument& arg) {
  if (!manager.executeCommands()) {
    return;
  }

  ID3D12RootSignature* rootSignature = nullptr;
  if (arg.rootSignatureKey) {
    ObjectInfoPlayer* info = manager.findObject(arg.rootSignatureKey);
    rootSignature = info ? static_cast<ID3D12RootSignature*>(info->object) : nullptr;
    arg.rootSignatureInfo = info;

    size_t offset = 0;
    while (offset < arg.value->SizeInBytes) {
      void* subobjectData = static_cast<char*>(arg.value->pPipelineStateSubobjectStream) + offset;
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

void updateInterface(PlayerManager& manager, D3D12_STATE_OBJECT_DESC_Argument& arg) {
  if (!manager.executeCommands()) {
    return;
  }

  for (unsigned index = 0; index < arg.value->NumSubobjects; ++index) {
    D3D12_STATE_SUBOBJECT* subobject =
        const_cast<D3D12_STATE_SUBOBJECT*>(&(arg.value->pSubobjects[index]));
    switch (subobject->Type) {
    case D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE: {
      D3D12_GLOBAL_ROOT_SIGNATURE* globalSignature =
          static_cast<D3D12_GLOBAL_ROOT_SIGNATURE*>(const_cast<void*>(subobject->pDesc));
      ObjectInfoPlayer* info = manager.findObject(arg.interfaceKeysBySubobject[index]);
      globalSignature->pGlobalRootSignature =
          info ? static_cast<ID3D12RootSignature*>(info->object) : nullptr;
      arg.objectInfosBySubobject[index] = info;
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE: {
      D3D12_LOCAL_ROOT_SIGNATURE* localSignature =
          static_cast<D3D12_LOCAL_ROOT_SIGNATURE*>(const_cast<void*>(subobject->pDesc));
      ObjectInfoPlayer* info = manager.findObject(arg.interfaceKeysBySubobject[index]);
      localSignature->pLocalRootSignature =
          info ? static_cast<ID3D12RootSignature*>(info->object) : nullptr;
      arg.objectInfosBySubobject[index] = info;
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION: {
      D3D12_EXISTING_COLLECTION_DESC* desc =
          static_cast<D3D12_EXISTING_COLLECTION_DESC*>(const_cast<void*>(subobject->pDesc));
      ObjectInfoPlayer* info = manager.findObject(arg.interfaceKeysBySubobject[index]);
      desc->pExistingCollection = info ? static_cast<ID3D12StateObject*>(info->object) : nullptr;
      arg.objectInfosBySubobject[index] = info;
    } break;
    }
  }
}

void updateInterface(PlayerManager& manager, D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument& arg) {
  if (!arg.value || !manager.executeCommands()) {
    return;
  }

  for (unsigned i = 0, j = 0; i < arg.size; ++i) {
    if (arg.value[i].EndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE) {
      ObjectInfoPlayer* infoSrcResource = manager.findObject(arg.resolveSrcResourceKeys[j]);
      arg.value[i].EndingAccess.Resolve.pSrcResource =
          infoSrcResource ? static_cast<ID3D12Resource*>(infoSrcResource->object) : nullptr;

      ObjectInfoPlayer* infoDstResource = manager.findObject(arg.resolveDstResourceKeys[j]);
      arg.value[i].EndingAccess.Resolve.pDstResource =
          infoDstResource ? static_cast<ID3D12Resource*>(infoDstResource->object) : nullptr;
      ++j;
    }
  }
}

void updateInterface(PlayerManager& manager, D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument& arg) {
  if (!arg.value || !manager.executeCommands()) {
    return;
  }

  if (arg.value->DepthEndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE) {
    ObjectInfoPlayer* infoSrcResource = manager.findObject(arg.resolveSrcDepthKey);
    arg.value->DepthEndingAccess.Resolve.pSrcResource =
        infoSrcResource ? static_cast<ID3D12Resource*>(infoSrcResource->object) : nullptr;

    ObjectInfoPlayer* infoDstResource = manager.findObject(arg.resolveDstDepthKey);
    arg.value->DepthEndingAccess.Resolve.pDstResource =
        infoDstResource ? static_cast<ID3D12Resource*>(infoDstResource->object) : nullptr;
  }
  if (arg.value->StencilEndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE) {
    ObjectInfoPlayer* infoSrcResource = manager.findObject(arg.resolveSrcStencilKey);
    arg.value->StencilEndingAccess.Resolve.pSrcResource =
        infoSrcResource ? static_cast<ID3D12Resource*>(infoSrcResource->object) : nullptr;

    ObjectInfoPlayer* infoDstResource = manager.findObject(arg.resolveDstStencilKey);
    arg.value->StencilEndingAccess.Resolve.pDstResource =
        infoDstResource ? static_cast<ID3D12Resource*>(infoDstResource->object) : nullptr;
  }
}

void updateInterface(PlayerManager& manager,
                     PointerArgument<INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC>& arg) {
  if (!manager.executeCommands()) {
    return;
  }

  if (arg.rootSignatureKey) {
    ObjectInfoPlayer* info = manager.findObject(arg.rootSignatureKey);
    arg.value->pD3D12Desc->pRootSignature =
        info ? static_cast<ID3D12RootSignature*>(info->object) : nullptr;
    arg.rootSignatureInfo = info;
  }
}

void updateInterface(PlayerManager& manager, DML_BINDING_TABLE_DESC_Argument& arg) {
  if (!manager.executeCommands()) {
    return;
  }

  if (arg.data.dispatchableKey) {
    ObjectInfoPlayer* info = manager.findObject(arg.data.dispatchableKey);
    arg.value->Dispatchable = info ? static_cast<IDMLDispatchable*>(info->object) : nullptr;
  }
}

void updateInterface(PlayerManager& manager, DML_BINDING_DESC_Argument& arg) {
  GITS_ASSERT(arg.resourceKeys.size() == arg.resourceKeysSize);
  if (!manager.executeCommands()) {
    return;
  }

  if (arg.value->Type == DML_BINDING_TYPE_BUFFER) {
    updateBinding(manager, arg.resourceKeys[0], arg.value->Desc);
  } else if (arg.value->Type == DML_BINDING_TYPE_BUFFER_ARRAY) {
    auto* bindingArray = static_cast<DML_BUFFER_ARRAY_BINDING*>(const_cast<void*>(arg.value->Desc));
    GITS_ASSERT(bindingArray);
    for (unsigned i = 0; i < bindingArray->BindingCount; ++i) {
      updateBinding(manager, arg.resourceKeys[i], &bindingArray->Bindings[i]);
    }
  }
}

void updateInterface(PlayerManager& manager, DML_BINDING_DESCs_Argument& arg) {
  GITS_ASSERT(arg.resourceKeys.size() == arg.resourceKeysSize);
  if (!manager.executeCommands()) {
    return;
  }

  unsigned bindingIdx = 0;
  for (unsigned i = 0; i < arg.size; ++i) {
    auto& binding = arg.value[i];
    if (binding.Type == DML_BINDING_TYPE_BUFFER) {
      updateBinding(manager, arg.resourceKeys[bindingIdx], binding.Desc);
      ++bindingIdx;
    } else if (binding.Type == DML_BINDING_TYPE_BUFFER_ARRAY) {
      auto* bindingArray = static_cast<DML_BUFFER_ARRAY_BINDING*>(const_cast<void*>(binding.Desc));
      GITS_ASSERT(bindingArray);
      for (unsigned j = 0; j < bindingArray->BindingCount; ++j) {
        updateBinding(manager, arg.resourceKeys[bindingIdx], &bindingArray->Bindings[j]);
        ++bindingIdx;
      }
    }
  }
}

void updateInterface(PlayerManager& manager, DML_GRAPH_DESC_Argument& arg) {
  GITS_ASSERT(arg.operatorKeysSize == arg.value->NodeCount);
  GITS_ASSERT(arg.operatorKeys.size() == arg.operatorKeysSize);
  if (!manager.executeCommands()) {
    return;
  }

  for (unsigned i = 0; i < arg.value->NodeCount; ++i) {
    auto& node = arg.value->Nodes[i];
    if (node.Type == DML_GRAPH_NODE_TYPE_OPERATOR) {
      auto* opNode = static_cast<DML_OPERATOR_GRAPH_NODE_DESC*>(const_cast<void*>(node.Desc));
      if (opNode->Operator) {
        ObjectInfoPlayer* info = manager.findObject(arg.operatorKeys[i]);
        GITS_ASSERT(info);
        opNode->Operator = static_cast<IDMLOperator*>(info->object);
      }
    }
  }
}

void updateInterface(PlayerManager& manager, xess_d3d12_init_params_t_Argument& arg) {
  if (!manager.executeCommands()) {
    return;
  }

  if (arg.tempBufferHeapKey) {
    ObjectInfoPlayer* info = manager.findObject(arg.tempBufferHeapKey);
    GITS_ASSERT(info);
    arg.value->pTempBufferHeap = static_cast<ID3D12Heap*>(info->object);
  }
  if (arg.tempTextureHeapKey) {
    ObjectInfoPlayer* info = manager.findObject(arg.tempTextureHeapKey);
    GITS_ASSERT(info);
    arg.value->pTempTextureHeap = static_cast<ID3D12Heap*>(info->object);
  }
  if (arg.pipelineLibraryKey) {
    ObjectInfoPlayer* info = manager.findObject(arg.pipelineLibraryKey);
    GITS_ASSERT(info);
    arg.value->pPipelineLibrary = static_cast<ID3D12PipelineLibrary*>(info->object);
  }
}

void updateInterface(PlayerManager& manager, xess_d3d12_execute_params_t_Argument& arg) {
  if (!manager.executeCommands()) {
    return;
  }

  if (arg.colorTextureKey) {
    ObjectInfoPlayer* info = manager.findObject(arg.colorTextureKey);
    GITS_ASSERT(info);
    arg.value->pColorTexture = static_cast<ID3D12Resource*>(info->object);
  }
  if (arg.velocityTextureKey) {
    ObjectInfoPlayer* info = manager.findObject(arg.velocityTextureKey);
    GITS_ASSERT(info);
    arg.value->pVelocityTexture = static_cast<ID3D12Resource*>(info->object);
  }
  if (arg.depthTextureKey) {
    ObjectInfoPlayer* info = manager.findObject(arg.depthTextureKey);
    GITS_ASSERT(info);
    arg.value->pDepthTexture = static_cast<ID3D12Resource*>(info->object);
  }
  if (arg.exposureScaleTextureKey) {
    ObjectInfoPlayer* info = manager.findObject(arg.exposureScaleTextureKey);
    GITS_ASSERT(info);
    arg.value->pExposureScaleTexture = static_cast<ID3D12Resource*>(info->object);
  }
  if (arg.responsivePixelMaskTextureKey) {
    ObjectInfoPlayer* info = manager.findObject(arg.responsivePixelMaskTextureKey);
    GITS_ASSERT(info);
    arg.value->pResponsivePixelMaskTexture = static_cast<ID3D12Resource*>(info->object);
  }
  if (arg.outputTextureKey) {
    ObjectInfoPlayer* info = manager.findObject(arg.outputTextureKey);
    GITS_ASSERT(info);
    arg.value->pOutputTexture = static_cast<ID3D12Resource*>(info->object);
  }
  if (arg.descriptorHeapKey) {
    ObjectInfoPlayer* info = manager.findObject(arg.descriptorHeapKey);
    GITS_ASSERT(info);
    arg.value->pDescriptorHeap = static_cast<ID3D12DescriptorHeap*>(info->object);
  }
}

void updateInterface(PlayerManager& manager, DSTORAGE_QUEUE_DESC_Argument& arg) {
  if (!manager.executeCommands()) {
    return;
  }

  if (arg.value->Device) {
    ObjectInfoPlayer* info = manager.findObject(arg.deviceKey);
    GITS_ASSERT(info);
    arg.value->Device = static_cast<ID3D12Device*>(info->object);
  }
}

void updateInterface(PlayerManager& manager, DSTORAGE_REQUEST_Argument& arg) {
  if (!manager.executeCommands()) {
    return;
  }
  switch (arg.value->Options.SourceType) {
  case DSTORAGE_REQUEST_SOURCE_FILE:
    if (arg.value->Source.File.Source) {
      ObjectInfoPlayer* info = manager.findObject(arg.fileKey);
      GITS_ASSERT(info);
      arg.value->Source.File.Source = static_cast<IDStorageFile*>(info->object);
    }
    break;
  }
  switch (arg.value->Options.DestinationType) {
  case DSTORAGE_REQUEST_DESTINATION_BUFFER:
    if (arg.value->Destination.Buffer.Resource) {
      ObjectInfoPlayer* info = manager.findObject(arg.resourceKey);
      GITS_ASSERT(info);
      arg.value->Destination.Buffer.Resource = static_cast<ID3D12Resource*>(info->object);
    }
    break;
  case DSTORAGE_REQUEST_DESTINATION_TEXTURE_REGION:
    if (arg.value->Destination.Texture.Resource) {
      ObjectInfoPlayer* info = manager.findObject(arg.resourceKey);
      GITS_ASSERT(info);
      arg.value->Destination.Texture.Resource = static_cast<ID3D12Resource*>(info->object);
    }
    break;
  case DSTORAGE_REQUEST_DESTINATION_MULTIPLE_SUBRESOURCES:
    if (arg.value->Destination.MultipleSubresources.Resource) {
      ObjectInfoPlayer* info = manager.findObject(arg.resourceKey);
      GITS_ASSERT(info);
      arg.value->Destination.MultipleSubresources.Resource =
          static_cast<ID3D12Resource*>(info->object);
    }
    break;
  case DSTORAGE_REQUEST_DESTINATION_TILES:
    if (arg.value->Destination.Tiles.Resource) {
      ObjectInfoPlayer* info = manager.findObject(arg.resourceKey);
      GITS_ASSERT(info);
      arg.value->Destination.Tiles.Resource = static_cast<ID3D12Resource*>(info->object);
    }
    break;
  }
}

void updateContext(PlayerManager& manager, XESSContextArgument& arg) {
  if (!manager.executeCommands()) {
    return;
  }
  arg.value =
      reinterpret_cast<xess_context_handle_t>(manager.getXessContextMap().getContext(arg.key));
}

void updateOutputContext(PlayerManager& manager, XESSContextOutputArgument& arg) {
  manager.getXessContextMap().setContext(arg.key, reinterpret_cast<std::uintptr_t>(*arg.value));
  arg.data = *arg.value;
}

} // namespace DirectX
} // namespace gits
