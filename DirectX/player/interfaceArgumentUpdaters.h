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
void UpdateInterface(PlayerManager& manager, InterfaceArrayArgument<T>& arg) {

  for (int i = 0; i < arg.Keys.size(); ++i) {
    if (!arg.Keys[i]) {
      arg.Value[i] = nullptr;
      continue;
    }
    arg.Value[i] = static_cast<T*>(manager.FindObject(arg.Keys[i]));
  }
}

template <typename T>
void UpdateInterface(PlayerManager& manager, InterfaceArgument<T>& arg) {
  if (!arg.Key) {
    return;
  }
  arg.Value = static_cast<T*>(manager.FindObject(arg.Key));
}

template <typename T>
void UpdateOutputInterface(PlayerManager& manager, InterfaceOutputArgument<T>& arg) {
  if (!arg.Value || !*arg.Value) {
    return;
  }
  manager.AddObject(arg.Key, static_cast<IUnknown*>(*arg.Value));
}

void UpdateInterface(PlayerManager& manager, D3D12_TEXTURE_COPY_LOCATION_Argument& arg);
void UpdateInterface(PlayerManager& manager, D3D12_RESOURCE_BARRIERs_Argument& arg);
void UpdateInterface(PlayerManager& manager, D3D12_GRAPHICS_PIPELINE_STATE_DESC_Argument& arg);
void UpdateInterface(PlayerManager& manager, D3D12_COMPUTE_PIPELINE_STATE_DESC_Argument& arg);
void UpdateInterface(PlayerManager& manager, D3D12_PIPELINE_STATE_STREAM_DESC_Argument& arg);
void UpdateInterface(PlayerManager& manager, D3D12_STATE_OBJECT_DESC_Argument& arg);
void UpdateInterface(PlayerManager& manager, D3D12_RENDER_PASS_RENDER_TARGET_DESCs_Argument& arg);
void UpdateInterface(PlayerManager& manager, D3D12_RENDER_PASS_DEPTH_STENCIL_DESC_Argument& arg);
void UpdateInterface(PlayerManager& manager,
                     PointerArgument<INTC_D3D12_COMPUTE_PIPELINE_STATE_DESC>& arg);
void UpdateInterface(PlayerManager& manager, D3D12_BARRIER_GROUPs_Argument& arg);
void UpdateInterface(PlayerManager& manager, D3D12_EXTENSION_ARGUMENTS_Argument& arg);
void UpdateInterface(PlayerManager& manager, D3D12_EXTENDED_OPERATION_DATA_Argument& arg);
void UpdateInterface(PlayerManager& manager, DML_BINDING_TABLE_DESC_Argument& arg);
void UpdateInterface(PlayerManager& manager, DML_BINDING_DESC_Argument& arg);
void UpdateInterface(PlayerManager& manager, DML_BINDING_DESCs_Argument& arg);
void UpdateInterface(PlayerManager& manager, DML_GRAPH_DESC_Argument& arg);
void UpdateInterface(PlayerManager& manager, xess_d3d12_init_params_t_Argument& arg);
void UpdateInterface(PlayerManager& manager, xess_d3d12_execute_params_t_Argument& arg);
void UpdateInterface(PlayerManager& manager, DSTORAGE_QUEUE_DESC_Argument& arg);
void UpdateInterface(PlayerManager& manager, DSTORAGE_REQUEST_Argument& arg);
void UpdateInterface(PlayerManager& manager, xefg_swapchain_d3d12_init_params_t_Argument& arg);
void UpdateInterface(PlayerManager& manager, xefg_swapchain_d3d12_resource_data_t_Argument& arg);

void UpdateContext(PlayerManager& manager, XESSContextArgument& arg);
void UpdateOutputContext(PlayerManager& manager, XESSContextOutputArgument& arg);
void UpdateContext(PlayerManager& maanger, XELLContextArgument& arg);
void UpdateOutputContext(PlayerManager& manager, XELLContextOutputArgument& arg);
void UpdateContext(PlayerManager& manager, XEFGContextArgument& arg);
void UpdateOutputContext(PlayerManager& manager, XEFGContextOutputArgument& arg);

} // namespace DirectX
} // namespace gits
