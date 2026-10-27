// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "command.h"

#include <string>
#include <utility>

namespace gits::opengl {

// Hand-written base for the generated `Layer` (see layerAuto.h, added by
// docs/development/opengl-layer-architecture-alignment-plan.md): generic Pre/Post
// hooks, run for every generated, enabled API command and every backend-specific player-side
// meta-command. Meta-commands are built directly rather than generated from an API registry, so
// layerAuto.h's template declares their typed overloads explicitly. `layerAuto.h` adds, for every
// enabled command only (per
// modern-opengl-backend-architecture.md's "Use enabled-only typed layers with generic
// fallbacks"), a typed `Pre(FooCommand&)`/`Post(FooCommand&)` override pair on top of this
// class, each with a default body that simply forwards to the generic hooks declared here - so
// a logging/metrics layer needs only one override here to see every command routed through a
// layer path, while specialized layers can still opt into a typed overload for compile-time
// parameter access.
//
// Deliberately its own, separately-named class rather than being named `Layer` itself: a fresh
// codegen run regenerates layerAuto.h's entire typed-overload surface from scratch every build
// (see OpenGL/codegen/generator_layer_interface.py), and a generated file must never be the
// place a hand-written, permanent implementation detail like this one lives (see repo AGENTS.md
// "Do not edit generated files"). `Layer` - the name every other file (`captureManager.h`,
// `recorderWrappersAuto.cpp.mako`, `commandRunnersAuto.cpp.mako`, and any future concrete layer
// subclass) actually spells out - is layerAuto.h's generated `class Layer : public LayerBase`,
// matching Vulkan's own `Layer` naming exactly at every one of those call sites even though the
// generic-fallback split underneath it is OpenGL-specific.
class LayerBase {
public:
  explicit LayerBase(std::string name) : m_Name(std::move(name)) {}
  virtual ~LayerBase() = default;

  const std::string& GetName() const {
    return m_Name;
  }

  virtual void Pre(Command& command) {}
  virtual void Post(Command& command) {}

private:
  std::string m_Name;
};

} // namespace gits::opengl
