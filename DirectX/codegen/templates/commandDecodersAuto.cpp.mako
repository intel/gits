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
void Decode(char* src, ${function.name}Command& command) {
  unsigned offset = 0;
  Decode(src, offset, command.Key);
  Decode(src, offset, command.ThreadId);
  %for param in function.params:
  Decode(src, offset, command.m_${param.name});
  %endfor
  %if not function.ret.is_void:
  Decode(src, offset, command.m_Result);
  %endif
}

%endfor
%for interface in interfaces:
%for function in interface.functions:
void Decode(char* src, ${interface.name}${function.name}Command& command) {
  unsigned offset = 0;
  Decode(src, offset, command.Key);
  Decode(src, offset, command.ThreadId);
  Decode(src, offset, command.m_Object.Key);
  %for param in function.params:
  Decode(src, offset, command.m_${param.name});
  %endfor
  %if not function.ret.is_void:
  Decode(src, offset, command.m_Result);
  %endif
}

%endfor
%endfor
} // namespace DirectX
} // namespace gits
