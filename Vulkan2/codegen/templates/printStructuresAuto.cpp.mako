// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "printStructuresAuto.h"
#include "printEnumsAuto.h"
#include "printCustom.h"

namespace gits {
namespace vulkan {

%for structure in structures:
%if structure.name not in structs_with_custom_print:

<% define = get_define(structure.platform) %>\
% if define:
#ifdef ${define}
% endif

FastOStream& operator<<(FastOStream& stream, const ${structure.name}& value) {
${print_struct_members(structure)}\
  return stream;
}

FastOStream& operator<<(FastOStream& stream, const ${structure.name}* value) {
  if (value) {
    stream << *value;
  } else {
    stream << "nullptr";
  }
  return stream;
}

% if define:
#endif
% endif

%endif
%endfor

} // namespace vulkan
} // namespace gits
