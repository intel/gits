// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "vulkanCommandFactory.h"
#include "commandRunnersAuto.h"
#include "commandRunnersCustom.h"
#include "commandIdsAuto.h"

namespace gits {
namespace vulkan {

stream::CommandRunner* VulkanCommandFactory::CreateCommand(unsigned id) {
  switch (static_cast<CommandId>(id)) {
  case CommandId::ID_INIT_START:
    return new StateRestoreBeginRunner();
  case CommandId::ID_INIT_END:
    return new StateRestoreEndRunner();
  case CommandId::ID_META_CREATE_WINDOW:
    return new CreateWindowMetaRunner();
  case CommandId::ID_META_UPDATE_WINDOW:
    return new UpdateWindowMetaRunner();
  case CommandId::ID_META_MAPPED_DATA:
    return new MappedDataMetaRunner();
  % for command in commands:
  <% define = get_define(command.platform) %>\
  % if define:
  #ifdef ${define}
  % endif
  case CommandId::ID_${command.name.upper()}:
    return new ${command.name}Runner();
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
