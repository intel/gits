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

namespace gits {
namespace DirectX {

%for function in functions:
class ${function.name}Serializer : public stream::CommandSerializer {
public:
  explicit ${function.name}Serializer(const ${function.name}Command& command);
  unsigned Id() const override;
};

%endfor
%for interface in interfaces:
%for function in interface.functions:
class ${interface.name}${function.name}Serializer : public stream::CommandSerializer {
public:
  explicit ${interface.name}${function.name}Serializer(const ${interface.name}${function.name}Command& command);
  unsigned Id() const override;
};

%endfor
%endfor
} // namespace DirectX
} // namespace gits
