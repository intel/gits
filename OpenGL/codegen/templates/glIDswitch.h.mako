// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

% for name, token_versions in gl_functions.items():
% for token in token_versions:
case ${make_id(name, token.version)}:
  return new ${make_cname(name, token.version)};
% endfor
% endfor
