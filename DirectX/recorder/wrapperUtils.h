// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "iunknownWrapper.h"
#include "arguments.h"
#include "directx.h"

#include <guiddef.h>
#include <stdint.h>
#include <vector>

namespace gits {
namespace DirectX {

void wrapObject(REFIID riid, void** object);
bool wrapObjectNoStore(REFIID riid, void** object);

class AtTopOfStackGlobal {
public:
  AtTopOfStackGlobal() = default;
  ~AtTopOfStackGlobal();
  AtTopOfStackGlobal(const AtTopOfStackGlobal&) = delete;
  AtTopOfStackGlobal& operator=(const AtTopOfStackGlobal&) = delete;

  operator bool();
};

class AtTopOfStackLocal {
public:
  AtTopOfStackLocal() = default;
  ~AtTopOfStackLocal();
  AtTopOfStackLocal(const AtTopOfStackLocal&) = delete;
  AtTopOfStackLocal& operator=(const AtTopOfStackLocal&) = delete;

  operator bool();
};

unsigned getWrapperKey(const IUnknown* object);

template <typename T>
void getWrapperKeys(std::vector<unsigned>& keys, T** objects) {
  for (int i = 0; i < keys.size(); ++i) {
    keys[i] = getWrapperKey(reinterpret_cast<IUnknown*>(objects[i]));
  }
}

} // namespace DirectX
} // namespace gits
