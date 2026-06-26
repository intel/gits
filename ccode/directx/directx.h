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
#include <d3d12sdklayers.h>
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

#include <dstorage.h>
#include "directStorageGuids.h"

#include <xess/xess.h>
#include <xess/xess_d3d12.h>
#include <xell/xell.h>
#include <xell/xell_d3d12.h>
#include <xess_fg/xefg_swapchain.h>
#include <xess_fg/xefg_swapchain_d3d12.h>
#include <xess_fg/xefg_swapchain_debug.h>

#include "directStorageService.h"

template <typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;
