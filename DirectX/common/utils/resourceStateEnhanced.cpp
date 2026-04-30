// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "resourceStateEnhanced.h"
#include "log.h"

namespace gits {
namespace DirectX {

ResourceStateEnhanced::ResourceStateEnhanced(ID3D12GraphicsCommandList* commandList,
                                             ID3D12Resource* resource,
                                             BarrierState currentState,
                                             unsigned subresource)
    : m_CommandList(commandList),
      m_Resource(resource),
      m_Subresource(subresource),
      m_CurrentState(currentState) {}

void ResourceStateEnhanced::SetState(D3D12_RESOURCE_STATES state) {
  m_State = state;

  if (m_CurrentState.Enhanced) {
    HRESULT hr = m_CommandList->QueryInterface(IID_PPV_ARGS(&m_CommandList7));
    GITS_ASSERT(hr == S_OK);
    if (m_CommandList->GetType() == D3D12_COMMAND_LIST_TYPE_COMPUTE) {
      m_CurrentState.Sync &= ~D3D12_BARRIER_SYNC_PIXEL_SHADING;
    }

    D3D12_BARRIER_GROUP barrierGroup{};
    D3D12_BUFFER_BARRIER bufferBarrier{};
    D3D12_TEXTURE_BARRIER textureBarrier{};
    D3D12_RESOURCE_DESC desc = m_Resource->GetDesc();
    if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
      bufferBarrier.SyncBefore = m_CurrentState.Sync;
      bufferBarrier.SyncAfter = GetSync(m_State);
      bufferBarrier.AccessBefore = m_CurrentState.Access;
      bufferBarrier.AccessAfter = GetAccess(m_State);
      bufferBarrier.pResource = m_Resource;
      bufferBarrier.Offset = 0;
      bufferBarrier.Size = UINT64_MAX;

      barrierGroup.Type = D3D12_BARRIER_TYPE_BUFFER;
      barrierGroup.pBufferBarriers = &bufferBarrier;
    } else {
      textureBarrier.SyncBefore = m_CurrentState.Sync;
      textureBarrier.SyncAfter = GetSync(m_State);
      textureBarrier.AccessBefore = m_CurrentState.Access;
      textureBarrier.AccessAfter = GetAccess(m_State);
      textureBarrier.LayoutBefore = m_CurrentState.Layout;
      textureBarrier.LayoutAfter = GetLayout(m_State);
      textureBarrier.Subresources.IndexOrFirstMipLevel = m_Subresource;
      textureBarrier.pResource = m_Resource;

      barrierGroup.Type = D3D12_BARRIER_TYPE_TEXTURE;
      barrierGroup.pTextureBarriers = &textureBarrier;
    }
    barrierGroup.NumBarriers = 1;
    m_CommandList7->Barrier(1, &barrierGroup);
    m_BarrierSet = true;

  } else {
    if (m_CurrentState.State == m_State) {
      return;
    }
    if (m_CurrentState.State == D3D12_RESOURCE_STATE_GENERIC_READ &&
        state & D3D12_RESOURCE_STATE_GENERIC_READ) {
      return;
    }
    if (m_CommandList->GetType() == D3D12_COMMAND_LIST_TYPE_COMPUTE) {
      m_CurrentState.State &= ~D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    }
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = m_Resource;
    barrier.Transition.StateBefore = m_CurrentState.State;
    barrier.Transition.StateAfter = m_State;
    m_CommandList->ResourceBarrier(1, &barrier);
    m_BarrierSet = true;
  }
}

void ResourceStateEnhanced::RevertState() {
  if (!m_BarrierSet) {
    return;
  }
  if (m_CurrentState.Enhanced) {
    D3D12_BUFFER_BARRIER barrier{};
    barrier.SyncBefore = GetSync(m_State);
    barrier.SyncAfter = m_CurrentState.Sync;
    barrier.AccessBefore = GetAccess(m_State);
    barrier.AccessAfter = m_CurrentState.Access;
    barrier.pResource = m_Resource;
    barrier.Offset = 0;
    barrier.Size = UINT64_MAX;

    D3D12_BARRIER_GROUP barrierGroup{};
    barrierGroup.Type = D3D12_BARRIER_TYPE_BUFFER;
    barrierGroup.NumBarriers = 1;
    barrierGroup.pBufferBarriers = &barrier;
    m_CommandList7->Barrier(1, &barrierGroup);
  } else {
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = m_Resource;
    barrier.Transition.StateBefore = m_State;
    barrier.Transition.StateAfter = m_CurrentState.State;
    m_CommandList->ResourceBarrier(1, &barrier);
  }
}

D3D12_BARRIER_SYNC ResourceStateEnhanced::GetSync(D3D12_RESOURCE_STATES state) {
  D3D12_BARRIER_SYNC sync = D3D12_BARRIER_SYNC_NONE;
  switch (state) {
  case D3D12_RESOURCE_STATE_COMMON:
    sync = D3D12_BARRIER_SYNC_ALL;
    break;
  case D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER:
    sync = D3D12_BARRIER_SYNC_ALL_SHADING;
    break;
  case D3D12_RESOURCE_STATE_INDEX_BUFFER:
    sync = D3D12_BARRIER_SYNC_INDEX_INPUT;
    break;
  case D3D12_RESOURCE_STATE_RENDER_TARGET:
    sync = D3D12_BARRIER_SYNC_RENDER_TARGET;
    break;
  case D3D12_RESOURCE_STATE_UNORDERED_ACCESS:
    sync = D3D12_BARRIER_SYNC_ALL_SHADING;
    break;
  case D3D12_RESOURCE_STATE_DEPTH_WRITE:
    sync = D3D12_BARRIER_SYNC_DEPTH_STENCIL;
    break;
  case D3D12_RESOURCE_STATE_DEPTH_READ:
    sync = D3D12_BARRIER_SYNC_DEPTH_STENCIL;
    break;
  case D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE:
    sync = D3D12_BARRIER_SYNC_NON_PIXEL_SHADING;
    break;
  case D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE:
    sync = D3D12_BARRIER_SYNC_PIXEL_SHADING;
    break;
  case D3D12_RESOURCE_STATE_STREAM_OUT:
    sync = D3D12_BARRIER_SYNC_VERTEX_SHADING;
    break;
  case D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT:
    sync = D3D12_BARRIER_SYNC_EXECUTE_INDIRECT;
    break;
  case D3D12_RESOURCE_STATE_COPY_DEST:
    sync = D3D12_BARRIER_SYNC_COPY;
    break;
  case D3D12_RESOURCE_STATE_COPY_SOURCE:
    sync = D3D12_BARRIER_SYNC_COPY;
    break;
  case D3D12_RESOURCE_STATE_RESOLVE_DEST:
    sync = D3D12_BARRIER_SYNC_RESOLVE;
    break;
  case D3D12_RESOURCE_STATE_RESOLVE_SOURCE:
    sync = D3D12_BARRIER_SYNC_RESOLVE;
    break;
  case D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE:
    sync = D3D12_BARRIER_SYNC_RAYTRACING;
    break;
  case D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE:
    sync = D3D12_BARRIER_SYNC_PIXEL_SHADING;
    break;
  }
  return sync;
}

D3D12_BARRIER_ACCESS ResourceStateEnhanced::GetAccess(D3D12_RESOURCE_STATES state) {
  D3D12_BARRIER_ACCESS access = D3D12_BARRIER_ACCESS_NO_ACCESS;
  switch (state) {
  case D3D12_RESOURCE_STATE_COMMON:
    access = D3D12_BARRIER_ACCESS_COMMON;
    break;
  case D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER:
    access = D3D12_BARRIER_ACCESS_VERTEX_BUFFER | D3D12_BARRIER_ACCESS_CONSTANT_BUFFER;
    break;
  case D3D12_RESOURCE_STATE_INDEX_BUFFER:
    access = D3D12_BARRIER_ACCESS_INDEX_BUFFER;
    break;
  case D3D12_RESOURCE_STATE_RENDER_TARGET:
    access = D3D12_BARRIER_ACCESS_RENDER_TARGET;
    break;
  case D3D12_RESOURCE_STATE_UNORDERED_ACCESS:
    access = D3D12_BARRIER_ACCESS_UNORDERED_ACCESS;
    break;
  case D3D12_RESOURCE_STATE_DEPTH_WRITE:
    access = D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE;
    break;
  case D3D12_RESOURCE_STATE_DEPTH_READ:
    access = D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ;
    break;
  case D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE:
    access = D3D12_BARRIER_ACCESS_SHADER_RESOURCE;
    break;
  case D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE:
    access = D3D12_BARRIER_ACCESS_SHADER_RESOURCE;
    break;
  case D3D12_RESOURCE_STATE_STREAM_OUT:
    access = D3D12_BARRIER_ACCESS_STREAM_OUTPUT;
    break;
  case D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT:
    access = D3D12_BARRIER_ACCESS_INDIRECT_ARGUMENT;
    break;
  case D3D12_RESOURCE_STATE_COPY_DEST:
    access = D3D12_BARRIER_ACCESS_COPY_DEST;
    break;
  case D3D12_RESOURCE_STATE_COPY_SOURCE:
    access = D3D12_BARRIER_ACCESS_COPY_SOURCE;
    break;
  case D3D12_RESOURCE_STATE_RESOLVE_DEST:
    access = D3D12_BARRIER_ACCESS_RESOLVE_DEST;
    break;
  case D3D12_RESOURCE_STATE_RESOLVE_SOURCE:
    access = D3D12_BARRIER_ACCESS_RESOLVE_SOURCE;
    break;
  case D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE:
    access = D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_READ |
             D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_WRITE;
    break;
  case D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE:
    access = D3D12_BARRIER_ACCESS_SHADING_RATE_SOURCE;
    break;
  }
  return access;
}

D3D12_BARRIER_LAYOUT ResourceStateEnhanced::GetLayout(D3D12_RESOURCE_STATES state) {
  D3D12_BARRIER_LAYOUT layout = D3D12_BARRIER_LAYOUT_UNDEFINED;
  switch (state) {
  case D3D12_RESOURCE_STATE_COMMON:
    layout = D3D12_BARRIER_LAYOUT_COMMON;
    break;
  case D3D12_RESOURCE_STATE_RENDER_TARGET:
    layout = D3D12_BARRIER_LAYOUT_RENDER_TARGET;
    break;
  case D3D12_RESOURCE_STATE_UNORDERED_ACCESS:
    layout = D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS;
    break;
  case D3D12_RESOURCE_STATE_DEPTH_WRITE:
    layout = D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE;
    break;
  case D3D12_RESOURCE_STATE_DEPTH_READ:
    layout = D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_READ;
    break;
  case D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE:
    layout = D3D12_BARRIER_LAYOUT_SHADER_RESOURCE;
    break;
  case D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE:
    layout = D3D12_BARRIER_LAYOUT_SHADER_RESOURCE;
    break;
  case D3D12_RESOURCE_STATE_COPY_DEST:
    layout = D3D12_BARRIER_LAYOUT_COPY_DEST;
    break;
  case D3D12_RESOURCE_STATE_COPY_SOURCE:
    layout = D3D12_BARRIER_LAYOUT_COPY_SOURCE;
    break;
  case D3D12_RESOURCE_STATE_RESOLVE_DEST:
    layout = D3D12_BARRIER_LAYOUT_RESOLVE_DEST;
    break;
  case D3D12_RESOURCE_STATE_RESOLVE_SOURCE:
    layout = D3D12_BARRIER_LAYOUT_RESOLVE_SOURCE;
    break;
  case D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE:
    layout = D3D12_BARRIER_LAYOUT_SHADING_RATE_SOURCE;
    break;
  }
  return layout;
}

} // namespace DirectX
} // namespace gits
