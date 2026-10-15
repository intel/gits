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

namespace gits::opengl2 {

// Generic Pre/Post hooks, run for every captured command. Once the first GL/WGL command
// is enabled, codegen adds a generated layerAuto.h with typed Pre(FooCommand&)/
// Post(FooCommand&) overloads per modern-opengl-backend-architecture.md ("Use
// enabled-only typed layers with generic fallbacks"). Their generated default bodies
// must delegate to these generic hooks (not be empty), so a logging/metrics layer needs
// only one override here to see every enabled command, while specialized layers can
// still opt into a typed overload for compile-time parameter access.
class Layer {
public:
  explicit Layer(std::string name) : m_Name(std::move(name)) {}
  virtual ~Layer() = default;

  const std::string& GetName() const {
    return m_Name;
  }

  virtual void Pre(Command& command) {}
  virtual void Post(Command& command) {}

private:
  std::string m_Name;
};

} // namespace gits::opengl2
