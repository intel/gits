// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "printGuidsAuto.h"
#include "directx.h"

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

FastOStream& operator<<(FastOStream& stream, REFIID riid) {
  auto it = g_guidToStringMap.find(riid);
  if (it != g_guidToStringMap.end()) {
    stream << it->second;
  } else {
    printHexFull(stream, riid.Data1) << "-";
    printHexFull(stream, riid.Data2) << "-";
    printHexFull(stream, riid.Data3) << "-";
    for (int i = 0; i < 2; ++i) {
      printHexFull(stream, static_cast<uint8_t>(riid.Data4[i]));
    }
    stream << "-";
    for (int i = 2; i < 8; ++i) {
      printHexFull(stream, static_cast<uint8_t>(riid.Data4[i]));
    }
  }
  return stream;
}

} // namespace DirectX
} // namespace gits
