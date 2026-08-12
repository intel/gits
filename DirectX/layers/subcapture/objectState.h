// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once
#include "arguments.h"

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
  std::unique_ptr<Command> CreationCommand;
  GITSKey Key{};
  GITSKey ParentKey{};
  GITSKey LinkedLifetimeKey{};
  std::unordered_set<GITSKey> ChildrenKeys{};
  IUnknown* Object{};
  std::wstring Name;
  D3D12_RESIDENCY_PRIORITY ResidencyPriority{};
  bool Restored{};
  int RefCount{};
  bool KeepDestroyed{};
  bool Destroyed{};
};

struct ResourceState : public ObjectState {
  ResourceState() {}
  GITSKey DeviceKey{};
  D3D12_RESOURCE_STATES InitialState{};
  D3D12_BARRIER_LAYOUT InitialLayout{};
  D3D12_RESOURCE_DIMENSION Dimension{};
  unsigned SampleCount{};
  D3D12_GPU_VIRTUAL_ADDRESS GpuVirtualAddress{};
  bool IsMappable{};
  bool BarrierRestricted{};
  bool DenyShaderResource{};
  D3D12_RESOURCE_STATES CurrentState{};
  GITSKey HeapKey{};
  bool IsRtas{};
};

struct HeapState : public ObjectState {
  GITSKey DeviceKey{};
};

struct D3D12DescriptorHeapState : public ObjectState {
  D3D12_GPU_DESCRIPTOR_HANDLE GpuDescriptorHandle{};
};

struct D3D12StateObjectPropertiesState : public ObjectState {
  std::map<std::wstring, std::array<uint8_t, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES>>
      ShaderIdentifiers;
};

struct D3D12HeapFromAddressState : public ObjectState {
  std::unique_ptr<Command> OpenExistingHeapFromAddressCommand;
};

} // namespace DirectX
} // namespace gits
