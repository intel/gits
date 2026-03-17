// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandRunner.h"

namespace gits {
namespace stream {

class CommandFactory {
public:
  CommandFactory() = default;
  virtual ~CommandFactory() {}
  CommandFactory(const CommandFactory&) = delete;
  CommandFactory& operator=(const CommandFactory&) = delete;

  virtual CommandRunner* CreateCommand(unsigned id) = 0;
};

} // namespace stream
} // namespace gits
