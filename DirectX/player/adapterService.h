// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "gits.h"

#include <dxgi.h>
#include <map>

namespace gits {
namespace DirectX {

class AdapterService {
public:
  void setCaptureAdapterLuid(unsigned key, LUID captureLuid) {
    captureLuids_[key] = captureLuid;
  }

  void setCurrentAdapterLuid(unsigned key, LUID currentLuid) {
    auto it = captureLuids_.find(key);
    GITS_ASSERT(it != captureLuids_.end());
    luidsByCaptureLuid_[it->second] = currentLuid;
    captureLuids_.erase(it);
  }

  LUID getCurrentLuid(LUID captureLuid) {
    if (luidsByCaptureLuid_.empty()) {
      return LUID{0};
    }
    auto it = luidsByCaptureLuid_.find(captureLuid);
    GITS_ASSERT(it != luidsByCaptureLuid_.end());
    return it->second;
  }

private:
  std::map<unsigned, LUID> captureLuids_;
  struct LessLuid {
    bool operator()(const LUID& l1, const LUID& l2) const {
      return l1.LowPart < l2.LowPart
                 ? true
                 : (l1.LowPart == l2.LowPart && l1.HighPart < l2.HighPart ? true : false);
    }
  };
  std::map<LUID, LUID, LessLuid> luidsByCaptureLuid_;
};

} // namespace DirectX
} // namespace gits
