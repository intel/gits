// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

${AUTO_GENERATED_HEADER}

#pragma once

#include "vulkanLog.h"
#include "vulkanDrivers.h"

namespace gits {

  template<class T>
  void trace_return_value(T r) {
    using namespace Vulkan;
    LOG_FORMAT_RAW
    LOG_TRACE << " = " << ToStr(r) << "\n";
  }

  template<>
  void trace_return_value<void_t>(void_t) {
    LOG_FORMAT_RAW
    LOG_TRACE << "\n";
  }

% for token in vk_functions:
<%
    params: str = args_to_str(token.args, 'const {type} {name}{array}, ', ', ').replace('const const ', 'const ').replace('**', '* const *')
%>\
  void ${token.name}_trace(${params});
% endfor
} // namespace gits
