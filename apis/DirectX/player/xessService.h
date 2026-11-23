// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "xessDispatchTableAuto.h"
#include "xellDispatchTableAuto.h"
#include "xefgDispatchTableAuto.h"

#include <filesystem>

namespace gits {
namespace DirectX {

class XessService {
public:
  XessService() = default;
  ~XessService();
  XessService(const XessService&) = delete;
  XessService& operator=(const XessService&) = delete;

  bool LoadXess(std::filesystem::path path);
  bool LoadXell(std::filesystem::path path);
  bool LoadXefg(std::filesystem::path path);

  XessDispatchTable& GetXessDispatchTable() {
    return m_XessDispatchTable;
  }

  XellDispatchTable& GetXellDispatchTable() {
    return m_XellDispatchTable;
  }

  XefgDispatchTable& GetXefgDispatchTable() {
    return m_XefgDispatchTable;
  }

private:
  HMODULE m_XessDll{};
  XessDispatchTable m_XessDispatchTable{};
  HMODULE m_XellDll{};
  XellDispatchTable m_XellDispatchTable{};
  HMODULE m_XefgDll{};
  XefgDispatchTable m_XefgDispatchTable{};
};

} // namespace DirectX
} // namespace gits
