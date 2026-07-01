// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "fastOStream.h"
#include "vulkanHeader2.h"

namespace gits {
namespace vulkan {

%for enum in enums:
<% define = get_define(enum.platform) %>\
% if define:
#ifdef ${define}
% endif

FastOStream& operator<<(FastOStream& stream, ${enum.name} value);
FastOStream& operator<<(FastOStream& stream, const ${enum.name}* value);

% if define:
#endif
% endif
%endfor

} // namespace vulkan
} // namespace gits
