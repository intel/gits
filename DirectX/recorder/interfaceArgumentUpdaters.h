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

  arg.Key = wrapper->GetKey();
  arg.Value = wrapper->GetWrappedObject<T>();
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
  arg.Key = wrapper->GetKey();
  arg.Value = wrapper->GetWrappedObject<T>();
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

    m_UnwrappedObjects = m_UnwrappedObjectsStatic;
    if (arg.Size > NUM) {
      m_UnwrappedObjectsDynamic.resize(arg.Size);
      m_UnwrappedObjects = m_UnwrappedObjectsDynamic.data();
    }

    arg.Value = m_UnwrappedObjects;

    for (unsigned i = 0; i < arg.Size; ++i) {
      IUnknownWrapper* wrapper = nullptr;
      HRESULT hr =
          objects[i]->QueryInterface(IID_IUnknownWrapper, reinterpret_cast<void**>(&wrapper));
      GITS_ASSERT(hr == S_OK);
      arg.Keys[i] = wrapper->GetKey();
      m_UnwrappedObjects[i] = wrapper->GetWrappedObject<T>();
    }
  }

private:
  static constexpr unsigned NUM = 64;
  T** m_UnwrappedObjects{nullptr};
  T* m_UnwrappedObjectsStatic[NUM]{};
  std::vector<T*> m_UnwrappedObjectsDynamic;
};

template <typename T>
class UpdateOutputInterface<InterfaceOutputArgument<T>, T> {
public:
  UpdateOutputInterface(InterfaceOutputArgument<T>& arg, HRESULT hr, REFIID riid, T** object)
      : m_Object(object) {

    if (hr == S_OK && m_Object && *m_Object) {
      T* wrappedObject = *m_Object;
      wrapObject(riid, reinterpret_cast<void**>(&wrappedObject));
      m_Wrapper = reinterpret_cast<IUnknownWrapper*>(wrappedObject);

      arg.Key = m_Wrapper->GetKey();
    }
  }
  ~UpdateOutputInterface() {
    if (m_Wrapper) {
      *m_Object = reinterpret_cast<T*>(m_Wrapper);
    }
  }
  UpdateOutputInterface(const UpdateOutputInterface&) = delete;
  UpdateOutputInterface& operator=(const UpdateOutputInterface&) = delete;

private:
  T** m_Object;
  IUnknownWrapper* m_Wrapper{};
};

template <>
class UpdateInterface<D3D12_TEXTURE_COPY_LOCATION_Argument, D3D12_TEXTURE_COPY_LOCATION> {
public:
  UpdateInterface(D3D12_TEXTURE_COPY_LOCATION_Argument& arg,
                  const D3D12_TEXTURE_COPY_LOCATION* value);

private:
  D3D12_TEXTURE_COPY_LOCATION m_UnwrapStructure;
};

template <>
class UpdateInterface<D3D12_RESOURCE_BARRIERs_Argument, D3D12_RESOURCE_BARRIER> {
public:
  UpdateInterface(D3D12_RESOURCE_BARRIERs_Argument& arg, const D3D12_RESOURCE_BARRIER* value);

private:
  static constexpr unsigned NUM = 128;
  D3D12_RESOURCE_BARRIER* m_Unwrapped{nullptr};
  D3D12_RESOURCE_BARRIER m_UnwrappedStatic[NUM];
  std::vector<D3D12_RESOURCE_BARRIER> m_UnwrappedDynamic;
};

template <>
class UpdateInterface<D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument,
                      D3D12_GRAPHICS_PIPELINE_STATE_DESC> {
public:
  UpdateInterface(D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument& arg,
                  const D3D12_GRAPHICS_PIPELINE_STATE_DESC* value);

private:
  D3D12_GRAPHICS_PIPELINE_STATE_DESC m_UnwrapStructure{};
};

template <>
class UpdateInterface<D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument,
                      D3D12_COMPUTE_PIPELINE_STATE_DESC> {
public:
  UpdateInterface(D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument& arg,
                  const D3D12_COMPUTE_PIPELINE_STATE_DESC* value);

private:
  D3D12_COMPUTE_PIPELINE_STATE_DESC m_UnwrapStructure{};
};

