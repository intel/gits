// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "command.h"
#include "ccodeStream.h"
#include "ccodeArguments.h"
#include "ccodeTypes.h"
#include "ccodeStructsAuto.h"
#include "to_string/toStr.h"

#include <ostream>
#include <sstream>
#include <string>
#include <type_traits>

namespace gits {
namespace DirectX {
namespace ccode {

class CommandPrinter {
public:
  CommandPrinter(Command& command, const char* name, unsigned objectId = 0);
  ~CommandPrinter();

  template <typename T>
  void addArgument(T& arg, CppParameterInfo& info);

  template <typename T>
  void addArgumentValue(T& value);

  std::string& getArgumentValue(unsigned index);

  void setPreCommand(const std::string& preCommand);
  void setPostCommand(const std::string& postCommand);
  void skip();
  void print();

private:
  Command& m_Command;
  std::string m_Name{};
  std::vector<std::string> m_CppArgValues{};
  std::vector<std::string> m_CppArgInitializations{};
  std::string m_CppPreCommand{};
  std::string m_CppPostCommand{};
  unsigned m_ObjectId{};
  bool m_Skip{false};
};

template <typename T>
inline void CommandPrinter::addArgument(T& arg, CppParameterInfo& info) {
  if (m_Skip) {
    return;
  }

  CppParameterOutput out;
  out.initialization = "DEFAULT_INITIALIZATION\n";
  out.value = "DEFAULT_NAME";
  out.decorator = "";

  argumentToCpp(arg, info, out);

  if (!out.initialization.empty()) {
    m_CppArgInitializations.push_back(out.initialization);
  }
  m_CppArgValues.push_back(out.decorator + out.value);
}

template <typename T>
inline void CommandPrinter::addArgumentValue(T& value) {
  if (m_Skip) {
    return;
  }
  m_CppArgValues.push_back(toStr(value));
}

} // namespace ccode
} // namespace DirectX
} // namespace gits
