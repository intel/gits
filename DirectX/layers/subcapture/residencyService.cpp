// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "residencyService.h"
#include "stateTrackingService.h"
#include "commandsAuto.h"
#include "commandWritersAuto.h"

namespace gits {
namespace DirectX {

void ResidencyService::createNotResident(const unsigned key, const unsigned deviceKey) {
  GITS_ASSERT(residency_.find(key) == residency_.end());
  residency_[key] = {0, deviceKey, true};
}

void ResidencyService::makeResident(const std::vector<unsigned>& keys, const unsigned deviceKey) {
  for (const auto key : keys) {
    if (residency_.find(key) == residency_.end()) {
      residency_[key] = {2, deviceKey};
    } else {
      ++residency_[key].residencyCount;
    }
  }
}

void ResidencyService::evict(const std::vector<unsigned>& keys, const unsigned deviceKey) {
  for (const auto key : keys) {
    if (residency_.find(key) == residency_.end()) {
      residency_[key] = {0, deviceKey};
    } else {
      if (residency_[key].residencyCount > 0) {
        --residency_[key].residencyCount;
      }
    }
  }
}

void ResidencyService::destroyObject(const unsigned key) {
  residency_.erase(key);
}

void gits::DirectX::ResidencyService::restoreResidency() {
  for (const auto& [objectKey, residencyInfo] : residency_) {
    if (!stateService_.stateRestored(objectKey)) {
      continue;
    }
    if (residencyInfo.residencyCount >= 2 ||
        (residencyInfo.createdNotResident && residencyInfo.residencyCount == 1)) {
      const auto repeatCount = residencyInfo.createdNotResident ? residencyInfo.residencyCount
                                                                : residencyInfo.residencyCount - 1;
      std::vector<unsigned> objectKeyRepeat(repeatCount, objectKey);

      ID3D12DeviceMakeResidentCommand c;
      c.key = stateService_.getUniqueCommandKey();
      c.object_.key = residencyInfo.deviceKey;
      c.NumObjects_.value = objectKeyRepeat.size();
      c.ppObjects_.size = objectKeyRepeat.size();
      c.ppObjects_.keys = std::move(objectKeyRepeat);
      ID3D12Pageable* fakePtr = reinterpret_cast<ID3D12Pageable*>(1);
      c.ppObjects_.value = &fakePtr;
      stateService_.getRecorder().record(new ID3D12DeviceMakeResidentWriter(c));
    } else if (residencyInfo.residencyCount == 0 && !residencyInfo.createdNotResident) {
      ID3D12DeviceEvictCommand c;
      c.key = stateService_.getUniqueCommandKey();
      c.object_.key = residencyInfo.deviceKey;
      c.NumObjects_.value = 1;
      c.ppObjects_.size = 1;
      c.ppObjects_.keys = {objectKey};
      ID3D12Pageable* fakePtr = reinterpret_cast<ID3D12Pageable*>(1);
      c.ppObjects_.value = &fakePtr;
      stateService_.getRecorder().record(new ID3D12DeviceEvictWriter(c));
    }
  }
}

} // namespace DirectX
} // namespace gits
