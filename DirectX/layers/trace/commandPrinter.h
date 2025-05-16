// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "command.h"
#include "printCustom.h"
#include "printArguments.h"
#include "printStructuresAuto.h"

#include <string>
#include <mutex>

namespace gits {
namespace DirectX {

struct CommandPrinterState {
  CommandPrinterState(std::mutex& m) : mutex(m) {}
  std::mutex& mutex;
  unsigned drawCount{};
  unsigned commandListExecutionCount{};
  bool stateRestorePhase{};
};

class CommandPrinter {
public:
  CommandPrinter(FastOStream& stream,
                 CommandPrinterState& state,
                 Command& command,
                 const char* name,
                 unsigned objectId = 0);

  template <typename T>
  void addArgument(T& arg) {
    if (firstArgumentPrinted_ && !returnPrinted_) {
      stream_ << ", ";
    }
    firstArgumentPrinted_ = true;

    stream_ << arg;
  }

  template <typename T>
  void addResult(T& arg) {
    returnPrinted_ = true;
    stream_ << ") = ";
    addArgument(arg);
  }

  void print(bool flush, bool newLine = true);

private:
  CommandPrinterState& state_;
  Command& command_;
  FastOStream& stream_;
  std::lock_guard<std::mutex> lock_;
  bool firstArgumentPrinted_{};
  bool returnPrinted_{};
};

} // namespace DirectX
} // namespace gits
