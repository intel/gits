// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "helperCL.h"

namespace api {
#define DEFINE_VARIABLE_OCL_FUNCTION(a, b, c, e, f) a(STDCALL*& b) c = gits::OpenCL::drvOcl.b;
OCL_FUNCTIONS(DEFINE_VARIABLE_)
} // namespace api
