// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "gits.h"

void CGits::StartPlaybackTimer() {
  _playback.timer.Start();
  _playback.frameStart = _frameNo;
}

float CGits::GetFPS() {
  int64_t timeElapsed = _playback.lastFrameTime;
  int64_t timePerFrame = timeElapsed / (_frameNo - _playback.frameStart);
  float averageFPS = 1 / (timePerFrame / 1e9);
  return averageFPS;
}
