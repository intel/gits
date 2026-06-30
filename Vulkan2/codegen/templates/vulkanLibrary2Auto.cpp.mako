// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "vulkanLibrary2.h"
#include "commandPlayersAuto.h"
#include "commandPlayersCustom.h"
#include "commandIdsAuto.h"

namespace gits {
namespace vulkan {

VulkanLibrary2& VulkanLibrary2::Get() {
  return static_cast<VulkanLibrary2&>(CGits::Instance().Library(ID_VULKAN2));
}

gits::CFunction* VulkanLibrary2::FunctionCreate(unsigned type) const {
  switch (static_cast<CommandId>(type)) {
  case CommandId::ID_META_CREATE_WINDOW:
	return new CreateWindowMetaPlayer();
  case CommandId::ID_META_MAPPED_DATA:
    return new MappedDataMetaPlayer();
  % for command in commands:
  <% define = get_define(command.platform) %>\
  % if define:
  #ifdef ${define}
  % endif
  case CommandId::ID_${command.name.upper()}:
	return new ${command.name}Player();
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