// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "gpuPatchLocalRootSignatureService.h"

#include <unordered_set>
#include <vector>
#include <wrl/client.h>

namespace gits {
namespace DirectX {

namespace {

constexpr uint64_t PATCH_MASK_ALL = CapturePlayerShaderIdentifierService::PATCH_MASK_ALL;

template <typename RootSignatureDesc>
std::optional<uint64_t> ComputeLocalRootSignaturePatchMask(const RootSignatureDesc& desc) {
  if (!(desc.Flags & D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE)) {
    return std::nullopt;
  }
  // Local root arguments follow the shader identifier in root parameter order. Root constants
  // take 4 bytes each, root descriptors and descriptor tables take 8 bytes and are 8-byte aligned.
  uint64_t mask = 0;
  size_t offset = 0;
  for (unsigned i = 0; i < desc.NumParameters; ++i) {
    const auto& parameter = desc.pParameters[i];
    if (parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS) {
      offset += sizeof(uint32_t) * parameter.Constants.Num32BitValues;
      continue;
    }
    size_t slot = (offset + sizeof(uint64_t) - 1) / sizeof(uint64_t);
    if (slot >= CapturePlayerShaderIdentifierService::PATCH_MASK_SLOT_COUNT) {
      return PATCH_MASK_ALL;
    }
    mask |= 1ull << slot;
    offset = (slot + 1) * sizeof(uint64_t);
  }
  return mask;
}

// For exports that must share one mask: differing masks mean the layout is unknown
void MergePatchMask(std::optional<uint64_t>& merged, uint64_t mask) {
  merged = !merged || *merged == mask ? mask : PATCH_MASK_ALL;
}

} // namespace

void GpuPatchLocalRootSignatureService::CreateRootSignature(
    ID3D12DeviceCreateRootSignatureCommand& c) {
  if (c.m_Result.Value != S_OK) {
    return;
  }
  Microsoft::WRL::ComPtr<ID3D12VersionedRootSignatureDeserializer> deserializer;
  HRESULT hr = D3D12CreateVersionedRootSignatureDeserializer(
      c.m_pBlobWithRootSignature.Value, c.m_blobLengthInBytes.Value, IID_PPV_ARGS(&deserializer));
  if (hr != S_OK) {
    return;
  }
  const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* desc = deserializer->GetUnconvertedRootSignatureDesc();
  std::optional<uint64_t> patchMask;
  switch (desc->Version) {
  case D3D_ROOT_SIGNATURE_VERSION_1_0:
    patchMask = ComputeLocalRootSignaturePatchMask(desc->Desc_1_0);
    break;
  case D3D_ROOT_SIGNATURE_VERSION_1_1:
    patchMask = ComputeLocalRootSignaturePatchMask(desc->Desc_1_1);
    break;
  case D3D_ROOT_SIGNATURE_VERSION_1_2:
    patchMask = ComputeLocalRootSignaturePatchMask(desc->Desc_1_2);
    break;
  }
  if (patchMask) {
    m_PatchMasksByLocalRootSignature[c.m_ppvRootSignature.Key] = *patchMask;
  }
}

void GpuPatchLocalRootSignatureService::CreateStateObject(
    ID3D12Device5CreateStateObjectCommand& c) {
  if (c.m_Result.Value != S_OK) {
    return;
  }
  StateObjectInfo info;
  FillStateObjectInfo(c.m_pDesc, info);
  m_StateObjectInfos[c.m_ppStateObject.Key] = std::move(info);
}

void GpuPatchLocalRootSignatureService::AddToStateObject(ID3D12Device7AddToStateObjectCommand& c) {
  if (c.m_Result.Value != S_OK) {
    return;
  }
  StateObjectInfo info;
  auto it = m_StateObjectInfos.find(ResolveStateObjectKey(c.m_pStateObjectToGrowFrom.Key));
  if (it != m_StateObjectInfos.end()) {
    info = it->second;
  } else {
    info.HasUnknownExports = true;
  }
  FillStateObjectInfo(c.m_pAddition, info);
  m_StateObjectInfos[c.m_ppNewStateObject.Key] = std::move(info);
}

void GpuPatchLocalRootSignatureService::QueryInterface(IUnknownQueryInterfaceCommand& c) {
  if (c.m_Result.Value != S_OK || !c.m_ppvObject.Key) {
    return;
  }
  // Each interface of a state object has its own key, e.g. ID3D12StateObjectProperties used for
  // GetShaderIdentifier
  ObjectKey stateObjectKey = ResolveStateObjectKey(c.m_Object.Key);
  if (c.m_ppvObject.Key != stateObjectKey && m_StateObjectInfos.contains(stateObjectKey)) {
    m_StateObjectsByInterface[c.m_ppvObject.Key] = stateObjectKey;
  }
}

void GpuPatchLocalRootSignatureService::DestroyInterface(ObjectKey key) {
  // State objects keep the masks of their local root signatures, which can be released earlier
  m_PatchMasksByLocalRootSignature.erase(key);
  ObjectKey stateObjectKey = ResolveStateObjectKey(key);
  if (m_StateObjectInfos.erase(stateObjectKey)) {
    std::erase_if(m_StateObjectsByInterface,
                  [stateObjectKey](const auto& alias) { return alias.second == stateObjectKey; });
  } else {
    m_StateObjectsByInterface.erase(key);
  }
}

uint64_t GpuPatchLocalRootSignatureService::GetPatchMask(ObjectKey stateObjectKey,
                                                         const wchar_t* exportName) const {
  auto it = m_StateObjectInfos.find(ResolveStateObjectKey(stateObjectKey));
  if (it == m_StateObjectInfos.end() || !exportName) {
    return PATCH_MASK_ALL;
  }
  return it->second.GetPatchMask(exportName);
}

uint64_t GpuPatchLocalRootSignatureService::StateObjectInfo::GetPatchMask(
    const wchar_t* exportName) const {
  auto it = PatchMasksByExport.find(exportName);
  return it != PatchMasksByExport.end() ? it->second : DefaultPatchMask;
}

ObjectKey GpuPatchLocalRootSignatureService::ResolveStateObjectKey(ObjectKey key) const {
  auto it = m_StateObjectsByInterface.find(key);
  return it != m_StateObjectsByInterface.end() ? it->second : key;
}

uint64_t GpuPatchLocalRootSignatureService::GetLocalRootSignaturePatchMask(
    ObjectKey rootSignatureKey) const {
  auto it = m_PatchMasksByLocalRootSignature.find(rootSignatureKey);
  return it != m_PatchMasksByLocalRootSignature.end() ? it->second : PATCH_MASK_ALL;
}

void GpuPatchLocalRootSignatureService::FillStateObjectInfo(
    const D3D12_STATE_OBJECT_DESC_Argument& stateObjectDesc, StateObjectInfo& info) const {
  const D3D12_STATE_OBJECT_DESC* desc = stateObjectDesc.Value;
  if (!desc) {
    return;
  }
  auto getSubobjectKey = [&stateObjectDesc](unsigned index) {
    auto it = stateObjectDesc.InterfaceKeysBySubobject.find(index);
    return it != stateObjectDesc.InterfaceKeysBySubobject.end() ? it->second : ObjectKey{};
  };

  std::unordered_map<const D3D12_STATE_SUBOBJECT*, uint64_t> localRootSignatures;
  std::unordered_set<const D3D12_STATE_SUBOBJECT*> associatedSubobjects;
  for (unsigned i = 0; i < desc->NumSubobjects; ++i) {
    const D3D12_STATE_SUBOBJECT& subobject = desc->pSubobjects[i];
    if (subobject.Type == D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE) {
      localRootSignatures[&subobject] = GetLocalRootSignaturePatchMask(getSubobjectKey(i));
    } else if (subobject.Type == D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION) {
      associatedSubobjects.insert(
          static_cast<const D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(subobject.pDesc)
              ->pSubobjectToAssociate);
    }
  }

  // An unassociated local root signature is an implicit default, ambiguous if there are several
  uint64_t defaultMask = info.DefaultPatchMask;
  unsigned implicitDefaultCount = 0;
  for (const auto& [subobject, mask] : localRootSignatures) {
    if (!associatedSubobjects.contains(subobject)) {
      defaultMask = implicitDefaultCount == 0 ? mask : PATCH_MASK_ALL;
      ++implicitDefaultCount;
    }
  }

  // Export names point into the description and are copied only when stored in info
  std::vector<LPCWSTR> exports;
  std::vector<LPCWSTR> dxilAssociatedExports;
  std::unordered_map<std::wstring, std::vector<LPCWSTR>> hitGroupsByShader;
  std::optional<uint64_t> unknownExportsMask;
  if (info.HasUnknownExports) {
    unknownExportsMask = info.DefaultPatchMask;
  }
  bool hasLocalUnknownExports = false;
  bool hasExplicitDefault = false;
  bool hasDxilDefault = false;

  for (unsigned i = 0; i < desc->NumSubobjects; ++i) {
    const D3D12_STATE_SUBOBJECT& subobject = desc->pSubobjects[i];
    switch (subobject.Type) {
    case D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY: {
      const auto& library = *static_cast<const D3D12_DXIL_LIBRARY_DESC*>(subobject.pDesc);
      hasLocalUnknownExports |= library.NumExports == 0;
      for (unsigned j = 0; j < library.NumExports; ++j) {
        exports.push_back(library.pExports[j].Name);
      }
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP: {
      const auto& hitGroup = *static_cast<const D3D12_HIT_GROUP_DESC*>(subobject.pDesc);
      exports.push_back(hitGroup.HitGroupExport);
      for (LPCWSTR shader : {hitGroup.AnyHitShaderImport, hitGroup.ClosestHitShaderImport,
                             hitGroup.IntersectionShaderImport}) {
        if (shader) {
          hitGroupsByShader[shader].push_back(hitGroup.HitGroupExport);
        }
      }
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION: {
      const auto& association =
          *static_cast<const D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(subobject.pDesc);
      auto it = localRootSignatures.find(association.pSubobjectToAssociate);
      if (it == localRootSignatures.end()) {
        break;
      }
      if (association.NumExports == 0) {
        defaultMask = it->second;
        hasExplicitDefault = true;
      }
      for (unsigned j = 0; j < association.NumExports; ++j) {
        info.PatchMasksByExport[association.pExports[j]] = it->second;
      }
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION: {
      // May associate a local root signature declared in a DXIL library
      const auto& association =
          *static_cast<const D3D12_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(subobject.pDesc);
      hasDxilDefault |= association.NumExports == 0;
      dxilAssociatedExports.insert(dxilAssociatedExports.end(), association.pExports,
                                   association.pExports + association.NumExports);
    } break;
    case D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION:
      ImportCollection(*static_cast<const D3D12_EXISTING_COLLECTION_DESC*>(subobject.pDesc),
                       getSubobjectKey(i), info, unknownExportsMask);
      break;
    }
  }

  // An export can have only one local root signature, so a DXIL association is ignored for
  // exports associated with one from the description
  for (LPCWSTR name : dxilAssociatedExports) {
    info.PatchMasksByExport.emplace(name, PATCH_MASK_ALL);
  }
  if (hasDxilDefault && !hasExplicitDefault) {
    defaultMask = PATCH_MASK_ALL;
  }

  // Hit groups without an explicit association use the local root signature of their shaders
  std::unordered_map<std::wstring, std::optional<uint64_t>> hitGroupMasks;
  for (const auto& [name, mask] : info.PatchMasksByExport) {
    auto it = hitGroupsByShader.find(name);
    if (it != hitGroupsByShader.end()) {
      for (LPCWSTR hitGroup : it->second) {
        MergePatchMask(hitGroupMasks[hitGroup], mask);
      }
    }
  }
  for (const auto& [hitGroup, mask] : hitGroupMasks) {
    info.PatchMasksByExport.emplace(hitGroup, *mask);
  }

  // Store the default per export, so it does not apply to other exports when this state object
  // is used as a collection
  for (LPCWSTR name : exports) {
    info.PatchMasksByExport.emplace(name, defaultMask);
  }
  if (hasLocalUnknownExports) {
    MergePatchMask(unknownExportsMask, defaultMask);
  }
  info.HasUnknownExports = unknownExportsMask.has_value();
  info.DefaultPatchMask = unknownExportsMask.value_or(defaultMask);
}

void GpuPatchLocalRootSignatureService::ImportCollection(
    const D3D12_EXISTING_COLLECTION_DESC& collectionDesc,
    ObjectKey collectionKey,
    StateObjectInfo& info,
    std::optional<uint64_t>& unknownExportsMask) const {
  auto it = m_StateObjectInfos.find(ResolveStateObjectKey(collectionKey));
  const StateObjectInfo* collection = it != m_StateObjectInfos.end() ? &it->second : nullptr;

  if (collectionDesc.NumExports == 0) {
    if (!collection) {
      MergePatchMask(unknownExportsMask, PATCH_MASK_ALL);
      return;
    }
    info.PatchMasksByExport.insert(collection->PatchMasksByExport.begin(),
                                   collection->PatchMasksByExport.end());
    if (collection->HasUnknownExports) {
      MergePatchMask(unknownExportsMask, collection->DefaultPatchMask);
    }
    return;
  }

  for (unsigned i = 0; i < collectionDesc.NumExports; ++i) {
    const D3D12_EXPORT_DESC& exportDesc = collectionDesc.pExports[i];
    LPCWSTR sourceName = exportDesc.ExportToRename ? exportDesc.ExportToRename : exportDesc.Name;
    info.PatchMasksByExport.emplace(
        exportDesc.Name, collection ? collection->GetPatchMask(sourceName) : PATCH_MASK_ALL);
  }
}

} // namespace DirectX
} // namespace gits
