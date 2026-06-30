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

namespace gits {
namespace vulkan {

%for union in unions:
<% define = get_define(union.platform) %>\
% if define:
#ifdef ${define}
% endif

FastOStream& operator<<(FastOStream& stream, const ${union.name}& value) {
${print_union_members(union)}\
  return stream;
}

FastOStream& operator<<(FastOStream& stream, const ${union.name}* value) {
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
%endfor

} // namespace vulkan
} // namespace gits
