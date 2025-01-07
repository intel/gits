// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <vector>

namespace gits {
template <class T>
T GetMaxUintVal() {
  T val = 0;
  const auto ptrBegin = (uint8_t*)&val;
  const auto ptrEnd = (uint8_t*)&val + sizeof(T);
  for (auto ptr = (uint8_t*)&val; ptr < ptrEnd && ptr >= ptrBegin; ptr++) {
    *ptr = 0xFF;
  }
  return val;
}
// Map based on a vector is faster than regular map in mapping/unmapping
// operations, but it also brings several limitations tied to it's nature.
// Size of the VectorMapper grows linearly to it's highest key value, so be
// careful using it to map huge values.
template <class T, std::uint64_t DEF_SIZE>
class VectorMapper {
public:
  typedef typename std::vector<T>::size_type size_type;

private:
  const T _notMappedVal;
  mutable size_type _vectorSize;
  mutable std::vector<T> _vector;
  void UpdateSize(size_type pos) const {
    if (pos >= _vectorSize) {
      auto newSize = pos + (pos * 20) / 100;
      _vector.resize(newSize, _notMappedVal);
      _vectorSize = newSize;
    }
  }

public:
  VectorMapper()
      : _notMappedVal(GetMaxUintVal<T>()),
        _vectorSize((size_type)DEF_SIZE),
        _vector((size_type)DEF_SIZE, _notMappedVal) {}
  const T& operator[](size_type pos) const {
    UpdateSize(pos);
    return _vector[pos];
  }
  T& operator[](size_type pos) {
    UpdateSize(pos);
    return _vector[pos];
  }

  T NotMappedVal() const {
    return _notMappedVal;
  }
  void Unmap(size_type pos) {
    (*this)[pos] = NotMappedVal();
  }
  size_t Size() const {
    return _vectorSize;
  }
};
} // namespace gits
