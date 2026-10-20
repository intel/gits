// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${AUTO_GENERATED_HEADER}

// Thin stream::CommandSerializer per enabled command: size+encode into a single owned buffer
// at construction time, using the GetSize/Encode pair from commandCodersAuto.h. Mirrors
// Vulkan/codegen/templates/commandSerializersAuto.h.mako.
//
// Constructors are defined in commandSerializersAuto.cpp: each one calls GetSize/Encode, whose
// overload sets span every command, so defining them inline would make every includer pay for
// resolving thousands of calls against thousands of overloads.

#pragma once

#include "commandCodersAuto.h"
#include "commandSerializer.h"
#include "commandsAuto.h"

namespace gits::opengl2 {

% for cmd in commands:
% if cmd.enabled:
class ${cmd.name}CommandSerializer : public stream::CommandSerializer {
public:
  explicit ${cmd.name}CommandSerializer(const ${cmd.name}Command& command);

  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_${cmd.name.upper()});
  }
};

% endif
% endfor
} // namespace gits::opengl2
