// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <cstdlib>
#include <memory>

namespace gits {

//Custom allocator, which uses malloc/free instead of new/delete.
template <typename T>
class malloc_allocator : public std::allocator<T> {
public:
  typename std::allocator<T>::pointer allocate(typename std::allocator<T>::size_type n,
                                               typename std::allocator<T>::const_pointer = 0) {
    return static_cast<typename std::allocator<T>::pointer>(malloc(n * sizeof(T)));
  }
  void deallocate(typename std::allocator<T>::pointer p, typename std::allocator<T>::size_type) {
    free(p);
  }
  template <typename U>
  struct rebind {
    typedef malloc_allocator<U> other;
  };
  malloc_allocator() throw() : std::allocator<T>() {}
  malloc_allocator(const malloc_allocator& object) throw() : std::allocator<T>(object) {}
  template <class U>
  malloc_allocator(const malloc_allocator<U>& object) throw() : std::allocator<T>(object) {}
  ~malloc_allocator() throw() {}
};

} // namespace gits
