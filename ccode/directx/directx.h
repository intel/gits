// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <windows.h>
#include <initguid.h>
#include <wrl/client.h>

#include <d3d12.h>
#include <dxgi.h>
#include <dxgidebug.h>
#include <dxgi1_2.h>
#include <dxgi1_3.h>
#include <dxgi1_4.h>
#include <dxgi1_5.h>
#include <dxgi1_6.h>

// IntelExtensions
#define INTC_IGDEXT_D3D12
#include <igdext.h>

template <typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;
