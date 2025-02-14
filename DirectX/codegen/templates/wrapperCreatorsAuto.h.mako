// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${header}

#pragma once

#include "wrapperUtils.h"

#include <unordered_map>
#include <functional>
#include <guiddef.h>

namespace gits {
namespace DirectX {

extern const IID IID_ID3DDestructionNotifier;

extern const std::unordered_map<IID, std::function<void(REFIID, void**)>, IIDHash>
    g_wrapperCreatorsDispatchTable;

extern const std::unordered_map<IID, std::function<void(REFIID, void**)>, IIDHash>
    g_wrapperCreatorsNoStoreDispatchTable;

} // namespace DirectX
} // namespace gits
