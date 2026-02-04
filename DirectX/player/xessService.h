// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "gits.h"
#include "xessDispatchTableAuto.h"
#include "xellDispatchTableAuto.h"

#include <filesystem>

namespace gits {
namespace DirectX {

class XessService : public gits::noncopyable {
public:
  XessService() = default;
  ~XessService();
  bool loadXess(std::filesystem::path path);
  bool loadXell(std::filesystem::path path);

  XessDispatchTable& getXessDispatchTable() {
    return xessDispatchTable_;
  }

  XellDispatchTable& getXellDispatchTable() {
    return xellDispatchTable_;
  }

private:
  HMODULE xessDll_{};
  XessDispatchTable xessDispatchTable_{};
  HMODULE xellDll_{};
  XellDispatchTable xellDispatchTable_{};
};

} // namespace DirectX
} // namespace gits
