// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   token.cpp
 *
 * @brief Definition of token (subject of capture/playback operation).
 *
 */

#include "platform.h"
#ifdef GITS_PLATFORM_WINDOWS
#include <Windows.h>
#endif

#if defined(GITS_PLATFORM_WINDOWS)
#include "renderDocUtil.h"
#endif

#include "token.h"
#include "exception.h"
#include "id.h"
#include "buffer.h"
#include "streams.h"
#include "gits.h"
#include "config.h"
#include "zone_allocator.h"
#include "function.h"
#include "scheduler.h"
#include "tools.h"
#if defined(GITS_PLATFORM_X11) && defined(WITH_VULKAN)
#include "vkWindowing.h"
#endif

#include "../OpenGL/common/include/openglLibrary.h"

#include <iostream>
#include <string>

DISABLE_WARNINGS
#include <boost/thread.hpp>
ENABLE_WARNINGS

namespace gits {

/* ******************************** TOKEN ****************************** */

CToken::~CToken() {}

static zone_allocator token_allocator;
void zone_allocator_next_zone() {
  if (Config::Get().common.useZoneAllocator) {
    token_allocator.use_next_zone();
  }
}
void zone_allocator_reinitialize(size_t zones, size_t size) {
  if (Config::Get().common.useZoneAllocator) {
    token_allocator.reinitialize(zones, size);
  }
}

void* CToken::operator new(size_t size) {
  if (Config::Get().common.useZoneAllocator) {
    return token_allocator.allocate(size);
  } else {
    return ::operator new(size);
  }
}

void CToken::operator delete(void* pointer) {
  if (!Config::Get().common.useZoneAllocator) {
    ::operator delete(pointer);
  }
}

void CToken::Serialize(CBinOStream& stream) {
  this->_isSerialized = true;
  CId(this->Id()).Write(stream);
  this->Write(stream);
}

CToken* CToken::Deserialize(CBinIStream& stream, CToken* (*ctor)(CId)) {
  CId id;
  id.Read(stream);

  // we failed to load any more ids, probably end of stream
  if (stream.eof()) {
    return nullptr;
  }

  auto token = ctor(id);
  token->Read(stream);
  return token;
}

void CToken::Serialize(CCodeOStream& stream) {
  this->_isSerialized = true;
  this->Write(stream);
}

/* ******************************** FRAME NUMBER ****************************** */

CTokenFrameNumber::CTokenFrameNumber(TId id, unsigned frameNumber)
    : _id(id), _frameNumber(frameNumber) {}

CTokenFrameNumber::CTokenFrameNumber(TId id) : _id(id), _frameNumber(0) {}

void CTokenFrameNumber::Write(CBinOStream& stream) const {}

void CTokenFrameNumber::Read(CBinIStream& stream) {}

void CTokenFrameNumber::Write(CCodeOStream& stream) const {
  std::stringstream functionName;
  switch (_id) {
  case CToken::ID_FRAME_START:
    functionName << "RunFrame" << std::setfill('0') << std::setw(6) << std::dec << _frameNumber
                 << "()";
    break;
  case CToken::ID_INIT_START:
    CGits::Instance().CCodeStateRestoreStart();
    functionName << "PrepareFrame" << std::setfill('0') << std::setw(6) << std::dec << _frameNumber
                 << "()";
    break;
  case CToken::ID_PRE_RECORD_START:
    CGits::Instance().CCodePreRecordStart();
    functionName << "NativePreRecord" << std::setfill('0') << std::setw(6) << std::dec
                 << _frameNumber << "()";
    break;
    //some frame related function ended - close its '}'
  case CToken::ID_PRE_RECORD_END:
    stream.select(CCodeOStream::GITS_PRE_RECORDER_CPP);
    stream.ScopeEnd();
    stream << "}" << std::endl;
    CGits::Instance().CCodePreRecordEnd();
    break;
  case CToken::ID_INIT_END:
    stream.select(CCodeOStream::GITS_STATE_RESTORE_CPP);
    stream.ScopeEnd();
    stream << "}" << std::endl;
    CGits::Instance().CCodeStateRestoreEnd();
    break;
  case CToken::ID_FRAME_END:
  case CToken::ID_CCODE_FINISH:
    stream.select(CCodeOStream::GITS_FRAMES_CPP);
    stream.ScopeEnd();
    stream << "}" << std::endl;
    break;
  default:;
  }

  auto fname = functionName.str();
  if (!fname.empty()) {
    //we need to add this function to state_restore.cpp or frames.cpp where its implementation is
    stream.select(stream.selectCCodeFile());
    stream << "void " << fname << "\n{" << std::endl;
    stream.ScopeBegin();

    //and we need to call it in main.cpp
    stream.select(CCodeOStream::GITS_MAIN_CPP)
        << "  extern void " << fname << ";\n  " << fname << ";" << std::endl;
  }
}

static void OnFrameBeginImpl() {
  if (Config::Get().player.benchmark) {
    CGits::Instance().Timers().frame.Restart();
  }
}

static void OnFrameEndImpl() {
  if (Config::Get().player.benchmark) {
    CGits::Instance().TimeSheet().add_frame_time("stamp", CGits::Instance().Timers().program.Get());
    CGits::Instance().TimeSheet().add_frame_time("cpu", CGits::Instance().Timers().frame.Get());
  }
}

void CTokenFrameNumber::Run() {
  using namespace OpenGL;
  auto& cfg = Config::Get();

  switch (_id) {
  case CToken::ID_INIT_START:
    CGits::Instance().Timers().init.Pause();
    CGits::Instance().Timers().restoration.Start();
    CGits::Instance().StateRestoreStarted();
    Log(INFO) << "Restoring state ...";

    if (cfg.common.useEvents) {
      CGits::Instance().PlaybackEvents().stateRestoreBegin();
    }
    if (cfg.player.traceSelectedFrames.empty() ||
        cfg.player.traceSelectedFrames[CGits::Instance().CurrentFrame()]) {
      CLog::SetLogLevel(cfg.common.thresholdLogLevel);
    } else {
      CLog::SetLogLevel(std::max(cfg.common.thresholdLogLevel, INFOV));
    }
    break;

  case CToken::ID_INIT_END:
    CGits::Instance().StateRestoreFinished();
    Log(INFO) << "Finished restoring state.";
    Log(TRACE) << "State restore end. Total drawcalls: " << CGits::Instance().CurrentDrawCount();

    if (CGits::Instance().apis.Has3D()) {
      CGits::Instance().apis.Iface3D().Play_StateRestoreEnd();
      if (cfg.player.swapAfterPrepare) {
        CGits::Instance().apis.Iface3D().Play_SwapAfterPrepare();
      }
    }
    if (CGits::Instance().apis.HasCompute()) {
      CGits::Instance().apis.IfaceCompute().Play_StateRestoreEnd();
    }
    if (cfg.common.useEvents) {
      CGits::Instance().PlaybackEvents().stateRestoreEnd();
    }

    CGits::Instance().Timers().restoration.Pause();
    break;

  case CToken::ID_FRAME_START:
    // If this is stream without state restore, init finishes on begin of first frame.

#if defined(GITS_PLATFORM_WINDOWS)
    if (cfg.player.renderDoc.frameRecEnabled &&
        cfg.player.renderDoc.captureRange[CGits::Instance().CurrentFrame()] &&
        CGits::Instance().CurrentFrame() != 1) {
      RenderDocUtil::GetInstance().StartRecording();
    }
#endif

    CGits::Instance().Timers().init.Pause();
    CGits::Instance().Timers().playback.Start();
    OnFrameBeginImpl();

    if (cfg.common.useEvents) {
      CGits::Instance().PlaybackEvents().frameBegin(CGits::Instance().CurrentFrame());
    }
    if (cfg.player.traceSelectedFrames.empty() ||
        cfg.player.traceSelectedFrames[CGits::Instance().CurrentFrame()]) {
      CLog::SetLogLevel(cfg.common.thresholdLogLevel);
    } else {
      CLog::SetLogLevel(std::max(cfg.common.thresholdLogLevel, INFOV));
    }
    break;

  case CToken::ID_FRAME_END:
    OnFrameEndImpl();
#if defined(GITS_PLATFORM_WINDOWS) || defined(GITS_PLATFORM_X11)
    if (cfg.player.showWindowBorder) {
      win_ptr_t window = GetWindowHandle();
#ifdef GITS_PLATFORM_WINDOWS
      WinTitle(window, "Current frame: " + std::to_string(CGits::Instance().CurrentFrame()));
#elif defined(GITS_PLATFORM_X11) && defined(WITH_VULKAN)
      Vulkan::WinTitle(window,
                       "Current frame: " + std::to_string(CGits::Instance().CurrentFrame()));
#endif
    }
#endif

#if defined(GITS_PLATFORM_WINDOWS)
    if (cfg.player.renderDoc.frameRecEnabled &&
        cfg.player.renderDoc.captureRange[CGits::Instance().CurrentFrame()]) {
      bool isLast = cfg.player.renderDoc.captureRange[CGits::Instance().CurrentFrame()] &&
                    !cfg.player.renderDoc
                         .captureRange[static_cast<uint64_t>(CGits::Instance().CurrentFrame()) + 1];
      if (!cfg.player.renderDoc.continuousCapture || isLast) {
        RenderDocUtil::GetInstance().StopRecording();
      }
      if (cfg.player.renderDoc.enableUI && isLast) {
        RenderDocUtil::GetInstance().LaunchRenderDocUI();
      }
    }
#endif
    if (cfg.player.endFrameSleep > 0) {
      sleep_millisec(Config::Get().player.endFrameSleep);
    }
    if (cfg.common.useEvents) {
      CGits::Instance().PlaybackEvents().frameEnd(CGits::Instance().CurrentFrame());
    }
    CGits::Instance().FrameCountUp();
    break;

  default: //WA - Warning on linux for not handling default case.
    break;
  }
}

/* ******************************** PLAYER RECORDER SYNC ****************************** */

CTokenPlayerRecorderSync::CTokenPlayerRecorderSync() {
  static uint64_t base = CGits::Instance().Timers().program.Get();
  _timeStamp = CGits::Instance().Timers().program.Get() - base;

  // Stream stores timestamps in microseconds.
  _timeStamp /= 1000;
}

void CTokenPlayerRecorderSync::Write(CBinOStream& stream) const {
  write_to_stream(stream, _timeStamp);
}

void CTokenPlayerRecorderSync::Read(CBinIStream& stream) {
  read_from_stream(stream, _timeStamp);
}

void CTokenPlayerRecorderSync::Run() {
  if (!Config::Get().player.syncWithRecorder) {
    return;
  }

  static uint64_t base = CGits::Instance().Timers().program.Get() / 1000;
  for (uint64_t current = CGits::Instance().Timers().program.Get() / 1000;
       current - base < _timeStamp; current = CGits::Instance().Timers().program.Get() / 1000) {
    using namespace boost;
    this_thread::sleep(posix_time::microsec(_timeStamp - (current - base)));
  }
}

/* ******************************** TOKEN MAKE CURRENT THREAD ****************************** */

CTokenMakeCurrentThread::CTokenMakeCurrentThread(int threadid) : _threadId(threadid) {
  CALL_ONCE[] {
    Log(INFO) << "Recorded Application uses multiple threads.";
    Log(WARN) << "Multithreaded applications have to be recorded from beginning. Subcapturing from "
                 "stream is possible without the --faithfulThreading option.";
    if (Config::Get().recorder.basic.dumpCCode && CGits::Instance().MultithreadedApp()) {
      Log(ERR) << "CCodeDump is not possible for multithreaded application. Please record binary "
                  "stream first and then recapture it to CCode";
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }
  };
  Log(TRACE) << "Current thread: " << threadid;
  CGits::Instance().CurrentThreadId(threadid);
}

void CTokenMakeCurrentThread::Write(CBinOStream& stream) const {
  write_to_stream(stream, _threadId);
}

void CTokenMakeCurrentThread::Read(CBinIStream& stream) {
  read_from_stream(stream, _threadId);
}

void CTokenMakeCurrentThread::Run() {
  CALL_ONCE[] {
    Log(INFO) << "Multithreaded stream";
  };
  CGits::Instance().CurrentThreadId(_threadId);
  if (OpenGL::SD().GetCurrentContextStateData().glBeginState &&
      !Config::Get().player.faithfulThreading) {
    Log(ERR) << "Multithreading bypass failed: Make current thread cannot be emitted between "
                "glBegin and glEnd";
    throw EOperationFailed(EXCEPTION_MESSAGE);
  }

  // The decision whether to truly switch threads is handled elsewhere by using either CAction or CSequentialExecutor.
  if (!Config::Get().player.faithfulThreading) {
    void* ctxFromThread = OpenGL::SD().GetContextFromThread(_threadId);
    OpenGL::SetCurrentContext(ctxFromThread);
    Log(TRACE) << "Make current thread: " << _threadId
               << " bypassed because option faithfulThreading is not used, set current context for "
                  "second thread emitted";
  } else {
    Log(TRACE) << "Make current thread: " << _threadId;
  }
}

/* ******************************** TOKEN MAKE CURRENT THREAD NO CTX SWITCH ****************************** */

CTokenMakeCurrentThreadNoCtxSwitch::CTokenMakeCurrentThreadNoCtxSwitch(int threadid)
    : _threadId(threadid) {
  CALL_ONCE[] {
    Log(INFO) << "Recorded Application uses multiple threads.";
    Log(WARN) << "Multithreaded applications have to be recorded from beginning. Subcapturing from "
                 "stream is possible without the --faithfulThreading option.";
    if (Config::Get().recorder.basic.dumpCCode && CGits::Instance().MultithreadedApp()) {
      Log(ERR) << "CCodeDump is not possible for multithreaded application. Please record binary "
                  "stream first and then recapture it to CCode";
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }
  };
  Log(TRACE) << "Current thread (no context switch token): " << threadid;
  CGits::Instance().CurrentThreadId(threadid);
}

void CTokenMakeCurrentThreadNoCtxSwitch::Write(CBinOStream& stream) const {
  write_to_stream(stream, _threadId);
}

void CTokenMakeCurrentThreadNoCtxSwitch::Read(CBinIStream& stream) {
  read_from_stream(stream, _threadId);
}

void CTokenMakeCurrentThreadNoCtxSwitch::Run() {
  CALL_ONCE[] {
    Log(INFO) << "Multithreaded stream.";
  };
  CGits::Instance().CurrentThreadId(_threadId);

  // The decision whether to truly switch threads is handled elsewhere by using either CAction or CSequentialExecutor.
  if (!Config::Get().player.faithfulThreading) {
    Log(TRACE) << "Make current thread (no context switch token): " << _threadId
               << " bypassed because option faithfulThreading is not used.";
  } else {
    Log(TRACE) << "Make current thread (no context switch token): " << _threadId;
  }
}

/* ******************************** RECORDER SCREEN RESOLUTION **************************** */

CTokenScreenResolution::CTokenScreenResolution(int width, int height)
    : _screenWidth(width), _screenHeight(height) {}

void CTokenScreenResolution::Write(CBinOStream& stream) const {
  write_to_stream(stream, _screenWidth);
  write_to_stream(stream, _screenHeight);
}

void CTokenScreenResolution::Read(CBinIStream& stream) {
  read_from_stream(stream, _screenWidth);
  read_from_stream(stream, _screenHeight);
}

void CTokenScreenResolution::Run() {
  if (!Config::Get().player.forceOrigScreenResolution) {
    return;
  }
#ifdef GITS_PLATFORM_WINDOWS
  DEVMODE dmScreenSettings;
  dmScreenSettings.dmSize = sizeof(dmScreenSettings);
  dmScreenSettings.dmPelsWidth = _screenWidth;
  dmScreenSettings.dmPelsHeight = _screenHeight;
  dmScreenSettings.dmBitsPerPel = 32;
  dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
  Log(INFO) << "Changing screen resolution to: " << _screenWidth << "x" << _screenHeight << ".";
  ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN);
#endif
}

} // namespace gits
