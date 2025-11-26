// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "commandPrinter.h"
#include "layerAuto.h"
#include "gits.h"

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
    : stream_(stream), state_(state), command_(command), lock_(state.mutex) {
  if (Configurator::Get().directx.features.trace.print.timestamp) {
    printDateTime(stream_);
  }
  if (command.skip) {
    stream_ << "[SKIPPED] ";
  }
  stream_ << callKeyToStr(command.key);
  stream_ << " T" << command.threadId;
  if (objectId) {
    stream_ << " ";
    printObjectKey(stream_, objectId);
  }
  stream_ << " " << name << "(";
}

void CommandPrinter::print(bool flush, bool newLine) {

  if (!returnPrinted_) {
    stream_ << ")";
  }

  if (command_.getId() == CommandId::ID_INIT_START) {
    state_.stateRestorePhase = true;
  } else if (command_.getId() == CommandId::ID_INIT_END) {
    state_.stateRestorePhase = false;
    state_.drawCount = 0;
    state_.commandListExecutionCount = 0;
  }

  if (command_.getId() == CommandId::ID_IDXGISWAPCHAIN_PRESENT &&
          !(static_cast<IDXGISwapChainPresentCommand&>(command_).Flags_.value &
            DXGI_PRESENT_TEST) ||
      command_.getId() == CommandId::ID_IDXGISWAPCHAIN1_PRESENT1 &&
          !(static_cast<IDXGISwapChain1Present1Command&>(command_).PresentFlags_.value &
            DXGI_PRESENT_TEST)) {
    stream_ << " Frame #"
            << (command_.key & Command::stateRestoreKeyMask ? 0 : CGits::Instance().CurrentFrame())
            << " end";
  } else if (command_.getId() == CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_DRAWINSTANCED ||
             command_.getId() == CommandId::ID_ID3D12GRAPHICSCOMMANDLIST_DRAWINDEXEDINSTANCED) {
    stream_ << " Draw #" << ++state_.drawCount << " from frame #"
            << (state_.stateRestorePhase ? 0 : CGits::Instance().CurrentFrame());
  } else if (command_.getId() == CommandId::ID_ID3D12COMMANDQUEUE_EXECUTECOMMANDLISTS &&
             command_.key & Command::executionSerializationKeyMask && !state_.stateRestorePhase) {
    unsigned currentFrame = CGits::Instance().CurrentFrame();
    // handling command list subcapture without presents
    if (currentFrame == 0 && !state_.stateRestorePhase) {
      currentFrame = 1;
    }
    stream_ << " Execute #" << ++state_.commandListExecutionCount << " from frame #"
            << currentFrame;
  }
  if (newLine) {
    stream_ << "\n";
  }

  if (flush) {
    stream_.flush();
  }
}

} // namespace DirectX
} // namespace gits
