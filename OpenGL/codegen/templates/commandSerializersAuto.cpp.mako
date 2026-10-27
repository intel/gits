// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${AUTO_GENERATED_HEADER}

#include "commandCodersAuto.h"
#include "commandSerializersAuto.h"

namespace gits::opengl {

% for cmd in commands:
% if cmd.enabled:
${cmd.name}CommandSerializer::${cmd.name}CommandSerializer(const ${cmd.name}Command& command) {
  m_DataSize = GetSize(command);
  m_Data.reset(new char[static_cast<size_t>(m_DataSize)]);
  Encode(command, m_Data.get());
}

% endif
% endfor
} // namespace gits::opengl
