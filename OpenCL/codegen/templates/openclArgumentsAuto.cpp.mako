// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "openclArgumentsAuto.h"

#include "openclHeader.h"

#include "log.h"
#include "tools.h"

namespace gits {
namespace OpenCL {
%for name, enum in enums.items():
std::string ${name}ToString(const ${enum.get('type') if enum.get('type') else name} value) {
  %if enum.get('bitfield'):
  ${enum.get('type') if enum.get('type') else name} res = 0;
  std::string text;
    %for arg in enum['vars']:
      %if arg['value'] == '0xFFFFFFFF':
  if(value == 0xFFFFFFFF) {
    return "${arg['name']}";
  }
      %else:
  if(value ${'==' if arg['value'] == '0' else '&'} static_cast<cl_bitfield>(${arg['value']})) {
    MaskAppend(text, "${arg['name']}");
    res |= static_cast<cl_bitfield>(${arg['value']});
  }
      %endif
    %endfor
  if(res != value || value == 0) {
    return std::to_string(value);
  }
  return text;
  %else:
  switch(value) {
    %for arg in enum['vars']:
  case ${arg['value']}: return "${arg['name']}";
    %endfor
  default: return ToStringHelper(reinterpret_cast<const void*>(value));
  }
  %endif
}

%endfor

%for name, enum in without_field(enums, 'custom_argument').items():
const char* C${name}::NAME = "${enum.get('type') if enum.get('type') else name}";
%endfor
%for name, arg in arguments.items():
const char* C${name}::NAME = "${name}";
%endfor
} // namespace OpenCL
} // namespace gits
