// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#ifndef GITS_API_H
#define GITS_API_H
#include "platform.h"

#define GITS_API_OCL
#define GITS_API_VK
#define GITS_API_OGL
#if defined GITS_PLATFORM_WINDOWS || defined GITS_PLATFORM_X11
#define GITS_API_L0
#endif

#endif
