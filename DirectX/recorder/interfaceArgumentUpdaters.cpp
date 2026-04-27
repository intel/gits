// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "interfaceArgumentUpdaters.h"
#include "log.h"

#include <d3dx12/d3dx12_pipeline_state_stream.h>

namespace gits {
namespace DirectX {

UpdateInterface<D3D12_TEXTURE_COPY_LOCATION_Argument, D3D12_TEXTURE_COPY_LOCATION>::UpdateInterface(
    D3D12_TEXTURE_COPY_LOCATION_Argument& arg, const D3D12_TEXTURE_COPY_LOCATION* value) {

  arg.Value = &m_UnwrapStructure;
  *arg.Value = *value;

  if (value->pResource) {
    IUnknownWrapper* wrapper = reinterpret_cast<IUnknownWrapper*>(value->pResource);
    arg.ResourceKey = wrapper->GetKey();
    arg.Value->pResource = wrapper->GetWrappedObject<ID3D12Resource>();
  }
}

UpdateInterface<D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument, D3D12_GRAPHICS_PIPELINE_STATE_DESC>::
    UpdateInterface(D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument& arg,
                    const D3D12_GRAPHICS_PIPELINE_STATE_DESC* value) {

  arg.Value = &m_UnwrapStructure;
  *arg.Value = *value;

  if (value->pRootSignature) {
    IUnknownWrapper* wrapper = reinterpret_cast<IUnknownWrapper*>(value->pRootSignature);
    arg.RootSignatureKey = wrapper->GetKey();
    arg.Value->pRootSignature = wrapper->GetWrappedObject<ID3D12RootSignature>();
  }
}

UpdateInterface<D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument, D3D12_COMPUTE_PIPELINE_STATE_DESC>::
    UpdateInterface(D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument& arg,
                    const D3D12_COMPUTE_PIPELINE_STATE_DESC* value) {

  arg.Value = &m_UnwrapStructure;
  *arg.Value = *value;

  if (value->pRootSignature) {
    IUnknownWrapper* wrapper = reinterpret_cast<IUnknownWrapper*>(value->pRootSignature);
    arg.RootSignatureKey = wrapper->GetKey();
    arg.Value->pRootSignature = wrapper->GetWrappedObject<ID3D12RootSignature>();
  }
}

UpdateInterface<D3D12_RESOURCE_BARRIERs_Argument, D3D12_RESOURCE_BARRIER>::UpdateInterface(
    D3D12_RESOURCE_BARRIERs_Argument& arg, const D3D12_RESOURCE_BARRIER* value) {

  m_Unwrapped = m_UnwrappedStatic;
  if (arg.Size > NUM) {
    m_UnwrappedDynamic.resize(arg.Size);
    m_Unwrapped = m_UnwrappedDynamic.data();
  }
  arg.Value = m_Unwrapped;

  for (unsigned i = 0; i < arg.Size; ++i) {

    arg.Value[i] = value[i];
    D3D12_RESOURCE_BARRIER& barrier = arg.Value[i];

    switch (barrier.Type) {
    case D3D12_RESOURCE_BARRIER_TYPE_ALIASING:
      if (barrier.Aliasing.pResourceBefore) {
        IUnknownWrapper* wrapper =
            reinterpret_cast<IUnknownWrapper*>(barrier.Aliasing.pResourceBefore);
        arg.ResourceKeys[i] = wrapper->GetKey();
        barrier.Aliasing.pResourceBefore = wrapper->GetWrappedObject<ID3D12Resource>();
      }
      if (barrier.Aliasing.pResourceAfter) {
        IUnknownWrapper* wrapper =
            reinterpret_cast<IUnknownWrapper*>(barrier.Aliasing.pResourceAfter);
        arg.ResourceAfterKeys[i] = wrapper->GetKey();
        barrier.Aliasing.pResourceAfter = wrapper->GetWrappedObject<ID3D12Resource>();
      }
      break;

    case D3D12_RESOURCE_BARRIER_TYPE_TRANSITION:
      if (barrier.Transition.pResource) {
        IUnknownWrapper* wrapper = reinterpret_cast<IUnknownWrapper*>(barrier.Transition.pResource);
        arg.ResourceKeys[i] = wrapper->GetKey();
        barrier.Transition.pResource = wrapper->GetWrappedObject<ID3D12Resource>();
      }
      break;

    case D3D12_RESOURCE_BARRIER_TYPE_UAV:
      if (barrier.UAV.pResource) {
        IUnknownWrapper* wrapper = reinterpret_cast<IUnknownWrapper*>(barrier.UAV.pResource);
        arg.ResourceKeys[i] = wrapper->GetKey();
        barrier.UAV.pResource = wrapper->GetWrappedObject<ID3D12Resource>();
      }
      break;
    }
  }
}

UpdateInterface<D3D12_PIPELINE_STATE_STREAM_DESC_Argument, D3D12_PIPELINE_STATE_STREAM_DESC>::
    UpdateInterface(D3D12_PIPELINE_STATE_STREAM_DESC_Argument& arg,
                    const D3D12_PIPELINE_STATE_STREAM_DESC* stateStreamtDesc) {

  m_StreamDescUnwrapped = *stateStreamtDesc;
  m_SubobjectsUnwrapped.resize(stateStreamtDesc->SizeInBytes);
  m_StreamDescUnwrapped.pPipelineStateSubobjectStream = m_SubobjectsUnwrapped.data();
  memcpy(m_SubobjectsUnwrapped.data(), stateStreamtDesc->pPipelineStateSubobjectStream,
         stateStreamtDesc->SizeInBytes);

  arg.Value = &m_StreamDescUnwrapped;

  size_t offset = 0;
  while (offset < stateStreamtDesc->SizeInBytes) {
    uint8_t* subobjectData = m_SubobjectsUnwrapped.data() + offset;
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE subobjectType =
        *reinterpret_cast<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE*>(subobjectData);

    if (subobjectType == D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE) {
      auto* rootSignatureSubobject =
          reinterpret_cast<CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE*>(subobjectData);

      ID3D12RootSignature* rootSignature = *rootSignatureSubobject;
      IUnknownWrapper* wrapper = reinterpret_cast<IUnknownWrapper*>(rootSignature);
      arg.RootSignatureKey = wrapper->GetKey();
      *rootSignatureSubobject = wrapper->GetWrappedObject<ID3D12RootSignature>();

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

UpdateInterface<D3D12_STATE_OBJECT_DESC_Argument, D3D12_STATE_OBJECT_DESC>::UpdateInterface(
    D3D12_STATE_OBJECT_DESC_Argument& arg, const D3D12_STATE_OBJECT_DESC* stateObjectDesc) {

  arg.Value = &m_StateObjectDescUnwrapped;
  *arg.Value = *stateObjectDesc;

  m_SubobjectsUnwrapped.reserve(stateObjectDesc->NumSubobjects);
  m_LocalSignatures.reserve(stateObjectDesc->NumSubobjects);
  existingCollectionDescs.reserve(stateObjectDesc->NumSubobjects);
  m_SubobjectToExportsAssociations.reserve(stateObjectDesc->NumSubobjects);

  for (unsigned index = 0; index < stateObjectDesc->NumSubobjects; ++index) {

    const D3D12_STATE_SUBOBJECT* subobject = &(stateObjectDesc->pSubobjects[index]);
    switch (subobject->Type) {

    case D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE: {
      m_GlobalSignatureUnwrapped =
          *static_cast<D3D12_GLOBAL_ROOT_SIGNATURE*>(const_cast<void*>(subobject->pDesc));
      IUnknownWrapper* wrapper =
          reinterpret_cast<IUnknownWrapper*>(m_GlobalSignatureUnwrapped.pGlobalRootSignature);
      arg.InterfaceKeysBySubobject[index] = wrapper->GetKey();
      m_GlobalSignatureUnwrapped.pGlobalRootSignature =
          wrapper->GetWrappedObject<ID3D12RootSignature>();

      m_SubobjectsUnwrapped.push_back(
          D3D12_STATE_SUBOBJECT{subobject->Type, &m_GlobalSignatureUnwrapped});
      m_WrappedSubobjectIndexes[subobject] = index;
    } break;

    case D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE: {
      m_LocalSignatures.push_back(
          *static_cast<D3D12_LOCAL_ROOT_SIGNATURE*>(const_cast<void*>(subobject->pDesc)));

      D3D12_LOCAL_ROOT_SIGNATURE* localRootSignature = &m_LocalSignatures.back();
      IUnknownWrapper* wrapper =
          reinterpret_cast<IUnknownWrapper*>(localRootSignature->pLocalRootSignature);
      arg.InterfaceKeysBySubobject[index] = wrapper->GetKey();
      localRootSignature->pLocalRootSignature = wrapper->GetWrappedObject<ID3D12RootSignature>();

      m_SubobjectsUnwrapped.push_back(D3D12_STATE_SUBOBJECT{subobject->Type, localRootSignature});
      m_WrappedSubobjectIndexes[subobject] = index;
    } break;

    case D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION: {
      existingCollectionDescs.push_back(
          *static_cast<D3D12_EXISTING_COLLECTION_DESC*>(const_cast<void*>(subobject->pDesc)));

      D3D12_EXISTING_COLLECTION_DESC* desc = &existingCollectionDescs.back();
      IUnknownWrapper* wrapper = reinterpret_cast<IUnknownWrapper*>(desc->pExistingCollection);
      arg.InterfaceKeysBySubobject[index] = wrapper->GetKey();
      desc->pExistingCollection = wrapper->GetWrappedObject<ID3D12StateObject>();

      m_SubobjectsUnwrapped.push_back(D3D12_STATE_SUBOBJECT{subobject->Type, desc});
      m_WrappedSubobjectIndexes[subobject] = index;
    } break;

    default:
      m_SubobjectsUnwrapped.push_back(D3D12_STATE_SUBOBJECT{subobject->Type, subobject->pDesc});
      m_WrappedSubobjectIndexes[subobject] = index;
      break;
    }
  }

  for (D3D12_STATE_SUBOBJECT& subobject : m_SubobjectsUnwrapped) {

    if (subobject.Type == D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION) {
      m_SubobjectToExportsAssociations.push_back(
          *static_cast<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(
              const_cast<void*>(subobject.pDesc)));

      auto it = m_WrappedSubobjectIndexes.find(
          m_SubobjectToExportsAssociations.back().pSubobjectToAssociate);
      GITS_ASSERT(it != m_WrappedSubobjectIndexes.end());
      unsigned associateIndex = it->second;
      m_SubobjectToExportsAssociations.back().pSubobjectToAssociate =
          &m_SubobjectsUnwrapped[associateIndex];

      subobject.pDesc = &m_SubobjectToExportsAssociations.back();
    }
  }

  m_StateObjectDescUnwrapped = *const_cast<D3D12_STATE_OBJECT_DESC*>(stateObjectDesc);
  m_StateObjectDescUnwrapped.pSubobjects = m_SubobjectsUnwrapped.data();
}

UpdateInterface<D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument,
                D3D12_RENDER_PASS_RENDER_TARGET_DESC>::
    UpdateInterface(D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument& arg,
                    const D3D12_RENDER_PASS_RENDER_TARGET_DESC* value) {
  if (!value) {
    return;
  }
  m_UnwrapStructures.resize(arg.Size);
  for (unsigned i = 0; i < arg.Size; ++i) {
    m_UnwrapStructures[i] = arg.Value[i];
    if (m_UnwrapStructures[i].EndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE) {
      if (m_UnwrapStructures[i].EndingAccess.Resolve.pSrcResource) {
        IUnknownWrapper* wrapperSrcResource = reinterpret_cast<IUnknownWrapper*>(
            m_UnwrapStructures[i].EndingAccess.Resolve.pSrcResource);
        arg.ResolveSrcResourceKeys.push_back(wrapperSrcResource->GetKey());
        m_UnwrapStructures[i].EndingAccess.Resolve.pSrcResource =
            wrapperSrcResource->GetWrappedObject<ID3D12Resource>();
      }
      if (m_UnwrapStructures[i].EndingAccess.Resolve.pDstResource) {
        IUnknownWrapper* wrapperDstResource = reinterpret_cast<IUnknownWrapper*>(
            m_UnwrapStructures[i].EndingAccess.Resolve.pDstResource);
        arg.ResolveDstResourceKeys.push_back(wrapperDstResource->GetKey());
        m_UnwrapStructures[i].EndingAccess.Resolve.pDstResource =
            wrapperDstResource->GetWrappedObject<ID3D12Resource>();
      }
    }
  }
  arg.Value = m_UnwrapStructures.data();
}

UpdateInterface<D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument,
                D3D12_RENDER_PASS_DEPTH_STENCIL_DESC>::
    UpdateInterface(D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument& arg,
                    const D3D12_RENDER_PASS_DEPTH_STENCIL_DESC* value) {
  if (!value) {
    return;
  }
  arg.Value = &m_UnwrapStructure;
  *arg.Value = *value;

  if (arg.Value->DepthEndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE) {
    if (arg.Value->DepthEndingAccess.Resolve.pSrcResource) {
      IUnknownWrapper* wrapperSrcResource =
          reinterpret_cast<IUnknownWrapper*>(arg.Value->DepthEndingAccess.Resolve.pSrcResource);
      arg.ResolveSrcDepthKey = wrapperSrcResource->GetKey();
      arg.Value->DepthEndingAccess.Resolve.pSrcResource =
          wrapperSrcResource->GetWrappedObject<ID3D12Resource>();
    }
    if (arg.Value->DepthEndingAccess.Resolve.pDstResource) {
      IUnknownWrapper* wrapperDstResource =
          reinterpret_cast<IUnknownWrapper*>(arg.Value->DepthEndingAccess.Resolve.pDstResource);
      arg.ResolveDstDepthKey = wrapperDstResource->GetKey();
      arg.Value->DepthEndingAccess.Resolve.pDstResource =
          wrapperDstResource->GetWrappedObject<ID3D12Resource>();
    }
  }

  if (arg.Value->StencilEndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE) {
    if (arg.Value->StencilEndingAccess.Resolve.pSrcResource) {
      IUnknownWrapper* wrapperSrcResource =
          reinterpret_cast<IUnknownWrapper*>(arg.Value->StencilEndingAccess.Resolve.pSrcResource);
      arg.ResolveSrcStencilKey = wrapperSrcResource->GetKey();
      arg.Value->StencilEndingAccess.Resolve.pSrcResource =
          wrapperSrcResource->GetWrappedObject<ID3D12Resource>();
    }
    if (arg.Value->StencilEndingAccess.Resolve.pDstResource) {
      IUnknownWrapper* wrapperDstResource =
          reinterpret_cast<IUnknownWrapper*>(arg.Value->StencilEndingAccess.Resolve.pDstResource);
      arg.ResolveDstStencilKey = wrapperDstResource->GetKey();
      arg.Value->StencilEndingAccess.Resolve.pDstResource =
          wrapperDstResource->GetWrappedObject<ID3D12Resource>();
    }
  }
}

UpdateInterface<D3D12_BARRIER_GROUPs_Argument, D3D12_BARRIER_GROUP>::UpdateInterface(
    D3D12_BARRIER_GROUPs_Argument& arg, const D3D12_BARRIER_GROUP* value) {

  m_Unwrapped.resize(arg.Size);
  arg.Value = m_Unwrapped.data();

  unsigned resourceKeyIndex = 0;

  for (unsigned i = 0; i < arg.Size; ++i) {
    const D3D12_BARRIER_GROUP& barrierGroup = value[i];
    D3D12_BARRIER_GROUP& barrierGroupUnwrapped = m_Unwrapped[i];
    barrierGroupUnwrapped = barrierGroup;

    if (barrierGroup.Type == D3D12_BARRIER_TYPE_GLOBAL) {
      m_UnwrappedGlobalBarrierGroups.emplace_back(
          std::make_unique<std::vector<D3D12_GLOBAL_BARRIER>>());
      for (unsigned j = 0; j < barrierGroup.NumBarriers; ++j) {
        m_UnwrappedGlobalBarrierGroups.back()->push_back(barrierGroup.pGlobalBarriers[j]);
      }
      barrierGroupUnwrapped.pGlobalBarriers = m_UnwrappedGlobalBarrierGroups.back()->data();
    } else if (barrierGroup.Type == D3D12_BARRIER_TYPE_TEXTURE) {
      m_UnwrappedTextureBarrierGroups.emplace_back(
          std::make_unique<std::vector<D3D12_TEXTURE_BARRIER>>());
      for (unsigned j = 0; j < barrierGroup.NumBarriers; ++j) {
        IUnknownWrapper* wrapper =
            reinterpret_cast<IUnknownWrapper*>(barrierGroup.pTextureBarriers[j].pResource);
        arg.ResourceKeys[resourceKeyIndex++] = wrapper->GetKey();
        m_UnwrappedTextureBarrierGroups.back()->push_back(barrierGroup.pTextureBarriers[j]);
        m_UnwrappedTextureBarrierGroups.back()->back().pResource =
            wrapper->GetWrappedObject<ID3D12Resource>();
      }
      barrierGroupUnwrapped.pTextureBarriers = m_UnwrappedTextureBarrierGroups.back()->data();
    } else if (barrierGroup.Type == D3D12_BARRIER_TYPE_BUFFER) {
      m_UnwrappedBufferBarrierGroups.emplace_back(
          std::make_unique<std::vector<D3D12_BUFFER_BARRIER>>());
      for (unsigned j = 0; j < barrierGroup.NumBarriers; ++j) {
        IUnknownWrapper* wrapper =
            reinterpret_cast<IUnknownWrapper*>(barrierGroup.pBufferBarriers[j].pResource);
        arg.ResourceKeys[resourceKeyIndex++] = wrapper->GetKey();
        m_UnwrappedBufferBarrierGroups.back()->push_back(barrierGroup.pBufferBarriers[j]);
        m_UnwrappedBufferBarrierGroups.back()->back().pResource =
            wrapper->GetWrappedObject<ID3D12Resource>();
      }
      barrierGroupUnwrapped.pBufferBarriers = m_UnwrappedBufferBarrierGroups.back()->data();
    }
  }
}

UpdateInterface<D3D12_EXTENSION_ARGUMENTS_Argument, D3D12_EXTENSION_ARGUMENTS>::UpdateInterface(
    D3D12_EXTENSION_ARGUMENTS_Argument& arg, const D3D12_EXTENSION_ARGUMENTS* value) {
  if (!value) {
    return;
  }
  GITS_ASSERT(false, "UpdateInterface not implemented for D3D12_EXTENSION_ARGUMENTS_Argument");
}

UpdateInterface<D3D12_EXTENDED_OPERATION_DATA_Argument, D3D12_EXTENDED_OPERATION_DATA>::
    UpdateInterface(D3D12_EXTENDED_OPERATION_DATA_Argument& arg,
                    const D3D12_EXTENDED_OPERATION_DATA* value) {
  if (!value) {
    return;
  }
  GITS_ASSERT(false, "UpdateInterface not implemented for D3D12_EXTENDED_OPERATION_DATA_Argument");
}

UpdateInterface<PointerArgument<INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC>,
                INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC>::
    UpdateInterface(PointerArgument<INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC>& arg,
                    const INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC* value) {

  arg.Value = &m_UnwrapStructure;
  *arg.Value = *value;

  if (value->pD3D12Desc->pRootSignature) {
    IUnknownWrapper* wrapper =
        reinterpret_cast<IUnknownWrapper*>(value->pD3D12Desc->pRootSignature);
    arg.RootSignatureKey = wrapper->GetKey();
    arg.Value->pD3D12Desc->pRootSignature = wrapper->GetWrappedObject<ID3D12RootSignature>();
  }
}

static unsigned getDmlBindingCount(const DML_BINDING_DESC* bindings, unsigned count) {
  unsigned bindingCount = 0;
  for (unsigned i = 0; i < count; ++i) {
    if (bindings[i].Type == DML_BINDING_TYPE_BUFFER) {
      ++bindingCount;
    } else if (bindings[i].Type == DML_BINDING_TYPE_BUFFER_ARRAY) {
      auto* bindingArray =
          static_cast<DML_BUFFER_ARRAY_BINDING*>(const_cast<void*>(bindings[i].Desc));
      bindingCount += bindingArray->BindingCount;
    }
  }
  return bindingCount;
}

static unsigned updateDmlBinding(const void* srcBinding, DML_BUFFER_BINDING* dstBinding) {
  auto* binding = static_cast<DML_BUFFER_BINDING*>(const_cast<void*>(srcBinding));
  GITS_ASSERT(binding);
  if (!binding->Buffer) {
    return 0;
  }

  // Copy DML_BUFFER_BINDING into the temporary data and replace its pointer
  memcpy(dstBinding, srcBinding, sizeof(DML_BUFFER_BINDING));
  binding = dstBinding;

  IUnknownWrapper* wrapper = reinterpret_cast<IUnknownWrapper*>(binding->Buffer);
  binding->Buffer = wrapper->GetWrappedObject<ID3D12Resource>();
  return wrapper->GetKey();
}

UpdateInterface<DML_BINDING_TABLE_DESC_Argument, DML_BINDING_TABLE_DESC>::UpdateInterface(
    DML_BINDING_TABLE_DESC_Argument& arg, const DML_BINDING_TABLE_DESC* value) {

  arg.Value = &m_UnwrapStructure;
  *arg.Value = *value;

  if (value->Dispatchable) {
    IUnknownWrapper* wrapper = reinterpret_cast<IUnknownWrapper*>(value->Dispatchable);
    arg.TableFields.DispatchableKey = wrapper->GetKey();
    arg.Value->Dispatchable = wrapper->GetWrappedObject<IDMLDispatchable>();
  }
}

UpdateInterface<DML_BINDING_DESC_Argument, DML_BINDING_DESC>::UpdateInterface(
    DML_BINDING_DESC_Argument& arg, const DML_BINDING_DESC* value) {
  if (!arg.Value) {
    return;
  }

  arg.Value = &m_UnwrapStructure;
  *arg.Value = *value;

  arg.ResourceKeysSize = getDmlBindingCount(arg.Value, 1);
  arg.ResourceKeys.resize(arg.ResourceKeysSize);
  m_Bindings.resize(arg.ResourceKeysSize);

  if (arg.ResourceKeysSize == 0) {
    return;
  }

  if (arg.Value->Type == DML_BINDING_TYPE_BUFFER) {
    arg.ResourceKeys[0] = updateDmlBinding(arg.Value->Desc, &m_Bindings[0]);
    arg.Value->Desc = m_Bindings.data();
  } else if (arg.Value->Type == DML_BINDING_TYPE_BUFFER_ARRAY) {
    auto* bindingArray = static_cast<DML_BUFFER_ARRAY_BINDING*>(const_cast<void*>(arg.Value->Desc));
    GITS_ASSERT(bindingArray);
    for (unsigned i = 0; i < bindingArray->BindingCount; ++i) {
      arg.ResourceKeys[i] = updateDmlBinding(&bindingArray->Bindings[i], &m_Bindings[i]);
    }
    m_BindingArray = *bindingArray;
    m_BindingArray.Bindings = m_Bindings.data();
    arg.Value->Desc = &m_BindingArray;
  }
}

UpdateInterface<DML_BINDING_DESCs_Argument, DML_BINDING_DESC>::UpdateInterface(
    DML_BINDING_DESCs_Argument& arg, const DML_BINDING_DESC* value) {
  if (!arg.Value) {
    return;
  }
  GITS_ASSERT(arg.Size > 0);

  m_BindingArrays.resize(arg.Size);
  m_UnwrapStructures.resize(arg.Size);
  m_UnwrapStructure = m_UnwrapStructures.data();
  arg.Value = m_UnwrapStructure;

  auto bindingCount = getDmlBindingCount(value, arg.Size);
  m_Bindings.resize(bindingCount);
  arg.ResourceKeysSize = bindingCount;
  arg.ResourceKeys.resize(bindingCount);

  memcpy(arg.Value, value, arg.Size * sizeof(DML_BINDING_DESC));

  unsigned bindingIdx = 0;
  for (unsigned i = 0; i < arg.Size; ++i) {
    if (bindingIdx >= bindingCount) {
      return;
    }
    auto* bindingSrc = &arg.Value[i];
    auto* bindingDst = &m_Bindings[bindingIdx];
    if (bindingSrc->Type == DML_BINDING_TYPE_BUFFER) {
      arg.ResourceKeys[bindingIdx] = updateDmlBinding(bindingSrc->Desc, bindingDst);
      bindingSrc->Desc = bindingDst;
      ++bindingIdx;
    } else if (bindingSrc->Type == DML_BINDING_TYPE_BUFFER_ARRAY) {
      auto* bindingArray =
          static_cast<DML_BUFFER_ARRAY_BINDING*>(const_cast<void*>(bindingSrc->Desc));
      GITS_ASSERT(bindingArray);
      m_BindingArrays[i] = *bindingArray;
      bindingArray = &m_BindingArrays[i];
      for (unsigned j = 0; j < bindingArray->BindingCount; ++j) {
        arg.ResourceKeys[bindingIdx] = updateDmlBinding(&bindingArray->Bindings[j], &bindingDst[j]);
        ++bindingIdx;
      }
      bindingArray->Bindings = bindingDst;
      bindingSrc->Desc = bindingArray;
    }
  }
}

UpdateInterface<DML_GRAPH_DESC_Argument, DML_GRAPH_DESC>::UpdateInterface(
    DML_GRAPH_DESC_Argument& arg, const DML_GRAPH_DESC* value) {
  if (!arg.Value) {
    return;
  }

  arg.Value = &m_UnwrapStructure;
  *arg.Value = *value;

  m_Nodes.resize(value->NodeCount);
  m_OpNodes.resize(value->NodeCount);
  arg.OperatorKeys.resize(value->NodeCount);
  arg.OperatorKeysSize = value->NodeCount;
  arg.Value->Nodes = m_Nodes.data();

  memcpy(m_Nodes.data(), value->Nodes, value->NodeCount * sizeof(DML_GRAPH_NODE_DESC));

  for (unsigned i = 0; i < value->NodeCount; ++i) {
    auto* node = &m_Nodes[i];
    if (node->Type == DML_GRAPH_NODE_TYPE_OPERATOR) {
      auto* opNode = static_cast<DML_OPERATOR_GRAPH_NODE_DESC*>(const_cast<void*>(node->Desc));
      m_OpNodes[i] = *opNode;
      opNode = &m_OpNodes[i];
      if (opNode->Operator) {
        IUnknownWrapper* wrapper = reinterpret_cast<IUnknownWrapper*>(opNode->Operator);
        opNode->Operator = wrapper->GetWrappedObject<IDMLOperator>();
        arg.OperatorKeys[i] = wrapper->GetKey();
      }
      node->Desc = opNode;
    }
  }
}

UpdateInterface<xess_d3d12_init_params_t_Argument, xess_d3d12_init_params_t>::UpdateInterface(
    xess_d3d12_init_params_t_Argument& arg, const xess_d3d12_init_params_t* value) {

  arg.Value = &m_UnwrapStructure;
  *arg.Value = *value;

  if (value->pTempBufferHeap) {
    IUnknownWrapper* wrapper = reinterpret_cast<IUnknownWrapper*>(value->pTempBufferHeap);
    arg.TempBufferHeapKey = wrapper->GetKey();
    arg.Value->pTempBufferHeap = wrapper->GetWrappedObject<ID3D12Heap>();
  }
  if (value->pTempTextureHeap) {
    IUnknownWrapper* wrapper = reinterpret_cast<IUnknownWrapper*>(value->pTempTextureHeap);
    arg.TempTextureHeapKey = wrapper->GetKey();
    arg.Value->pTempTextureHeap = wrapper->GetWrappedObject<ID3D12Heap>();
  }
  if (value->pPipelineLibrary) {
    IUnknownWrapper* wrapper = reinterpret_cast<IUnknownWrapper*>(value->pPipelineLibrary);
    arg.PipelineLibraryKey = wrapper->GetKey();
    arg.Value->pPipelineLibrary = wrapper->GetWrappedObject<ID3D12PipelineLibrary>();
  }
}

UpdateInterface<xess_d3d12_execute_params_t_Argument, xess_d3d12_execute_params_t>::UpdateInterface(
    xess_d3d12_execute_params_t_Argument& arg, const xess_d3d12_execute_params_t* value) {

  arg.Value = &m_UnwrapStructure;
  *arg.Value = *value;

  if (value->pColorTexture) {
    IUnknownWrapper* wrapper = reinterpret_cast<IUnknownWrapper*>(value->pColorTexture);
    arg.ColorTextureKey = wrapper->GetKey();
    arg.Value->pColorTexture = wrapper->GetWrappedObject<ID3D12Resource>();
  }
  if (value->pVelocityTexture) {
    IUnknownWrapper* wrapper = reinterpret_cast<IUnknownWrapper*>(value->pVelocityTexture);
    arg.VelocityTextureKey = wrapper->GetKey();
    arg.Value->pVelocityTexture = wrapper->GetWrappedObject<ID3D12Resource>();
  }
  if (value->pDepthTexture) {
    IUnknownWrapper* wrapper = reinterpret_cast<IUnknownWrapper*>(value->pDepthTexture);
    arg.DepthTextureKey = wrapper->GetKey();
    arg.Value->pDepthTexture = wrapper->GetWrappedObject<ID3D12Resource>();
  }
  if (value->pExposureScaleTexture) {
    IUnknownWrapper* wrapper = reinterpret_cast<IUnknownWrapper*>(value->pExposureScaleTexture);
    arg.ExposureScaleTextureKey = wrapper->GetKey();
    arg.Value->pExposureScaleTexture = wrapper->GetWrappedObject<ID3D12Resource>();
  }
  if (value->pResponsivePixelMaskTexture) {
    IUnknownWrapper* wrapper =
        reinterpret_cast<IUnknownWrapper*>(value->pResponsivePixelMaskTexture);
    arg.ResponsivePixelMaskTextureKey = wrapper->GetKey();
    arg.Value->pResponsivePixelMaskTexture = wrapper->GetWrappedObject<ID3D12Resource>();
  }
  if (value->pOutputTexture) {
    IUnknownWrapper* wrapper = reinterpret_cast<IUnknownWrapper*>(value->pOutputTexture);
    arg.OutputTextureKey = wrapper->GetKey();
    arg.Value->pOutputTexture = wrapper->GetWrappedObject<ID3D12Resource>();
  }
  if (value->pDescriptorHeap) {
    IUnknownWrapper* wrapper = reinterpret_cast<IUnknownWrapper*>(value->pDescriptorHeap);
    arg.DescriptorHeapKey = wrapper->GetKey();
    arg.Value->pDescriptorHeap = wrapper->GetWrappedObject<ID3D12DescriptorHeap>();
  }
}

UpdateInterface<DSTORAGE_QUEUE_DESC_Argument, DSTORAGE_QUEUE_DESC>::UpdateInterface(
    DSTORAGE_QUEUE_DESC_Argument& arg, const DSTORAGE_QUEUE_DESC* value) {
  arg.Value = &m_UnwrapStructure;
  *arg.Value = *value;
  if (value->Device) {
    IUnknownWrapper* wrapper = reinterpret_cast<IUnknownWrapper*>(value->Device);
    arg.DeviceKey = wrapper->GetKey();
    arg.Value->Device = wrapper->GetWrappedObject<ID3D12Device>();
  }
}

UpdateInterface<DSTORAGE_REQUEST_Argument, DSTORAGE_REQUEST>::UpdateInterface(
    DSTORAGE_REQUEST_Argument& arg, const DSTORAGE_REQUEST* value) {
  arg.Value = &m_UnwrapStructure;
  *arg.Value = *value;
  switch (value->Options.SourceType) {
  case DSTORAGE_REQUEST_SOURCE_FILE:
    if (value->Source.File.Source) {
      IUnknownWrapper* wrapper = reinterpret_cast<IUnknownWrapper*>(value->Source.File.Source);
      arg.FileKey = wrapper->GetKey();
      arg.Value->Source.File.Source = wrapper->GetWrappedObject<IDStorageFile>();
    }
    break;
  }
  switch (value->Options.DestinationType) {
  case DSTORAGE_REQUEST_DESTINATION_BUFFER:
    if (value->Destination.Buffer.Resource) {
      IUnknownWrapper* wrapper =
          reinterpret_cast<IUnknownWrapper*>(value->Destination.Buffer.Resource);
      arg.ResourceKey = wrapper->GetKey();
      arg.Value->Destination.Buffer.Resource = wrapper->GetWrappedObject<ID3D12Resource>();
    }
    break;
  case DSTORAGE_REQUEST_DESTINATION_TEXTURE_REGION:
    if (value->Destination.Texture.Resource) {
      IUnknownWrapper* wrapper =
          reinterpret_cast<IUnknownWrapper*>(value->Destination.Texture.Resource);
      arg.ResourceKey = wrapper->GetKey();
      arg.Value->Destination.Texture.Resource = wrapper->GetWrappedObject<ID3D12Resource>();
    }
    break;
  case DSTORAGE_REQUEST_DESTINATION_MULTIPLE_SUBRESOURCES:
    if (value->Destination.MultipleSubresources.Resource) {
      IUnknownWrapper* wrapper =
          reinterpret_cast<IUnknownWrapper*>(value->Destination.MultipleSubresources.Resource);
      arg.ResourceKey = wrapper->GetKey();
      arg.Value->Destination.MultipleSubresources.Resource =
          wrapper->GetWrappedObject<ID3D12Resource>();
    }
    break;
  case DSTORAGE_REQUEST_DESTINATION_TILES:
    if (value->Destination.Tiles.Resource) {
      IUnknownWrapper* wrapper =
          reinterpret_cast<IUnknownWrapper*>(value->Destination.Tiles.Resource);
      arg.ResourceKey = wrapper->GetKey();
      arg.Value->Destination.Tiles.Resource = wrapper->GetWrappedObject<ID3D12Resource>();
    }
    break;
  }
}

UpdateInterface<xefg_swapchain_d3d12_init_params_t_Argument, xefg_swapchain_d3d12_init_params_t>::
    UpdateInterface(xefg_swapchain_d3d12_init_params_t_Argument& arg,
                    const xefg_swapchain_d3d12_init_params_t* value) {
  arg.Value = &m_UnwrapStructure;
  *arg.Value = *value;
  if (value->pApplicationSwapChain) {
    IUnknownWrapper* wrapper = nullptr;
    if (value->pApplicationSwapChain->QueryInterface(IID_IUnknownWrapper,
                                                     reinterpret_cast<void**>(&wrapper)) != S_OK) {
      return;
    }
    arg.ApplicationSwapChainKey = wrapper->GetKey();
    arg.Value->pApplicationSwapChain = wrapper->GetWrappedObject<IDXGISwapChain>();
  }
  if (value->pTempBufferHeap) {
    IUnknownWrapper* wrapper = reinterpret_cast<IUnknownWrapper*>(value->pTempBufferHeap);
    arg.TempBufferHeapKey = wrapper->GetKey();
    arg.Value->pTempBufferHeap = wrapper->GetWrappedObject<ID3D12Heap>();
  }
  if (value->pTempTextureHeap) {
    IUnknownWrapper* wrapper = reinterpret_cast<IUnknownWrapper*>(value->pTempTextureHeap);
    arg.TempTextureHeapKey = wrapper->GetKey();
    arg.Value->pTempTextureHeap = wrapper->GetWrappedObject<ID3D12Heap>();
  }
  if (value->pPipelineLibrary) {
    IUnknownWrapper* wrapper = reinterpret_cast<IUnknownWrapper*>(value->pPipelineLibrary);
    arg.PipelineLibraryKey = wrapper->GetKey();
    arg.Value->pPipelineLibrary = wrapper->GetWrappedObject<ID3D12PipelineLibrary>();
  }
}

UpdateInterface<xefg_swapchain_d3d12_resource_data_t_Argument,
                xefg_swapchain_d3d12_resource_data_t>::
    UpdateInterface(xefg_swapchain_d3d12_resource_data_t_Argument& arg,
                    const xefg_swapchain_d3d12_resource_data_t* value) {
  arg.Value = &m_UnwrapStructure;
  *arg.Value = *value;
  if (value->pResource) {
    IUnknownWrapper* wrapper = reinterpret_cast<IUnknownWrapper*>(value->pResource);
    arg.ResourceKey = wrapper->GetKey();
    arg.Value->pResource = wrapper->GetWrappedObject<ID3D12Resource>();
  }
}

} // namespace DirectX
} // namespace gits
