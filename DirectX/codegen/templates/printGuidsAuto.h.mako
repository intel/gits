// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include <guiddef.h>
#include <initguid.h>
#include <string>
#include "fastOStream.h"

namespace gits {
namespace DirectX {

FastOStream& operator<<(FastOStream& stream, REFIID riid);

} // namespace DirectX
} // namespace gits
