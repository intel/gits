// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "commandPrinter.h"
#include "layerAuto.h"
#include "keyUtils.h"
#include "configurator.h"

namespace gits {
namespace DirectX {

static void printDateTime(FastOStream& stream) {
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
    : m_State(state), m_Command(command), m_Stream(stream), m_Lock(state.mutex) {

  if (m_Command.GetId() == CommandId::ID_INIT_START) {
    m_State.stateRestorePhase = true;
    m_State.frameCount = 0;
    return;
  } else if (m_Command.GetId() == CommandId::ID_INIT_END) {
    m_State.stateRestorePhase = false;
    m_State.frameCount = 1;
    m_State.drawCount = 0;
    m_State.dispatchCount = 0;
    m_State.commandListExecutionCount = 0;
    return;
  }

  if (Configurator::Get().directx.features.trace.print.timestamp) {
    printDateTime(m_Stream);
  }
  if (command.Skip) {
    m_Stream << "[SKIPPED] ";
  }
  m_Stream << keyToStr(command.Key);
  m_Stream << " T" << command.ThreadId;
  if (objectId) {
    m_Stream << " ";
    printObjectKey(m_Stream, objectId);
  }
  m_Stream << " " << name << "(";
}

void CommandPrinter::print(bool flush, bool newLine) {
  if (!m_ReturnPrinted) {
    m_Stream << ")";
  }

  if ((m_Command.GetId() == CommandId::ID_IDXGISWAPCHAIN_PRESENT &&
           !(static_cast<IDXGISwapChainPresentCommand&>(m_Command).m_Flags.Value &
             DXGI_PRESENT_TEST) ||
       m_Command.GetId() == CommandId::ID_IDXGISWAPCHAIN1_PRESENT1 &&
           !(static_cast<IDXGISwapChain1Present1Command&>(m_Command).m_PresentFlags.Value &
             DXGI_PRESENT_TEST)) &&
      !IsStateRestoreKey(m_Command.Key)) {
    m_Stream << " Frame #" << m_State.frameCount << " end";
    ++m_State.frameCount;
    m_State.drawCount = 0;
    m_State.dispatchCount = 0;
    m_State.commandListExecutionCount = 0;
  } else if (m_Command.GetId() == CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_DRAWINSTANCED ||
             m_Command.GetId() == CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_DRAWINDEXEDINSTANCED) {
    m_Stream << " Frame #" << m_State.frameCount << " Frame Draw #" << ++m_State.drawCount;
  } else if (m_Command.GetId() == CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_DISPATCH) {
    m_Stream << " Frame #" << m_State.frameCount << " Frame Dispatch #" << ++m_State.dispatchCount;
  } else if (m_Command.GetId() == CommandId::ID_ID3D12COMMANDQUEUE_EXECUTECOMMANDLISTS &&
             IsExecutionSerializationKey(m_Command.Key) && !m_State.stateRestorePhase) {
    m_Stream << " Frame #" << m_State.frameCount << " Frame Execute #"
             << ++m_State.commandListExecutionCount;
  }
  if (newLine) {
    m_Stream << "\n";
  }

  if (flush) {
    m_Stream.Flush();
  }
}

} // namespace DirectX
} // namespace gits
