// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <ostream>
#include <istream>
#include <unordered_map>
#include "config.h"
#include "streams.h"

namespace gits {

struct CArgument {};
struct CCodeOStream : std::ostream {};
enum MappedArrayAction {
  ADD_MAPPING = 0,
  REMOVE_MAPPING = 1,
  NO_ACTION = 2
};

namespace OpenGL {

template <class T, class U>
struct CArgumentSizedArray {};
template <class T, class U, MappedArrayAction T_ACTION>
struct CArgumentMappedSizedArray {};
} // namespace OpenGL

} // namespace gits
