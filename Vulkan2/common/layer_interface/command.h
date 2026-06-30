// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandIdsAuto.h"
#include "arguments.h"

namespace gits {
namespace vulkan {

class Command {
public:
  Command(CommandId id, uint32_t threadId_) : m_Id(id), m_ThreadId(threadId_) {}
  Command(CommandId id) : m_Id(id) {}
  virtual ~Command() {}

  CommandId GetId() const {
    return m_Id;
  }

public:
  GITSKey m_Key{};
  uint32_t m_ThreadId{};
  bool m_Skip{};

private:
  CommandId m_Id{};
};

} // namespace vulkan
} // namespace gits
