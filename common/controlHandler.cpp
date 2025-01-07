// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   controlHandler.cpp
*
* @brief Definition of signal handler function
*
*/

#include "controlHandler.h"
#include "log.h"
#include "recorder.h"
#include "tools.h"

#ifdef _WIN32
#include <Windows.h>

namespace {

bool CtrlHandler(DWORD fdwCtrlType) {

  switch (fdwCtrlType) {
  case CTRL_C_EVENT:
    Log(INFO) << "ctrl-c";
    break;
  case CTRL_CLOSE_EVENT:
    Log(INFO) << "ctrl-close";
    break;
  case CTRL_BREAK_EVENT:
    Log(INFO) << "ctrl-break";
    break;
  case CTRL_LOGOFF_EVENT:
    Log(INFO) << "log off";
    break;
  case CTRL_SHUTDOWN_EVENT:
    Log(INFO) << "shutdown";
    break;
  default:
    Log(INFO) << "Stopping due to unknown event.";
  }
  call_once_impl([]() { gits::CRecorder::Instance().Close(); });
  return false;
}

} // namespace

void gits::SignalsHandler() {
  SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE);
}

void gits::RemoveSignalsHandler() {
  SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, FALSE);
}

#endif /*_WIN32 */
