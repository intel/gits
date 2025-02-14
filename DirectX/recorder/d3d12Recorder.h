// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "d3d12RecorderInterface.h"

namespace gits {
namespace DirectX {

class RecorderWrapper : public IRecorderWrapper {
public:
  void ExchangeDXGIDispatchTables(const DXGIDispatchTable& systemTable,
                                  DXGIDispatchTable& wrapperTable) override;
  void ExchangeD3D12DispatchTables(const D3D12DispatchTable& systemTable,
                                   D3D12DispatchTable& wrapperTable) override;
};

} // namespace DirectX
} // namespace gits
