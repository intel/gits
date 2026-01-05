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

#include <sstream>
#include <unordered_map>
#include <string>
#include <iomanip>

namespace gits {
namespace DirectX {

struct IIDHash {
  size_t operator()(REFIID riid) const {
    const uint32_t* p = reinterpret_cast<const uint32_t*>(&riid);
    return p[0] ^ p[1] ^ p[2] ^ p[3];
  }
};

const IID IID_GfxBenchWrapper = {
    0x8B11DE73, 0xC379, 0x4A6F, {0xAE, 0xBD, 0x41, 0x9B, 0xAB, 0xDE, 0x73, 0xD8}};

const IID IID_ID3DDestructionNotifier = {
    0XA06EB39A, 0X50DA, 0X425B, {0X8C, 0X31, 0X4E, 0XEC, 0XD6, 0XC2, 0X70, 0XF3}};

const std::unordered_map<IID, std::string, IIDHash> g_guidToStringMap {
  { IID_GfxBenchWrapper, "IID_GfxBenchWrapper" },
  { IID_IUnknown, "IID_IUnknown" },
  %for interface in interfaces:
  { __uuidof(${interface.name}), "IID_${interface.name}" },
  %endfor
};

std::string toStr(REFIID riid) {
  std::stringstream ss;
  auto it = g_guidToStringMap.find(riid);
  if (it != g_guidToStringMap.end()) {
    ss << it->second;
  } else {
    ss << std::setfill('0') << std::hex << std::uppercase;
    ss << std::setw(8) << riid.Data1 << "-";
    ss << std::setw(4) << riid.Data2 << "-";
    ss << std::setw(4) << riid.Data3 << "-";
    for (int i = 0; i < 2; ++i) {
      ss << std::setw(2) << unsigned(riid.Data4[i]);
    }
    ss << "-";
    for (int i = 2; i < 8; ++i) {
      ss << std::setw(2) << unsigned(riid.Data4[i]);
    }
  }
  return ss.str();
}

} // namespace DirectX
} // namespace gits
