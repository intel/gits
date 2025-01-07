// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   gits.cpp
 *
 * @brief Definition of version class.
 *
 */

#include "version.h"
#include "streams.h"
#include "exception.h"

#include <iomanip>
#include <algorithm>
#include <vector>
#include <iterator>
#include <ostream>
#include <istream>
#include <sstream>
#include <cmath>

namespace gits {

namespace {
union Alias64w16 {
  uint64_t v64;
  uint16_t v16[4];
};
const char magic[] = "GITS_";
const int format[] = {2, 2, 2, 3};
const size_t ver_comps = 4;
void enforce_version(uint64_t ver) {
  Alias64w16 v;
  v.v64 = ver;
  for (size_t i = 0; i < ver_comps; ++i) {
    if (v.v16[ver_comps - i - 1] >= pow(10, format[i])) {
      throw std::runtime_error("invalid version value");
    }
  }
}
} // namespace

CVersion::CVersion() : _version(0) {}

CVersion::CVersion(uint64_t version) : _version(version) {}

CVersion::CVersion(unsigned short v0, unsigned short v1, unsigned short v2, unsigned short v3) {
  Alias64w16 v;
  v.v16[3] = v0;
  v.v16[2] = v1;
  v.v16[1] = v2;
  v.v16[0] = v3;
  _version = v.v64;
  enforce_version(_version);
}

bool operator<(const CVersion& lhs, const CVersion& rhs) {
  return lhs.version() < rhs.version();
}

std::ostream& operator<<(std::ostream& output_stream, const CVersion& version) {
  Alias64w16 v;
  v.v64 = version.version();

  output_stream << magic;
  for (size_t i = 0; i < ver_comps; i++) {
    output_stream << std::setfill('0') << std::setw(format[i]) << v.v16[ver_comps - i - 1];
    if (i != ver_comps - 1) {
      output_stream << ".";
    }
  }

  return output_stream;
}

CBinIStream& operator>>(CBinIStream& input_stream, CVersion& version) {
  char magic_r[sizeof(magic) - 1]; //magic contains null terminator
  input_stream.ReadHelper(magic_r, sizeof(magic_r));
  if (!std::equal(magic_r, magic_r + sizeof(magic_r), magic)) {
    throw std::runtime_error("unrecognized version format");
  }

  Alias64w16 v;
  for (size_t i = 0; i < ver_comps; i++) {
    std::vector<char> chunk(format[i] + 1);
    input_stream.ReadHelper(&chunk[0], format[i]);

    std::stringstream stream(&chunk[0]);
    uint16_t value;
    stream >> value;
    v.v16[ver_comps - i - 1] = value;

    if (i != ver_comps - 1) {
      char dot;
      input_stream.ReadHelper(&dot, 1);
      if (dot != '.') {
        throw std::runtime_error("unrecognized version format");
      }
    }
  }

  version = CVersion(v.v64);
  return input_stream;
}

} // namespace gits
