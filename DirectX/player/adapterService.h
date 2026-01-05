// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "gits.h"
#include "directx.h"

#include <map>
#include <wrl/client.h>

namespace gits {
namespace DirectX {

class AdapterService : public gits::noncopyable {
public:
  AdapterService() = default;
  ~AdapterService() = default;

  void loadAdapters();
  bool isAdapterOverride() const;
  IDXGIAdapter1* getAdapter() const;

  void setCaptureAdapterLuid(unsigned key, LUID captureLuid);
  void setCurrentAdapterLuid(unsigned key, LUID currentLuid);
  LUID getCurrentLuid(LUID captureLuid);

private:
  struct LessLuid {
    bool operator()(const LUID& l1, const LUID& l2) const {
      return l1.LowPart < l2.LowPart
                 ? true
                 : (l1.LowPart == l2.LowPart && l1.HighPart < l2.HighPart ? true : false);
    }
  };

  Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter_;
  std::map<unsigned, LUID> captureLuids_;
  std::map<LUID, LUID, LessLuid> luidsByCaptureLuid_;
};

} // namespace DirectX
} // namespace gits
