// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

// A small, hand-written Post-side Layer (see layerAuto.h/layerBase.h and
// CaptureManager::GetPostLayers()) that replaces a captured wglSetPixelFormat call's own
// PIXELFORMATDESCRIPTOR with the REAL one the driver actually resolved, read back via
// GetPixelFormat()/DescribePixelFormat() right after the real SetPixelFormat call - see this
// class's own .cpp for the full rationale.
//
// Runs in Post(), after the real driver dispatch (every generated wrapper calls the real
// function unconditionally before entering its Post-layer loop - see
// recorderWrappersAuto.cpp.mako's own header comment) but before EncoderLayer serializes the
// command onto the wire, since EncoderLayer is always the LAST-registered Post layer
// (CaptureManager::LoadLayers()) - so mutating `command.m_ppfd` here still changes what
// actually gets encoded.
#include "commandsAuto.h"
#include "layerAuto.h"

namespace gits::opengl2 {

class PixelFormatCaptureLayer : public Layer {
public:
  PixelFormatCaptureLayer() : Layer("PixelFormatCaptureLayer") {}

  // Bring Layer's generic Pre(Command&)/Post(Command&) (and every OTHER typed overload this
  // class does not itself override) back into this class's own overload set - see
  // WindowGeometryTrackingLayer's identical `using` declarations for the full rationale (member
  // name hiding across scopes).
  using Layer::Post;
  using Layer::Pre;

  void Post(wglSetPixelFormatCommand& command) override;
};

} // namespace gits::opengl2
