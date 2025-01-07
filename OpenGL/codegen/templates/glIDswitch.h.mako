// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

${AUTO_GENERATED_HEADER}


% for name, token_versions in gl_functions.items():
% for token in token_versions:
case ${make_id(name, token.version)}:
  return new ${make_cname(name, token.version)};
% endfor
% endfor
