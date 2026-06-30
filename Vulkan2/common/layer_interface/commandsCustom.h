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

class CreateWindowMetaCommand : public Command {
public:
  CreateWindowMetaCommand(uint32_t threadId)
      : Command{CommandId::ID_META_CREATE_WINDOW, threadId} {}
  CreateWindowMetaCommand() : Command(CommandId::ID_META_CREATE_WINDOW) {}

public:
  Argument<int32_t> m_X{};
  Argument<int32_t> m_Y{};
  Argument<int32_t> m_Width{};
  Argument<int32_t> m_Height{};
  Argument<bool> m_Visible{};
  Argument<uint64_t> m_Hwnd{};
  Argument<uint64_t> m_Hinstance{};
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
