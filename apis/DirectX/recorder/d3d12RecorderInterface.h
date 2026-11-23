// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "functionDispatchTables.h"

namespace gits {
namespace DirectX {

class IRecorderWrapper {
public:
  virtual void ExchangeDXGIDispatchTables(const DXGIDispatchTable& systemTable,
                                          DXGIDispatchTable& wrapperTable) = 0;
  virtual void ExchangeD3D12DispatchTables(const D3D12DispatchTable& systemTable,
                                           D3D12DispatchTable& wrapperTable) = 0;
};

} // namespace DirectX
} // namespace gits

extern "C" {
gits::DirectX::IRecorderWrapper* __stdcall GITSRecorderD3D12();
}
