// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   keyEvents.h
 *
 */

#pragma once
#include <string>
#include <map>
#include <vector>
#include <cstdint>

typedef unsigned int uint;

namespace gits {
void InitKeyMap(std::map<std::string, unsigned>& keyMap);
bool IsKeyPressed(unsigned key);
bool AreAllKeysPressed(std::vector<unsigned> keys);
uint32_t GetKeyVal(const std::string& str);
} // namespace gits
