// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
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
#include "MemorySniffer.h"

namespace gits {
void ApisIface::UseApi3dIface(std::shared_ptr<Api3d> iface) {
  assert(iface.get() != 0);
  if (iface->Type() == A3D) {
    if (_3d.get() == nullptr) {
      _3d = iface;
    } else if (iface->Api() != _3d->Api()) {
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }
  } else {
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
}

void ApisIface::UseApiComputeIface(std::shared_ptr<ApiCompute> iface) {
  assert(iface.get() != 0);
  if (iface->Type() == ACompute) {
    if (_compute.get() == nullptr) {
      _compute = iface;
    } else if (iface->Api() != _compute->Api()) {
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }
  } else {
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
}

bool ApisIface::ApiCompute::MemorySnifferInstall() const {
  static auto installed = MemorySniffer::Install();
  if (installed) {
    MemorySniffer::Get().SetWholeMemoryRegionUnveiling();
  }
  return installed;
}

void ApisIface::ApiCompute::MemorySnifferProtect(PagedMemoryRegionHandle& handle) const {
  if (handle != nullptr && *handle != nullptr && !(**handle).Protected() &&
      !MemorySniffer::Get().Protect(handle)) {
    Log(WARN) << "Protecting memory region: " << (**handle).BeginAddress() << " - "
              << (**handle).EndAddress() << " FAILED!.";
  }
}

void ApisIface::ApiCompute::MemorySnifferUnProtect(PagedMemoryRegionHandle& handle) const {
  if (handle != nullptr && *handle != nullptr && (**handle).Protected() &&
      !MemorySniffer::Get().UnProtect(handle)) {
    Log(WARN) << "Unprotecting memory region: " << (**handle).BeginAddress() << " - "
              << (**handle).EndAddress() << " FAILED!.";
  }
}

void ApisIface::ApiCompute::EnableMemorySnifferForPointer(void* ptr,
                                                          const size_t& size,
                                                          PagedMemoryRegionHandle& handle) const {
  if (handle == nullptr) {
    handle = MemorySniffer::Get().CreateRegion(ptr, size);
  }
  if (handle == nullptr || *handle == nullptr) {
    Log(ERR) << "MemorySniffer setup for usm pointer: " << ptr << " failed.";
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }
  MemorySnifferProtect(handle);
}
} // namespace gits
