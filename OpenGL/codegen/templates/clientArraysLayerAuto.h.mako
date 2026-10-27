// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${AUTO_GENERATED_HEADER}

#pragma once

#include "layerAuto.h"

namespace gits::opengl {

// Pre layer hooking every draw classified by client_array_draws.py. Each hook hands the draw's
// vertex bounds to ClientArraysService, which records a ClientArraysMetaCommand under the spare
// key of the draw's key pair (see generator_recorder.py's PRE_HOOK_KEY_PAIR_COMMANDS), so the
// arrays are re-specified on replay before the draw reads them.
class ClientArraysLayer : public Layer {
public:
  ClientArraysLayer() : Layer("ClientArraysLayer") {}

  using Layer::Post;
  using Layer::Pre;

% for name in client_array_draws:
  void Pre(${name}Command& command) override;
% endfor
};

} // namespace gits::opengl
