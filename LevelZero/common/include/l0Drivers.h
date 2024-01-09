// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "dynamic_linker.h"
#include "platform.h"

#include "l0Header.h"

namespace gits {
namespace l0 {
class CDriver : public ze_dispatch_table_t {
public:
  CDriver();
  CDriver(const CDriver& other) = delete;
  CDriver& operator=(const CDriver& other) = delete;
  ~CDriver();
  void Initialize();
  bool OpenLibrary(const std::string& path);
  dl::SharedObject Library() const {
    return lib_;
  }
  ze_dispatch_table_t original = {};
  ze_dispatch_table_t inject = {};

private:
  bool initialized_ = false;
  dl::SharedObject lib_ = nullptr;
};

extern CDriver drv;
} // namespace l0
} // namespace gits