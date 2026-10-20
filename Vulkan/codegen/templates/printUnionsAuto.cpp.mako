// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "printUnionsAuto.h"
#include "printCustom.h"
#include "printGeneric.h"

namespace gits {
namespace vulkan {

%for union in unions:
<% define = get_define(union.platform) %>\
% if define:
#ifdef ${define}
% endif

FastOStream& Print${union.name}(FastOStream& stream, const ${union.name}& value) {
${print_union_members(union, bitmasks)}\
  return stream;
}

FastOStream& Print${union.name}(FastOStream& stream, const ${union.name}* value) {
  if (value) {
    return Print${union.name}(stream, *value);
  }
  return PrintText(stream, "nullptr");
}

FastOStream& operator<<(FastOStream& stream, const ${union.name}& value) {
  return Print${union.name}(stream, value);
}

FastOStream& operator<<(FastOStream& stream, const ${union.name}* value) {
  return Print${union.name}(stream, value);
}

% if define:
#endif
% endif
%endfor

} // namespace vulkan
} // namespace gits
