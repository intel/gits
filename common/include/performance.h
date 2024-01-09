// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
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

#include "pragmas.h"
#include "tools.h"

#include <vector>
#include <string>
#include <algorithm>
#include <map>
#include <string>
#include <cstdint>

DISABLE_WARNINGS
#include <boost/optional.hpp>
ENABLE_WARNINGS

namespace gits {
class FrameTimeSheet {
public:
  FrameTimeSheet() {}

  void add_frame_time(const char* row_id, uint64_t time);
  void add_frame_data(const char* row_id, const char* data);
  template <class T>
  void add_frame_data(const char* row_id, T data) {
    auto str = std::to_string(data);
    add_frame_data(row_id, str.c_str());
  }

  const std::vector<uint64_t>& row_times(const char* row_id) const;
  const std::vector<std::string>& row_data(const char* row_id) const;

  const std::vector<std::string>& row_names() const;

  void OutputTimeData(std::ostream& stream, bool addFps);

private:
  size_t get_name_idx(const char* row_name) const;
  boost::optional<size_t> try_get_name_idx(const char* row_name) const;
  std::map<size_t, std::vector<uint64_t>> _times;
  std::map<size_t, std::vector<std::string>> _aux;
  std::vector<std::string> _rows;
};
} // namespace gits
