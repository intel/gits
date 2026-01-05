// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <map>
#include <optional>
#include <cassert>

namespace gits {

template <typename T>
class IntervalSet {
private:
  std::map<T, T> _map;

public:
  void insert(T begin, T end) {
    assert(begin <= end);
    auto itNext = _map.upper_bound(begin);
    if (itNext != _map.begin()) {
      auto itPrev = std::prev(itNext);
      if (itPrev->second >= begin) {
        begin = itPrev->first;
        _map.erase(itPrev);
      }
    }
    while (itNext != _map.end() && itNext->first <= end) {
      end = std::max(itNext->second, end);
      itNext = _map.erase(itNext);
    }
    _map[begin] = end;
  }

  std::optional<std::pair<T, T>> find(const T& val) {
    auto itNext = _map.upper_bound(val);
    if (itNext == _map.begin()) {
      return std::nullopt;
    }
    auto it = std::prev(itNext);
    if (it->first <= val && val < it->second) {
      return *it;
    }
    return std::nullopt;
  }

  void clear() {
    _map.clear();
  }

  std::map<T, T> getIntervals() const {
    return _map;
  }
};

} // namespace gits
