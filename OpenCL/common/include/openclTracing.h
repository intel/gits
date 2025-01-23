// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <string>

#include "tools.h"

namespace gits {
namespace OpenCL {
class TOclType;

namespace {
// base function to print an array as string
// used by Printer struct
// func should provide formatting for a single element
template <typename Base>
static std::string PrintElementsBase(size_t size,
                                     const Base* array,
                                     std::function<std::string(const Base)> func) {
  if (array != nullptr) {
    std::string elements(gits::hex(array).ToString());
    elements += " { ";
    // limit amount of elements to 8
    size_t count = std::min((size_t)8, size);
    for (size_t i = 0; i < count; ++i) {
      elements += func(array[i]);
      if (i + 1 < count) {
        elements += ", ";
      }
    }
    if (count != size) {
      elements += "...";
    }
    elements += " }";
    return elements;
  } else {
    return "nullptr";
  }
}

// provides formatting for TOclArray
template <typename Base, typename Wrapper, bool = std::is_base_of<TOclType, Wrapper>::value>
struct Printer {
  static std::string PrintElements(size_t size, const Base* array);
};

// partial specialization for Wrappers derived from TOclType
template <typename Base, typename Wrapper>
struct Printer<Base, Wrapper, true> {
  static std::string PrintElements(size_t size, const Base* array) {
    std::function<std::string(const Base)> print = [](const Base value) {
      return Wrapper(value).ToString();
    };
    return PrintElementsBase(size, array, std::move(print));
  }
};

// partial specialization for Wrappers that don't have ToString() method
// they usually are the same as Base
template <typename Base, typename Wrapper>
struct Printer<Base, Wrapper, false> {
  static std::string PrintElements(size_t size, const Base* array) {
    std::function<std::string(const Base)> print = [](const Base value) {
      return ToStringHelper(value);
    };
    return PrintElementsBase(size, array, std::move(print));
  }
};

// specialization for void* pointers (buffers)
// if size is 4 or 8 it's probably a pointer (or some number)
// format it as int which is more readable (see clSetKernelArg)
// else treat it as array of bytes
template <>
struct Printer<void, void, false> {
  static std::string PrintElements(size_t size, const void* array) {
    if (size == 4) {
      auto print = [](const int32_t value) { return hex(value).ToString(); };
      return PrintElementsBase<int32_t>(1, reinterpret_cast<const int32_t*>(array),
                                        std::move(print));
    } else if (size == 8) {
      auto print = [](const int64_t value) { return hex(value).ToString(); };
      return PrintElementsBase<int64_t>(1, reinterpret_cast<const int64_t*>(array),
                                        std::move(print));
    }
    auto print = [](const char value) { return hex((const unsigned char)value).ToString(); };
    return PrintElementsBase<const char>(size, static_cast<const char*>(array), std::move(print));
  }
};
} // namespace

#ifndef BUILD_FOR_CCODE
// In openclDriversAuto.h there is an additional argument to OCL_FUNCTION generated
// with TOclType derived wrappers

// TOclType is base type for wrappers which provide string representations
class TOclType {
public:
  virtual std::string ToString() const {
    return "<unknown type>";
  }
};

// TOclArray represents an array of elements of type Base,
// with Wrapper providing string representations
// Wrapper doesn't have to derive from TOclType
template <typename Base, typename Wrapper>
class TOclArray : public TOclType {
protected:
  const Base* value;
  size_t size;

public:
  TOclArray(size_t size, const Base* val) : value(val), size(size) {}
  TOclArray(const Base* val, size_t size) : value(val), size(size) {}

  TOclArray(const Base* val, const int terminator, const int term_pos) : value(val) {
    // 0 should be included so size + 1
    size = val ? GetTermArraySize(val, static_cast<const Base>(terminator), term_pos) + 1 : 0;
  }

  virtual std::string ToString() const {
    return Printer<Base, Wrapper>::PrintElements(size, value);
  }
};
#endif

} // namespace OpenCL
} // namespace gits
