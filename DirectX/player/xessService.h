// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "gits.h"
#include "xessDispatchTableAuto.h"

namespace gits {
namespace DirectX {

class XessService : public gits::noncopyable {
public:
  XessService() = default;
  ~XessService();
  void loadXess();

  XessDispatchTable& getXessDispatchTable() {
    return xessDispatchTable_;
  }

private:
  HMODULE xessDll_{};
  XessDispatchTable xessDispatchTable_{};
};

} // namespace DirectX
} // namespace gits
