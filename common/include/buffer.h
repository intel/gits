// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   buffer.h
 *
 * @brief Declaration of a class that handles binary file operations.
 *
 */

#pragma once

#include "tools.h"

namespace gits {
class CBinOStream;
class CBinIStream;

/**
   * @brief Class that is responsible for binary file operations
   *
   * gits::CBuffer class is responsible for handling binary file
   * write/read operations. Sequential bytes are ordered in opposite
   * byte order than the network byte order. That byte order was
   * selected because it is the memory order of most platforms that
   * run graphics applications.
   */
class CBuffer : private gits::noncopyable {
  char* _buffer;  /**< @brief buffer to use */
  size_t _length; /**< @brief size of the buffer */
  bool _mutable;

public:
  CBuffer(void* buffer, size_t length);
  CBuffer(const void* buffer, size_t length);

  template <typename T>
  explicit CBuffer(T& t) {
    initialize(&t, sizeof(t), true);
  }
  template <typename T>
  explicit CBuffer(const T& t) {
    initialize(&t, sizeof(t), false);
  }

  friend CBinOStream& operator<<(CBinOStream& stream, const CBuffer& buffer);
  friend CBinIStream& operator>>(CBinIStream& stream, const CBuffer& buffer);

private:
  void initialize(const void* buffer, size_t lenght, bool is_mutable);
};

CBinOStream& operator<<(CBinOStream& stream, const CBuffer& buffer);
CBinIStream& operator>>(CBinIStream& stream, const CBuffer& buffer);
} // namespace gits