template <>
class UpdateInterface<D3D12_PIPELINE_STATE_STREAM_DESC_Argument, D3D12_PIPELINE_STATE_STREAM_DESC> {
public:
  UpdateInterface(D3D12_PIPELINE_STATE_STREAM_DESC_Argument& arg,
                  const D3D12_PIPELINE_STATE_STREAM_DESC* stateObjectDesc);

private:
  D3D12_PIPELINE_STATE_STREAM_DESC m_StreamDescUnwrapped{};
  std::vector<uint8_t> m_SubobjectsUnwrapped;
};

template <>
class UpdateInterface<D3D12_STATE_OBJECT_DESC_Argument, D3D12_STATE_OBJECT_DESC> {
public:
  UpdateInterface(D3D12_STATE_OBJECT_DESC_Argument& arg,
                  const D3D12_STATE_OBJECT_DESC* stateObjectDesc);

private:
  D3D12_STATE_OBJECT_DESC m_StateObjectDescUnwrapped{};
  std::vector<D3D12_STATE_SUBOBJECT> m_SubobjectsUnwrapped;
  D3D12_GLOBAL_ROOT_SIGNATURE m_GlobalSignatureUnwrapped{};
  std::vector<D3D12_LOCAL_ROOT_SIGNATURE> m_LocalSignatures;
  std::vector<D3D12_EXISTING_COLLECTION_DESC> existingCollectionDescs;
  std::vector<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION> m_SubobjectToExportsAssociations;
  std::map<const D3D12_STATE_SUBOBJECT*, unsigned> m_WrappedSubobjectIndexes;
};

template <>
class UpdateInterface<D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument,
                      D3D12_RENDER_PASS_RENDER_TARGET_DESC> {
public:
  UpdateInterface(D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument& arg,
                  const D3D12_RENDER_PASS_RENDER_TARGET_DESC* value);

private:
  std::vector<D3D12_RENDER_PASS_RENDER_TARGET_DESC> m_UnwrapStructures;
};

template <>
class UpdateInterface<D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument,
                      D3D12_RENDER_PASS_DEPTH_STENCIL_DESC> {
public:
  UpdateInterface(D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument& arg,
                  const D3D12_RENDER_PASS_DEPTH_STENCIL_DESC* value);

private:
  D3D12_RENDER_PASS_DEPTH_STENCIL_DESC m_UnwrapStructure{};
};

template <>
class UpdateInterface<D3D12_BARRIER_GROUPs_Argument, D3D12_BARRIER_GROUP> {
public:
  UpdateInterface(D3D12_BARRIER_GROUPs_Argument& arg, const D3D12_BARRIER_GROUP* value);

private:
  std::vector<std::unique_ptr<std::vector<D3D12_GLOBAL_BARRIER>>> m_UnwrappedGlobalBarrierGroups;
  std::vector<std::unique_ptr<std::vector<D3D12_TEXTURE_BARRIER>>> m_UnwrappedTextureBarrierGroups;
  std::vector<std::unique_ptr<std::vector<D3D12_BUFFER_BARRIER>>> m_UnwrappedBufferBarrierGroups;
  std::vector<D3D12_BARRIER_GROUP> m_Unwrapped;
};

template <>
class UpdateInterface<D3D12_EXTENSION_ARGUMENTS_Argument, D3D12_EXTENSION_ARGUMENTS> {
public:
  UpdateInterface(D3D12_EXTENSION_ARGUMENTS_Argument& arg, const D3D12_EXTENSION_ARGUMENTS* value);

private:
  D3D12_EXTENSION_ARGUMENTS m_UnwrapStructure{};
};

template <>
class UpdateInterface<D3D12_EXTENDED_OPERATION_DATA_Argument, D3D12_EXTENDED_OPERATION_DATA> {
public:
  UpdateInterface(D3D12_EXTENDED_OPERATION_DATA_Argument& arg,
                  const D3D12_EXTENDED_OPERATION_DATA* value);

private:
  D3D12_EXTENDED_OPERATION_DATA m_UnwrapStructure{};
};

template <>
class UpdateInterface<PointerArgument<INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC>,
                      INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC> {
public:
  UpdateInterface(PointerArgument<INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC>& arg,
                  const INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC* value);

private:
  INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC m_UnwrapStructure{};
};

