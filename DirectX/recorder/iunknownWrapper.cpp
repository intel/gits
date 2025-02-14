// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "iunknownWrapper.h"

#include "captureManager.h"
#include "wrapperUtils.h"
#include "commandsCustom.h"
#include "layerAuto.h"
#include "interfaceArgumentUpdaters.h"

#include <guiddef.h>
#include <processthreadsapi.h>

namespace gits {
namespace DirectX {

IUnknownWrapper::IUnknownWrapper(REFIID riid, IUnknown* object) : iid_(riid), object_(object) {
  insertIID(IID_IUnknown);
  key_ = CaptureManager::get().createWrapperKey();
}

HRESULT STDMETHODCALLTYPE IUnknownWrapper::QueryInterface(REFIID riid, void** ppvObject) {

  if (IsEqualIID(riid, IID_IUnknownWrapper)) {
    *ppvObject = this;
    return S_OK;
  }

  HRESULT result{};

  auto& manager = CaptureManager::get();
  if (auto atTopOfStack = AtTopOfStackLocal()) {

    IUnknownQueryInterfaceCommand command(manager.createCommandKey(), GetCurrentThreadId(),
                                          reinterpret_cast<IUnknown*>(this), riid, ppvObject);
    updateInterface(command.object_, this);

    for (Layer* layer : manager.getPreLayers()) {
      layer->pre(command);
    }

    result = object_->QueryInterface(command.riid_.value, command.ppvObject_.value);

    object_->AddRef();
    ULONG ret = object_->Release();

    if (SUCCEEDED(result)) {
      if (isIID(riid)) {
        *ppvObject = this;
        command.ppvObject_.key = key_;
        command.ppvObject_.objectInfo = &objectInfos_;
      } else {
        bool found = false;
        for (auto& wrapper : secondaryWrappers_) {
          if (wrapper->isIID(riid)) {
            *ppvObject = wrapper.get();
            command.ppvObject_.key = wrapper->key_;
            command.ppvObject_.objectInfo = &objectInfos_;
            found = true;
            break;
          }
        }
        if (!found) {
          if (wrapObjectNoStore(riid, ppvObject)) {
            IUnknownWrapper* wrapper = *reinterpret_cast<IUnknownWrapper**>(ppvObject);
            command.ppvObject_.key = wrapper->key_;
            command.ppvObject_.objectInfo = &objectInfos_;
            secondaryWrappers_.emplace_back(wrapper);
          }
        }
      }
    }

    command.result_.value = result;

    for (Layer* layer : manager.getPostLayers()) {
      layer->post(command);
    }
  } else {
    result = object_->QueryInterface(riid, ppvObject);
  }

  return result;
}

ULONG STDMETHODCALLTYPE IUnknownWrapper::AddRef() {

  ULONG result{};

  auto& manager = CaptureManager::get();
  if (auto atTopOfStack = AtTopOfStackLocal()) {

    IUnknownAddRefCommand command(manager.createCommandKey(), GetCurrentThreadId(),
                                  reinterpret_cast<IUnknown*>(this));
    updateInterface(command.object_, this);

    for (Layer* layer : manager.getPreLayers()) {
      layer->pre(command);
    }

    result = object_->AddRef();
    command.result_.value = result;

    for (Layer* layer : manager.getPostLayers()) {
      layer->post(command);
    }
  } else {
    result = object_->AddRef();
  }

  return result;
}

ULONG STDMETHODCALLTYPE IUnknownWrapper::Release() {

  ULONG result{};

  auto& manager = CaptureManager::get();
  if (auto atTopOfStack = AtTopOfStackLocal()) {

    IUnknownReleaseCommand command(manager.createCommandKey(), GetCurrentThreadId(),
                                   reinterpret_cast<IUnknown*>(this));
    updateInterface(command.object_, this);

    for (Layer* layer : manager.getPreLayers()) {
      layer->pre(command);
    }

    object_->AddRef();
    result = object_->Release();

    if (result == 1) {
      CaptureManager::get().removeWrapper(this);
    }

    result = object_->Release();
    command.result_.value = result;

    for (Layer* layer : manager.getPostLayers()) {
      layer->post(command);
    }

    if (result == 0) {
      delete this;
    }
  } else {
    result = object_->Release();
  }

  return result;
}

IUnknown* IUnknownWrapper::getRootIUnknown(IUnknown* object) {
  IUnknown* unknown = nullptr;
  HRESULT hr = object->QueryInterface(IID_PPV_ARGS(&unknown));
  unknown->Release();
  GITS_ASSERT(hr == S_OK);
  return unknown;
}

} // namespace DirectX
} // namespace gits
