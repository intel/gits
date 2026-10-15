// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${AUTO_GENERATED_HEADER}

#pragma once

#include "commandsAuto.h"
#include "layerBase.h"

namespace gits::opengl2 {

// Adds a typed Pre(FooCommand&)/Post(FooCommand&) override pair per enabled command on top of
// LayerBase's hand-written, generic Pre(Command&)/Post(Command&) hooks (layerBase.h) - see
// modern-opengl-backend-architecture.md's "Use enabled-only typed layers with generic
// fallbacks" decision and docs/development/opengl2-layer-architecture-alignment-plan.md for
// the full rationale. Mirrors Vulkan/common/layer_interface/layerAuto.h's shape, minus
// per-platform #ifdef guards (GL/WGL commands have no Vulkan-style platform-conditional subset).
//
// Every default body below simply forwards to the generic hook via an explicit static_cast to
// Command& (never recurses back into itself: the argument's STATIC type after the cast is
// exactly Command&, so overload resolution picks LayerBase::Pre/Post - the only overload that
// matches - not one of this class's own more-derived typed overloads). EncoderLayer
// (encoderLayerAuto.h) is the only concrete Layer subclass that overrides any of these typed
// hooks today, and the generated wrapper/runner code (recorderWrappersAuto.cpp.mako,
// commandRunnersAuto.cpp.mako) is what actually dispatches through them.
class Layer : public LayerBase {
public:
  using LayerBase::LayerBase;

  // Bring LayerBase's generic overloads back into this class's overload set - declaring the
  // same-named Pre/Post below would otherwise hide them per C++'s member name-hiding-across-
  // scopes rule, making Pre(Command&)/Post(Command&) uncallable through a Layer&/Layer*.
  using LayerBase::Pre;
  using LayerBase::Post;

% for cmd in commands:
% if cmd.enabled:
  virtual void Pre(${cmd.name}Command& command) {
    Pre(static_cast<Command&>(command));
  }
  virtual void Post(${cmd.name}Command& command) {
    Post(static_cast<Command&>(command));
  }
% endif
% endfor
};

} // namespace gits::opengl2
