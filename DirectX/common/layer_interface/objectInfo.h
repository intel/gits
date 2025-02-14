// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <map>
#include <memory>
#include <mutex>

namespace gits {
namespace DirectX {

class Layer;

class ObjectInfo {
public:
  virtual ~ObjectInfo() {}
};

class ObjectInfos {
public:
  virtual ~ObjectInfos() {}

  virtual void addObjectInfo(Layer* layer, ObjectInfo* objectInfo) = 0;
  virtual ObjectInfo* getObjectInfo(Layer* layer) = 0;
  virtual std::mutex* getMutex() {
    return nullptr;
  }

  template <typename T>
  T* getObjectInfo(Layer* layer) {
    return static_cast<T*>(getObjectInfo(layer));
  }
};

} // namespace DirectX
} // namespace gits
