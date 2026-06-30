// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandPlayer.h"
#include "commandsCustom.h"
#include "commandCodersCustom.h"

namespace gits {
namespace vulkan {

class CreateWindowMetaPlayer : public CommandPlayer {
public:
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_META_CREATE_WINDOW);
  }
  const char* Name() const override {
    return "CreateWindowMetaCommand";
  }
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data.get(), m_Command);
  }

private:
  CreateWindowMetaCommand m_Command;
};

class MappedDataMetaPlayer : public CommandPlayer {
public:
  unsigned Id() const override {
    return static_cast<unsigned>(CommandId::ID_META_MAPPED_DATA);
  }
  const char* Name() const override {
    return "MappedDataMetaCommand";
  }
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data.get(), m_Command);
  }

private:
  MappedDataMetaCommand m_Command;
};

} // namespace vulkan
} // namespace gits
