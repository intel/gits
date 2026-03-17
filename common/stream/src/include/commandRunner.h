// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

namespace gits {
namespace stream {

class CommandRunner {
public:
  virtual ~CommandRunner() {}

  void DecodeData(char* data) {
    m_Data = data;
    DecodeCommand();
  }
  virtual void Run() = 0;

protected:
  virtual void DecodeCommand() {}

protected:
  char* m_Data{};
};

} // namespace stream
} // namespace gits
