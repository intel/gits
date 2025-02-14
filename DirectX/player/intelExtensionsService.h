// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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

  void loadIntelExtensions(const uint32_t& vendorID, const uint32_t& deviceID);
  void setApplicationName(const std::string& appName);

private:
  bool intelExtensionLoaded_{};
  bool applicationNameSet_{};
};

} // namespace DirectX
} // namespace gits
