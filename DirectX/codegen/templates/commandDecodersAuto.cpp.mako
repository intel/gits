// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "commandDecodersAuto.h"
#include "commandsAuto.h"
#include "argumentDecoders.h"

#include "streams.h"

namespace gits {
namespace DirectX {

%for function in functions:
void decode(char* src, ${function.name}Command& command) {
  unsigned offset = 0;
  decode(src, offset, command.key);
  decode(src, offset, command.threadId);
  %for param in function.params:
  decode(src, offset, command.${param.name}_);
  %endfor
  %if not function.ret.is_void:
  decode(src, offset, command.result_);
  %endif
}

%endfor
%for interface in interfaces:
%for function in interface.functions:
void decode(char* src, ${interface.name}${function.name}Command& command) {
  unsigned offset = 0;
  decode(src, offset, command.key);
  decode(src, offset, command.threadId);
  decode(src, offset, command.object_.key);
  %for param in function.params:
  decode(src, offset, command.${param.name}_);
  %endfor
  %if not function.ret.is_void:
  decode(src, offset, command.result_);
  %endif
}

%endfor
%endfor
} // namespace DirectX
} // namespace gits
