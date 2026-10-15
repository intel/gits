// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

${AUTO_GENERATED_HEADER}

#include "traceLayerAuto.h"

namespace gits::opengl2 {

% for cmd in commands:
% if cmd.enabled:
void TraceLayer::Pre(${cmd.name}Command& command) {
  CommandPrinter printer(m_Stream, m_Mutex, command, "${cmd.name}");
% for param in cmd.params:
% if param.name not in consumed_pointer_params(cmd.name):
<% pointer = pointer_override_for(cmd, param.name) %>\
% if pointer_needs_raw_offset_capture(pointer):
  if (!command.m_${param.name}.HasValue) {
    printer.AddOpaque(command.m_${param.name}RawPointer.Value);
  } else {
    printer.Add(command.m_${param.name});
  }
% elif (cmd.name, param.name) in TEXT_OUTPUT_BUFFERS:
<% length_param = TEXT_OUTPUT_BUFFERS[(cmd.name, param.name)] %>\
  if (command.m_${param.name}.HasValue) {
    const auto& text = command.m_${param.name}.Data;
    const auto& lengthData = command.m_${length_param}.Data;
    const auto* reportedLength = lengthData.empty() ? nullptr : lengthData.data();
    printer.AddText(std::string_view(text.empty() ? "" : text.data(),
                                     TextOutputLength(text.data(), text.size(), reportedLength)));
  } else {
    printer.Add(command.m_${param.name});
  }
% elif pointer is not None and pointer.length_source_kind == LengthSourceKind.NULL_TERMINATED and param.type_facts.base_type == 'GLchar':
  if (command.m_${param.name}.HasValue) {
    const auto& text = command.m_${param.name}.Data;
    // The captured null terminator is needed for replay, but is not part of the displayed text.
    printer.AddText(std::string_view(text.data(), text.size() - 1));
  } else {
    printer.Add(command.m_${param.name});
  }
% elif param.type_facts.base_type in ('GLenum', 'GLbitfield') and pointer is None:
  printer.AddEnum(command.m_${param.name}.Value, GLenumApi::${cmd.api},
                  GLenumGroup::${param.group if param.group in enum_groups else 'Unknown'});
% elif param.type.strip() == 'GLboolean':
  printer.AddBoolean(command.m_${param.name}.Value);
% elif pointer is not None and pointer.length_source_kind == LengthSourceKind.OPAQUE_BIT_PATTERN:
  printer.AddOpaque(command.m_${param.name}.Value);
% elif pointer is not None and param.type_facts.base_type in ('GLenum', 'GLbitfield'):
  printer.AddEnumArray(command.m_${param.name}, GLenumApi::${cmd.api},
                       GLenumGroup::${param.group if param.group in enum_groups else 'Unknown'});
% else:
  printer.Add(command.m_${param.name});
% endif
% if pointer_needs_pixel_transfer_unknown_flag(pointer):
  if (command.m_${param.name}SizeUnknown.Value) {
    printer.AnnotateUnknownSize();
  }
% endif
% endif
% endfor
% if cmd.return_value.type.strip().lower() != 'void':
% if cmd.return_value.type_facts.base_type == 'GLenum':
  printer.ResultEnum(command.m_Return.Value, GLenumApi::${cmd.api},
                     GLenumGroup::${cmd.return_value.group if cmd.return_value.group in enum_groups else 'Unknown'});
% elif cmd.return_value.type.strip() == 'GLboolean':
  printer.ResultBoolean(command.m_Return.Value);
% else:
  printer.Result(command.m_Return);
% endif
% endif
  printer.Finish(m_Flush);
}

% endif
% endfor
} // namespace gits::opengl2