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

#include <vector>

namespace gits {
namespace DirectX {

class Command {
public:
  Command(CommandId id, unsigned threadId_) : id_(id), threadId(threadId_) {}
  Command(CommandId id) : id_{id} {}
  virtual ~Command() {}

  CommandId getId() const {
    return id_;
  }

public:
  unsigned key{};
  unsigned threadId{};
  bool skip{false};

private:
  CommandId id_{};
};

} // namespace DirectX
} // namespace gits
