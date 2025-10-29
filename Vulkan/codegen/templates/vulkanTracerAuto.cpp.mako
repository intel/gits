// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

${AUTO_GENERATED_HEADER}

#include "vulkanLog.h"

namespace gits {

% for token in vk_functions:
<%
    params: str = args_to_str(token.args, 'const {type} {name}{array}, ', ', ').replace('const const ', 'const ').replace('**', '* const *')
    log: str = make_token_log_code(token.args)
%>\
  void ${token.name}_trace(${params}) {
    using namespace gits::Vulkan;
    LOG_FORMAT_RAW
    LOG_TRACE << ${log};
  };

% endfor
} // namespace gits
