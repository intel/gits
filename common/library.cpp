// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "library.h"
#include "streams.h"

namespace gits {

CLibrary::CLibrary(TId id, std::function<CState*()> func)
    : _id(static_cast<uint16_t>(id)), _func(std::move(func)) {}

CLibrary::~CLibrary() {}

CLibrary::TId CLibrary::Id() const {
  return static_cast<TId>(*_id);
}

CState* CLibrary::StateCreate() const {
  if (!_func) {
    return nullptr;
  }
  return _func();
}

} // namespace gits
