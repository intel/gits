// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "config.h"
#include "keyEvents.h"

#ifndef BUILD_FOR_CCODE
#include "diagnostic.h"
#include "lua_bindings.h"
#include "configYamlTemplates.h"
#endif

#include <regex>
#include <chrono>
#include <filesystem>
#include <fstream>

#ifdef GITS_PLATFORM_WINDOWS
#include <process.h>
#define getpid _getpid
#endif
