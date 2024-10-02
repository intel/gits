// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "eventHandlers.h"
#include "log.h"
#include "recorder.h"

gits::ExitEventHandler::ExitEventHandler() : _stopWaiting(false) {
  _hEventExit = CreateEvent(NULL, TRUE, FALSE, "GitsExitEvent");
  if (_hEventExit == NULL) {
    Log(ERR) << "Failed to create exit event";
  }
}

gits::ExitEventHandler::~ExitEventHandler() {
  if (_hEventExit) {
    ResetEvent(_hEventExit);
    CloseHandle(_hEventExit);
  }
}

void gits::ExitEventHandler::Start() {
  _eventThread = std::thread(&ExitEventHandler::WaitForExitEvent, this);
}

void gits::ExitEventHandler::Stop() {
  if (!_stopWaiting) {
    _stopWaiting = true;
    SetEvent(_hEventExit);
  }

  if (_eventThread.joinable()) {
    _eventThread.join();
  }
}

void gits::ExitEventHandler::WaitForExitEvent() {
  if (_hEventExit == NULL) {
    return;
  }

  DWORD waitExitResult = WaitForSingleObject(_hEventExit, INFINITE);

  if (waitExitResult == WAIT_OBJECT_0) {
    if (!_stopWaiting) {
      Log(INFO) << "Received 'GitsExitEvent'. Exiting...";
      gits::CRecorder::Instance().MarkForDeletion();
      _stopWaiting = true;
    }
  } else {
    Log(ERR) << "Failed to wait for 'GitsExitEvent'. Exiting...";
  }
}
