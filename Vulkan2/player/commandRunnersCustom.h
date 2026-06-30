// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandRunner.h"
#include "commandsCustom.h"
#include "commandCodersCustom.h"

namespace gits {
namespace vulkan {

class CreateWindowMetaRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, m_Command);
  }

private:
  CreateWindowMetaCommand m_Command;
};

class MappedDataMetaRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, m_Command);
  }

private:
  MappedDataMetaCommand m_Command;
};

} // namespace vulkan
} // namespace gits
