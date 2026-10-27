// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${AUTO_GENERATED_HEADER}

// Generated from codegen/pname_value_counts.py's MULTI_VALUE_PNAMES.

#include "pixelTransferSize.h"

namespace gits::opengl {

uint32_t ComputePnameValueCount(GLenum pname) {
  switch (pname) {
% for value, count, names in cases:
  case ${value}: // ${', '.join(names)}
    return ${count};
% endfor
  default:
    return 1;
  }
}

} // namespace gits::opengl
