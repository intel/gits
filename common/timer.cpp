// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "timer.h"

std::chrono::steady_clock::time_point Timer::GetCurrentTime() const {
  return std::chrono::steady_clock::now();
}

Timer::Timer(bool paused) : paused_(paused) {
  Start();
}

void Timer::Start() {
  ResetStartTime();
  cumulated_time_ = 0;
}

int64_t Timer::GetElapsedTime() const {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(GetCurrentTime() - start_time_)
      .count();
}

void Timer::ResetStartTime() {
  start_time_ = GetCurrentTime();
}

int64_t Timer::Get() const {
  if (paused_) {
    return cumulated_time_;
  }

  return cumulated_time_ + GetElapsedTime();
}

void Timer::Pause() {
  if (!paused_) {
    cumulated_time_ += GetElapsedTime();
  }

  paused_ = true;
}

void Timer::Resume() {
  if (paused_) {
    ResetStartTime();
  }

  paused_ = false;
}
