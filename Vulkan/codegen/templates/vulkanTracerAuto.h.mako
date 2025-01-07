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
    VkLog(TRACE, NO_PREFIX) << " = " << r;
  }

  template<>
  void trace_return_value<void_t>(void_t) {
    VkLog(TRACE, NO_PREFIX) << "";
  }

% for token in vk_functions:
<%
    params: str = args_to_str(token.args, '{type} {name}{array}, ', ', ')
%>\
  void ${token.name}_trace(${params});
% endfor
} // namespace gits
