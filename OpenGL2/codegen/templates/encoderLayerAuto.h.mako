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

namespace gits::opengl2 {

// Sole, last-registered Post layer for the OpenGL2 recorder - see
// docs/development/opengl2-layer-architecture-alignment-plan.md. Post(FooCommand&)
// does exactly what the generated wrapper (recorderWrappersAuto.cpp.mako) used to inline
// directly at the same call site: CaptureManager::Get().RecordCommand(command, new
// FooCommandSerializer(command)) - see encoderLayerAuto.cpp.mako. Mirrors
// Vulkan/recorder/encoderLayerAuto.h's shape and role exactly, minus the constructor-injected
// stream::OrderingRecorder& (OpenGL2's CaptureManager::RecordCommand already wraps that
// indirection, so EncoderLayer needs no extra state of its own - see its header comment).
//
// Player-side has no encoder-equivalent yet: the player never records anything, so there is
// nothing for a PostLayer on that side to do (see playerManager.h's own header comment on why
// its Post-layer vector stays permanently empty for now).
class EncoderLayer : public Layer {
public:
  EncoderLayer() : Layer("Encoder") {}

% for cmd in commands:
% if cmd.enabled:
  void Post(${cmd.name}Command& command) override;
% endif
% endfor
};

} // namespace gits::opengl2
