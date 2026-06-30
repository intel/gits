// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "commandSerializersFactory.h"
#include "commandSerializersAuto.h"
#include "commandSerializersCustom.h"

namespace gits {
namespace vulkan {

stream::CommandSerializer* CreateCommandSerializer(Command* command) {
  switch (command->GetId()) {
    case CommandId::ID_META_CREATE_WINDOW:
      return new CreateWindowMetaSerializer(*static_cast<CreateWindowMetaCommand*>(command));
    case CommandId::ID_META_MAPPED_DATA:
      return new MappedDataMetaSerializer(*static_cast<MappedDataMetaCommand*>(command));
    % for command in commands:
    <% define = get_define(command.platform) %>\
    % if define:
    #ifdef ${define}
    % endif
    case CommandId::ID_${command.name.upper()}:
      return new ${command.name}Serializer(*static_cast<${command.name}Command*>(command));
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
