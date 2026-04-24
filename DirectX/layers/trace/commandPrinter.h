// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
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
  unsigned frameCount{1};
  unsigned drawCount{};
  unsigned dispatchCount{};
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
    if (m_FirstArgumentPrinted && !m_ReturnPrinted) {
      m_Stream << ", ";
    }
    m_FirstArgumentPrinted = true;

    m_Stream << arg;
  }

  template <typename T>
  void addResult(T& arg) {
    m_ReturnPrinted = true;
    m_Stream << ") = ";
    addArgument(arg);
  }

  void print(bool flush, bool newLine = true);

private:
  CommandPrinterState& m_State;
  Command& m_Command;
  FastOStream& m_Stream;
  std::lock_guard<std::mutex> m_Lock;
  bool m_FirstArgumentPrinted{};
  bool m_ReturnPrinted{};
};

} // namespace DirectX
} // namespace gits
