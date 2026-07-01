// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "commandPrinter.h"
#include "layerAuto.h"
#include "configurator.h"

namespace gits {
namespace vulkan {

static void PrintDateTime(FastOStream& stream) {
  auto now = std::chrono::system_clock::now();
  auto timeT = std::chrono::system_clock::to_time_t(now);
  auto ms = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()) % 1000000;

  // Print formatted date and time on a pre-allocated buffer
  char buffer[32];
  size_t offset =
      std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&timeT));
  std::snprintf(&buffer[offset], sizeof(buffer) - offset, ".%06d ", static_cast<int>(ms.count()));

  stream << static_cast<char*>(buffer);
}

CommandPrinter::CommandPrinter(FastOStream& stream,
                               CommandPrinterState& state,
                               Command& command,
                               const char* name,
                               unsigned objectId)
    : m_Stream(stream), m_State(state), m_Command(command), m_Lock(state.Mutex) {

  if (m_Command.GetId() == CommandId::ID_INIT_START) {
    m_State.StateRestorePhase = true;
    m_State.FrameCount = 0;
    return;
  } else if (m_Command.GetId() == CommandId::ID_INIT_END) {
    m_State.StateRestorePhase = false;
    m_State.FrameCount = 1;
    m_State.DrawCount = 0;
    m_State.DispatchCount = 0;
    m_State.CommandListExecutionCount = 0;
    return;
  }

  if (Configurator::Get().common.shared.trace.print.timestamp) {
    PrintDateTime(m_Stream);
  }
  if (command.m_Skip) {
    m_Stream << "[SKIPPED] ";
  }
  m_Stream << command.m_Key;
  m_Stream << " T" << command.m_ThreadId;
  if (objectId) {
    m_Stream << " ";
    PrintObjectKey(m_Stream, objectId);
  }
  m_Stream << " " << name << "(";
}

void CommandPrinter::print(bool flush, bool newLine) {
  if (!m_ReturnPrinted) {
    m_Stream << ")";
  }

  CommandId id = m_Command.GetId();

  // Frame boundary: vkQueuePresentKHR
  if (id == CommandId::ID_VKQUEUEPRESENTKHR && !m_State.StateRestorePhase) {
    m_Stream << " Frame #" << m_State.FrameCount << " end";
    ++m_State.FrameCount;
    m_State.DrawCount = 0;
    m_State.DispatchCount = 0;
    m_State.CommandListExecutionCount = 0;
  }
  // Draw commands
  else if (id == CommandId::ID_VKCMDDRAW || id == CommandId::ID_VKCMDDRAWINDEXED ||
           id == CommandId::ID_VKCMDDRAWINDIRECT || id == CommandId::ID_VKCMDDRAWINDEXEDINDIRECT ||
           id == CommandId::ID_VKCMDDRAWINDIRECTCOUNT ||
           id == CommandId::ID_VKCMDDRAWINDIRECTCOUNTKHR ||
           id == CommandId::ID_VKCMDDRAWINDIRECTCOUNTAMD ||
           id == CommandId::ID_VKCMDDRAWINDEXEDINDIRECTCOUNT ||
           id == CommandId::ID_VKCMDDRAWINDEXEDINDIRECTCOUNTKHR ||
           id == CommandId::ID_VKCMDDRAWINDEXEDINDIRECTCOUNTAMD ||
           id == CommandId::ID_VKCMDDRAWMULTIEXT || id == CommandId::ID_VKCMDDRAWMULTIINDEXEDEXT ||
           id == CommandId::ID_VKCMDDRAWMESHTASKSNV ||
           id == CommandId::ID_VKCMDDRAWMESHTASKSINDIRECTNV ||
           id == CommandId::ID_VKCMDDRAWMESHTASKSINDIRECTCOUNTNV ||
           id == CommandId::ID_VKCMDDRAWMESHTASKSEXT ||
           id == CommandId::ID_VKCMDDRAWMESHTASKSINDIRECTEXT ||
           id == CommandId::ID_VKCMDDRAWMESHTASKSINDIRECTCOUNTEXT) {
    m_Stream << " Frame #" << m_State.FrameCount << " Frame Draw #" << ++m_State.DrawCount;
  }
  // Dispatch commands
  else if (id == CommandId::ID_VKCMDDISPATCH || id == CommandId::ID_VKCMDDISPATCHINDIRECT ||
           id == CommandId::ID_VKCMDDISPATCHBASE || id == CommandId::ID_VKCMDDISPATCHBASEKHR) {
    m_Stream << " Frame #" << m_State.FrameCount << " Frame Dispatch #" << ++m_State.DispatchCount;
  }
  // Queue submit (execution)
  else if ((id == CommandId::ID_VKQUEUESUBMIT || id == CommandId::ID_VKQUEUESUBMIT2 ||
            id == CommandId::ID_VKQUEUESUBMIT2KHR) &&
           !m_State.StateRestorePhase) {
    m_Stream << " Frame #" << m_State.FrameCount << " Frame Execute #"
             << ++m_State.CommandListExecutionCount;
  }
  // Trace rays
  else if (id == CommandId::ID_VKCMDTRACERAYSKHR || id == CommandId::ID_VKCMDTRACERAYSNV ||
           id == CommandId::ID_VKCMDTRACERAYSINDIRECTKHR ||
           id == CommandId::ID_VKCMDTRACERAYSINDIRECT2KHR) {
    m_Stream << " Frame #" << m_State.FrameCount << " Frame Dispatch #" << ++m_State.DispatchCount;
  }

  if (newLine) {
    m_Stream << "\n";
  }

  if (flush) {
    m_Stream.Flush();
  }
}

} // namespace vulkan
} // namespace gits
