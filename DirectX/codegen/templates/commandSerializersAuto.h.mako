// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "commandSerializer.h"
#include "commandsAuto.h"
#include "commandsCustom.h"
#include "commandIdsAuto.h"
#include "commandEncodersCustom.h"
#include "commandEncodersAuto.h"

namespace gits {
namespace DirectX {

%for function in functions:
class ${function.name}Serializer : public stream::CommandSerializer {
public:
  ${function.name}Serializer(const ${function.name}Command& command) {
    m_DataSize = GetSize(command);
    m_Data.reset(new char[m_DataSize]);
    Encode(command, m_Data.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_${function.name.upper()});
  }
};

%endfor
%for interface in interfaces:
%for function in interface.functions:
class ${interface.name}${function.name}Serializer : public stream::CommandSerializer {
public:
  ${interface.name}${function.name}Serializer(const ${interface.name}${function.name}Command& command) {
    m_DataSize = GetSize(command);
    m_Data.reset(new char[m_DataSize]);
    Encode(command, m_Data.get());
  }
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_${interface.name.upper()}_${function.name.upper()});
  }
};

%endfor
%endfor
} // namespace DirectX
} // namespace gits
