// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "pragmas.h"

#include <cstdint>
#include <chrono>

class Timer {
public:
  //sets up the timer, performs implicit Start() by default
  explicit Timer(bool paused = false);

  //will get time elapsed since invocation of this member function
  //function is idempotent
  void Start();

  //reset cumulated time and start counting again
  void Restart();

  //get time elapsed from last Start in nanoseconds
  int64_t Get() const;

  //pause the timer - don't measure time until Resume() is called
  void Pause();

  //timer is measuring the time again
  void Resume();

private:
  std::chrono::steady_clock::time_point GetCurrentTime() const;
  void ResetStartTime();
  int64_t GetElapsedTime() const;

  std::chrono::steady_clock::time_point start_time_;
  int64_t cumulated_time_;
  bool paused_;
};
