// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once
#include "arguments.h"
#include "commandsAuto.h"
#include "commandsCustom.h"
#include "capturePlayerShaderIdentifierService.h"

#include <d3d12.h>
#include <optional>
#include <string>
#include <unordered_map>

namespace gits {
namespace DirectX {

// Computes the shader record patch masks (see CapturePlayerShaderIdentifierService) of state
// object exports from their local root signatures. PATCH_MASK_ALL is used whenever the local root
// signature of an export cannot be determined.
//
// Arguments come from the stream, so commands are tracked even when skipped (e.g. state object
// creation moved to a worker thread by MultithreadedShaderCompilationScheduleLayer).
class GpuPatchLocalRootSignatureService {
public:
  void CreateRootSignature(ID3D12DeviceCreateRootSignatureCommand& c);
  void CreateStateObject(ID3D12Device5CreateStateObjectCommand& c);
  void AddToStateObject(ID3D12Device7AddToStateObjectCommand& c);
  void QueryInterface(IUnknownQueryInterfaceCommand& c);
  void DestroyInterface(ObjectKey key);
  // stateObjectKey may be the key of any interface of the state object
  uint64_t GetPatchMask(ObjectKey stateObjectKey, const wchar_t* exportName) const;

private:
  static constexpr uint64_t PATCH_MASK_ALL = CapturePlayerShaderIdentifierService::PATCH_MASK_ALL;

  struct StateObjectInfo {
    std::unordered_map<std::wstring, uint64_t> PatchMasksByExport;
    // Used for exports missing from PatchMasksByExport, e.g. exports of DXIL libraries that
    // export all their symbols
    uint64_t DefaultPatchMask{PATCH_MASK_ALL};
    bool HasUnknownExports{};

    uint64_t GetPatchMask(const wchar_t* exportName) const;
  };
  void FillStateObjectInfo(const D3D12_STATE_OBJECT_DESC_Argument& stateObjectDesc,
                           StateObjectInfo& info) const;
  void ImportCollection(const D3D12_EXISTING_COLLECTION_DESC& collectionDesc,
                        ObjectKey collectionKey,
                        StateObjectInfo& info,
                        std::optional<uint64_t>& unknownExportsMask) const;
  ObjectKey ResolveStateObjectKey(ObjectKey key) const;
  uint64_t GetLocalRootSignaturePatchMask(ObjectKey rootSignatureKey) const;

  std::unordered_map<ObjectKey, uint64_t> m_PatchMasksByLocalRootSignature;
  std::unordered_map<ObjectKey, StateObjectInfo> m_StateObjectInfos;
  std::unordered_map<ObjectKey, ObjectKey> m_StateObjectsByInterface;
};

} // namespace DirectX
} // namespace gits
