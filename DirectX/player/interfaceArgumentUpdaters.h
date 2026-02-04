// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "playerManager.h"

namespace gits {
namespace DirectX {

template <typename T>
void updateInterface(PlayerManager& manager, InterfaceArrayArgument<T>& arg) {

  for (int i = 0; i < arg.keys.size(); ++i) {
    if (!arg.keys[i]) {
      arg.value[i] = nullptr;
      continue;
    }
    arg.value[i] = static_cast<T*>(manager.findObject(arg.keys[i]));
  }
}

template <typename T>
void updateInterface(PlayerManager& manager, InterfaceArgument<T>& arg) {
  if (!arg.key) {
    return;
  }
  arg.value = static_cast<T*>(manager.findObject(arg.key));
}

template <typename T>
void updateOutputInterface(PlayerManager& manager, InterfaceOutputArgument<T>& arg) {
  if (!arg.value || !*arg.value) {
    return;
  }
  manager.addObject(arg.key, static_cast<IUnknown*>(*arg.value));
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
void updateInterface(PlayerManager& manager, D3D12_BARRIER_GROUPs_Argument& arg);
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
void updateContext(PlayerManager& maanger, XELLContextArgument& arg);
void updateOutputContext(PlayerManager& manager, XELLContextOutputArgument& arg);

} // namespace DirectX
} // namespace gits
