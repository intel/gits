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
#include "commandSerializersAuto.h"

namespace gits {
namespace DirectX {

void ResidencyService::CreateNotResident(const unsigned key, const unsigned DeviceKey) {
  GITS_ASSERT(m_Residency.find(key) == m_Residency.end());
  m_Residency[key] = {0, DeviceKey, true};
}

void ResidencyService::MakeResident(const std::vector<unsigned>& keys, const unsigned DeviceKey) {
  for (const auto key : keys) {
    if (m_Residency.find(key) == m_Residency.end()) {
      m_Residency[key] = {2, DeviceKey};
    } else {
      ++m_Residency[key].ResidencyCount;
    }
  }
}

void ResidencyService::Evict(const std::vector<unsigned>& keys, const unsigned DeviceKey) {
  for (const auto key : keys) {
    if (m_Residency.find(key) == m_Residency.end()) {
      m_Residency[key] = {0, DeviceKey};
    } else {
      if (m_Residency[key].ResidencyCount > 0) {
        --m_Residency[key].ResidencyCount;
      }
    }
  }
}

void ResidencyService::DestroyObject(const unsigned key) {
  m_Residency.erase(key);
}

void gits::DirectX::ResidencyService::RestoreResidency() {
  for (const auto& [objectKey, residencyInfo] : m_Residency) {
    if (!m_StateService.StateRestored(objectKey)) {
      continue;
    }
    if (residencyInfo.ResidencyCount >= 2 ||
        (residencyInfo.CreatedNotResident && residencyInfo.ResidencyCount == 1)) {
      const auto repeatCount = residencyInfo.CreatedNotResident ? residencyInfo.ResidencyCount
                                                                : residencyInfo.ResidencyCount - 1;
      std::vector<unsigned> objectKeyRepeat(repeatCount, objectKey);

      ID3D12DeviceMakeResidentCommand c;
      c.Key = m_StateService.GetUniqueCommandKey();
      c.m_Object.Key = residencyInfo.DeviceKey;
      c.m_NumObjects.Value = objectKeyRepeat.size();
      c.m_ppObjects.Size = objectKeyRepeat.size();
      c.m_ppObjects.Keys = std::move(objectKeyRepeat);
      ID3D12Pageable* fakePtr = reinterpret_cast<ID3D12Pageable*>(1);
      c.m_ppObjects.Value = &fakePtr;
      m_StateService.GetRecorder().Record(ID3D12DeviceMakeResidentSerializer(c));
    } else if (residencyInfo.ResidencyCount == 0 && !residencyInfo.CreatedNotResident) {
      ID3D12DeviceEvictCommand c;
      c.Key = m_StateService.GetUniqueCommandKey();
      c.m_Object.Key = residencyInfo.DeviceKey;
      c.m_NumObjects.Value = 1;
      c.m_ppObjects.Size = 1;
      c.m_ppObjects.Keys = {objectKey};
      ID3D12Pageable* fakePtr = reinterpret_cast<ID3D12Pageable*>(1);
      c.m_ppObjects.Value = &fakePtr;
      m_StateService.GetRecorder().Record(ID3D12DeviceEvictSerializer(c));
    }
  }
}

} // namespace DirectX
} // namespace gits
