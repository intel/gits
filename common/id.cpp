// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   id.cpp
 *
 * @brief Definition of a class that handles entity identifiers.
 *
 */

#include "id.h"
#include "buffer.h"
#include "streams.h"

namespace gits {

CId::CId() : _id(0) {}

CId::CId(uint32_t id) : _id(id) {}

void CId::Write(CBinOStream& stream) const {
  uint16_t subid = (uint16_t)(_id & 0xFFFF);
  char type = (char)(_id >> 16 & 0xFF);
  write_to_stream(stream, type);
  write_to_stream(stream, subid);
}

void CId::Read(CBinIStream& stream) {
  // streams has written:
  // uint8_t -> api type
  // uint16_t -> token subid
  uint8_t id[3];
  read_from_stream(stream, id);
  _id = id[0] << 16 | id[2] << 8 | id[1] << 0;
}

} // namespace gits
