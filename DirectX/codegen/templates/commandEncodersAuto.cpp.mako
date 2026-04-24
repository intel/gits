// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "commandEncodersAuto.h"
#include "argumentEncoders.h"

namespace gits {
namespace DirectX {

%for function in functions:
unsigned GetSize(const ${function.name}Command& command) {
  return ${command_encoders_sum_sizes(function, False)}
}

void Encode(const ${function.name}Command& command, char* dest) {
  unsigned offset = 0;
  Encode(dest, offset, command.Key);
  Encode(dest, offset, command.ThreadId);
  %for param in function.params:
  Encode(dest, offset, command.m_${param.name});
  %endfor
  %if not function.ret.is_void:
  Encode(dest, offset, command.m_Result);
  %endif
}

%endfor

%for interface in interfaces:
%for function in interface.functions:
unsigned GetSize(const ${interface.name}${function.name}Command& command) {
  return ${command_encoders_sum_sizes(function, True)}
}

void Encode(const ${interface.name}${function.name}Command& command, char* dest) {
  unsigned offset = 0;
  Encode(dest, offset, command.Key);
  Encode(dest, offset, command.ThreadId);
  Encode(dest, offset, command.m_Object.Key);
  %for param in function.params:
  Encode(dest, offset, command.m_${param.name});
  %endfor
  %if not function.ret.is_void:
  Encode(dest, offset, command.m_Result);
  %endif
}

%endfor
%endfor

} // namespace DirectX
} // namespace gits
