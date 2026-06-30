// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "vulkanHeader.h"

namespace gits {
namespace vulkan {

struct VkGlobalLevelDispatchTable {
% for command in commands:
% if command.dispatch_level == 'global':
<% define = get_define(command.platform) %>\
% if define:
#ifdef ${define}
% endif
  PFN_${command.name} ${command.name};
% if define:
#endif
% endif
% endif
% endfor
};

struct VkInstanceLevelDispatchTable {
  VkInstance instance;
% for command in commands:
% if command.dispatch_level == 'instance':
<% define = get_define(command.platform) %>\
% if define:
#ifdef ${define}
% endif
  PFN_${command.name} ${command.name};
% if define:
#endif
% endif
% endif
% endfor
};

struct VkDeviceLevelDispatchTable {
  VkDevice device;
% for command in commands:
% if command.dispatch_level == 'device':
<% define = get_define(command.platform) %>\
% if define:
#ifdef ${define}
% endif
  PFN_${command.name} ${command.name};
% if define:
#endif
% endif
% endif
% endfor
};

static void LoadGlobalLevelFunctions(PFN_vkGetInstanceProcAddr getProcAddr, VkGlobalLevelDispatchTable& dispatchTable) {
  % for command in commands:
  % if command.dispatch_level == 'global':
  <% define = get_define(command.platform) %>\
  % if define:
#ifdef ${define}
  % endif
  dispatchTable.${command.name} = reinterpret_cast<PFN_${command.name}>(getProcAddr(nullptr, "${command.name}"));
  % if define:
#endif
  % endif
  % endif
  % endfor
}

static void LoadInstanceLevelFunctions(PFN_vkGetInstanceProcAddr getProcAddr, VkInstance instance, VkInstanceLevelDispatchTable& dispatchTable) {
  dispatchTable.instance = instance;
  % for command in commands:
  % if command.dispatch_level == 'instance':
  % if command.name == 'vkGetInstanceProcAddr':
  dispatchTable.${command.name} = getProcAddr;
  % else:
  <% define = get_define(command.platform) %>\
  % if define:
#ifdef ${define}
  % endif
  dispatchTable.${command.name} = reinterpret_cast<PFN_${command.name}>(getProcAddr(instance, "${command.name}"));
  % endif
  % if define:
#endif
  % endif
  % endif
  % endfor
}

static void LoadDeviceLevelFunctions(PFN_vkGetDeviceProcAddr getProcAddr, VkDevice device, VkDeviceLevelDispatchTable& dispatchTable) {
  dispatchTable.device = device;
  % for command in commands:
  % if command.dispatch_level == 'device':
  % if command.name == 'vkGetDeviceProcAddr':
  dispatchTable.${command.name} = getProcAddr;
  % else:
  <% define = get_define(command.platform) %>\
  % if define:
#ifdef ${define}
  % endif
  dispatchTable.${command.name} = reinterpret_cast<PFN_${command.name}>(getProcAddr(device, "${command.name}"));
  % endif
  % if define:
#endif
  % endif
  % endif
  % endfor
}

} // namespace vulkan
} // namespace gits
