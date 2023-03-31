// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   performance.cpp
 *
 * @brief Definitions of function calls performance.
 *
 */
#include "performance.h"
#include "exception.h"

#include <algorithm>

namespace gits {

void FrameTimeSheet::add_frame_time(const char* row_id, uint64_t time) {
  boost::optional<size_t> idx = try_get_name_idx(row_id);
  if (!idx) {
    idx = _rows.size();
    _rows.push_back(row_id);
  }
  _times[*idx].push_back(time);
}

void FrameTimeSheet::add_frame_data(const char* row_id, const char* data) {
  boost::optional<size_t> idx = try_get_name_idx(row_id);
  if (!idx) {
    idx = _rows.size();
    _rows.push_back(row_id);
  }
  _aux[*idx].push_back(data);
}

const std::vector<uint64_t>& FrameTimeSheet::row_times(const char* row_id) const {
  auto idx = get_name_idx(row_id);
  return _times.at(idx);
}

const std::vector<std::string>& FrameTimeSheet::row_data(const char* row_id) const {
  auto idx = get_name_idx(row_id);
  return _aux.at(idx);
}

const std::vector<std::string>& FrameTimeSheet::row_names() const {
  return _rows;
}

size_t FrameTimeSheet::get_name_idx(const char* row_name) const {
  boost::optional<size_t> idx = try_get_name_idx(row_name);
  if (!idx) {
    throw std::runtime_error("Unknown row name '" + std::string(row_name) + "'");
  }
  return *idx;
}

boost::optional<size_t> FrameTimeSheet::try_get_name_idx(const char* row_name) const {
  std::vector<std::string>::const_iterator iter = std::find(_rows.begin(), _rows.end(), row_name);
  if (iter == _rows.end()) {
    return boost::optional<size_t>();
  }
  return iter - _rows.begin();
}

namespace {
bool dont_convert_to_seconds(const std::string& name) {
  return name == "frame-no";
}
bool should_compute_inverse(const std::string& name) {
  return name == "cpu" || name == "gpu";
}
} // namespace

void FrameTimeSheet::OutputTimeData(std::ostream& stream, bool addFps) {
  stream << "nr, ";
  for (const auto& name : _rows) {
    stream << name << ", ";
    if (addFps && should_compute_inverse(name)) {
      stream << name << "-inv, ";
    }
  }
  stream << "\n";

  size_t max_size = 0;
  for (const auto& elem : _times) {
    max_size = std::max(max_size, elem.second.size());
  }
  for (const auto& elem : _aux) {
    max_size = std::max(max_size, elem.second.size());
  }

  // Has data.
  if (!_times.empty() || !_aux.empty()) {
    // we will make 'stamp' based on first 'cpu' sample
    boost::optional<size_t> stamp_idx = try_get_name_idx("stamp");
    boost::optional<size_t> cpu_idx = try_get_name_idx("cpu");
    uint64_t stamp_base = 0;

    if (stamp_idx) {
      stamp_base = _times[*stamp_idx][0];
    }

    if (cpu_idx) {
      stamp_base -= _times[*cpu_idx][0];
    }

    for (size_t i = 0; i < max_size; ++i) {
      stream << i + 1 << ", ";

      for (size_t j = 0; j < _rows.size(); ++j) {
        bool no_scale = dont_convert_to_seconds(_rows[j]);

        if (_times.find(j) != _times.end()) {
          // this is element from 'times'
          if (stamp_idx && j == *stamp_idx) {
            stream << (_times[j][i] - stamp_base) / (no_scale ? 1 : 1e9);
          } else {
            stream << _times[j][i] / (no_scale ? 1 : 1e9);
          }

          if (addFps && should_compute_inverse(_rows[j])) {
            stream << ", " << 1e9 / _times[j][i];
          }
        } else if (_aux.find(j) != _aux.end()) {
          // this is data element from 'aux'
          stream << _aux[j][i];
        }

        stream << ", ";
      }
      stream << "\n";
    }
  }
}

} // namespace gits
