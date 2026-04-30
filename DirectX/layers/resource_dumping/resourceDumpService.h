// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandsAuto.h"
#include "resourceStateTracker.h"
#include "resourceDump.h"
#include "keyUtils.h"
#include "directx.h"

#include <unordered_set>
#include <map>

namespace gits {
namespace DirectX {

class ResourceDumpService {
public:
  ResourceDumpService();
  void CreateResource(unsigned ResourceKey,
                      ID3D12Resource* resource,
                      D3D12_RESOURCE_STATES initialState);
  void DestroyResource(unsigned ResourceKey);
  void CommandListCall(unsigned callKey, ID3D12GraphicsCommandList* commandList);
  void ResourceBarrier(ID3D12GraphicsCommandListResourceBarrierCommand& c);
  void ExecuteCommandLists(unsigned key,
                           unsigned commandQueueKey,
                           ID3D12CommandQueue* commandQueue,
                           ID3D12CommandList** commandLists,
                           unsigned commandListNum);
  void CommandQueueWait(unsigned key,
                        unsigned commandQueueKey,
                        unsigned fenceKey,
                        UINT64 fenceValue);
  void CommandQueueSignal(unsigned key,
                          unsigned commandQueueKey,
                          unsigned fenceKey,
                          UINT64 fenceValue);
  void FenceSignal(unsigned key, unsigned fenceKey, UINT64 fenceValue);

private:
  ConfigKeySet m_ResourceKeys;
  ConfigKeySet m_CallKeys;
  std::wstring m_DumpPath;
  std::map<unsigned, ID3D12Resource*> m_Resources;
  ResourceStateTracker m_ResourceStateTracker;
  ResourceDump m_ResourceDump;
};

} // namespace DirectX
} // namespace gits
