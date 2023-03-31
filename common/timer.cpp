// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "platform.h"
#if defined GITS_PLATFORM_UNIX
#include <ctime>
#include <sys/time.h>
#elif defined GITS_PLATFORM_WINDOWS
#include <windows.h>
#endif

#include "timer.h"

namespace {
int64_t CurrentTime() {
#if defined GITS_PLATFORM_WINDOWS
  static int64_t tick_duration;
  if (tick_duration == 0) {
    static LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    tick_duration = static_cast<int64_t>(1e9 / freq.QuadPart);
  }
  LARGE_INTEGER current;
  QueryPerformanceCounter(&current);
  return current.QuadPart * tick_duration;
#elif defined GITS_PLATFORM_LINUX
  timespec current;
  clock_gettime(CLOCK_MONOTONIC_RAW, &current);
  return current.tv_sec * 1000000000ll + current.tv_nsec;
#else
  timeval current;
  gettimeofday(&current, 0);
  return current.tv_sec * 1000000000ll + current.tv_usec * 1000ll;
#endif
}
} // namespace

Timer::Timer(bool paused) : paused_(paused) {
  Start();
}

void Timer::Start() {
  ResetStartTime();
  cumulated_time_ = 0;
}

int64_t Timer::GetElapsedTime() const {
  return CurrentTime() - start_time_;
}

void Timer::ResetStartTime() {
  start_time_ = CurrentTime();
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
