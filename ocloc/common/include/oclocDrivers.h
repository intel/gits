// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "dynamic_linker.h"

#include "oclocHeader.h"

namespace gits {
namespace ocloc {
class CDriver {
public:
  CDriver();
  CDriver(const CDriver& other) = delete;
  CDriver& operator=(const CDriver& other) = delete;
  ~CDriver();
  void Initialize();
  dl::SharedObject Library() const {
    return lib_;
  }
  pfn_oclocInvoke orig_oclocInvoke = nullptr;
  pfn_oclocInvoke oclocInvoke;
  pfn_oclocFreeOutput orig_oclocFreeOutput = nullptr;
  pfn_oclocFreeOutput oclocFreeOutput;

private:
  bool initialized_ = false;
  dl::SharedObject lib_ = nullptr;
};

extern CDriver drv;
} // namespace ocloc
} // namespace gits
