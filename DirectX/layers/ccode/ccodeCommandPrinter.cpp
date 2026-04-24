// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "ccodeCommandPrinter.h"
#include "layerAuto.h"
#include "ccodeArguments.h"
#include "configurator.h"

namespace gits {
namespace DirectX {
namespace ccode {

CommandPrinter::CommandPrinter(Command& command, const char* name, unsigned objectId)
    : m_Command(command), m_Name(name), m_ObjectId(objectId) {}

CommandPrinter::~CommandPrinter() {}

std::string& CommandPrinter::getArgumentValue(unsigned index) {
  GITS_ASSERT(index < m_CppArgValues.size(), "Argument index out of range.");
  return m_CppArgValues[index];
}

void CommandPrinter::setPreCommand(const std::string& preCommand) {
  m_CppPreCommand = preCommand;
}

void CommandPrinter::setPostCommand(const std::string& postCommand) {
  m_CppPostCommand = postCommand;
}

void CommandPrinter::skip() {
  m_Skip = true;
}

void CommandPrinter::print() {
  auto& stream = ccode::CCodeStream::getInstance();
  auto& ss = stream.getCurrentBlock();
  if (m_Skip) {
    ss << "// [SKIPPED] Command " << keyToStr(m_Command.Key) << " " << m_Name << std::endl;
    return;
  }

  bool needsScope =
      !m_CppArgInitializations.empty() || !m_CppPreCommand.empty() || !m_CppPostCommand.empty();

  // Command #N name
  ss << "// Command " << keyToStr(m_Command.Key) << " " << m_Name << std::endl;
  if (needsScope) {
    ss << "{" << std::endl;
  }

  // Print argument declarations
  for (const auto& initialization : m_CppArgInitializations) {
    ss << initialization;
  }

  // Print command (with pre / post)
  ss << m_CppPreCommand;
  bool hasObjectAsArgument = false;
  if (Configurator::Get().directx.player.cCode.wrapApiCalls) {
    // Wrapped: CC_Function(g_OX, a0, a1, ...);
    ss << "CC_" << m_Name << "(";
    if (m_ObjectId != 0) {
      hasObjectAsArgument = true;
      ss << objKeyToPtrStr(m_ObjectId);
    }
  } else {
    // Default: g_OX->Function(a0, a1, ...);
    if (m_ObjectId != 0) {
      ss << "g_" << objKeyToStr(m_ObjectId) << "->";
    }
    ss << m_Name << "(";
  }
  // Print arguments
  for (size_t i = 0; i < m_CppArgValues.size(); ++i) {
    if (hasObjectAsArgument || (i > 0)) {
      ss << ", ";
    }
    ss << m_CppArgValues[i];
  }
  ss << ");" << std::endl;
  ss << m_CppPostCommand;

  // Closing scope
  if (needsScope) {
    ss << "}" << std::endl;
  }
}

} // namespace ccode
} // namespace DirectX
} // namespace gits
