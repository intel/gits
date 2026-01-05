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

#include <filesystem>

namespace gits {
namespace DirectX {

class XessService : public gits::noncopyable {
public:
  XessService() = default;
  ~XessService();
  bool loadXess(std::filesystem::path path);

  XessDispatchTable& getXessDispatchTable() {
    return xessDispatchTable_;
  }

private:
  HMODULE xessDll_{};
  XessDispatchTable xessDispatchTable_{};
};

} // namespace DirectX
} // namespace gits
