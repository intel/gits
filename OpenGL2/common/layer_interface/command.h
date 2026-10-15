// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandIdsAuto.h"

#include <cstdint>

namespace gits::opengl2 {

// Backend-local capture envelope shared by every captured command, including
// meta-commands such as UnsupportedCommand. Per modern-opengl-backend-architecture.md
// ("Layer and command model"), this metadata is not OpenGL driver state and its
// serialized schema must remain stable after release. Typed commands generated for
// enabled GL/WGL calls (commandsAuto.h, once it exists) derive from this class and add
// their own typed argument members.
class Command {
public:
  Command(CommandId id, uint32_t threadId) : m_Id(id), m_ThreadId(threadId) {}
  explicit Command(CommandId id) : m_Id(id) {}
  virtual ~Command() = default;

  CommandId GetId() const {
    return m_Id;
  }

private:
  CommandId m_Id{};

public:
  uint64_t m_Key{};
  uint32_t m_ThreadId{};
  bool m_Skip{};
};

} // namespace gits::opengl2
