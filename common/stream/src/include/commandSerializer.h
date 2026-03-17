// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <memory>

namespace gits {
namespace stream {

class CommandSerializer {
public:
  virtual ~CommandSerializer() {}

  virtual unsigned Id() const = 0;
  uint64_t Size() const {
    return m_DataSize;
  }
  const char* Data() const {
    return m_Data.get();
  }

protected:
  std::unique_ptr<char[]> m_Data;
  uint64_t m_DataSize{};
};

} // namespace stream
} // namespace gits
