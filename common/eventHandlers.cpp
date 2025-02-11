// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "eventHandlers.h"
#include "log.h"
#include "recorder.h"

namespace gits {

ExitEventHandler::ExitEventHandler() : _waitForExitEvent(true) {
  _hEventExit = CreateEvent(NULL, TRUE, FALSE, "GitsExitEvent");
  if (_hEventExit == NULL) {
    Log(ERR) << "ExitEventHandler: Failed to create event 'GitsExitEvent'";
  }
}

ExitEventHandler::~ExitEventHandler() {
  if (_hEventExit) {
    ResetEvent(_hEventExit);
    CloseHandle(_hEventExit);
  }
}

void ExitEventHandler::Start() {
  _eventThread = std::thread(&ExitEventHandler::WaitForExitEvent, this);
}

void ExitEventHandler::Stop() {
  _waitForExitEvent = false;
  if (_eventThread.joinable()) {
    _eventThread.join();
  }
}

void ExitEventHandler::WaitForExitEvent() {
  if (_hEventExit == NULL) {
    return;
  }

  while (_waitForExitEvent) {
    // Poll for GitsExitEvent every 4ms
    DWORD waitExitResult = WaitForSingleObject(_hEventExit, 4);
    if (waitExitResult == WAIT_OBJECT_0) {
      Log(INFO) << "ExitEventHandler: Received 'GitsExitEvent'. Exiting...";
      gits::CRecorder::Instance().MarkForDeletion();
      _waitForExitEvent = false;
    }
  }
}

} // namespace gits
