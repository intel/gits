// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "commandWritersFactory.h"
#include "commandWritersAuto.h"
#include "commandWritersCustom.h"

namespace gits {
namespace vulkan {

CommandWriter* CreateCommandWriter(Command* command) {
  switch (command->GetId()) {
    case CommandId::ID_META_CREATE_WINDOW:
      return new CreateWindowMetaWriter(*static_cast<CreateWindowMetaCommand*>(command));
    % for command in commands:
    <% define = get_define(command.platform) %>\
    % if define:
    #ifdef ${define}
    % endif
    case CommandId::ID_${command.name.upper()}:
      return new ${command.name}Writer(*static_cast<${command.name}Command*>(command));
    % if define:
    #endif
    % endif
    % endfor
    default:
      return nullptr;
  }
}

} // namespace vulkan
} // namespace gits
