// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "bit_range.h"
#include "glTypes.h"
#include "layerAuto.h"

#include <filesystem>
#include <unordered_map>

namespace gits::opengl2 {

// Writes the default framebuffer of the window a selected frame's swap presents, read before the
// swap. Frames end at the swaps the recorder ended a frame at, as for subcapture.
class ScreenshotsLayer : public Layer {
public:
  ScreenshotsLayer();

  void Pre(wglSwapBuffersCommand& command) override;
  void Pre(wglSwapLayerBuffersCommand& command) override;
  void Post(wglMakeCurrentCommand& command) override;
  void Post(wglMakeContextCurrentARBCommand& command) override;
  void Post(wglMakeContextCurrentEXTCommand& command) override;
  void Post(wglDeleteContextCommand& command) override;
  void Pre(glXSwapBuffersCommand& command) override;

private:
  void Swap(HDC recordedDc, bool endsFrame);
  void NoteCurrent();
  void Dump(HDC dc);
  void DumpGlx(GLXDrawable drawable);

  BitRange m_Range;
  std::filesystem::path m_DumpPath;
  unsigned m_Frame{};
  // The context last made current on each DC. A swap of a DC other than the current one is read
  // with it.
  std::unordered_map<HDC, HGLRC> m_DcContexts;
};

} // namespace gits::opengl2
