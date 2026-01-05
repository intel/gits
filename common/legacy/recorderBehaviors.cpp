// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   recorderBehaviors.cpp
 *
 * @brief Definitions of function calls recorder behaviors.
 *
 */

#include "gits.h"
#include "recorderBehaviors.h"
#include "streams.h"
#include "apis_iface.h"

#include <iostream>
#include <ctime>

namespace gits {

CBehavior::CBehavior(CRecorder& recorder, bool captureOnKeypress)
    : _captureOnKeypress(captureOnKeypress) {
  const CGits& inst = CGits::Instance();
  if (inst.apis.Has3D()) {
    int dumpFrameStart = (int)inst.apis.Iface3D().CfgRec_StartFrame();
    if (_captureOnKeypress) {
      _captureBeginFrame = dumpFrameStart;
      _captureState = CAP_NOT_INITIATED;
    } else {
      // If no user action capture is assumed to be relative to begin of sequence
      _captureBeginFrame = 1;
      _captureState = (dumpFrameStart == 1) ? CAP_CAPTURING : CAP_INITIATED;
    }
  } else if (inst.apis.HasCompute()) {
    // TODO: Make it so that any frame number logic is not necessary in compute-only streams.
    if (inst.apis.IfaceCompute().CfgRec_IsAllMode()) {
      // Compute-only full streams.
      _captureBeginFrame = 1;
      _captureState = CAP_CAPTURING;
    } else if (inst.apis.IfaceCompute().CfgRec_IsSingleKernelMode() ||
               inst.apis.IfaceCompute().CfgRec_IsKernelsRangeMode()) {
      // Compute-only subcaptures.
      _captureBeginFrame = static_cast<unsigned>(-1);
      _captureState = CAP_INITIATED;
    } else {
      LOG_ERROR << "Unknown compute recording mode.";
      throw ENotImplemented(EXCEPTION_MESSAGE);
    }
  } else {
    LOG_ERROR << "There are no 3D nor compute APIs.";
    throw ENotSupported(EXCEPTION_MESSAGE);
  }
  // Subcaptures of 3D+Compute streams are not supported.
}

/**
 * @brief On frame rendering end handler
 *
 * A handler that is run when proxy library detects that rendering of
 * current frame has ended. It stops and resumes the capture following rules
 * defined in configuration file.
 *
 */
void CBehavior::OnFrameEnd() {
  // Capture is not initiated -> exit immediately.
  if (_captureState == CAP_NOT_INITIATED) {
    return;
  }

  if (CGits::Instance().apis.Has3D()) {
    // Frame number relative to beginning of the whole capture.
    unsigned relFrameNumber =
        CGits::Instance().CurrentFrame() - _captureBeginFrame + 1; // Number frames from 1.
    unsigned dumpFrameStart = CGits::Instance().apis.Iface3D().CfgRec_StartFrame();
    unsigned dumpFrameStop = CGits::Instance().apis.Iface3D().CfgRec_StopFrame();

    // Still waiting for capturing.
    if (relFrameNumber < dumpFrameStart) {
      if (relFrameNumber == dumpFrameStart - 1) { // Begin capture on frame end before our frame.
        _captureState = CAP_CAPTURING;
      }
      return;
    }

    // Capture finished, stop, exit.
    if (relFrameNumber == dumpFrameStop) {
      _captureState = CAP_FINISHED; // No further captures will follow.
      return;
    }
  }
}

/**
 * @brief Used to initiate capture with user action
 *
 * Framework is assumed to call this member function when specified keys
 * are pressed.
 *
 */
void CBehavior::OnStartAction() {
  if (_captureOnKeypress && _captureState == CAP_NOT_INITIATED) {
    if (CGits::Instance().apis.Has3D()) {
      _captureBeginFrame =
          CGits::Instance().apis.Iface3D().CfgRec_StartFrame() + CGits::Instance().CurrentFrame();
    }
    _captureState = CAP_INITIATED;
  }
}

} // namespace gits
