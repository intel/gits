// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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
unsigned getSize(const ${function.name}Command& command) {
  return ${command_encoders_sum_sizes(function, False, '                  ')}
}

void encode(const ${function.name}Command& command, char* dest) {
  unsigned offset = 0;
  encode(dest, offset, command.key);
  encode(dest, offset, command.threadId);
  %for param in function.params:
  encode(dest, offset, command.${param.name}_);
  %endfor
  %if not function.ret.is_void:
  encode(dest, offset, command.result_);
  %endif
}

%endfor

%for interface in interfaces:
%for function in interface.functions:
unsigned getSize(const ${interface.name}${function.name}Command& command) {
  return ${command_encoders_sum_sizes(function, True, '                  ')}
}

void encode(const ${interface.name}${function.name}Command& command, char* dest) {
  unsigned offset = 0;
  encode(dest, offset, command.key);
  encode(dest, offset, command.threadId);
  encode(dest, offset, command.object_.key);
  %for param in function.params:
  encode(dest, offset, command.${param.name}_);
  %endfor
  %if not function.ret.is_void:
  encode(dest, offset, command.result_);
  %endif
}

%endfor
%endfor

} // namespace DirectX
} // namespace gits
