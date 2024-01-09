// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   buffer.cpp
 *
 * @brief Definition of a class that handles binary file operations.
 *
 */

#include "buffer.h"
#include "exception.h"
#include "streams.h"
#include "config.h"

namespace gits {
void CBuffer::initialize(const void* buffer, size_t length, bool is_mutable) {
  _buffer = const_cast<char*>(static_cast<const char*>(buffer));
  _length = length;
  _mutable = is_mutable;
}

CBuffer::CBuffer(void* buffer, size_t length) {
  initialize(buffer, length, true);
}

CBuffer::CBuffer(const void* buffer, size_t length) {
  initialize(buffer, length, false);
}

CBinOStream& operator<<(CBinOStream& stream, const CBuffer& buffer) {
  if (!Config::Get().recorder.extras.utilities.nullIO) {
    stream.write(buffer._buffer, buffer._length);
  }
  return stream;
}

CBinIStream& operator>>(CBinIStream& stream, const CBuffer& buffer) {
  if (!buffer._mutable) {
    throw std::invalid_argument((std::string)EXCEPTION_MESSAGE +
                                "cannot read data into immutable CBuffer");
  }

  stream.read(buffer._buffer, buffer._length);
  return stream;
}
} //namespace gits
