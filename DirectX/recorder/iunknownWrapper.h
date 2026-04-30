// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <comip.h>
#include <unknwn.h>
#include <unordered_set>
#include <vector>
#include <memory>

namespace gits {
namespace DirectX {

struct IIDHash {
  size_t operator()(REFIID riid) const {
    const uint32_t* p = reinterpret_cast<const uint32_t*>(&riid);
    return p[0] ^ p[1] ^ p[2] ^ p[3];
  }
};

const IID IID_IUnknownWrapper = {
    0XACC5279F, 0XF194, 0X4659, {0XAE, 0X9B, 0X40, 0XD0, 0X57, 0XBE, 0XD1, 0XA6}};

class IUnknownWrapper {
public:
  IUnknownWrapper(REFIID riid, IUnknown* object);

  virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** object);
  virtual ULONG STDMETHODCALLTYPE AddRef();
  virtual ULONG STDMETHODCALLTYPE Release();

  template <typename T>
  T* getWrappedObject() {
    return static_cast<T*>(m_Object);
  }

  template <typename T>
  const T* getWrappedObject() const {
    return static_cast<const T*>(m_Object);
  }

  static IUnknown* getRootIUnknown(IUnknown* object);

  IUnknown* getRootIUnknown() {
    return getRootIUnknown(m_Object);
  }

  unsigned getKey() {
    return m_Key;
  }

protected:
  void insertIID(REFIID riid) {
    m_Iids.insert(riid);
  }

private:
  bool isIID(REFIID riid) {
    return m_Iids.find(riid) != m_Iids.end();
  }

private:
  IID m_Iid;
  IUnknown* m_Object;
  unsigned m_Key;
  std::unordered_set<IID, IIDHash> m_Iids;
  std::vector<std::unique_ptr<IUnknownWrapper>> m_SecondaryWrappers;
};

} // namespace DirectX
} // namespace gits
