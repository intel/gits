// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "command.h"
#include "intelExtensions.h"
#include "directx.h"

#include <vector>
#include <memory>
#include <string>
#include <unordered_set>
#include <map>

namespace gits {
namespace DirectX {

struct ObjectState {
  ObjectState() {}
  virtual ~ObjectState() = default;
  std::unique_ptr<Command> creationCommand;
  unsigned key{};
  unsigned parentKey{};
  std::unordered_set<unsigned> childrenKeys{};
  IUnknown* object{};
  std::wstring name;
  D3D12_RESIDENCY_PRIORITY residencyPriority{};
  bool restored{};
  int refCount{};
  bool keepDestroyed{};
  bool destroyed{};
};

struct ResourceState : public ObjectState {
  ResourceState() {}
  unsigned deviceKey{};
  D3D12_RESOURCE_STATES initialState{};
  D3D12_BARRIER_LAYOUT initialLayout{};
  D3D12_RESOURCE_DIMENSION dimension{};
  unsigned sampleCount{};
  D3D12_GPU_VIRTUAL_ADDRESS gpuVirtualAddress{};
  bool isMappable{};
  bool isGenericRead{};
  bool isBarrierRestricted{};
  unsigned heapKey{};
};

struct HeapState : public ObjectState {
  unsigned deviceKey{};
};

struct D3D12DescriptorHeapState : public ObjectState {
  D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle{};
};

struct D3D12StateObjectPropertiesState : public ObjectState {
  std::map<std::wstring, std::array<uint8_t, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES>>
      shaderIdentifiers;
};

struct D3D12HeapFromAddressState : public ObjectState {
  std::unique_ptr<Command> openExistingHeapFromAddressCommand;
};

} // namespace DirectX
} // namespace gits
