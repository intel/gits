// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
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
#include "exception.h"
#include "gitsVersion.h"
#include "gitsBuildInfo.h"

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
const unsigned short first_semver_major = 3;
void enforce_version(uint64_t ver) {
  Alias64w16 v;
  v.v64 = ver;
  for (size_t i = 0; i < ver_comps; ++i) {
    if (v.v16[ver_comps - i - 1] >= pow(10, format[i])) {
      throw std::runtime_error("invalid version value");
    }
  }
  if (v.v16[3] >= first_semver_major && v.v16[1] != 0) {
    throw std::runtime_error("invalid version value: third component of SemVer versions must be 0");
  }
}

static_assert(GITS_VERSION_MAJOR >= first_semver_major);
static_assert(GITS_VERSION_PATCH <= GITS_VERSION_PATCH_DEVELOPMENT);
static_assert((GITS_VERSION_PATCH == GITS_VERSION_PATCH_DEVELOPMENT) == (GITS_BUILD_ID[0] != '\0'),
              "Only development builds have build metadata.");
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

bool CVersion::IsLegacy() const {
  return Major() < first_semver_major || ((_version >> 16) & 0xFFFF) != 0;
}

unsigned short CVersion::Major() const {
  return static_cast<unsigned short>(_version >> 48);
}

unsigned short CVersion::Minor() const {
  return static_cast<unsigned short>(_version >> 32);
}

unsigned short CVersion::Patch() const {
  return static_cast<unsigned short>(_version);
}

bool CVersion::IsDevelopment() const {
  return !IsLegacy() && Patch() == GITS_VERSION_PATCH_DEVELOPMENT;
}

std::string CVersion::ToString() const {
  std::ostringstream text;
  if (IsLegacy()) {
    text << *this;
  } else {
    text << Major() << '.' << Minor() << '.' << Patch();
    if (IsDevelopment()) {
      text << "-dev";
    }
  }
  return text.str();
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

std::istream& operator>>(std::istream& input_stream, CVersion& version) {
  char magic_r[sizeof(magic) - 1]; //magic contains null terminator
  if (!input_stream.read(magic_r, sizeof(magic_r)) ||
      !std::equal(magic_r, magic_r + sizeof(magic_r), magic)) {
    throw std::runtime_error("unrecognized version format");
  }

  Alias64w16 v;
  for (size_t i = 0; i < ver_comps; i++) {
    std::vector<char> chunk(format[i] + 1);
    if (!input_stream.read(&chunk[0], format[i])) {
      throw std::runtime_error("unrecognized version format");
    }

    std::stringstream stream(&chunk[0]);
    uint16_t value;
    stream >> value;
    v.v16[ver_comps - i - 1] = value;

    if (i != ver_comps - 1) {
      char dot;
      if (!input_stream.read(&dot, 1) || dot != '.') {
        throw std::runtime_error("unrecognized version format");
      }
    }
  }

  version = CVersion(v.v64);
  return input_stream;
}

const CVersion& CurrentVersion() {
  static const CVersion version(GITS_VERSION_MAJOR, GITS_VERSION_MINOR, 0, GITS_VERSION_PATCH);
  return version;
}

const char* GetVersionString() {
  static const std::string text = [] {
    std::string version = CurrentVersion().ToString();
    if (GITS_BUILD_ID[0] != '\0') {
      version += std::string("+") + GITS_BUILD_ID;
    }
    return version;
  }();
  return text.c_str();
}

const char* GetBuildCommit() {
  return GITS_BUILD_COMMIT;
}

} // namespace gits
