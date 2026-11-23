// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once
#include "arguments.h"

#include "keyUtils.h"

namespace gits {
namespace DirectX {

class ExecutionSerializationKeyAllocator {
public:
  CommandKey GetUniqueCommandKey() {
    return ++m_CommandKey;
  }
  void RemapCommandKey(CommandKey& key) {
    if (key & EXECUTION_SERIALIZATION_KEY_MASK) {
      key = GetUniqueCommandKey();
    }
  }

private:
  CommandKey m_CommandKey{EXECUTION_SERIALIZATION_KEY_MASK};
};

} // namespace DirectX
} // namespace gits
