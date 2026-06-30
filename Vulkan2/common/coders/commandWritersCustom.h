// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandWriter.h"
#include "commandsCustom.h"
#include "commandCodersCustom.h"

namespace gits {
namespace vulkan {

class CreateWindowMetaWriter : public CommandWriter {
public:
  CreateWindowMetaWriter(CreateWindowMetaCommand& command) {
    m_DataSize = GetSize(command);
    m_Data.reset(new char[m_DataSize]);
    Encode(command, m_Data.get());
  }

  uint32_t Id() const override {
    return static_cast<uint32_t>(CommandId::ID_META_CREATE_WINDOW);
  }
};

class MappedDataMetaWriter : public CommandWriter {
public:
  MappedDataMetaWriter(MappedDataMetaCommand& command) {
    m_DataSize = GetSize(command);
    m_Data.reset(new char[m_DataSize]);
    Encode(command, m_Data.get());
  }

  uint32_t Id() const override {
    return static_cast<uint32_t>(CommandId::ID_META_MAPPED_DATA);
  }
};

} // namespace vulkan
} // namespace gits
