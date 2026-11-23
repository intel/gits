// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   tools.h
 *
 * @brief Common tools for GITS project.
 *
 */

#pragma once

#include "tools_lite.h"
#include "log.h"
#include "exception.h"
#include "platform.h"
#include "dynamic_linker.h"

#include <set>
#include <map>
#include <deque>
#include <filesystem>
#include <climits>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <vector>
#include <functional>

#ifdef GITS_PLATFORM_LINUX
#include <sys/types.h>
#endif

#ifdef GITS_PLATFORM_X11
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#endif

#include "nlohmann/json.hpp"

#ifndef _DEBUG
#ifndef NDEBUG
#define NDEBUG 1
#endif
#endif
#include <cassert>

#include "configurationLib.h"

namespace gits {
// Returns XXH32 of the data in the low 32 bits and the data size in the high 32 bits.
uint64_t ComputeHash(const void* data, size_t size);

std::string CommandOutput(const std::string& command, bool isRecorder);

std::string GetWindowsProcessName(int processID);
#if defined GITS_PLATFORM_LINUX
std::string GetLinuxProcessName(pid_t processID);
#endif
} // namespace gits
