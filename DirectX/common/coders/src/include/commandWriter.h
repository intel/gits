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
namespace DirectX {

class CommandWriter : public CToken {
public:
  void Run() override {}

private:
  void Read(CBinIStream& stream) override {}

  void Write(CBinOStream& stream) const {
    stream.write(reinterpret_cast<const char*>(&dataSize_), sizeof(unsigned));
    stream.write(data_.get(), dataSize_);
  }
  uint64_t Size() const override {
    return sizeof(unsigned) + dataSize_;
  }

protected:
  std::unique_ptr<char[]> data_;
  unsigned dataSize_{};
};

} // namespace DirectX
} // namespace gits
