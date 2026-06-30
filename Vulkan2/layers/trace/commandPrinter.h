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
#include "printStructuresCustom.h"
#include "printStructuresAuto.h"

#include <string>
#include <mutex>

namespace gits {
namespace vulkan {

struct CommandPrinterState {
  CommandPrinterState(std::mutex& m) : Mutex(m) {}
  std::mutex& Mutex;
  unsigned FrameCount{1};
  unsigned DrawCount{};
  unsigned DispatchCount{};
  unsigned CommandListExecutionCount{};
  bool StateRestorePhase{};
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
  FastOStream& m_Stream;
  CommandPrinterState& m_State;
  Command& m_Command;
  std::lock_guard<std::mutex> m_Lock;
  bool m_FirstArgumentPrinted{};
  bool m_ReturnPrinted{};
};

} // namespace vulkan
} // namespace gits
