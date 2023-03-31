// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once
#include "resource_manager.h"
#include "timer.h"

DISABLE_WARNINGS
#include <boost/lexical_cast.hpp>
ENABLE_WARNINGS

struct PlaybackTimer {
  Timer timer;
  uint32_t frameStart;
  int64_t lastFrameTime;
  PlaybackTimer() : frameStart(0) {}
};

struct CGits {
private:
  Task<gits::Image> _imageWriter;
  uint32_t _frameNo;
  PlaybackTimer _playback;

public:
  static CGits& Instance() {
    static CGits cg;
    return cg;
  }

  CGits() : traceGLAPIBypass(false) {}
  bool IsStateRestoration() const {
    return false;
  }
  int CurrentThreadId() const {
    return 0;
  }
  CGits& ResourceManager() {
    return Instance();
  }
  template <class T, class U, class W>
  int put(T, U, W) {
    return 0;
  }
  bool traceGLAPIBypass;
  void WriteImage(const std::string& filename,
                  size_t width,
                  size_t height,
                  bool hasAlpha,
                  std::vector<uint8_t>& data, // Destroys the data argument.
                  bool flip = true,
                  bool isBGR = false,
                  bool isSRGB = false) {
    if (!_imageWriter.running()) {
      _imageWriter.start(gits::ImageWriter());
    }

    gits::Image img(filename, width, height, hasAlpha, data, flip, isBGR, isSRGB);
    _imageWriter.queue().produce(img);
  }

  uint32_t CurrentFrame() const {
    return _frameNo;
  }
  void FrameCountUp() {
    ++_frameNo;
  }
  void StartPlaybackTimer();
  void SetLastFrameTime() {
    _playback.lastFrameTime = _playback.timer.Get();
  }
  int64_t GetLastFrameTime() {
    return _playback.lastFrameTime;
  }
  float GetFPS();
};

#define RESOURCE_TEXTURE 1
