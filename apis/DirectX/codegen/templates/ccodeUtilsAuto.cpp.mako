// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "directx.h"

#include <unordered_map>

namespace gits {
namespace DirectX {

struct IIDHash {
  size_t operator()(REFIID riid) const {
    const uint32_t* p = reinterpret_cast<const uint32_t*>(&riid);
    return p[0] ^ p[1] ^ p[2] ^ p[3];
  }
};

const std::unordered_map<IID, IID, IIDHash> g_guidMap {
  { IID_IUnknown, IID_IUnknown },
  %for interface in interfaces:
  { __uuidof(${interface.name}), __uuidof(${interface.latest_interface}) },
  %endfor
};

REFIID getLatestInterface(REFIID riid) {
  return g_guidMap.at(riid);
}

} // namespace DirectX
} // namespace gits
