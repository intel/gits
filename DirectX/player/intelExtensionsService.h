// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "intelExtensions.h"
#include "gits.h"

namespace gits {
namespace DirectX {

class IntelExtensionsService : public gits::noncopyable {
public:
  IntelExtensionsService() = default;
  ~IntelExtensionsService();

  void loadIntelExtensions(IDXGIAdapter1* adapter);
  void setApplicationInfo();
  const INTCExtensionAppInfo1& getAppInfo() const;

private:
  bool intelExtensionLoaded_{};
  bool applicationNameSet_{};

  std::wstring appName_{};
  std::wstring engineName_{};
  INTCExtensionAppInfo1 appInfo_{};
};

} // namespace DirectX
} // namespace gits
