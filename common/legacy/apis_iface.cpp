// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   apis_iface.cpp
 *
 * @brief Definition of classes that passes generic API data between lower, generic layers and specialized APIs layers.
 *
 */

#include "apis_iface.h"
#include "gits.h"

namespace gits {
void ApisIface::UseApi3dIface(std::shared_ptr<Api3d> iface) {
  assert(iface.get() != 0);
  if (_3d.get() == nullptr) {
    _3d = std::move(iface);
  } else if (iface->Api() != _3d->Api()) {
    LOG_ERROR << "Only one 3D API can be recorded at a time.";
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
}
} // namespace gits
