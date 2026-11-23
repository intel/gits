// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "commandSerializersAuto.h"
#include "commandIdsAuto.h"
#include "commandEncodersAuto.h"

namespace gits {
namespace DirectX {

%for function in functions:
${function.name}Serializer::${function.name}Serializer(const ${function.name}Command& command) {
  m_DataSize = GetSize(command);
  m_Data.reset(new char[m_DataSize]);
  Encode(command, m_Data.get());
}

unsigned ${function.name}Serializer::Id() const {
  return static_cast<unsigned>(CommandId::ID_${function.name.upper()});
}

%endfor
%for interface in interfaces:
%for function in interface.functions:
${interface.name}${function.name}Serializer::${interface.name}${function.name}Serializer(
    const ${interface.name}${function.name}Command& command) {
  m_DataSize = GetSize(command);
  m_Data.reset(new char[m_DataSize]);
  Encode(command, m_Data.get());
}

unsigned ${interface.name}${function.name}Serializer::Id() const {
  return static_cast<unsigned>(CommandId::ID_${interface.name.upper()}_${function.name.upper()});
}

%endfor
%endfor
} // namespace DirectX
} // namespace gits
