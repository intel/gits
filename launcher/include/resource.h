// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#define APP_ICON      101
#define APP_FILE_ICON 102
#define APP_VERSION   "0.1.9"

inline const char* RELEASE_NOTES = R"(
Release Notes v0.1.9
====================
- Added ability to send GITS log via email

Release Notes v0.1.8.1
====================
- Added Resource Dump retry mode
- Added CCode generation mechanism
- Added save button for GITS log
- Added drag and drop support for target executable, config and stream files

Release Notes v0.1.8
====================
- Added release notes window
- Added system for default paths for screenshots & trace based on playback stream
- New default path for subcapture output based on original stream path
- Added option to enable custom executable name for gitsPlayer

- New system to determine base gits installation path
- Added buttons to use configuration files from base gits installation
- Revised system to select capture API & derive required configuration

- Enabled DirectX HUD by default in playback
- During subcapture: playback stops at the end subcapture frame
- Read & use metadata when loading playback stream

- Added DirectX plugins panel
- Added Resource Dump panel

- Removed option to specify custom gitsPlayer.exe
)";
