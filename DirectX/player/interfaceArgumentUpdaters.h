// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "playerManager.h"

namespace gits {
namespace DirectX {

template <typename T>
void updateInterface(PlayerManager& manager, InterfaceArrayArgument<T>& arg) {

  arg.objectInfos.resize(arg.size);
  for (int i = 0; i < arg.keys.size(); ++i) {
    if (!arg.keys[i]) {
      arg.value[i] = nullptr;
      continue;
    }
    ObjectInfoPlayer* info = manager.findObject(arg.keys[i]);
    arg.value[i] = info ? static_cast<T*>(info->object) : nullptr;
    arg.objectInfos[i] = info;
  }
}

template <typename T>
void updateInterface(PlayerManager& manager, InterfaceArgument<T>& arg) {
  if (!arg.key) {
    return;
  }
  ObjectInfoPlayer* info = manager.findObject(arg.key);
  arg.value = info ? static_cast<T*>(info->object) : nullptr;
  arg.objectInfo = info;
}

template <typename T>
void updateOutputInterface(PlayerManager& manager, InterfaceOutputArgument<T>& arg) {
  if (!arg.value || !*arg.value) {
    return;
  }

  ObjectInfoPlayer* info = new ObjectInfoPlayer();
  info->object = static_cast<IUnknown*>(*arg.value);
  arg.objectInfo = info;
  manager.addObject(arg.key, info);
}

void updateInterface(PlayerManager& manager, D3D12_TEXTURE_COPY_LOCATION_Argument& arg);
void updateInterface(PlayerManager& manager, D3D12_RESOURCE_BARRIERs_Argument& arg);
void updateInterface(PlayerManager& manager, D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument& arg);
void updateInterface(PlayerManager& manager, D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument& arg);
void updateInterface(PlayerManager& manager, D3D12_PIPELINE_STATE_STREAM_DESC_Argument& arg);
void updateInterface(PlayerManager& manager, D3D12_STATE_OBJECT_DESC_Argument& arg);
void updateInterface(PlayerManager& manager, D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument& arg);
void updateInterface(PlayerManager& manager, D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument& arg);
void updateInterface(PlayerManager& manager,
                     PointerArgument<INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC>& arg);
void updateInterface(PlayerManager& manager, DML_BINDING_TABLE_DESC_Argument& arg);
void updateInterface(PlayerManager& manager, DML_BINDING_DESC_Argument& arg);
void updateInterface(PlayerManager& manager, DML_BINDING_DESCs_Argument& arg);
void updateInterface(PlayerManager& manager, DML_GRAPH_DESC_Argument& arg);
void updateInterface(PlayerManager& manager, xess_d3d12_init_params_t_Argument& arg);
void updateInterface(PlayerManager& manager, xess_d3d12_execute_params_t_Argument& arg);
void updateInterface(PlayerManager& manager, DSTORAGE_QUEUE_DESC_Argument& arg);
void updateInterface(PlayerManager& manager, DSTORAGE_REQUEST_Argument& arg);

void updateContext(PlayerManager& manager, XESSContextArgument& arg);
void updateOutputContext(PlayerManager& manager, XESSContextOutputArgument& arg);

} // namespace DirectX
} // namespace gits
