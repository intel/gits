// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandIdsAuto.h"

#include <vector>

namespace gits {
namespace DirectX {

class Command {
public:
  Command(CommandId id, unsigned threadId) : m_Id(id), ThreadId(threadId) {}
  Command(CommandId id) : m_Id{id} {}
  virtual ~Command() {}

  CommandId GetId() const {
    return m_Id;
  }

public:
  unsigned Key{};
  unsigned ThreadId{};
  bool Skip{false};

private:
  CommandId m_Id{};
};

} // namespace DirectX
} // namespace gits
