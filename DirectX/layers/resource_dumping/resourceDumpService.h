// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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
  void createResource(unsigned resourceKey,
                      ID3D12Resource* resource,
                      D3D12_RESOURCE_STATES initialState);
  void destroyResource(unsigned resourceKey);
  void commandListCall(unsigned callKey, ID3D12GraphicsCommandList* commandList);
  void resourceBarrier(ID3D12GraphicsCommandListResourceBarrierCommand& c);
  void executeCommandLists(unsigned key,
                           unsigned commandQueueKey,
                           ID3D12CommandQueue* commandQueue,
                           ID3D12CommandList** commandLists,
                           unsigned commandListNum);
  void commandQueueWait(unsigned key,
                        unsigned commandQueueKey,
                        unsigned fenceKey,
                        UINT64 fenceValue);
  void commandQueueSignal(unsigned key,
                          unsigned commandQueueKey,
                          unsigned fenceKey,
                          UINT64 fenceValue);
  void fenceSignal(unsigned key, unsigned fenceKey, UINT64 fenceValue);

private:
  ConfigKeySet resourceKeys_;
  ConfigKeySet callKeys_;
  std::wstring dumpPath_;
  std::map<unsigned, ID3D12Resource*> resources_;
  ResourceStateTracker resourceStateTracker_;
  ResourceDump resourceDump_;
};

} // namespace DirectX
} // namespace gits
