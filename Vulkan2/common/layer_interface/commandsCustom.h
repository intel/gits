// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "command.h"
#include "commandIdsAuto.h"
#include "arguments.h"

namespace gits {
namespace vulkan {

class StateRestoreBeginCommand : public Command {
public:
  StateRestoreBeginCommand() : Command(CommandId::ID_INIT_START) {}
};

class StateRestoreEndCommand : public Command {
public:
  StateRestoreEndCommand() : Command(CommandId::ID_INIT_END) {}
};

class FrameEndCommand : public Command {
public:
  FrameEndCommand() : Command(CommandId::ID_FRAME_END) {}
};

class MarkerUInt64Command : public Command {
public:
  enum Value : uint64_t {
    NONE = 0x10000 + 1, // CTokenMarkerUInt64::COMMON_RESERVED + 1
  };
  MarkerUInt64Command(uint64_t value) : Command(CommandId::ID_MARKER_UINT64), value_(value) {}
  MarkerUInt64Command() : Command(CommandId::ID_MARKER_UINT64) {}

public:
  Argument<uint64_t> value_{};
};

class CreateWindowMetaCommand : public Command {
public:
  enum DisplayProtocol : uint32_t {
    WIN,
    XLIB,
    XCB,
    WAYLAND
  };

  CreateWindowMetaCommand(uint32_t threadId)
      : Command{CommandId::ID_META_CREATE_WINDOW, threadId} {}
  CreateWindowMetaCommand() : Command(CommandId::ID_META_CREATE_WINDOW) {}

public:
  Argument<uint32_t> m_DisplayProtocol{};
  Argument<int32_t> m_X{};
  Argument<int32_t> m_Y{};
  Argument<int32_t> m_Width{};
  Argument<int32_t> m_Height{};
  Argument<bool> m_Visible{};
  Argument<uint64_t> m_Hwnd{};
  Argument<uint64_t> m_Hinstance{};
};

class UpdateWindowMetaCommand : public Command {
public:
  UpdateWindowMetaCommand(uint32_t threadId)
      : Command{CommandId::ID_META_UPDATE_WINDOW, threadId} {}
  UpdateWindowMetaCommand() : Command(CommandId::ID_META_UPDATE_WINDOW) {}

public:
  Argument<uint64_t> m_Hwnd{};
  Argument<int32_t> m_Width{};
  Argument<int32_t> m_Height{};
  Argument<bool> m_Visible{};
};

class MappedDataMetaCommand : public Command {
public:
  MappedDataMetaCommand(uint32_t threadId) : Command{CommandId::ID_META_MAPPED_DATA, threadId} {}
  MappedDataMetaCommand() : Command(CommandId::ID_META_MAPPED_DATA) {}

public:
  HandleArgument<VkDevice> m_Device{};
  HandleArgument<VkDeviceMemory> m_Memory{};
  MemoryRegions m_Regions{};
};

} // namespace vulkan
} // namespace gits
