// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "ccodeCommandPrinter.h"
#include "layerAuto.h"
#include "gits.h"
#include "ccodeArguments.h"

namespace gits {
namespace DirectX {
namespace ccode {

CommandPrinter::CommandPrinter(Command& command, const char* name, unsigned objectId)
    : command_(command), name_(name), objectId_(objectId) {}

CommandPrinter::~CommandPrinter() {}

std::string& CommandPrinter::getArgumentValue(unsigned index) {
  GITS_ASSERT(index < cppArgValues_.size(), "Argument index out of range.");
  return cppArgValues_[index];
}

void CommandPrinter::setPreCommand(const std::string& preCommand) {
  cppPreCommand_ = preCommand;
}

void CommandPrinter::setPostCommand(const std::string& postCommand) {
  cppPostCommand_ = postCommand;
}

void CommandPrinter::skip() {
  skip_ = true;
}

void CommandPrinter::print() {
  auto& stream = ccode::CCodeStream::getInstance();
  auto& ss = stream.getCurrentBlock();
  if (skip_) {
    ss << "// [SKIPPED] Command " << keyToStr(command_.key) << " " << name_ << std::endl;
    return;
  }

  bool needsScope =
      !cppArgInitializations_.empty() || !cppPreCommand_.empty() || !cppPostCommand_.empty();

  // Command #N name
  ss << "// Command " << keyToStr(command_.key) << " " << name_ << std::endl;
  if (needsScope) {
    ss << "{" << std::endl;
  }

  // Print argument declarations
  for (const auto& initialization : cppArgInitializations_) {
    ss << initialization;
  }

  // Print command (with pre / post)
  ss << cppPreCommand_;
  bool hasObjectAsArgument = false;
  if (Configurator::Get().directx.player.cCode.wrapApiCalls) {
    // Wrapped: CC_Function(g_OX, a0, a1, ...);
    ss << "CC_" << name_ << "(";
    if (objectId_ != 0) {
      hasObjectAsArgument = true;
      ss << objKeyToPtrStr(objectId_);
    }
  } else {
    // Default: g_OX->Function(a0, a1, ...);
    if (objectId_ != 0) {
      ss << "g_" << objKeyToStr(objectId_) << "->";
    }
    ss << name_ << "(";
  }
  // Print arguments
  for (size_t i = 0; i < cppArgValues_.size(); ++i) {
    if (hasObjectAsArgument || (i > 0)) {
      ss << ", ";
    }
    ss << cppArgValues_[i];
  }
  ss << ");" << std::endl;
  ss << cppPostCommand_;

  // Closing scope
  if (needsScope) {
    ss << "}" << std::endl;
  }
}

} // namespace ccode
} // namespace DirectX
} // namespace gits
