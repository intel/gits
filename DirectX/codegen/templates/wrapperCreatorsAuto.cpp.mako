// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#include "wrapperCreatorsAuto.h"

#include "wrappersAuto.h"
#include "captureManager.h"

namespace gits {
namespace DirectX {

const IID IID_ID3DDestructionNotifier = {
    0XA06EB39A, 0X50DA, 0X425B, {0X8C, 0X31, 0X4E, 0XEC, 0XD6, 0XC2, 0X70, 0XF3}};

void wrapIUnknown(REFIID riid, void** object) {
  IUnknownWrapper* wrapper = CaptureManager::get().findWrapper(*reinterpret_cast<IUnknown**>(object));
  if (!wrapper) {
    wrapper = new IUnknownWrapper(riid, *reinterpret_cast<IUnknown**>(object));
    CaptureManager::get().addWrapper(wrapper);
  }
  *object = wrapper;
}

void wrapIUnknownNoStore(REFIID riid, void** object) {
  *object = new IUnknownWrapper(riid, *reinterpret_cast<IUnknown**>(object));
}

%for interface in interfaces:
void wrap${interface.name}(REFIID riid, void** object) {
  IUnknownWrapper* wrapper = CaptureManager::get().findWrapper(*reinterpret_cast<IUnknown**>(object));
  if (!wrapper) {
    wrapper = new ${interface.latest_interface}Wrapper(riid, *reinterpret_cast<IUnknown**>(object));
    CaptureManager::get().addWrapper(wrapper);
  }
  *object = wrapper;
}

void wrap${interface.name}NoStore(REFIID riid, void** object) {
  *object = new ${interface.latest_interface}Wrapper(riid, *reinterpret_cast<IUnknown**>(object));
}

%endfor

const std::unordered_map<IID, std::function<void(REFIID, void**)>, IIDHash> g_wrapperCreatorsDispatchTable {

  { IID_IUnknown, wrapIUnknown },
  %for interface in interfaces:
  { __uuidof(${interface.name}), wrap${interface.name} },
  %endfor
};

const std::unordered_map<IID, std::function<void(REFIID, void**)>, IIDHash> g_wrapperCreatorsNoStoreDispatchTable {
  
  { IID_IUnknown, wrapIUnknownNoStore },
  %for interface in interfaces:
  { __uuidof(${interface.name}), wrap${interface.name}NoStore },
  %endfor
};

} // namespace DirectX
} // namespace gits
