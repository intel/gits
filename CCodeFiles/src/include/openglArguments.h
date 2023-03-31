// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <ostream>
#include <istream>
#include <unordered_map>
#include "config.h"

namespace gits {

struct CArgument {};
struct CBinOStream : std::ostream {};
struct CCodeOStream : std::ostream {};
struct CBinIStream : std::istream {};

template <int Value>
int identity() {
  return Value;
}
inline void ensure_unique_ptr(uint64_t value) {
  static std::unordered_map<uint32_t, uint64_t> values;
  auto old = values[static_cast<uint32_t>(value)];
  if (old != 0 && old != value) {
    throw std::runtime_error("Collision detected for pointer value held in the stream.");
  }
  values[static_cast<uint32_t>(value)] = value;
}
template <typename T>
void write_to_stream(std::ostream& o, const T& value) {
  if (!Config::Get().recorder.extras.utilities.nullIO) {
    o.write(reinterpret_cast<const char*>(&value), sizeof(value));
  }
}

template <typename T>
void read_from_stream(std::istream& i, T& value) {
  i.read(reinterpret_cast<char*>(&value), sizeof(value));
}

template <class T>
void write_name_to_stream(std::ostream& o, T value) {
  write_to_stream(o, value);
}
template <class T>
void write_name_to_stream(std::ostream& o, T* value) {
  uint64_t v = reinterpret_cast<uintptr_t>(value);
  write_to_stream(o, v);
}

template <class T>
void read_name_from_stream(std::istream& i, T& value) {
  read_from_stream(i, value);
}
template <class T>
void read_name_from_stream(std::istream& i, T*& value) {
  uint64_t v = 0U;
  read_from_stream(i, v);
  if (v <= UINT64_MAX) {
    if (identity<sizeof(T*) != sizeof(uint64_t)>()) {
      ensure_unique_ptr(v);
    }
    value = reinterpret_cast<T*>(static_cast<uintptr_t>(v));
  }
}

template <class T>
void read_name_from_stream(CBinIStream& i, T& value) {
  read_from_stream(i, value);
}
template <class T>
void read_name_from_stream(CBinIStream& i, T*& value) {
  uint64_t v;
  read_from_stream(i, v);
  if (identity<sizeof(T*) != sizeof(uint64_t)>()) {
    ensure_unique_ptr(v);
  }
  value = reinterpret_cast<T*>((uintptr_t)v);
}

enum MappedArrayAction { ADD_MAPPING = 0, REMOVE_MAPPING = 1, NO_ACTION = 2 };

namespace OpenGL {

template <class T, class U>
struct CArgumentSizedArray {};
template <class T, class U, MappedArrayAction T_ACTION>
struct CArgumentMappedSizedArray {};
} // namespace OpenGL

} // namespace gits
