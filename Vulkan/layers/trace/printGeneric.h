// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "fastOStream.h"

#include <cstddef>

namespace gits {
namespace vulkan {

// Print through the generic FastOStream operator<< directly. An unqualified stream << inside
// gits::vulkan is resolved against every operator<< generated for Vulkan types, which is slow to
// compile, so generated printers use these for literals and for values which only the generic
// operator<< prints.
template <size_t N>
FastOStream& PrintText(FastOStream& stream, const char (&text)[N]) {
  return gits::operator<<(stream, text);
}

template <typename T>
FastOStream& PrintValue(FastOStream& stream, const T& value) {
  return gits::operator<<(stream, value);
}

} // namespace vulkan
} // namespace gits
