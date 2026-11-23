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

// Generated printers call the uniquely named Print<Type> functions instead of operator<<, which
// would be resolved against every operator<< overload below at each call.
%for structure in structures:
%if structure.name not in structs_with_custom_print:

<% define = get_define(structure.platform) %>\
% if define:
#ifdef ${define}
% endif
FastOStream& Print${structure.name}(FastOStream& stream, const ${structure.name}& value);
FastOStream& Print${structure.name}(FastOStream& stream, const ${structure.name}* value);
FastOStream& operator<<(FastOStream& stream, const ${structure.name}& value);
FastOStream& operator<<(FastOStream& stream, const ${structure.name}* value);
% if define:
#endif
% endif

%endif
%endfor

} // namespace vulkan
} // namespace gits
