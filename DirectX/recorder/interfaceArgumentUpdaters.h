// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "iunknownWrapper.h"
#include "arguments.h"
#include "wrapperUtils.h"
#include "intelExtensions.h"

namespace gits {
namespace DirectX {

template <typename T>
void updateInterface(InterfaceArgument<T>& arg, IUnknownWrapper* wrapper) {

  arg.key = wrapper->getKey();
  arg.value = wrapper->getWrappedObject<T>();
}

template <typename T>
void updateInterface(InterfaceArgument<T>& arg, T* object) {

  if (!object) {
    return;
  }

  IUnknownWrapper* wrapper = nullptr;
  if (object->QueryInterface(IID_IUnknownWrapper, reinterpret_cast<void**>(&wrapper)) != S_OK) {
    return;
  }
  arg.key = wrapper->getKey();
  arg.value = wrapper->getWrappedObject<T>();
}

template <typename T, typename Arg>
class UpdateInterface {};

template <typename T, typename Arg>
class UpdateOutputInterface {};

template <typename T>
class UpdateInterface<InterfaceArrayArgument<T>, T> {
public:
  UpdateInterface(InterfaceArrayArgument<T>& arg, T** objects) {

    if (!objects) {
      return;
    }

    unwrappedObjects_ = unwrappedObjectsStatic_;
    if (arg.size > NUM) {
      unwrappedObjectsDynamic_.resize(arg.size);
      unwrappedObjects_ = unwrappedObjectsDynamic_.data();
    }

    arg.value = unwrappedObjects_;

    for (unsigned i = 0; i < arg.size; ++i) {
      IUnknownWrapper* wrapper = nullptr;
      HRESULT hr =
          objects[i]->QueryInterface(IID_IUnknownWrapper, reinterpret_cast<void**>(&wrapper));
      GITS_ASSERT(hr == S_OK);
      arg.keys[i] = wrapper->getKey();
      unwrappedObjects_[i] = wrapper->getWrappedObject<T>();
    }
  }

private:
  static constexpr unsigned NUM = 64;
  T** unwrappedObjects_{nullptr};
  T* unwrappedObjectsStatic_[NUM]{};
  std::vector<T*> unwrappedObjectsDynamic_;
};

template <typename T>
class UpdateOutputInterface<InterfaceOutputArgument<T>, T> : public gits::noncopyable {
public:
  UpdateOutputInterface(InterfaceOutputArgument<T>& arg, HRESULT hr, REFIID riid, T** object)
      : object_(object) {

    if (hr == S_OK && object_ && *object_) {
      T* wrappedObject = *object_;
      wrapObject(riid, reinterpret_cast<void**>(&wrappedObject));
      wrapper_ = reinterpret_cast<IUnknownWrapper*>(wrappedObject);

      arg.key = wrapper_->getKey();
    }
  }
  ~UpdateOutputInterface() {
    if (wrapper_) {
      *object_ = reinterpret_cast<T*>(wrapper_);
    }
  }

private:
  T** object_;
  IUnknownWrapper* wrapper_{};
};

template <>
class UpdateInterface<D3D12_TEXTURE_COPY_LOCATION_Argument, D3D12_TEXTURE_COPY_LOCATION> {
public:
  UpdateInterface(D3D12_TEXTURE_COPY_LOCATION_Argument& arg,
                  const D3D12_TEXTURE_COPY_LOCATION* value);

private:
  D3D12_TEXTURE_COPY_LOCATION unwrapStructure_;
};

template <>
class UpdateInterface<D3D12_RESOURCE_BARRIERs_Argument, D3D12_RESOURCE_BARRIER> {
public:
  UpdateInterface(D3D12_RESOURCE_BARRIERs_Argument& arg, const D3D12_RESOURCE_BARRIER* value);

private:
  static constexpr unsigned NUM = 128;
  D3D12_RESOURCE_BARRIER* unwrapped_{nullptr};
  D3D12_RESOURCE_BARRIER unwrappedStatic_[NUM];
  std::vector<D3D12_RESOURCE_BARRIER> unwrappedDynamic_;
};

template <>
class UpdateInterface<D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument,
                      D3D12_GRAPHICS_PIPELINE_STATE_DESC> {
public:
  UpdateInterface(D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument& arg,
                  const D3D12_GRAPHICS_PIPELINE_STATE_DESC* value);

private:
  D3D12_GRAPHICS_PIPELINE_STATE_DESC unwrapStructure_{};
};

template <>
class UpdateInterface<D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument,
                      D3D12_COMPUTE_PIPELINE_STATE_DESC> {
public:
  UpdateInterface(D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument& arg,
                  const D3D12_COMPUTE_PIPELINE_STATE_DESC* value);

private:
  D3D12_COMPUTE_PIPELINE_STATE_DESC unwrapStructure_{};
};

template <>
class UpdateInterface<D3D12_PIPELINE_STATE_STREAM_DESC_Argument, D3D12_PIPELINE_STATE_STREAM_DESC> {
public:
  UpdateInterface(D3D12_PIPELINE_STATE_STREAM_DESC_Argument& arg,
                  const D3D12_PIPELINE_STATE_STREAM_DESC* stateObjectDesc);

private:
  D3D12_PIPELINE_STATE_STREAM_DESC streamDescUnwrapped_{};
  std::vector<uint8_t> subobjectsUnwrapped_;
};

template <>
class UpdateInterface<D3D12_STATE_OBJECT_DESC_Argument, D3D12_STATE_OBJECT_DESC> {
public:
  UpdateInterface(D3D12_STATE_OBJECT_DESC_Argument& arg,
                  const D3D12_STATE_OBJECT_DESC* stateObjectDesc);

private:
  D3D12_STATE_OBJECT_DESC stateObjectDescUnwrapped_{};
  std::vector<D3D12_STATE_SUBOBJECT> subobjectsUnwrapped_;
  D3D12_GLOBAL_ROOT_SIGNATURE globalSignatureUnwrapped_{};
  std::vector<D3D12_LOCAL_ROOT_SIGNATURE> localSignatures_;
  std::vector<D3D12_EXISTING_COLLECTION_DESC> existingCollectionDescs;
  std::vector<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION> subobjectToExportsAssociations_;
  std::map<const D3D12_STATE_SUBOBJECT*, unsigned> wrappedSubobjectIndexes_;
};

template <>
class UpdateInterface<D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument,
                      D3D12_RENDER_PASS_RENDER_TARGET_DESC> {
public:
  UpdateInterface(D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument& arg,
                  const D3D12_RENDER_PASS_RENDER_TARGET_DESC* value);

private:
  std::vector<D3D12_RENDER_PASS_RENDER_TARGET_DESC> unwrapStructures_;
};

template <>
class UpdateInterface<D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument,
                      D3D12_RENDER_PASS_DEPTH_STENCIL_DESC> {
public:
  UpdateInterface(D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument& arg,
                  const D3D12_RENDER_PASS_DEPTH_STENCIL_DESC* value);

private:
  D3D12_RENDER_PASS_DEPTH_STENCIL_DESC unwrapStructure_{};
};

template <>
class UpdateInterface<D3D12_BARRIER_GROUPs_Argument, D3D12_BARRIER_GROUP> {
public:
  UpdateInterface(D3D12_BARRIER_GROUPs_Argument& arg, const D3D12_BARRIER_GROUP* value);

private:
  std::vector<std::unique_ptr<std::vector<D3D12_GLOBAL_BARRIER>>> unwrappedGlobalBarrierGroups_;
  std::vector<std::unique_ptr<std::vector<D3D12_TEXTURE_BARRIER>>> unwrappedTextureBarrierGroups_;
  std::vector<std::unique_ptr<std::vector<D3D12_BUFFER_BARRIER>>> unwrappedBufferBarrierGroups_;
  std::vector<D3D12_BARRIER_GROUP> unwrapped_;
};

template <>
class UpdateInterface<PointerArgument<INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC>,
                      INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC> {
public:
  UpdateInterface(PointerArgument<INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC>& arg,
                  const INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC* value);

private:
  INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC unwrapStructure_{};
};

template <>
class UpdateInterface<DML_BINDING_TABLE_DESC_Argument, DML_BINDING_TABLE_DESC> {
public:
  UpdateInterface(DML_BINDING_TABLE_DESC_Argument& arg, const DML_BINDING_TABLE_DESC* value);

private:
  DML_BINDING_TABLE_DESC unwrapStructure_{};
};

template <>
class UpdateInterface<DML_BINDING_DESC_Argument, DML_BINDING_DESC> {
public:
  UpdateInterface(DML_BINDING_DESC_Argument& arg, const DML_BINDING_DESC* value);

private:
  DML_BINDING_DESC unwrapStructure_{};
  DML_BUFFER_ARRAY_BINDING bindingArray_{};
  std::vector<DML_BUFFER_BINDING> bindings_;
};

template <>
class UpdateInterface<DML_BINDING_DESCs_Argument, DML_BINDING_DESC> {
public:
  UpdateInterface(DML_BINDING_DESCs_Argument& arg, const DML_BINDING_DESC* value);

private:
  DML_BINDING_DESC* unwrapStructure_{nullptr};
  std::vector<DML_BINDING_DESC> unwrapStructures_;
  std::vector<DML_BUFFER_ARRAY_BINDING> bindingArrays_;
  std::vector<DML_BUFFER_BINDING> bindings_;
};

template <>
class UpdateInterface<DML_GRAPH_DESC_Argument, DML_GRAPH_DESC> {
public:
  UpdateInterface(DML_GRAPH_DESC_Argument& arg, const DML_GRAPH_DESC* value);

private:
  DML_GRAPH_DESC unwrapStructure_{};
  std::vector<DML_GRAPH_NODE_DESC> nodes_;
  std::vector<DML_OPERATOR_GRAPH_NODE_DESC> opNodes_;
};

template <>
class UpdateInterface<xess_d3d12_init_params_t_Argument, xess_d3d12_init_params_t> {
public:
  UpdateInterface(xess_d3d12_init_params_t_Argument& arg, const xess_d3d12_init_params_t* value);

private:
  xess_d3d12_init_params_t unwrapStructure_{};
};

template <>
class UpdateInterface<xess_d3d12_execute_params_t_Argument, xess_d3d12_execute_params_t> {
public:
  UpdateInterface(xess_d3d12_execute_params_t_Argument& arg,
                  const xess_d3d12_execute_params_t* value);

private:
  xess_d3d12_execute_params_t unwrapStructure_{};
};

template <>
class UpdateInterface<DSTORAGE_QUEUE_DESC_Argument, DSTORAGE_QUEUE_DESC> {
public:
  UpdateInterface(DSTORAGE_QUEUE_DESC_Argument& arg, const DSTORAGE_QUEUE_DESC* value);

private:
  DSTORAGE_QUEUE_DESC unwrapStructure_{};
};

template <>
class UpdateInterface<DSTORAGE_REQUEST_Argument, DSTORAGE_REQUEST> {
public:
  UpdateInterface(DSTORAGE_REQUEST_Argument& arg, const DSTORAGE_REQUEST* value);

private:
  DSTORAGE_REQUEST unwrapStructure_{};
};

} // namespace DirectX
} // namespace gits
