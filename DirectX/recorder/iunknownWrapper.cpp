// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
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

IUnknownWrapper::IUnknownWrapper(REFIID riid, IUnknown* object) : m_Iid(riid), m_Object(object) {
  InsertIID(IID_IUnknown);
  m_Key = CaptureManager::get().createWrapperKey();
}

HRESULT STDMETHODCALLTYPE IUnknownWrapper::QueryInterface(REFIID riid, void** ppvObject) {

  if (IsEqualIID(riid, IID_IUnknownWrapper)) {
    *ppvObject = this;
    return S_OK;
  }

  HRESULT result{};

  auto& manager = CaptureManager::get();
  if (auto atTopOfStack = AtTopOfStackLocal()) {

    IUnknownQueryInterfaceCommand command(GetCurrentThreadId(), reinterpret_cast<IUnknown*>(this),
                                          riid, ppvObject);
    UpdateInterface(command.m_Object, this);

    for (Layer* layer : manager.GetPreLayers()) {
      layer->Pre(command);
    }

    command.Key = manager.createCommandKey();
    result = m_Object->QueryInterface(command.m_riid.Value, command.m_ppvObject.Value);

    m_Object->AddRef();
    ULONG ret = m_Object->Release();

    if (SUCCEEDED(result)) {
      if (IsIID(riid)) {
        *ppvObject = this;
        command.m_ppvObject.Key = m_Key;
      } else {
        bool found = false;
        for (auto& wrapper : m_SecondaryWrappers) {
          if (wrapper->IsIID(riid)) {
            *ppvObject = wrapper.get();
            command.m_ppvObject.Key = wrapper->m_Key;
            found = true;
            break;
          }
        }
        if (!found) {
          if (wrapObjectNoStore(riid, ppvObject)) {
            IUnknownWrapper* wrapper = *reinterpret_cast<IUnknownWrapper**>(ppvObject);
            command.m_ppvObject.Key = wrapper->m_Key;
            m_SecondaryWrappers.emplace_back(wrapper);
          }
        }
      }
    }

    command.m_Result.Value = result;

    for (Layer* layer : manager.GetPostLayers()) {
      layer->Post(command);
    }
  } else {
    result = m_Object->QueryInterface(riid, ppvObject);
  }

  return result;
}

ULONG STDMETHODCALLTYPE IUnknownWrapper::AddRef() {

  ULONG result{};

  auto& manager = CaptureManager::get();
  if (auto atTopOfStack = AtTopOfStackLocal()) {

    IUnknownAddRefCommand command(GetCurrentThreadId(), reinterpret_cast<IUnknown*>(this));
    UpdateInterface(command.m_Object, this);

    for (Layer* layer : manager.GetPreLayers()) {
      layer->Pre(command);
    }

    command.Key = manager.createCommandKey();
    result = m_Object->AddRef();
    command.m_Result.Value = result;

    for (Layer* layer : manager.GetPostLayers()) {
      layer->Post(command);
    }
  } else {
    result = m_Object->AddRef();
  }

  return result;
}

ULONG STDMETHODCALLTYPE IUnknownWrapper::Release() {

  ULONG result{};

  auto& manager = CaptureManager::get();
  if (auto atTopOfStack = AtTopOfStackLocal()) {

    IUnknownReleaseCommand command(GetCurrentThreadId(), reinterpret_cast<IUnknown*>(this));
    UpdateInterface(command.m_Object, this);

    for (Layer* layer : manager.GetPreLayers()) {
      layer->Pre(command);
    }

    command.Key = manager.createCommandKey();
    m_Object->AddRef();
    result = m_Object->Release();

    if (result == 1) {
      CaptureManager::get().removeWrapper(this);
    }

    result = m_Object->Release();
    command.m_Result.Value = result;

    for (Layer* layer : manager.GetPostLayers()) {
      layer->Post(command);
    }

    if (result == 0) {
      delete this;
    }
  } else {
    result = m_Object->Release();
  }

  return result;
}

IUnknown* IUnknownWrapper::GetRootIUnknown(IUnknown* object) {
  IUnknown* unknown = nullptr;
  HRESULT hr = object->QueryInterface(IID_PPV_ARGS(&unknown));
  unknown->Release();
  GITS_ASSERT(hr == S_OK);
  return unknown;
}

} // namespace DirectX
} // namespace gits
