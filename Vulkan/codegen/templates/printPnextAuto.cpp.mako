// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "printPnextAuto.h"
#include "printStructuresAuto.h"
#include "printStructuresCustom.h"

namespace gits {
namespace vulkan {

FastOStream& PrintPNext(FastOStream& stream, const void* pNext) {
  if (!pNext) {
    stream << "nullptr";
    return stream;
  }
  const VkBaseInStructure* base = reinterpret_cast<const VkBaseInStructure*>(pNext);
  switch (base->sType) {
%for structure in structures:
%if structure.stype_value and structure.struct_extends:
<% define = get_define(structure.platform) %>\
% if define:
#ifdef ${define}
% endif
    case ${structure.stype_value}:
      stream << *reinterpret_cast<const ${structure.name}*>(pNext);
      break;
% if define:
#endif
% endif
%endif
%endfor
    default:
      stream << "(unknown pNext sType " << static_cast<int>(base->sType) << ")";
      break;
  }
  return stream;
}

} // namespace vulkan
} // namespace gits
