// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "library.h"

namespace gits {
namespace DirectX {

class DirectXLibrary : public gits::CLibrary {
public:
  static DirectXLibrary& Get();
  DirectXLibrary(gits::CLibrary::state_creator_t stc = gits::CLibrary::state_creator_t())
      : gits::CLibrary(ID_DirectX, std::move(stc)) {}
  ~DirectXLibrary() {}

  gits::CFunction* FunctionCreate(unsigned type) const override;

  const char* Name() const override {
    return "DirectX";
  }
};

} // namespace DirectX
} // namespace gits
