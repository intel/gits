// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "wrapperUtils.h"
#include "arguments.h"
#include "wrapperCreatorsAuto.h"
#include "captureManager.h"
#include "exception.h"
#include "log.h"
#include "to_string/toStr.h"

namespace gits {
namespace DirectX {

bool wrapObject(REFIID riid, void** object) {

  if (object && *object) {
    auto it = g_wrapperCreatorsDispatchTable.find(riid);
    if (it != g_wrapperCreatorsDispatchTable.end()) {
      it->second(riid, object);
      return true;
    }
    LOG_ERROR << "wrapObject failed: no wrapper creator for " << toStr(riid);
    return false;
  }
  return true;
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
    auto& manager = CaptureManager::Get();
    manager.DecrementGlobalStackDepth();
    manager.DecrementLocalStackDepth();
  } catch (...) {
    topmost_exception_handler("AtTopOfStackGlobal::~AtTopOfStackGlobal()");
  }
}

AtTopOfStackGlobal::operator bool() {
  auto& manager = CaptureManager::Get();
  unsigned globalStackDepth = manager.IncrementGlobalStackDepth();
  unsigned localStackDepth = manager.IncrementLocalStackDepth();
  return globalStackDepth == 1 && localStackDepth == 1;
}

AtTopOfStackLocal::~AtTopOfStackLocal() {
  try {
    CaptureManager::Get().DecrementLocalStackDepth();
  } catch (...) {
    topmost_exception_handler("AtTopOfStackLocal::~AtTopOfStackLocal()");
  }
}

AtTopOfStackLocal::operator bool() {
  unsigned localStackDepth = CaptureManager::Get().IncrementLocalStackDepth();
  return localStackDepth == 1;
}

NestedCaptureScope::NestedCaptureScope() {
  try {
    CaptureManager::Get().DecrementLocalStackDepth();
  } catch (...) {
    topmost_exception_handler("NestedCaptureScope::NestedCaptureScope()");
  }
}

NestedCaptureScope::~NestedCaptureScope() {
  try {
    CaptureManager::Get().IncrementLocalStackDepth();
  } catch (...) {
    topmost_exception_handler("NestedCaptureScope::~NestedCaptureScope()");
  }
}

ObjectKey getWrapperKey(const IUnknown* object) {
  if (object) {
    IUnknownWrapper* wrapper = nullptr;
    if (SUCCEEDED(const_cast<IUnknown*>(object)->QueryInterface(
            IID_IUnknownWrapper, reinterpret_cast<void**>(&wrapper)))) {
      return wrapper->GetKey();
    }
  }
  return ObjectKey{};
}

} // namespace DirectX
} // namespace gits
