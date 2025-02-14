// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "objectInfo.h"

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

class ObjectInfosCapture : public ObjectInfos {
public:
  void addObjectInfo(Layer* layer, ObjectInfo* objectInfo) override {

    std::lock_guard<std::mutex> lock(mutex_);
    infos_[layer].reset(objectInfo);
  }

  std::mutex* getMutex() override {
    return &mutex_;
  }

  ObjectInfo* getObjectInfo(Layer* layer) override {

    auto it = infos_.find(layer);
    if (it == infos_.end()) {
      return nullptr;
    }
    return it->second.get();
  }

private:
  std::map<Layer*, std::unique_ptr<ObjectInfo>> infos_;
  std::mutex mutex_;
};

class IUnknownWrapper {
public:
  IUnknownWrapper(REFIID riid, IUnknown* object);

  virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** object);
  virtual ULONG STDMETHODCALLTYPE AddRef();
  virtual ULONG STDMETHODCALLTYPE Release();

  template <typename T>
  T* getWrappedObject() {
    return static_cast<T*>(object_);
  }

  template <typename T>
  const T* getWrappedObject() const {
    return static_cast<const T*>(object_);
  }

  static IUnknown* getRootIUnknown(IUnknown* object);

  IUnknown* getRootIUnknown() {
    return getRootIUnknown(object_);
  }

  unsigned getKey() {
    return key_;
  }

  ObjectInfos* getObjectInfos() {
    return &objectInfos_;
  }

protected:
  void insertIID(REFIID riid) {
    iids_.insert(riid);
  }

private:
  bool isIID(REFIID riid) {
    return iids_.find(riid) != iids_.end();
  }

private:
  IID iid_;
  IUnknown* object_;
  unsigned key_;
  std::unordered_set<IID, IIDHash> iids_;
  std::vector<std::unique_ptr<IUnknownWrapper>> secondaryWrappers_;
  ObjectInfosCapture objectInfos_;
};

} // namespace DirectX
} // namespace gits
