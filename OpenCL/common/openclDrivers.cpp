// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   openclDrivers.cpp
*
* @brief
*/
#include "openclDrivers.h"
#include "openclArgumentsAuto.h"
#include "openclTools.h"
#include "opencl_apis_iface.h"

#include <tuple>
#include <type_traits>

#include "gits.h"

#include <filesystem>

namespace gits {
namespace OpenCL {
void COclDriver::Initialize() {
  if (_initialized) {
    return;
  }
  std::filesystem::path path = Configurator::Get().common.shared.libClPath;
  _lib = std::make_unique<SharedLibrary>(path.string());
  _initialized = _lib->getHandle() != nullptr;
  CGits::Instance().apis.UseApiComputeIface(std::make_shared<OpenCL::OpenCLApi>());
}

COclDriver::~COclDriver() {
  _initialized = false;
}

COclDriver drvOcl;
} // namespace OpenCL
} // namespace gits
