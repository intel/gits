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
#define APP_VERSION   "1.0.1"

inline const char* RELEASE_NOTES = R"(
Release Notes v1.0.1
====================
- Initial release
- Control playback of GITS-Streams
  - Generate and show structured API trace file
  - Show Metadata of streams
- Run live workloads with GITS interposer
- Run GITS-Modules with GITS-Streams or live workloads
- RTSR GITS-Module supports:
  - Work with Pipelines:
    - Inspect all pipelines (gfx, comp, rayt), shaders, pipeline properties
    - Edit & replace shader source code
  - Performance metrics:
    - Frame-to-frame performance timings
    - View perf stats over (multiple) selected frames:
      pipeline occurances, min/max/total duration
)";
