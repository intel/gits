// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once
#include "streams.h"
#include "pragmas.h"
#include <fstream>
#include <stdexcept>
#include "tools.h"
#include <filesystem>

namespace gits {
template <typename T>
void write_map(const std::filesystem::path& filename, const T& m) {
  std::ofstream file(filename, std::ios::binary);
  if (!file.is_open()) {
    CheckMinimumAvailableDiskSize();
    throw std::runtime_error("couldn't create file: " + filename.string());
  }

  typename T::const_iterator iter = m.begin();
  for (; iter != m.end(); ++iter) {
    write_to_stream(file, iter->first);
    write_to_stream(file, iter->second);
  }
}

template <typename KeyType, typename ValueType>
void append_map(const std::filesystem::path& filename, const KeyType& key, const ValueType& value) {
  std::ofstream file(filename, std::ios::binary | std::ios::app);
  if (!file.is_open()) {
    CheckMinimumAvailableDiskSize();
    throw std::runtime_error("couldn't append to a file: " + filename.string());
  }

  write_to_stream(file, key);
  write_to_stream(file, value);
  file.flush();
}

template <typename T>
T read_map(const std::filesystem::path& filename) {
  T retval;
  std::ifstream file(filename, std::ios::binary);
  if (!file.is_open()) {
    throw std::runtime_error("corrupted key-val store (couldn't open file): " + filename.string());
  }

  file.peek();
  while (!file.eof()) {
    typename T::key_type key;
    typename T::mapped_type value;
    read_from_stream(file, key);
    if (!file) {
      throw std::runtime_error("corrupted key-val store (bad key): " + filename.string());
    }

    read_from_stream(file, value);
    if (!file) {
      throw std::runtime_error("corrupted key-val store (bad value): " + filename.string());
    }

    retval[key] = value;

    //lookahead for eof
    file.peek();
  }
  return retval;
}
} // namespace gits
