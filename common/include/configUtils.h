// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "keyEvents.h"

#include <string>
#include <sstream>
#include <vector>

namespace gits {
template <class T>
bool parse_pair(const std::string& s, T& pair) {
  std::stringstream str(s);
  str >> pair.first;
  str.ignore();
  str >> pair.second;
  return !(!str.eof() || str.fail() || str.bad());
}

template <class T>
bool parse_vector(const std::string& s, std::vector<T>& vector, size_t size) {
  std::stringstream str(s);
  std::vector<T> vec;
  while (!str.eof() && !str.fail() && !str.bad()) {
    vec.push_back((T)0);
    str >> vec.back();
    str.ignore();
  }
  if (size == vec.size()) {
    vector = std::move(vec);
    return true;
  }
  return false;
}

void parse_vector(const std::string& s, std::vector<std::string>& vector);
std::vector<uint32_t> parseKeys(const std::vector<std::string>& input);
} // namespace gits
