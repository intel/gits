// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
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
#include <string>
#define FFull 0xFFFFFFFFFFFFFFFF
#define GITS_MAKE_VERSION4(v0, v1, v2, v3)                                                         \
  (((v0 & FFull) << 48) | ((v1 & FFull) << 32) | ((v2 & FFull) << 16) | (v3 & FFull))
#define GITS_MAKE_VERSION3(v0, v1, v2) GITS_MAKE_VERSION4(v0, v1, v2, 0)
// SemVer versions (major 3 and above) store PATCH in the last component; the third is always 0.
#define GITS_MAKE_SEMVER(major, minor, patch) GITS_MAKE_VERSION4(major, minor, 0, patch)

namespace gits {
/**
   * @brief Class responsible for handling project version operations
   *
   * gits::CGits::CVersion is a class that is responsible for handling
   * project version operations. It stores current version, compares 2
   * version classes and writes version information in text or binary format.
   *
   * Streams store the version as the fixed-width label GITS_dd.dd.dd.ddd, which every GITS
   * version can read. Versions before 3.0 use all four components; SemVer versions map
   * MAJOR.MINOR.PATCH to GITS_MAJOR.MINOR.00.PATCH.
   */
class CVersion {
public:
  CVersion();
  CVersion(uint64_t version);
  CVersion(unsigned short v0, unsigned short v1, unsigned short v2, unsigned short v3);

  uint64_t version() const {
    return _version;
  }

  bool IsLegacy() const;
  unsigned short Major() const;
  unsigned short Minor() const;
  // Only meaningful when !IsLegacy().
  unsigned short Patch() const;
  bool IsDevelopment() const;

  // SemVer text such as 3.0.123 or 3.0.999-dev, or the GITS_dd.dd.dd.ddd label of legacy versions.
  std::string ToString() const;

private:
  uint64_t _version;
};

bool operator<(const CVersion& lhs, const CVersion& rhs);
// Writes the GITS_dd.dd.dd.ddd label stored in streams; use ToString() for display.
std::ostream& operator<<(std::ostream& stream, const CVersion& version);
std::istream& operator>>(std::istream& stream, CVersion& version);

// Version of this GITS build.
const CVersion& CurrentVersion();
// Full SemVer version of this GITS build, with build metadata for development builds, e.g.
// 3.0.999-dev+pr-123.sha-abc1234.
const char* GetVersionString();
const char* GetBuildCommit();
} // namespace gits
