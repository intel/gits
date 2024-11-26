// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

${AUTO_GENERATED_HEADER}

% for token in vk_functions:
case ${make_id(token.name, token.version)}:
  return new ${make_cname(token.name, token.version)};
% endfor
