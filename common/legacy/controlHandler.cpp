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
#include "log2.h"
#include "recorder.h"
#include "tools.h"

#ifdef _WIN32
#include <Windows.h>

namespace {

bool CtrlHandler(DWORD fdwCtrlType) {

  switch (fdwCtrlType) {
  case CTRL_C_EVENT:
    LOG_INFO << "ctrl-c";
    break;
  case CTRL_CLOSE_EVENT:
    LOG_INFO << "ctrl-close";
    break;
  case CTRL_BREAK_EVENT:
    LOG_INFO << "ctrl-break";
    break;
  case CTRL_LOGOFF_EVENT:
    LOG_INFO << "log off";
    break;
  case CTRL_SHUTDOWN_EVENT:
    LOG_INFO << "shutdown";
    break;
  default:
    LOG_INFO << "Stopping due to unknown event.";
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
