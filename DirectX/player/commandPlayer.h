// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "function.h"
#include "argument.h"

#include <vector>

namespace gits {
namespace DirectX {

class FakeArgument : public CArgument {
public:
  const char* Name() const override {
    return "";
  }
  void Write(CBinOStream& stream) const override {}
  void Read(CBinIStream& stream) override {}
  void Write(CCodeOStream& stream) const {}
};

class CommandPlayer : public CFunction {
public:
  unsigned ArgumentCount() const override {
    return 0;
  }
  CArgument& Argument(unsigned idx) override {
    return argument_;
  }
  const CArgument* Return() const override {
    return nullptr;
  }
  unsigned ResultCount() const override {
    return 0;
  }
  CArgument& Result(unsigned idx) override {
    return argument_;
  }
  CLibrary::TId LibraryId() const override {
    return CLibrary::TId::ID_DirectX;
  }

  void Read(CBinIStream& stream) override {
    unsigned size = 0;
    stream.read(reinterpret_cast<char*>(&size), sizeof(unsigned));
    data_ = HeapArray<char>(size);
    stream.read(data_.data(), size);

    decodeCommand();
  }

  uint64_t Size() const override {
    return sizeof(unsigned) + data_.size();
  }

protected:
  virtual void decodeCommand() {}

protected:
  HeapArray<char> data_;

private:
  FakeArgument argument_;
};

} // namespace DirectX
} // namespace gits
