// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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

#include <algorithm>
#include <list>
#include <stdexcept>
#include <string>
#include <vector>
#include <cstdint>

#ifndef INIT_NEW_STATIC_OBJ
// INIT_NEW_STATIC_OBJ creates a static object that won't be destroyed till the end of the application.
#define INIT_NEW_STATIC_OBJ(obj, type) static type& obj = *(new type);
#endif

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

int log2i(int value);

// Returns a string containing a hexadecimal representation of given bytes.
std::string bytesToHex(const uint8_t* const bytes, const size_t length);

void ReverseByPairs(std::string& str);

bool caseInsensitiveEquals(const std::string& a, const std::string& b);

// Returns a lowercase copy of a given string.
std::string ToLowerCopy(const std::string& s);

// Returns a CCode variable name based on a pointer and an optional prefix.
const std::string getVarName(const std::string prefix, const void* ptr);
const std::string getVarName(const void* ptr);

std::istream& uniGetLine(std::istream& is, std::string& line);

void sleep_millisec(int duration);

std::vector<char> GetBinaryFileContents(const std::string& filename);
void SaveBinaryFileContents(const std::string& filename, const std::vector<char>& data);
bool StringEndsWith(const std::string& name, const std::string& suffix);
/**
   * @brief Clears STL sequence container
   *
   * Deletes all pointers and clears STL sequence container.
   */
template <class Seq>
void Purge(Seq& container) {
  for (auto ptr : container) {
    delete ptr;
  }
  container.clear();
}

template <typename T>
const typename T::value_type::second_type& get(const T& m, typename T::value_type::first_type& k) {
  typename T::const_iterator iter = m.find(k);
  if (iter == m.end()) {
    throw std::runtime_error("key not found in map");
  }
  return iter->second;
}

class noncopyable {
  // Disallow copying. (Note: it can still be moved!)
  noncopyable(const noncopyable&) = delete;
  noncopyable& operator=(const noncopyable&) = delete;

  // Ensure it's only usable as a base class.
protected:
  noncopyable() = default;
  ~noncopyable() = default;
};

/**
   * @brief A helper class to make printing hex values easier and consistent.
   */
class hex {
public:
  template <typename T>
  explicit hex(T value, size_t width = sizeof(T) * 2) : _value((uintptr_t)value), _width(width) {}

  void Write(std::ostream& stream) const;
  std::string ToString() const;

private:
  uintptr_t _value;
  size_t _width;
};
std::ostream& operator<<(std::ostream& stream, const hex& h);

// This PtrConverter is for the basic conversion between a type and a type pointer. Some classes
// will have their own specialized PtrConverters with more conversions or more complicated ones.
template <typename T>
class PtrConverter {
  T* _ptr;

public:
  explicit PtrConverter(T* ptr) : _ptr(ptr) {}
  operator T*() const {
    return _ptr;
  }
  operator T() const {
    return *_ptr;
  }
};

template <size_t SizeAlignment, typename T>
constexpr T Align(T value) {
  const auto ret = (static_cast<size_t>(value) + SizeAlignment - 1) & (~(SizeAlignment - 1));
  return static_cast<T>(ret);
}
namespace alignment {
constexpr size_t pageSize1KB = 1024U;
constexpr size_t pageSize1MB = pageSize1KB * 1024U;
constexpr size_t pageSize4KB = pageSize1KB * 4U;
constexpr size_t pageSize64KB = pageSize1KB * 64U;
constexpr size_t pageSize2MB = pageSize1MB * 2U;
} // namespace alignment

} // namespace gits
