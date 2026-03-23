// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   performance.h
 *
 * @brief Declaration of function calls performance.
 *
 */

#pragma once

#include <iostream>
#include <vector>
#include <cstdint>

namespace gits {
class FrameTimeSheet {
public:
  void AddFrameTime(uint64_t time);
  void OutputTimeData(std::ostream& stream);

private:
  unsigned m_FrameNumber{};
  std::vector<std::pair<unsigned, double>> m_FrameTimes;
};

} // namespace gits
