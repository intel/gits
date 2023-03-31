// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "platform.h"
#include "openglTypes.h"

namespace gits {

void GetAttribsFromPFD(const PIXELFORMATDESCRIPTOR* pfd, int size, int* attribs, int* values);
void GetPFDFromAttribs(int size, const int* attribs, const int* values, PIXELFORMATDESCRIPTOR* pfd);

} // namespace gits
