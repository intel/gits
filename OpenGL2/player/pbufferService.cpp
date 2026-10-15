// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "pbufferService.h"

#include <iterator>
#include <utility>

namespace gits::opengl2 {

PbufferService& PbufferService::Get() {
  static PbufferService instance;
  return instance;
}

void PbufferService::AcquireDc(HDC recordedDc, GITSKey pbufferKey, HDC realDc) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  m_Dcs[recordedDc].push_back(PbufferDc{pbufferKey, realDc});
}

std::optional<HDC> PbufferService::FindDc(HDC recordedDc) {
  if (recordedDc == nullptr) {
    return std::nullopt;
  }
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_Dcs.find(recordedDc);
  if (it == m_Dcs.end()) {
    return std::nullopt;
  }
  return it->second.back().Real;
}

void PbufferService::ReleaseDc(HDC recordedDc) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  auto it = m_Dcs.find(recordedDc);
  if (it == m_Dcs.end()) {
    return;
  }
  it->second.pop_back();
  if (it->second.empty()) {
    m_Dcs.erase(it);
  }
}

void PbufferService::ErasePbuffer(GITSKey pbufferKey) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  for (auto it = m_Dcs.begin(); it != m_Dcs.end();) {
    std::erase_if(it->second,
                  [pbufferKey](const PbufferDc& dc) { return dc.Pbuffer == pbufferKey; });
    it = it->second.empty() ? m_Dcs.erase(it) : std::next(it);
  }
}

} // namespace gits::opengl2
