// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   recorderBehaviors.h
 *
 * @brief Declarations of function calls recorder behaviors.
 *
 */

#pragma once

#include "recorder.h"
#include "tools_lite.h"

#include <string>
#include <set>

namespace gits {
/**
    * @brief Recorder behavior
    *
    * gits::CBehavior class defines a behavior of gits::CRecorder
    * class. OnInit() and OnFrameEnd() are handlers that are run when
    * corresponding event occurs on the recorder. Using those handlers
    * and gits::CRecorder::FrameNumber() method behavior class can find
    * out when to call gits::CRecorder::Start(), gits::CRecorder::Stop(),
    * gits::CRecorder::Save() and gits::CRecorder::Clear() methods.
    */
class CBehavior : private gits::noncopyable {
  unsigned _captureBeginFrame;
  bool _captureOnKeypress;

  enum CaptureState {
    CAP_NOT_INITIATED, // User yet to press capture begin keysequence.
    CAP_INITIATED,     // User pressed keysequence, but frame to capture not yet encountered.
    CAP_CAPTURING,     // Frame is being captured.
    CAP_FINISHED       // We are after last frame of interest.
  } _captureState;

public:
  CBehavior(CRecorder& recorder, bool captureOnKeypress);

  void OnStartAction();
  void OnFrameEnd();

  bool ShouldCapture() const {
    return _captureState == CAP_CAPTURING;
  }
  bool CaptureFinished() const {
    return _captureState == CAP_FINISHED;
  }
};
} // namespace gits
