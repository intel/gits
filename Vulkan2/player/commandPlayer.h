// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "function.h"
#include "argument.h"

#include <memory>

namespace gits {
namespace vulkan {

class FakeArgument : public CArgument {
public:
  const char* Name() const override {
    return "";
  }
  void Write(CBinOStream& stream) const override {}
  void Read(CBinIStream& stream) override {}
};

class CommandPlayer : public CFunction {
public:
  unsigned ArgumentCount() const override {
    return 0;
  }
  CArgument& Argument(unsigned idx) override {
    return m_Argument;
  }
  const CArgument* Return() const override {
    return nullptr;
  }
  unsigned ResultCount() const override {
    return 0;
  }
  CArgument& Result(unsigned idx) override {
    return m_Argument;
  }
  CLibrary::TId LibraryId() const override {
    return CLibrary::ID_VULKAN2;
  }

  void Read(CBinIStream& stream) override {
    stream.read(reinterpret_cast<char*>(&m_DataSize), sizeof(m_DataSize));
    m_Data.reset(new char[m_DataSize]);
    stream.read(m_Data.get(), m_DataSize);

    DecodeCommand();
  }

  uint64_t Size() const override {
    return sizeof(m_DataSize) + m_DataSize;
  }

protected:
  virtual void DecodeCommand() {}

protected:
  std::unique_ptr<char[]> m_Data;

private:
  FakeArgument m_Argument;
  uint32_t m_DataSize{};
};

} // namespace vulkan
} // namespace gits
