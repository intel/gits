// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "platform.h"
#ifdef GITS_PLATFORM_WINDOWS
#include <windows.h>
#endif

#include "gits.h"
#include "playerOptions.h"
#include "log.h"
#include "config.h"
#include "getopt_.h"
#include "pragmas.h"
#include "exception.h"
#include "tools.h"
#include "lua_bindings.h"
#include "ptblLibrary.h"

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <filesystem>
