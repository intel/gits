// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "commandCodersAuto.h"
#include "argumentCodersAuto.h"

namespace gits {
namespace vulkan {
% for command in commands:
<% define = get_define(command.platform) %>\
% if define:
#ifdef ${define}
% endif
uint32_t GetSize(const ${command.name}Command& command) {
  return GetSize(command.m_Key) +
         GetSize(command.m_ThreadId) +
         % for param in command.params:
         GetSize(command.m_${param.name})${' + ' if not loop.last else ''}
         % endfor
         % if command.return_type != 'void':
          + GetSize(command.m_Return)
         % endif
         ;
}

void Encode(const ${command.name}Command& command, char* dest) {
  uint32_t offset = 0;
  Encode(dest, offset, command.m_Key);
  Encode(dest, offset, command.m_ThreadId);
  % for param in command.params:
  Encode(dest, offset, command.m_${param.name});
  % endfor
  % if command.return_type != 'void':
  Encode(dest, offset, command.m_Return);
  % endif
}

void Decode(char* src, ${command.name}Command& command) {
  uint32_t offset = 0;
  Decode(src, offset, command.m_Key);
  Decode(src, offset, command.m_ThreadId);
  % for param in command.params:
  Decode(src, offset, command.m_${param.name});
  % endfor
  % if command.return_type != 'void':
  Decode(src, offset, command.m_Return);
  % endif
}
% if define:
#endif
% endif

% endfor
} // namespace vulkan
} // namespace gits
