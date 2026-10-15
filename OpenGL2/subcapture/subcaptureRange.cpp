// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "subcaptureRange.h"

#include "exception.h"

#include <charconv>
#include <string_view>

namespace gits::opengl2 {
namespace {

bool ParseFrame(std::string_view text, uint32_t& frame) {
  const char* end = text.data() + text.size();
  const auto [last, error] = std::from_chars(text.data(), end, frame);
  return error == std::errc() && last == end;
}

} // namespace

SubcaptureRange::SubcaptureRange(const std::string& frames) {
  const std::string_view text = frames;
  const auto dash = text.find('-');
  const std::string_view start = text.substr(0, dash);
  const std::string_view end = dash == std::string_view::npos ? start : text.substr(dash + 1);
  if (!ParseFrame(start, m_StartFrame) || !ParseFrame(end, m_EndFrame) || m_StartFrame == 0 ||
      m_EndFrame < m_StartFrame) {
    throw Exception("Subcapture: invalid frame range '" + frames + "'");
  }
}

} // namespace gits::opengl2
