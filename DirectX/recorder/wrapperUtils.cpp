// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "wrapperUtils.h"
#include "wrapperCreatorsAuto.h"
#include "captureManager.h"

namespace gits {
namespace DirectX {

void wrapObject(REFIID riid, void** object) {

  if (object && *object) {
    auto it = g_wrapperCreatorsDispatchTable.find(riid);
    GITS_ASSERT(it != g_wrapperCreatorsDispatchTable.end());
    if (it != g_wrapperCreatorsDispatchTable.end()) {
      it->second(riid, object);
    }
  }
}

bool wrapObjectNoStore(REFIID riid, void** object) {

  if (object && *object) {
    auto it = g_wrapperCreatorsNoStoreDispatchTable.find(riid);
    if (it != g_wrapperCreatorsNoStoreDispatchTable.end()) {
      it->second(riid, object);
      return true;
    }
  }
  return false;
}

AtTopOfStackGlobal::~AtTopOfStackGlobal() {
  try {
    auto& manager = CaptureManager::get();
    manager.decrementGlobalStackDepth();
    manager.decrementLocalStackDepth();
  } catch (...) {
    topmost_exception_handler("AtTopOfStackGlobal::~AtTopOfStackGlobal()");
  }
}

AtTopOfStackGlobal::operator bool() {
  auto& manager = CaptureManager::get();
  unsigned globalStackDepth = manager.incrementGlobalStackDepth();
  unsigned localStackDepth = manager.incrementLocalStackDepth();
  return globalStackDepth == 1 && localStackDepth == 1;
}

AtTopOfStackLocal::~AtTopOfStackLocal() {
  try {
    CaptureManager::get().decrementLocalStackDepth();
  } catch (...) {
    topmost_exception_handler("AtTopOfStackLocal::~AtTopOfStackLocal()");
  }
}

AtTopOfStackLocal::operator bool() {
  unsigned localStackDepth = CaptureManager::get().incrementLocalStackDepth();
  return localStackDepth == 1;
}

unsigned getWrapperKey(const IUnknown* object) {
  if (object) {
    IUnknownWrapper* wrapper = nullptr;
    if (SUCCEEDED(const_cast<IUnknown*>(object)->QueryInterface(
            IID_IUnknownWrapper, reinterpret_cast<void**>(&wrapper)))) {
      return wrapper->getKey();
    }
  }
  return 0;
}

} // namespace DirectX
} // namespace gits
