// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   version.h
 *
 * @brief Declaration of Version class.
 *
 */

#pragma once

#include <iosfwd>
#include <cstdint>
#define FFull 0xFFFFFFFFFFFFFFFF
#define GITS_MAKE_VERSION4(v0, v1, v2, v3)                                                         \
  (((v0 & FFull) << 48) | ((v1 & FFull) << 32) | ((v2 & FFull) << 16) | (v3 & FFull))
#define GITS_MAKE_VERSION3(v0, v1, v2) GITS_MAKE_VERSION4(v0, v1, v2, 0)

namespace gits {
class CBinIStream;
/**
   * @brief Class responsible for handling project version operations
   *
   * gits::CGits::CVersion is a class that is responsible for handling
   * project version operations. It stores current version, compares 2
   * version classes and writes version information in text or binary format.
   */
class CVersion {
public:
  CVersion();
  CVersion(uint64_t version);
  CVersion(unsigned short v0, unsigned short v1, unsigned short v2, unsigned short v3);

  uint64_t version() const {
    return _version;
  }

private:
  uint64_t _version;
};

bool operator<(const CVersion& lhs, const CVersion& rhs);
std::ostream& operator<<(std::ostream& stream, const CVersion& version);
CBinIStream& operator>>(CBinIStream& stream, CVersion& version);
} // namespace gits
