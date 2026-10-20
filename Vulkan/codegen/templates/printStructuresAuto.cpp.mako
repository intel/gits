// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "printStructuresAuto.h"
#include "printPnextAuto.h"
#include "printBitmasksAuto.h"
#include "printEnumsAuto.h"
#include "printCustom.h"
#include "printGeneric.h"

namespace gits {
namespace vulkan {

%for structure in structures:
%if structure.name not in structs_with_custom_print:

<% define = get_define(structure.platform) %>\
% if define:
#ifdef ${define}
% endif

FastOStream& Print${structure.name}(FastOStream& stream, const ${structure.name}& value) {
${print_struct_members(structure, bitmasks)}\
  return stream;
}

FastOStream& Print${structure.name}(FastOStream& stream, const ${structure.name}* value) {
  if (value) {
    return Print${structure.name}(stream, *value);
  }
  return PrintText(stream, "nullptr");
}

FastOStream& operator<<(FastOStream& stream, const ${structure.name}& value) {
  return Print${structure.name}(stream, value);
}

FastOStream& operator<<(FastOStream& stream, const ${structure.name}* value) {
  return Print${structure.name}(stream, value);
}

% if define:
#endif
% endif

%endif
%endfor

} // namespace vulkan
} // namespace gits
