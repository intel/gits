// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "bit_range.h"
#include "commandsAuto.h"
#include "layerAuto.h"

#include <cstdint>
#include <filesystem>
#include <map>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

namespace gits::opengl2 {

// Writes the default framebuffer of the window a selected frame's swap presents, read before the
// swap and written once it succeeded, since only a successful swap ends a frame. Frames are
// numbered as the player numbers them, in the order of the swaps' keys, so its screenshots
// compare with these.
class RecorderScreenshotsLayer : public Layer {
public:
  RecorderScreenshotsLayer();

  using Layer::Post;
  using Layer::Pre;

  void Pre(wglSwapBuffersCommand& command) override;
  void Post(wglSwapBuffersCommand& command) override;
  void Pre(wglSwapLayerBuffersCommand& command) override;
  void Post(wglSwapLayerBuffersCommand& command) override;
  void Post(wglMakeCurrentCommand& command) override;
  void Post(wglMakeContextCurrentARBCommand& command) override;
  void Post(wglMakeContextCurrentEXTCommand& command) override;
  void Post(wglDeleteContextCommand& command) override;
  void Pre(glXSwapBuffersCommand& command) override;
  void Post(glXSwapBuffersCommand& command) override;

private:
  struct Image {
    int Width{};
    int Height{};
    std::vector<uint8_t> Pixels;
  };
  struct Swap {
    bool Ended{};
    bool Succeeded{};
    std::optional<Image> Screenshot;
  };

  // Records the swap with `key` as pending, and returns the first frame it can become if any of
  // the frames it can become is selected.
  std::optional<unsigned> BeginSwap(uint64_t key);
  void Read(uint64_t key, HDC dc);
  void ReadGlx(uint64_t key, Display* display, GLXDrawable drawable);
  void EndFrame(uint64_t key, bool succeeded);
  void NoteCurrent();

  BitRange m_Range;
  std::filesystem::path m_DumpPath;
  std::mutex m_Mutex;
  unsigned m_Frame{};
  // Swaps not numbered yet, by key. A swap is numbered once every swap before it ended.
  std::map<uint64_t, Swap> m_Swaps;
  // The context last made current on each DC. A swap of a DC other than the current one is read
  // with it.
  std::unordered_map<HDC, HGLRC> m_DcContexts;
};

} // namespace gits::opengl2
