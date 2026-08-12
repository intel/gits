// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once
#include "arguments.h"

#include <dxgi.h>
#include <map>
#include <wrl/client.h>

namespace gits {
namespace DirectX {

class AdapterService {
public:
  void LoadAdapters();
  bool IsAdapterOverride() const;
  IDXGIAdapter1* GetAdapter() const;

  void SetCaptureAdapterLuid(GITSKey key, LUID captureLuid);
  void SetCurrentAdapterLuid(GITSKey key, LUID currentLuid);
  LUID GetCurrentLuid(LUID captureLuid);

private:
  struct LessLuid {
    bool operator()(const LUID& l1, const LUID& l2) const {
      return l1.LowPart < l2.LowPart
                 ? true
                 : (l1.LowPart == l2.LowPart && l1.HighPart < l2.HighPart ? true : false);
    }
  };

  Microsoft::WRL::ComPtr<IDXGIAdapter1> m_Adapter;
  std::map<GITSKey, LUID> m_CaptureLuids;
  std::map<LUID, LUID, LessLuid> m_LuidsByCaptureLuid;
};

} // namespace DirectX
} // namespace gits
