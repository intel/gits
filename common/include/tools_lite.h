// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
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

void MaskAppend(std::string& str, const std::string& maskStr);

bool caseInsensitiveEquals(const std::string& a, const std::string& b);

// Returns a lowercase copy of a given string.
std::string ToLowerCopy(const std::string& s);

// Returns a CCode variable name based on a pointer and an optional prefix.
const std::string getVarName(const std::string prefix, const void* ptr);
const std::string getVarName(const void* ptr);

std::istream& uniGetLine(std::istream& is, std::string& line);

// Returns number of elements of an array.
template <class T, size_t S>
size_t size(const T (&)[S]) {
  return S;
}

void sleep_millisec(int duration);

// Returns a number representing a given pointer.
uint32_t MapPointer(void* ptr);

std::vector<char> GetBinaryFileContents(const std::string& filename);
void SaveBinaryFileContents(const std::string& filename, const std::vector<char>& data);

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

template <typename T, typename F>
auto FindIf(T&& container, F&& predicate) -> decltype(begin(container)) {
  return std::find_if(begin(container), end(container), std::forward<F>(predicate));
}
#define FIND_ELEM(container, ...)                                                                  \
  FindIf(container, [&](decltype(*begin(container)) elem) __VA_ARGS__)

template <typename T>
const typename T::value_type::second_type& get(const T& m, typename T::value_type::first_type& k) {
  typename T::const_iterator iter = m.find(k);
  if (iter == m.end()) {
    throw std::runtime_error("key not found in map");
  }
  return iter->second;
}

template <class T>
T value_at_offset(const void* memory, size_t offset) {
  const void* pointer = static_cast<const char*>(memory) + offset;
  return *static_cast<const T*>(pointer);
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

template <class T>
class IncrementInScope {
  T& _var;

public:
  IncrementInScope(T& var) : _var(var) {
    _var++;
  }
  ~IncrementInScope() {
    _var--;
  }
};
#define INCREMENT_IN_SCOPE(x) IncrementInScope<decltype(x)> incrementInScope(x);

// Evaluates an average value from n recently inserted values.
template <class T>
class AvgFromRecentValues {
  std::list<T> _values;
  unsigned _limit; // Limit for values to evaluate average from.
public:
  AvgFromRecentValues(unsigned limit) : _limit(limit) {}
  void Insert(T val) {
    _values.push_front(val);
    if (_values.size() > _limit) {
      _values.pop_back();
    }
  }
  T Result() {
    auto size = _values.size();
    if (size > 0) {
      T total{};
      for (auto& value : _values) {
        total = total + value;
      }
      return total / size;
    } else {
      return T{};
    }
  }
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
} // namespace gits
