// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "iunknownWrapper.h"
#include "arguments.h"
#include "directx.h"
#include "tools_lite.h"
#include "gits.h"

#include <guiddef.h>
#include <stdint.h>
#include <vector>

namespace gits {
namespace DirectX {

void wrapObject(REFIID riid, void** object);
bool wrapObjectNoStore(REFIID riid, void** object);

class AtTopOfStackGlobal : public gits::noncopyable {
public:
  ~AtTopOfStackGlobal();
  operator bool();
};

class AtTopOfStackLocal : public gits::noncopyable {
public:
  ~AtTopOfStackLocal();
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
