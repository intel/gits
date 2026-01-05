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
#include "commandWritersAuto.h"

namespace gits {
namespace DirectX {

void MapStateService::restoreMapState() {
  for (auto& itResource : mappedDataBySubresource_) {
    if (!stateService_.stateRestored(itResource.first)) {
      continue;
    }
    for (auto& itSubresource : itResource.second) {
      ID3D12ResourceMapCommand c;
      c.key = stateService_.getUniqueCommandKey();
      c.object_.key = itResource.first;
      c.Subresource_.value = itSubresource.first;
      c.pReadRange_.value = nullptr;
      c.ppData_.captureValue = itSubresource.second;
      c.ppData_.value = &itSubresource.second;
      stateService_.getRecorder().record(new ID3D12ResourceMapWriter(c));
    }
  }
}

} // namespace DirectX
} // namespace gits
