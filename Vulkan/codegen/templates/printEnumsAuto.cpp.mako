// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "printEnumsAuto.h"
#include "enumToStrAuto.h"
#include "printGeneric.h"

namespace gits {
namespace vulkan {

%for enum in enums:
<% define = get_define(enum.platform) %>\
% if define:
#ifdef ${define}
% endif

FastOStream& Print${enum.name}(FastOStream& stream, ${enum.name} value) {
  return PrintValue(stream, toStr(value));
}

FastOStream& Print${enum.name}(FastOStream& stream, const ${enum.name}* value) {
  if (value) {
    return Print${enum.name}(stream, *value);
  }
  return PrintText(stream, "nullptr");
}

FastOStream& operator<<(FastOStream& stream, ${enum.name} value) {
  return Print${enum.name}(stream, value);
}

FastOStream& operator<<(FastOStream& stream, const ${enum.name}* value) {
  return Print${enum.name}(stream, value);
}

% if define:
#endif
% endif
%endfor

} // namespace vulkan
} // namespace gits
