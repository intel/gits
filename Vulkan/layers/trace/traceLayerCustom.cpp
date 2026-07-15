// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "traceLayerAuto.h"

#include <unordered_map>
#include <sstream>

namespace gits {
namespace vulkan {

static std::string Uint64MarkerToStr(uint64_t value) {
  static const std::unordered_map<uint64_t, std::string> enumMap = {
      {MarkerUInt64Command::Value::NONE, "NONE"},
  };

  return enumMap.contains(value) ? enumMap.at(value) : "UNKNOWN";
}

void TraceLayer::Pre(StateRestoreBeginCommand& command) {
  if (printPre_) {
    CommandPrinter p(streamPre_, statePre_, command, "StateRestoreBegin");
    streamPre_ << "STATE_RESTORE_BEGIN\n";
    if (flush_) {
      streamPre_.Flush();
    }
  }
}

void TraceLayer::Post(StateRestoreBeginCommand& command) {
  if (printPost_) {
    CommandPrinter p(streamPost_, statePost_, command, "StateRestoreBegin");
    streamPost_ << "STATE_RESTORE_BEGIN\n";
    if (flush_) {
      streamPost_.Flush();
    }
  }
}

void TraceLayer::Pre(StateRestoreEndCommand& command) {
  if (printPre_) {
    CommandPrinter p(streamPre_, statePre_, command, "StateRestoreEnd");
    streamPre_ << "STATE_RESTORE_END\n";
    if (flush_) {
      streamPre_.Flush();
    }
  }
}

void TraceLayer::Post(StateRestoreEndCommand& command) {
  if (printPost_) {
    CommandPrinter p(streamPost_, statePost_, command, "StateRestoreEnd");
    streamPost_ << "STATE_RESTORE_END\n";
    if (flush_) {
      streamPost_.Flush();
    }
  }
}

void TraceLayer::Pre(MarkerUInt64Command& command) {
  if (printPre_) {
    streamPre_ << "MARKER_" << Uint64MarkerToStr(command.value_.Value) << "\n";
    if (flush_) {
      streamPre_.Flush();
    }
  }
}

void TraceLayer::Post(MarkerUInt64Command& command) {
  if (printPost_) {
    streamPost_ << "MARKER_" << Uint64MarkerToStr(command.value_.Value) << "\n";
    if (flush_) {
      streamPost_.Flush();
    }
  }
}

void TraceLayer::Pre(CreateWindowMetaCommand& command) {
  if (printPre_) {
    CommandPrinter p(streamPre_, statePre_, command, "CreateWindowMetaCommand");
    p.addArgument(command.m_Hwnd);
    p.addArgument(command.m_X);
    p.addArgument(command.m_Y);
    p.addArgument(command.m_Width);
    p.addArgument(command.m_Height);
    p.addArgument(command.m_Visible);
    p.addArgument(command.m_Hinstance);
    p.print(flush_);
  }
}

void TraceLayer::Post(CreateWindowMetaCommand& command) {
  if (printPost_) {
    CommandPrinter p(streamPost_, statePost_, command, "CreateWindowMetaCommand");
    p.addArgument(command.m_Hwnd);
    p.addArgument(command.m_X);
    p.addArgument(command.m_Y);
    p.addArgument(command.m_Width);
    p.addArgument(command.m_Height);
    p.addArgument(command.m_Visible);
    p.addArgument(command.m_Hinstance);
    p.print(flush_);
  }
}

void TraceLayer::Pre(MappedDataMetaCommand& command) {
  if (printPre_) {
    CommandPrinter p(streamPre_, statePre_, command, "MappedDataMetaCommand");
    p.addArgument(command.m_Device);
    p.addArgument(command.m_Key);
    p.addArgument(command.m_Memory);
    p.addArgument(command.m_Regions);
    p.print(flush_);
  }
}

void TraceLayer::Post(MappedDataMetaCommand& command) {
  if (printPost_) {
    CommandPrinter p(streamPost_, statePost_, command, "MappedDataMetaCommand");
    p.addArgument(command.m_Device);
    p.addArgument(command.m_Key);
    p.addArgument(command.m_Memory);
    p.addArgument(command.m_Regions);
    p.print(flush_);
  }
}

void TraceLayer::Pre(RestoreContentManifestCommand& command) {
  if (printPre_) {
    CommandPrinter p(streamPre_, statePre_, command, "RestoreContentManifestCommand");
    uint64_t bufferCount = command.m_Buffers.size();
    uint64_t imageCount = command.m_Images.size();
    p.addArgument(command.m_DeviceKey);
    p.addArgument(command.m_PhysDevKey);
    p.addArgument(command.m_QueueKey);
    p.addArgument(command.m_CommandPoolKey);
    p.addArgument(command.m_TotalBytes);
    p.addArgument(bufferCount);
    p.addArgument(imageCount);
    p.print(flush_);
  }
}

void TraceLayer::Post(RestoreContentManifestCommand& command) {
  if (printPost_) {
    CommandPrinter p(streamPost_, statePost_, command, "RestoreContentManifestCommand");
    uint64_t bufferCount = command.m_Buffers.size();
    uint64_t imageCount = command.m_Images.size();
    p.addArgument(command.m_DeviceKey);
    p.addArgument(command.m_PhysDevKey);
    p.addArgument(command.m_QueueKey);
    p.addArgument(command.m_CommandPoolKey);
    p.addArgument(command.m_TotalBytes);
    p.addArgument(bufferCount);
    p.addArgument(imageCount);
    p.print(flush_);
  }
}

void TraceLayer::Pre(RestoreContentDataCommand& command) {
  if (printPre_) {
    CommandPrinter p(streamPre_, statePre_, command, "RestoreContentDataCommand");
    p.addArgument(command.m_DeviceKey);
    p.addArgument(command.m_Regions);
    p.print(flush_);
  }
}

void TraceLayer::Post(RestoreContentDataCommand& command) {
  if (printPost_) {
    CommandPrinter p(streamPost_, statePost_, command, "RestoreContentDataCommand");
    p.addArgument(command.m_DeviceKey);
    p.addArgument(command.m_Regions);
    p.print(flush_);
  }
}

} // namespace vulkan
} // namespace gits
