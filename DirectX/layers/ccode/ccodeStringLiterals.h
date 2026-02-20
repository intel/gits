// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <string>

namespace gits {
namespace DirectX {
namespace ccode {

// File: objects.h
const std::string g_ObjectsH = R"(
#pragma once

#include "directx/directx.h"

)";

// File: commands.h
const std::string g_CommandsH = R"(
#pragma once

#include "objects.h"

void SetupEnvironment();
void TeardownEnvironment();
void StateRestore();
void RunFrames();

)";

// File: commands.cpp
const std::string g_CommandsCpp = R"(
#include "commands.h"
#include "directx/utils.h"

#include <plog/Log.h>
#include <vector>

// DirectX 12 runtime libraries
struct D3D12Context {
  HMODULE d3d12AgilitySdk{nullptr};
};
D3D12Context g_D3D12Context;

void SetupEnvironment() {
  LOG_INFO << "CCode - Preparing DirectX 12 environment...";
  g_D3D12Context.d3d12AgilitySdk = directx::LoadAgilitySdk("D3D12/");
  directx::LoadIntelExtensions();
}

void TeardownEnvironment() {
  LOG_INFO << "CCode - Tearing down DirectX 12 environment...";
  if (g_D3D12Context.d3d12AgilitySdk) {
    FreeLibrary(g_D3D12Context.d3d12AgilitySdk);
    g_D3D12Context.d3d12AgilitySdk = nullptr;
  }
}

)";

// File: commands_X.h
const std::string g_CommandsXCpp = R"(
#include "commands.h"
#include "dataService.h"
#include "windowService.h"
#include "directx/utils.h"
#include "directx/mapTrackingService.h"
#include "directx/descriptorHeapService.h"
#include "directx/gpuAddressService.h"
#include "directx/heapAllocationService.h"
#include "directx/wrappers/ccodeApiWrappers.h"

#include <plog/Log.h>
#include <vector>

)";

} // namespace ccode
} // namespace DirectX
} // namespace gits
