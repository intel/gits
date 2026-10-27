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
// (CaptureLayerManager::LoadLayers()) - so mutating `command.m_ppfd` here still changes what
// actually gets encoded.
//
// Also a Pre layer for wglCreatePbufferARB/EXT and wglSetPixelFormat, whose pixel format
// attributes it records in a PixelFormatMetaCommand (pixelFormatMetaTypes.h): a pbuffer's under
// the command's pre-hook key (generator_recorder.py's PRE_HOOK_KEY_PAIR_COMMANDS), a window's
// under its pixel format key (PRE_HOOK_EXTRA_KEY_COMMANDS). The GLX commands taking a
// GLXFBConfig get the attributes of that configuration the same way, glXCreateWindow as a window
// and the others as a pbuffer, and glXCreateContext those of the configuration its visual
// describes, as an application may hold a visual from any source, e.g. XGetVisualInfo.
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

  void Pre(wglCreatePbufferARBCommand& command) override;
  void Pre(wglCreatePbufferEXTCommand& command) override;
  void Pre(wglSetPixelFormatCommand& command) override;
  void Post(wglSetPixelFormatCommand& command) override;
  void Post(wglMakeCurrentCommand& command) override;
  void Post(wglMakeContextCurrentARBCommand& command) override;
  void Post(wglMakeContextCurrentEXTCommand& command) override;
  void Pre(glXGetVisualFromFBConfigCommand& command) override;
  void Pre(glXCreateWindowCommand& command) override;
  void Pre(glXCreatePbufferCommand& command) override;
  void Pre(glXCreateNewContextCommand& command) override;
  void Pre(glXCreateContextAttribsARBCommand& command) override;
  void Pre(glXCreateContextCommand& command) override;
};

} // namespace gits::opengl2
