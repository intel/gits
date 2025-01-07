// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================


#include "helperCL.h"

namespace api {
%for name, func in without_field(functions, 'version').items():
${get_return_type(func)} \
(STDCALL*& ${name}) \
(${make_params(func, with_types=True, one_line=True)}) \
= gits::OpenCL::drvOcl.${name};
%endfor
} // namespace api
