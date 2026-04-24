// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "intelExtensions.h"

namespace gits {
namespace DirectX {

class IntelExtensionsService {
public:
  IntelExtensionsService() = default;
  ~IntelExtensionsService();
  IntelExtensionsService(const IntelExtensionsService&) = delete;
  IntelExtensionsService& operator=(const IntelExtensionsService&) = delete;

  void LoadIntelExtensions(IDXGIAdapter1* adapter);
  void SetApplicationInfo();
  const INTCExtensionAppInfo1& GetAppInfo() const;

private:
  bool m_IntelExtensionLoaded{};
  bool m_ApplicationNameSet{};

  std::wstring m_AppName{};
  std::wstring m_EngineName{};
  INTCExtensionAppInfo1 m_AppInfo{};
};

} // namespace DirectX
} // namespace gits
