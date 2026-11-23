// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "commandCodersAuto.h"
#include "commandSerializersAuto.h"

namespace gits {
namespace vulkan {

% for command in commands:
<% define = get_define(command.platform) %>\
% if define:
#ifdef ${define}
% endif
${command.name}Serializer::${command.name}Serializer(const ${command.name}Command& command) {
  m_DataSize = GetSize(command);
  m_Data.reset(new char[m_DataSize]);
  Encode(command, m_Data.get());
}
% if define:
#endif
% endif

% endfor
} // namespace vulkan
} // namespace gits
