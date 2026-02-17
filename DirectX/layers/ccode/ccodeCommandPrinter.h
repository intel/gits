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
  Command& command_;
  std::string name_{};
  std::vector<std::string> cppArgValues_{};
  std::vector<std::string> cppArgInitializations_{};
  std::string cppPreCommand_{};
  std::string cppPostCommand_{};
  unsigned objectId_{};
  bool skip_{false};
};

template <typename T>
inline void CommandPrinter::addArgument(T& arg, CppParameterInfo& info) {
  if (skip_) {
    return;
  }

  CppParameterOutput out;
  out.initialization = "DEFAULT_INITIALIZATION\n";
  out.value = "DEFAULT_NAME";
  out.decorator = "";

  argumentToCpp(arg, info, out);

  if (!out.initialization.empty()) {
    cppArgInitializations_.push_back(out.initialization);
  }
  cppArgValues_.push_back(out.decorator + out.value);
}

template <typename T>
inline void CommandPrinter::addArgumentValue(T& value) {
  if (skip_) {
    return;
  }
  cppArgValues_.push_back(toStr(value));
}

} // namespace ccode
} // namespace DirectX
} // namespace gits
