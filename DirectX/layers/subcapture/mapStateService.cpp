// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "mapStateService.h"
#include "stateTrackingService.h"
#include "commandsAuto.h"
#include "commandSerializersAuto.h"

namespace gits {
namespace DirectX {

void MapStateService::RestoreMapState() {
  for (auto& itResource : m_MappedDataBySubresource) {
    if (!m_StateService.StateRestored(itResource.first)) {
      continue;
    }
    for (auto& itSubresource : itResource.second) {
      ID3D12ResourceMapCommand c;
      c.Key = m_StateService.GetUniqueCommandKey();
      c.m_Object.Key = itResource.first;
      c.m_Subresource.Value = itSubresource.first;
      c.m_pReadRange.Value = nullptr;
      c.m_ppData.CaptureValue = itSubresource.second;
      c.m_ppData.Value = &itSubresource.second;
      m_StateService.GetRecorder().Record(ID3D12ResourceMapSerializer(c));
    }
  }
}

} // namespace DirectX
} // namespace gits
