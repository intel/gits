// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "keyUtils.h"

namespace gits {
namespace DirectX {

class ExecutionSerializationKeyAllocator {
public:
  unsigned GetUniqueCommandKey() {
    return ++m_CommandKey;
  }
  void RemapCommandKey(unsigned& key) {
    if (key & EXECUTION_SERIALIZATION_KEY_MASK) {
      key = GetUniqueCommandKey();
    }
  }

private:
  unsigned m_CommandKey{EXECUTION_SERIALIZATION_KEY_MASK};
};

} // namespace DirectX
} // namespace gits
