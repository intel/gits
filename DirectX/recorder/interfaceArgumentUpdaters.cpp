// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "interfaceArgumentUpdaters.h"
#include "gits.h"

#include <d3dx12/d3dx12_pipeline_state_stream.h>

namespace gits {
namespace DirectX {

UpdateInterface<D3D12_TEXTURE_COPY_LOCATION_Argument, D3D12_TEXTURE_COPY_LOCATION>::UpdateInterface(
    D3D12_TEXTURE_COPY_LOCATION_Argument& arg, const D3D12_TEXTURE_COPY_LOCATION* value) {

  arg.value = &unwrapStructure_;
  *arg.value = *value;

  if (value->pResource) {
    IUnknownWrapper* wrapper = reinterpret_cast<IUnknownWrapper*>(value->pResource);
    arg.resourceKey = wrapper->getKey();
    arg.value->pResource = wrapper->getWrappedObject<ID3D12Resource>();
  }
}

UpdateInterface<D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument, D3D12_GRAPHICS_PIPELINE_STATE_DESC>::
    UpdateInterface(D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument& arg,
                    const D3D12_GRAPHICS_PIPELINE_STATE_DESC* value) {

  arg.value = &unwrapStructure_;
  *arg.value = *value;

  if (value->pRootSignature) {
    IUnknownWrapper* wrapper = reinterpret_cast<IUnknownWrapper*>(value->pRootSignature);
    arg.rootSignatureKey = wrapper->getKey();
    arg.value->pRootSignature = wrapper->getWrappedObject<ID3D12RootSignature>();
  }
}

UpdateInterface<D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument, D3D12_COMPUTE_PIPELINE_STATE_DESC>::
    UpdateInterface(D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument& arg,
                    const D3D12_COMPUTE_PIPELINE_STATE_DESC* value) {

  arg.value = &unwrapStructure_;
  *arg.value = *value;

  if (value->pRootSignature) {
    IUnknownWrapper* wrapper = reinterpret_cast<IUnknownWrapper*>(value->pRootSignature);
    arg.rootSignatureKey = wrapper->getKey();
    arg.value->pRootSignature = wrapper->getWrappedObject<ID3D12RootSignature>();
  }
}

UpdateInterface<D3D12_RESOURCE_BARRIERs_Argument, D3D12_RESOURCE_BARRIER>::UpdateInterface(
    D3D12_RESOURCE_BARRIERs_Argument& arg, const D3D12_RESOURCE_BARRIER* value) {

  unwrapped_ = unwrappedStatic_;
  if (arg.size > NUM) {
    unwrappedDynamic_.resize(arg.size);
    unwrapped_ = unwrappedDynamic_.data();
  }
  arg.value = unwrapped_;

  for (unsigned i = 0; i < arg.size; ++i) {

    arg.value[i] = value[i];
    D3D12_RESOURCE_BARRIER& barrier = arg.value[i];

    switch (barrier.Type) {
    case D3D12_RESOURCE_BARRIER_TYPE_ALIASING:
      if (barrier.Aliasing.pResourceBefore) {
        IUnknownWrapper* wrapper =
            reinterpret_cast<IUnknownWrapper*>(barrier.Aliasing.pResourceBefore);
        arg.resourceKeys[i] = wrapper->getKey();
        barrier.Aliasing.pResourceBefore = wrapper->getWrappedObject<ID3D12Resource>();
      }
      if (barrier.Aliasing.pResourceAfter) {
        IUnknownWrapper* wrapper =
            reinterpret_cast<IUnknownWrapper*>(barrier.Aliasing.pResourceAfter);
        arg.resourceAfterKeys[i] = wrapper->getKey();
        barrier.Aliasing.pResourceAfter = wrapper->getWrappedObject<ID3D12Resource>();
      }
      break;

    case D3D12_RESOURCE_BARRIER_TYPE_TRANSITION:
      if (barrier.Transition.pResource) {
        IUnknownWrapper* wrapper = reinterpret_cast<IUnknownWrapper*>(barrier.Transition.pResource);
        arg.resourceKeys[i] = wrapper->getKey();
        barrier.Transition.pResource = wrapper->getWrappedObject<ID3D12Resource>();
      }
      break;

    case D3D12_RESOURCE_BARRIER_TYPE_UAV:
      if (barrier.UAV.pResource) {
        IUnknownWrapper* wrapper = reinterpret_cast<IUnknownWrapper*>(barrier.UAV.pResource);
        arg.resourceKeys[i] = wrapper->getKey();
        barrier.UAV.pResource = wrapper->getWrappedObject<ID3D12Resource>();
      }
      break;
    }
  }
}

UpdateInterface<D3D12_PIPELINE_STATE_STREAM_DESC_Argument, D3D12_PIPELINE_STATE_STREAM_DESC>::
    UpdateInterface(D3D12_PIPELINE_STATE_STREAM_DESC_Argument& arg,
                    const D3D12_PIPELINE_STATE_STREAM_DESC* stateStreamtDesc) {

  streamDescUnwrapped_ = *stateStreamtDesc;
  subobjectsUnwrapped_.resize(stateStreamtDesc->SizeInBytes);
  streamDescUnwrapped_.pPipelineStateSubobjectStream = subobjectsUnwrapped_.data();
  memcpy(subobjectsUnwrapped_.data(), stateStreamtDesc->pPipelineStateSubobjectStream,
         stateStreamtDesc->SizeInBytes);

  arg.value = &streamDescUnwrapped_;

  size_t offset = 0;
  while (offset < stateStreamtDesc->SizeInBytes) {
    uint8_t* subobjectData = subobjectsUnwrapped_.data() + offset;
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE subobjectType =
        *reinterpret_cast<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE*>(subobjectData);

    if (subobjectType == D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE) {
      auto* rootSignatureSubobject =
          reinterpret_cast<CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE*>(subobjectData);

      ID3D12RootSignature* rootSignature = *rootSignatureSubobject;
      IUnknownWrapper* wrapper = reinterpret_cast<IUnknownWrapper*>(rootSignature);
      arg.rootSignatureKey = wrapper->getKey();
      *rootSignatureSubobject = wrapper->getWrappedObject<ID3D12RootSignature>();

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

  arg.value = &stateObjectDescUnwrapped_;
  *arg.value = *stateObjectDesc;

  subobjectsUnwrapped_.reserve(stateObjectDesc->NumSubobjects);
  localSignatures_.reserve(stateObjectDesc->NumSubobjects);
  existingCollectionDescs.reserve(stateObjectDesc->NumSubobjects);
  subobjectToExportsAssociations_.reserve(stateObjectDesc->NumSubobjects);

  for (unsigned index = 0; index < stateObjectDesc->NumSubobjects; ++index) {

    const D3D12_STATE_SUBOBJECT* subobject = &(stateObjectDesc->pSubobjects[index]);
    switch (subobject->Type) {

    case D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE: {
      globalSignatureUnwrapped_ =
          *static_cast<D3D12_GLOBAL_ROOT_SIGNATURE*>(const_cast<void*>(subobject->pDesc));
      IUnknownWrapper* wrapper =
          reinterpret_cast<IUnknownWrapper*>(globalSignatureUnwrapped_.pGlobalRootSignature);
      arg.interfaceKeysBySubobject[index] = wrapper->getKey();
      globalSignatureUnwrapped_.pGlobalRootSignature =
          wrapper->getWrappedObject<ID3D12RootSignature>();

      subobjectsUnwrapped_.push_back(
          D3D12_STATE_SUBOBJECT{subobject->Type, &globalSignatureUnwrapped_});
      wrappedSubobjectIndexes_[subobject] = index;
    } break;

    case D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE: {
      localSignatures_.push_back(
          *static_cast<D3D12_LOCAL_ROOT_SIGNATURE*>(const_cast<void*>(subobject->pDesc)));

      D3D12_LOCAL_ROOT_SIGNATURE* localRootSignature = &localSignatures_.back();
      IUnknownWrapper* wrapper =
          reinterpret_cast<IUnknownWrapper*>(localRootSignature->pLocalRootSignature);
      arg.interfaceKeysBySubobject[index] = wrapper->getKey();
      localRootSignature->pLocalRootSignature = wrapper->getWrappedObject<ID3D12RootSignature>();

      subobjectsUnwrapped_.push_back(D3D12_STATE_SUBOBJECT{subobject->Type, localRootSignature});
      wrappedSubobjectIndexes_[subobject] = index;
    } break;

    case D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION: {
      existingCollectionDescs.push_back(
          *static_cast<D3D12_EXISTING_COLLECTION_DESC*>(const_cast<void*>(subobject->pDesc)));

      D3D12_EXISTING_COLLECTION_DESC* desc = &existingCollectionDescs.back();
      IUnknownWrapper* wrapper = reinterpret_cast<IUnknownWrapper*>(desc->pExistingCollection);
      arg.interfaceKeysBySubobject[index] = wrapper->getKey();
      desc->pExistingCollection = wrapper->getWrappedObject<ID3D12StateObject>();

      subobjectsUnwrapped_.push_back(D3D12_STATE_SUBOBJECT{subobject->Type, desc});
      wrappedSubobjectIndexes_[subobject] = index;
    } break;

    default:
      subobjectsUnwrapped_.push_back(D3D12_STATE_SUBOBJECT{subobject->Type, subobject->pDesc});
      wrappedSubobjectIndexes_[subobject] = index;
      break;
    }
  }

  for (D3D12_STATE_SUBOBJECT& subobject : subobjectsUnwrapped_) {

    if (subobject.Type == D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION) {
      subobjectToExportsAssociations_.push_back(
          *static_cast<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(
              const_cast<void*>(subobject.pDesc)));

      auto it = wrappedSubobjectIndexes_.find(
          subobjectToExportsAssociations_.back().pSubobjectToAssociate);
      GITS_ASSERT(it != wrappedSubobjectIndexes_.end());
      unsigned associateIndex = it->second;
      subobjectToExportsAssociations_.back().pSubobjectToAssociate =
          &subobjectsUnwrapped_[associateIndex];

      subobject.pDesc = &subobjectToExportsAssociations_.back();
    }
  }

  stateObjectDescUnwrapped_ = *const_cast<D3D12_STATE_OBJECT_DESC*>(stateObjectDesc);
  stateObjectDescUnwrapped_.pSubobjects = subobjectsUnwrapped_.data();
}

UpdateInterface<D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument,
                D3D12_RENDER_PASS_RENDER_TARGET_DESC>::
    UpdateInterface(D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument& arg,
                    const D3D12_RENDER_PASS_RENDER_TARGET_DESC* value) {
  if (!value) {
    return;
  }
  unwrapStructures_.reserve(arg.size);
  for (unsigned i = 0; i < arg.size; ++i) {
    unwrapStructures_[i] = arg.value[i];
    if (unwrapStructures_[i].EndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE) {
      if (unwrapStructures_[i].EndingAccess.Resolve.pSrcResource) {
        IUnknownWrapper* wrapperSrcResource = reinterpret_cast<IUnknownWrapper*>(
            unwrapStructures_[i].EndingAccess.Resolve.pSrcResource);
        arg.resolveSrcResourceKeys.push_back(wrapperSrcResource->getKey());
        unwrapStructures_[i].EndingAccess.Resolve.pSrcResource =
            wrapperSrcResource->getWrappedObject<ID3D12Resource>();
      }
      if (unwrapStructures_[i].EndingAccess.Resolve.pDstResource) {
        IUnknownWrapper* wrapperDstResource = reinterpret_cast<IUnknownWrapper*>(
            unwrapStructures_[i].EndingAccess.Resolve.pDstResource);
        arg.resolveDstResourceKeys.push_back(wrapperDstResource->getKey());
        unwrapStructures_[i].EndingAccess.Resolve.pDstResource =
            wrapperDstResource->getWrappedObject<ID3D12Resource>();
      }
    }
  }
  arg.value = unwrapStructures_.data();
}

UpdateInterface<D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument,
                D3D12_RENDER_PASS_DEPTH_STENCIL_DESC>::
    UpdateInterface(D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument& arg,
                    const D3D12_RENDER_PASS_DEPTH_STENCIL_DESC* value) {
  if (!value) {
    return;
  }
  arg.value = &unwrapStructure_;
  *arg.value = *value;

  if (arg.value->DepthEndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE) {
    if (arg.value->DepthEndingAccess.Resolve.pSrcResource) {
      IUnknownWrapper* wrapperSrcResource =
          reinterpret_cast<IUnknownWrapper*>(arg.value->DepthEndingAccess.Resolve.pSrcResource);
      arg.resolveSrcDepthKey = wrapperSrcResource->getKey();
      arg.value->DepthEndingAccess.Resolve.pSrcResource =
          wrapperSrcResource->getWrappedObject<ID3D12Resource>();
    }
    if (arg.value->DepthEndingAccess.Resolve.pDstResource) {
      IUnknownWrapper* wrapperDstResource =
          reinterpret_cast<IUnknownWrapper*>(arg.value->DepthEndingAccess.Resolve.pDstResource);
      arg.resolveDstDepthKey = wrapperDstResource->getKey();
      arg.value->DepthEndingAccess.Resolve.pDstResource =
          wrapperDstResource->getWrappedObject<ID3D12Resource>();
    }
  }

  if (arg.value->StencilEndingAccess.Type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE) {
    if (arg.value->StencilEndingAccess.Resolve.pSrcResource) {
      IUnknownWrapper* wrapperSrcResource =
          reinterpret_cast<IUnknownWrapper*>(arg.value->StencilEndingAccess.Resolve.pSrcResource);
      arg.resolveSrcStencilKey = wrapperSrcResource->getKey();
      arg.value->StencilEndingAccess.Resolve.pSrcResource =
          wrapperSrcResource->getWrappedObject<ID3D12Resource>();
    }
    if (arg.value->StencilEndingAccess.Resolve.pDstResource) {
      IUnknownWrapper* wrapperDstResource =
          reinterpret_cast<IUnknownWrapper*>(arg.value->StencilEndingAccess.Resolve.pDstResource);
      arg.resolveDstStencilKey = wrapperDstResource->getKey();
      arg.value->StencilEndingAccess.Resolve.pDstResource =
          wrapperDstResource->getWrappedObject<ID3D12Resource>();
    }
  }
}

UpdateInterface<D3D12_BARRIER_GROUPs_Argument, D3D12_BARRIER_GROUP>::UpdateInterface(
    D3D12_BARRIER_GROUPs_Argument& arg, const D3D12_BARRIER_GROUP* value) {

  unwrapped_.resize(arg.size);
  arg.value = unwrapped_.data();

  unsigned resourceKeyIndex = 0;

  for (unsigned i = 0; i < arg.size; ++i) {
    const D3D12_BARRIER_GROUP& barrierGroup = value[i];
    D3D12_BARRIER_GROUP& barrierGroupUnwrapped = unwrapped_[i];
    barrierGroupUnwrapped = barrierGroup;

    if (barrierGroup.Type == D3D12_BARRIER_TYPE_GLOBAL) {
      unwrappedGlobalBarrierGroups_.emplace_back(
          std::make_unique<std::vector<D3D12_GLOBAL_BARRIER>>());
      for (unsigned j = 0; j < barrierGroup.NumBarriers; ++j) {
        unwrappedGlobalBarrierGroups_.back()->push_back(barrierGroup.pGlobalBarriers[j]);
      }
      barrierGroupUnwrapped.pGlobalBarriers = unwrappedGlobalBarrierGroups_.back()->data();
    } else if (barrierGroup.Type == D3D12_BARRIER_TYPE_TEXTURE) {
      unwrappedTextureBarrierGroups_.emplace_back(
          std::make_unique<std::vector<D3D12_TEXTURE_BARRIER>>());
      for (unsigned j = 0; j < barrierGroup.NumBarriers; ++j) {
        IUnknownWrapper* wrapper =
            reinterpret_cast<IUnknownWrapper*>(barrierGroup.pTextureBarriers[j].pResource);
        arg.resourceKeys[resourceKeyIndex++] = wrapper->getKey();
        unwrappedTextureBarrierGroups_.back()->push_back(barrierGroup.pTextureBarriers[j]);
        unwrappedTextureBarrierGroups_.back()->back().pResource =
            wrapper->getWrappedObject<ID3D12Resource>();
      }
      barrierGroupUnwrapped.pTextureBarriers = unwrappedTextureBarrierGroups_.back()->data();
    } else if (barrierGroup.Type == D3D12_BARRIER_TYPE_BUFFER) {
      unwrappedBufferBarrierGroups_.emplace_back(
          std::make_unique<std::vector<D3D12_BUFFER_BARRIER>>());
      for (unsigned j = 0; j < barrierGroup.NumBarriers; ++j) {
        IUnknownWrapper* wrapper =
            reinterpret_cast<IUnknownWrapper*>(barrierGroup.pBufferBarriers[j].pResource);
        arg.resourceKeys[resourceKeyIndex++] = wrapper->getKey();
        unwrappedBufferBarrierGroups_.back()->push_back(barrierGroup.pBufferBarriers[j]);
        unwrappedBufferBarrierGroups_.back()->back().pResource =
            wrapper->getWrappedObject<ID3D12Resource>();
      }
      barrierGroupUnwrapped.pBufferBarriers = unwrappedBufferBarrierGroups_.back()->data();
    }
  }
}

UpdateInterface<PointerArgument<INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC>,
                INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC>::
    UpdateInterface(PointerArgument<INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC>& arg,
                    const INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC* value) {

  arg.value = &unwrapStructure_;
  *arg.value = *value;

  if (value->pD3D12Desc->pRootSignature) {
    IUnknownWrapper* wrapper =
        reinterpret_cast<IUnknownWrapper*>(value->pD3D12Desc->pRootSignature);
    arg.rootSignatureKey = wrapper->getKey();
    arg.value->pD3D12Desc->pRootSignature = wrapper->getWrappedObject<ID3D12RootSignature>();
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
  binding->Buffer = wrapper->getWrappedObject<ID3D12Resource>();
  return wrapper->getKey();
}

UpdateInterface<DML_BINDING_TABLE_DESC_Argument, DML_BINDING_TABLE_DESC>::UpdateInterface(
    DML_BINDING_TABLE_DESC_Argument& arg, const DML_BINDING_TABLE_DESC* value) {

  arg.value = &unwrapStructure_;
  *arg.value = *value;

  if (value->Dispatchable) {
    IUnknownWrapper* wrapper = reinterpret_cast<IUnknownWrapper*>(value->Dispatchable);
    arg.data.dispatchableKey = wrapper->getKey();
    arg.value->Dispatchable = wrapper->getWrappedObject<IDMLDispatchable>();
  }
}

UpdateInterface<DML_BINDING_DESC_Argument, DML_BINDING_DESC>::UpdateInterface(
    DML_BINDING_DESC_Argument& arg, const DML_BINDING_DESC* value) {
  if (!arg.value) {
    return;
  }

  arg.value = &unwrapStructure_;
  *arg.value = *value;

  arg.resourceKeysSize = getDmlBindingCount(arg.value, 1);
  arg.resourceKeys.resize(arg.resourceKeysSize);
  bindings_.resize(arg.resourceKeysSize);

  if (arg.resourceKeysSize == 0) {
    return;
  }

  if (arg.value->Type == DML_BINDING_TYPE_BUFFER) {
    arg.resourceKeys[0] = updateDmlBinding(arg.value->Desc, &bindings_[0]);
    arg.value->Desc = bindings_.data();
  } else if (arg.value->Type == DML_BINDING_TYPE_BUFFER_ARRAY) {
    auto* bindingArray = static_cast<DML_BUFFER_ARRAY_BINDING*>(const_cast<void*>(arg.value->Desc));
    GITS_ASSERT(bindingArray);
    for (unsigned i = 0; i < bindingArray->BindingCount; ++i) {
      arg.resourceKeys[i] = updateDmlBinding(&bindingArray->Bindings[i], &bindings_[i]);
    }
    bindingArray_ = *bindingArray;
    bindingArray_.Bindings = bindings_.data();
    arg.value->Desc = &bindingArray_;
  }
}

UpdateInterface<DML_BINDING_DESCs_Argument, DML_BINDING_DESC>::UpdateInterface(
    DML_BINDING_DESCs_Argument& arg, const DML_BINDING_DESC* value) {
  if (!arg.value) {
    return;
  }
  GITS_ASSERT(arg.size > 0);

  bindingArrays_.resize(arg.size);
  unwrapStructures_.resize(arg.size);
  unwrapStructure_ = unwrapStructures_.data();
  arg.value = unwrapStructure_;

  auto bindingCount = getDmlBindingCount(value, arg.size);
  bindings_.resize(bindingCount);
  arg.resourceKeysSize = bindingCount;
  arg.resourceKeys.resize(bindingCount);

  memcpy(arg.value, value, arg.size * sizeof(DML_BINDING_DESC));

  unsigned bindingIdx = 0;
  for (unsigned i = 0; i < arg.size; ++i) {
    if (bindingIdx >= bindingCount) {
      return;
    }
    auto* bindingSrc = &arg.value[i];
    auto* bindingDst = &bindings_[bindingIdx];
    if (bindingSrc->Type == DML_BINDING_TYPE_BUFFER) {
      arg.resourceKeys[bindingIdx] = updateDmlBinding(bindingSrc->Desc, bindingDst);
      bindingSrc->Desc = bindingDst;
      ++bindingIdx;
    } else if (bindingSrc->Type == DML_BINDING_TYPE_BUFFER_ARRAY) {
      auto* bindingArray =
          static_cast<DML_BUFFER_ARRAY_BINDING*>(const_cast<void*>(bindingSrc->Desc));
      GITS_ASSERT(bindingArray);
      bindingArrays_[i] = *bindingArray;
      bindingArray = &bindingArrays_[i];
      for (unsigned j = 0; j < bindingArray->BindingCount; ++j) {
        arg.resourceKeys[bindingIdx] = updateDmlBinding(&bindingArray->Bindings[j], &bindingDst[j]);
        ++bindingIdx;
      }
      bindingArray->Bindings = bindingDst;
      bindingSrc->Desc = bindingArray;
    }
  }
}

UpdateInterface<DML_GRAPH_DESC_Argument, DML_GRAPH_DESC>::UpdateInterface(
    DML_GRAPH_DESC_Argument& arg, const DML_GRAPH_DESC* value) {
  if (!arg.value) {
    return;
  }

  arg.value = &unwrapStructure_;
  *arg.value = *value;

  nodes_.resize(value->NodeCount);
  opNodes_.resize(value->NodeCount);
  arg.operatorKeys.resize(value->NodeCount);
  arg.operatorKeysSize = value->NodeCount;
  arg.value->Nodes = nodes_.data();

  memcpy(nodes_.data(), value->Nodes, value->NodeCount * sizeof(DML_GRAPH_NODE_DESC));

  for (unsigned i = 0; i < value->NodeCount; ++i) {
    auto* node = &nodes_[i];
    if (node->Type == DML_GRAPH_NODE_TYPE_OPERATOR) {
      auto* opNode = static_cast<DML_OPERATOR_GRAPH_NODE_DESC*>(const_cast<void*>(node->Desc));
      opNodes_[i] = *opNode;
      opNode = &opNodes_[i];
      if (opNode->Operator) {
        IUnknownWrapper* wrapper = reinterpret_cast<IUnknownWrapper*>(opNode->Operator);
        opNode->Operator = wrapper->getWrappedObject<IDMLOperator>();
        arg.operatorKeys[i] = wrapper->getKey();
      }
      node->Desc = opNode;
    }
  }
}

UpdateInterface<xess_d3d12_init_params_t_Argument, xess_d3d12_init_params_t>::UpdateInterface(
    xess_d3d12_init_params_t_Argument& arg, const xess_d3d12_init_params_t* value) {

  arg.value = &unwrapStructure_;
  *arg.value = *value;

  if (value->pTempBufferHeap) {
    IUnknownWrapper* wrapper = reinterpret_cast<IUnknownWrapper*>(value->pTempBufferHeap);
    arg.tempBufferHeapKey = wrapper->getKey();
    arg.value->pTempBufferHeap = wrapper->getWrappedObject<ID3D12Heap>();
  }
  if (value->pTempTextureHeap) {
    IUnknownWrapper* wrapper = reinterpret_cast<IUnknownWrapper*>(value->pTempTextureHeap);
    arg.tempTextureHeapKey = wrapper->getKey();
    arg.value->pTempTextureHeap = wrapper->getWrappedObject<ID3D12Heap>();
  }
  if (value->pPipelineLibrary) {
    IUnknownWrapper* wrapper = reinterpret_cast<IUnknownWrapper*>(value->pPipelineLibrary);
    arg.pipelineLibraryKey = wrapper->getKey();
    arg.value->pPipelineLibrary = wrapper->getWrappedObject<ID3D12PipelineLibrary>();
  }
}

UpdateInterface<xess_d3d12_execute_params_t_Argument, xess_d3d12_execute_params_t>::UpdateInterface(
    xess_d3d12_execute_params_t_Argument& arg, const xess_d3d12_execute_params_t* value) {

  arg.value = &unwrapStructure_;
  *arg.value = *value;

  if (value->pColorTexture) {
    IUnknownWrapper* wrapper = reinterpret_cast<IUnknownWrapper*>(value->pColorTexture);
    arg.colorTextureKey = wrapper->getKey();
    arg.value->pColorTexture = wrapper->getWrappedObject<ID3D12Resource>();
  }
  if (value->pVelocityTexture) {
    IUnknownWrapper* wrapper = reinterpret_cast<IUnknownWrapper*>(value->pVelocityTexture);
    arg.velocityTextureKey = wrapper->getKey();
    arg.value->pVelocityTexture = wrapper->getWrappedObject<ID3D12Resource>();
  }
  if (value->pDepthTexture) {
    IUnknownWrapper* wrapper = reinterpret_cast<IUnknownWrapper*>(value->pDepthTexture);
    arg.depthTextureKey = wrapper->getKey();
    arg.value->pDepthTexture = wrapper->getWrappedObject<ID3D12Resource>();
  }
  if (value->pExposureScaleTexture) {
    IUnknownWrapper* wrapper = reinterpret_cast<IUnknownWrapper*>(value->pExposureScaleTexture);
    arg.exposureScaleTextureKey = wrapper->getKey();
    arg.value->pExposureScaleTexture = wrapper->getWrappedObject<ID3D12Resource>();
  }
  if (value->pResponsivePixelMaskTexture) {
    IUnknownWrapper* wrapper =
        reinterpret_cast<IUnknownWrapper*>(value->pResponsivePixelMaskTexture);
    arg.responsivePixelMaskTextureKey = wrapper->getKey();
    arg.value->pResponsivePixelMaskTexture = wrapper->getWrappedObject<ID3D12Resource>();
  }
  if (value->pOutputTexture) {
    IUnknownWrapper* wrapper = reinterpret_cast<IUnknownWrapper*>(value->pOutputTexture);
    arg.outputTextureKey = wrapper->getKey();
    arg.value->pOutputTexture = wrapper->getWrappedObject<ID3D12Resource>();
  }
  if (value->pDescriptorHeap) {
    IUnknownWrapper* wrapper = reinterpret_cast<IUnknownWrapper*>(value->pDescriptorHeap);
    arg.descriptorHeapKey = wrapper->getKey();
    arg.value->pDescriptorHeap = wrapper->getWrappedObject<ID3D12DescriptorHeap>();
  }
}

UpdateInterface<DSTORAGE_QUEUE_DESC_Argument, DSTORAGE_QUEUE_DESC>::UpdateInterface(
    DSTORAGE_QUEUE_DESC_Argument& arg, const DSTORAGE_QUEUE_DESC* value) {
  arg.value = &unwrapStructure_;
  *arg.value = *value;
  if (value->Device) {
    IUnknownWrapper* wrapper = reinterpret_cast<IUnknownWrapper*>(value->Device);
    arg.deviceKey = wrapper->getKey();
    arg.value->Device = wrapper->getWrappedObject<ID3D12Device>();
  }
}

UpdateInterface<DSTORAGE_REQUEST_Argument, DSTORAGE_REQUEST>::UpdateInterface(
    DSTORAGE_REQUEST_Argument& arg, const DSTORAGE_REQUEST* value) {
  arg.value = &unwrapStructure_;
  *arg.value = *value;
  switch (value->Options.SourceType) {
  case DSTORAGE_REQUEST_SOURCE_FILE:
    if (value->Source.File.Source) {
      IUnknownWrapper* wrapper = reinterpret_cast<IUnknownWrapper*>(value->Source.File.Source);
      arg.fileKey = wrapper->getKey();
      arg.value->Source.File.Source = wrapper->getWrappedObject<IDStorageFile>();
    }
    break;
  }
  switch (value->Options.DestinationType) {
  case DSTORAGE_REQUEST_DESTINATION_BUFFER:
    if (value->Destination.Buffer.Resource) {
      IUnknownWrapper* wrapper =
          reinterpret_cast<IUnknownWrapper*>(value->Destination.Buffer.Resource);
      arg.resourceKey = wrapper->getKey();
      arg.value->Destination.Buffer.Resource = wrapper->getWrappedObject<ID3D12Resource>();
    }
    break;
  case DSTORAGE_REQUEST_DESTINATION_TEXTURE_REGION:
    if (value->Destination.Texture.Resource) {
      IUnknownWrapper* wrapper =
          reinterpret_cast<IUnknownWrapper*>(value->Destination.Texture.Resource);
      arg.resourceKey = wrapper->getKey();
      arg.value->Destination.Texture.Resource = wrapper->getWrappedObject<ID3D12Resource>();
    }
    break;
  case DSTORAGE_REQUEST_DESTINATION_MULTIPLE_SUBRESOURCES:
    if (value->Destination.MultipleSubresources.Resource) {
      IUnknownWrapper* wrapper =
          reinterpret_cast<IUnknownWrapper*>(value->Destination.MultipleSubresources.Resource);
      arg.resourceKey = wrapper->getKey();
      arg.value->Destination.MultipleSubresources.Resource =
          wrapper->getWrappedObject<ID3D12Resource>();
    }
    break;
  case DSTORAGE_REQUEST_DESTINATION_TILES:
    if (value->Destination.Tiles.Resource) {
      IUnknownWrapper* wrapper =
          reinterpret_cast<IUnknownWrapper*>(value->Destination.Tiles.Resource);
      arg.resourceKey = wrapper->getKey();
      arg.value->Destination.Tiles.Resource = wrapper->getWrappedObject<ID3D12Resource>();
    }
    break;
  }
}

} // namespace DirectX
} // namespace gits
