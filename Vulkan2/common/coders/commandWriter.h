// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "token.h"
#include "streams.h"

#include <memory>

namespace gits {
namespace vulkan {

class CommandWriter : public CToken {
public:
  void Run() override {}

private:
  void Read(CBinIStream& stream) override {}

  void Write(CBinOStream& stream) const override {
    stream.write(reinterpret_cast<const char*>(&m_DataSize), sizeof(uint32_t));
    stream.write(m_Data.get(), m_DataSize);
  }

  uint64_t Size() const override {
    return sizeof(uint32_t) + m_DataSize;
  }

protected:
  std::unique_ptr<char[]> m_Data;
  uint32_t m_DataSize{};
};

} // namespace vulkan
} // namespace gits
