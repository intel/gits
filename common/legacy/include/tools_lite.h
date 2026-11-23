// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   tools_lite.h
 *
 * @brief Common dependencyless utilities for GITS project.
 *
 * This file contains various tools and utilities that do not require including
 * headers other than the standard library. This is in contrast to the regular
 * tools.h, which does not have this constraint.
 */

#pragma once

#include <string>
#include <cstdint>

/**
 * @brief Macro for one time initialization.
 *
 * Use by passing it a lambda doing actual work.
 */
#define COMBINE1(X, Y) X##Y
#define COMBINE(X, Y)  COMBINE1(X, Y)
#define CALL_ONCE      call_once_impl COMBINE(call_once_instance, __LINE__) =
struct call_once_impl {
  template <class T>
  call_once_impl(const T& callable) {
    static bool called;
    if (!called) {
      called = true;
      callable();
    }
  }
  // Empty dtor suppresses unused var warning for GCC.
  ~call_once_impl() {}

private:
  // Function pointer won't provide unique instantiation of ctor
  // (and thus static call flag, so they are forbidden).
  template <class T>
  call_once_impl(T* callable) = delete;
};

namespace gits {

// Returns a string containing a hexadecimal representation of given bytes.
std::string bytesToHex(const uint8_t* const bytes, const size_t length);

void ReverseByPairs(std::string& str);

// C++20 u8string uses char8_t which is incompatible with regular char/std::string.
// This is a helper function for easy conversion
std::string to_string(const std::u8string& u8str);

class noncopyable {
  // Disallow copying. (Note: it can still be moved!)
  noncopyable(const noncopyable&) = delete;
  noncopyable& operator=(const noncopyable&) = delete;

  // Ensure it's only usable as a base class.
protected:
  noncopyable() = default;
  ~noncopyable() = default;
};

} // namespace gits
