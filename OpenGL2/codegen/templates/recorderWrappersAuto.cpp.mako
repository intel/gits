// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${AUTO_GENERATED_HEADER}

#include "recorderWrappers.h"

#include "openglDriversAuto.h"
#include "recursionGuard.h"
#include "unsupportedCommand.h"

#include <string>
#include <unordered_map>

namespace gits::opengl2 {

% for cmd in commands:
<%
    ret = cmd.return_value.type.strip()
    is_void = ret.lower() == 'void'  # WGL commands use Win32 'VOID'.
    param_decls = ', '.join(f'{p.type} {p.name}' for p in cmd.params)
    param_names = ', '.join(p.name for p in cmd.params)
%>\
${ret} APIENTRY RecorderWrapper_${cmd.name}(${param_decls}) {
  const RecursionGuard recursionGuard;
  const auto realFunction = g_DispatchTable.${cmd.name}.load(std::memory_order_acquire);
% if not cmd.enabled:
  if (!recursionGuard.IsNested()) {
    ReportUnsupportedCommand("${cmd.name}");
  }
% endif
% if is_void:
  realFunction(${param_names});
% else:
  return realFunction(${param_names});
% endif
}

% endfor
void* GetGeneratedRecorderWrapper(const char* name) {
  static const std::unordered_map<std::string, void*> wrappers = {
% for cmd in commands:
    {"${cmd.name}", reinterpret_cast<void*>(&RecorderWrapper_${cmd.name})},
% endfor
  };
  const auto it = wrappers.find(name);
  return it == wrappers.end() ? nullptr : it->second;
}

} // namespace gits::opengl2