template <>
class UpdateInterface<DML_BINDING_TABLE_DESC_Argument, DML_BINDING_TABLE_DESC> {
public:
  UpdateInterface(DML_BINDING_TABLE_DESC_Argument& arg, const DML_BINDING_TABLE_DESC* value);

private:
  DML_BINDING_TABLE_DESC m_UnwrapStructure{};
};

template <>
class UpdateInterface<DML_BINDING_DESC_Argument, DML_BINDING_DESC> {
public:
  UpdateInterface(DML_BINDING_DESC_Argument& arg, const DML_BINDING_DESC* value);

private:
  DML_BINDING_DESC m_UnwrapStructure{};
  DML_BUFFER_ARRAY_BINDING m_BindingArray{};
  std::vector<DML_BUFFER_BINDING> m_Bindings;
};

template <>
class UpdateInterface<DML_BINDING_DESCs_Argument, DML_BINDING_DESC> {
public:
  UpdateInterface(DML_BINDING_DESCs_Argument& arg, const DML_BINDING_DESC* value);

private:
  DML_BINDING_DESC* m_UnwrapStructure{nullptr};
  std::vector<DML_BINDING_DESC> m_UnwrapStructures;
  std::vector<DML_BUFFER_ARRAY_BINDING> m_BindingArrays;
  std::vector<DML_BUFFER_BINDING> m_Bindings;
};

template <>
class UpdateInterface<DML_GRAPH_DESC_Argument, DML_GRAPH_DESC> {
public:
  UpdateInterface(DML_GRAPH_DESC_Argument& arg, const DML_GRAPH_DESC* value);

private:
  DML_GRAPH_DESC m_UnwrapStructure{};
  std::vector<DML_GRAPH_NODE_DESC> m_Nodes;
  std::vector<DML_OPERATOR_GRAPH_NODE_DESC> m_OpNodes;
};

template <>
class UpdateInterface<xess_d3d12_init_params_t_Argument, xess_d3d12_init_params_t> {
public:
  UpdateInterface(xess_d3d12_init_params_t_Argument& arg, const xess_d3d12_init_params_t* value);

private:
  xess_d3d12_init_params_t m_UnwrapStructure{};
};

template <>
class UpdateInterface<xess_d3d12_execute_params_t_Argument, xess_d3d12_execute_params_t> {
public:
  UpdateInterface(xess_d3d12_execute_params_t_Argument& arg,
                  const xess_d3d12_execute_params_t* value);

private:
  xess_d3d12_execute_params_t m_UnwrapStructure{};
};

template <>
class UpdateInterface<DSTORAGE_QUEUE_DESC_Argument, DSTORAGE_QUEUE_DESC> {
public:
  UpdateInterface(DSTORAGE_QUEUE_DESC_Argument& arg, const DSTORAGE_QUEUE_DESC* value);

private:
  DSTORAGE_QUEUE_DESC m_UnwrapStructure{};
};

template <>
class UpdateInterface<DSTORAGE_REQUEST_Argument, DSTORAGE_REQUEST> {
public:
  UpdateInterface(DSTORAGE_REQUEST_Argument& arg, const DSTORAGE_REQUEST* value);

private:
  DSTORAGE_REQUEST m_UnwrapStructure{};
};

template <>
class UpdateInterface<xefg_swapchain_d3d12_init_params_t_Argument,
                      xefg_swapchain_d3d12_init_params_t> {
public:
  UpdateInterface(xefg_swapchain_d3d12_init_params_t_Argument& arg,
                  const xefg_swapchain_d3d12_init_params_t* value);

private:
  xefg_swapchain_d3d12_init_params_t m_UnwrapStructure{};
};

template <>
class UpdateInterface<xefg_swapchain_d3d12_resource_data_t_Argument,
                      xefg_swapchain_d3d12_resource_data_t> {
public:
  UpdateInterface(xefg_swapchain_d3d12_resource_data_t_Argument& arg,
                  const xefg_swapchain_d3d12_resource_data_t* value);

private:
  xefg_swapchain_d3d12_resource_data_t m_UnwrapStructure{};
};

} // namespace DirectX
} // namespace gits
