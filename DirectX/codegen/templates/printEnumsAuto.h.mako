// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "directx.h"

#include "fastOStream.h"

namespace gits {
namespace DirectX {

%for enum in enums:
FastOStream& operator<<(FastOStream& stream, ${enum.name} value);
FastOStream& operator<<(FastOStream& stream, const ${enum.name}* value);
%endfor

} // namespace DirectX
} // namespace gits
