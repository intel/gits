// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include <string>
#include "vulkanHeader2.h"

namespace gits {
namespace vulkan {

%for enum in enums:
%if enum.name not in excluded_enums:
<% define = get_define(enum.platform) %>\
% if define:
#ifdef ${define}
% endif
std::string toStr(${enum.name} value);
% if define:
#endif
% endif
%endif
%endfor

} // namespace vulkan
} // namespace gits
