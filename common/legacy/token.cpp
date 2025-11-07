// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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
#ifdef WITH_VULKAN
#include "vulkanRenderDocUtil.h"
#endif
#endif

#include "token.h"
#include "exception.h"
#include "id.h"
#include "buffer.h"
#include "streams.h"
#include "gits.h"
#include "function.h"
#include "scheduler.h"
#include "tools.h"
#if defined(GITS_PLATFORM_X11) && defined(WITH_VULKAN)
#include "vkWindowing.h"
#endif

#include "../../OpenGL/common/include/openglLibrary.h"

#include <iostream>
#include <string>
#include <thread>

namespace gits {

/* ******************************** TOKEN ****************************** */

CToken::~CToken() {}

void* CToken::operator new(size_t size) {
  return ::operator new(size);
}

void CToken::operator delete(void* pointer) {
  ::operator delete(pointer);
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

/* ******************************** MARKER ****************************** */

CTokenMarker::CTokenMarker(TId id) : _id(id) {}

void CTokenMarker::Write(CBinOStream& stream) const {}

void CTokenMarker::Read(CBinIStream& stream) {}

static void OnFrameBeginImpl() {
  auto& gits = CGits::Instance();
  if (Configurator::Get().common.player.benchmark) {
    gits.Timers().frame.Restart();
  }
}

static void OnFrameEndImpl() {
  if (Configurator::Get().common.player.benchmark) {
    CGits::Instance().TimeSheet().add_frame_time("stamp", CGits::Instance().Timers().program.Get());
    CGits::Instance().TimeSheet().add_frame_time("cpu", CGits::Instance().Timers().frame.Get());
  }
}

void CTokenMarker::Run() {
  using namespace OpenGL;
  auto& cfg = Configurator::Get();

  switch (_id) {
  case CToken::ID_INIT_START:
    CGits::Instance().Timers().init.Pause();
    CGits::Instance().Timers().restoration.Start();
    CGits::Instance().StateRestoreStarted();
    LOG_INFO << "Restoring state ...";

    if (cfg.common.shared.useEvents) {
      CGits::Instance().PlaybackEvents().stateRestoreBegin();
    }
    if (cfg.common.player.traceSelectedFrames.empty() ||
        cfg.common.player.traceSelectedFrames[CGits::Instance().CurrentFrame()]) {
      plog::get()->setMaxSeverity(log::GetSeverity(cfg.common.shared.thresholdLogLevel));
    } else {
      plog::get()->setMaxSeverity(
          log::GetSeverity(std::max(cfg.common.shared.thresholdLogLevel, LogLevel::INFOV)));
    }
    break;

  case CToken::ID_INIT_END:
    CGits::Instance().StateRestoreFinished();
    LOG_INFO << "Finished restoring state.";
    LOG_TRACE << "State restore end. Total drawcalls: " << CGits::Instance().CurrentDrawCount();

    if (CGits::Instance().apis.Has3D()) {
      CGits::Instance().apis.Iface3D().Play_StateRestoreEnd();
      if (cfg.common.player.swapAfterPrepare) {
        CGits::Instance().apis.Iface3D().Play_SwapAfterPrepare();
      }
    }
    if (CGits::Instance().apis.HasCompute()) {
      CGits::Instance().apis.IfaceCompute().Play_StateRestoreEnd();
    }
    if (cfg.common.shared.useEvents) {
      CGits::Instance().PlaybackEvents().stateRestoreEnd();
    }

    CGits::Instance().Timers().restoration.Pause();
    break;

  case CToken::ID_FRAME_START:
    // If this is stream without state restore, init finishes on begin of first frame.

#if defined(GITS_PLATFORM_WINDOWS) && defined(WITH_VULKAN)
    if (cfg.vulkan.player.renderDoc.mode == TVkRenderDocCaptureMode::FRAMES &&
        cfg.vulkan.player.renderDoc.captureRange[CGits::Instance().CurrentFrame()]) {
      Vulkan::RenderDocUtil::GetInstance().StartRecording();
    }
#endif

    CGits::Instance().Timers().init.Pause();
    CGits::Instance().Timers().playback.Start();
    OnFrameBeginImpl();

    if (cfg.common.shared.useEvents) {
      CGits::Instance().PlaybackEvents().frameBegin(CGits::Instance().CurrentFrame());
    }
    if (cfg.common.player.traceSelectedFrames.empty() ||
        cfg.common.player.traceSelectedFrames[CGits::Instance().CurrentFrame()]) {
      plog::get()->setMaxSeverity(log::GetSeverity(cfg.common.shared.thresholdLogLevel));
    } else {
      plog::get()->setMaxSeverity(
          log::GetSeverity(std::max(cfg.common.shared.thresholdLogLevel, LogLevel::INFOV)));
    }
    break;

  case CToken::ID_FRAME_END:
    OnFrameEndImpl();
#if defined(GITS_PLATFORM_WINDOWS) || defined(GITS_PLATFORM_X11)
    if (cfg.common.player.showWindowBorder) {
      win_ptr_t window = GetWindowHandle();
      if (window) {
#ifdef GITS_PLATFORM_WINDOWS
        WinTitle(window, "Current frame: " + std::to_string(CGits::Instance().CurrentFrame()));
#elif defined(GITS_PLATFORM_X11) && defined(WITH_VULKAN)
        Vulkan::WinTitle(window,
                         "Current frame: " + std::to_string(CGits::Instance().CurrentFrame()));
#endif
      }
    }
#endif

#if defined(GITS_PLATFORM_WINDOWS) && defined(WITH_VULKAN)
    if (cfg.vulkan.player.renderDoc.mode == TVkRenderDocCaptureMode::FRAMES &&
        cfg.vulkan.player.renderDoc.captureRange[CGits::Instance().CurrentFrame()]) {
      bool isLast = cfg.vulkan.player.renderDoc.captureRange[CGits::Instance().CurrentFrame()] &&
                    !cfg.vulkan.player.renderDoc
                         .captureRange[static_cast<uint64_t>(CGits::Instance().CurrentFrame()) + 1];
      if (!cfg.vulkan.player.renderDoc.continuousCapture || isLast) {
        Vulkan::RenderDocUtil::GetInstance().StopRecording();
      }
      if (cfg.vulkan.player.renderDoc.enableUI && isLast) {
        Vulkan::RenderDocUtil::GetInstance().LaunchRenderDocUI();
      }
    }
#endif
    if (cfg.common.player.endFrameSleep > 0) {
      sleep_millisec(Configurator::Get().common.player.endFrameSleep);
    }
    if (cfg.common.shared.useEvents) {
      CGits::Instance().PlaybackEvents().frameEnd(CGits::Instance().CurrentFrame());
    }
#ifdef GITS_PLATFORM_WINDOWS
    if (!cfg.directx.features.subcapture.enabled) {
      CGits::Instance().FrameCountUp();
    }
#else
    CGits::Instance().FrameCountUp();
#endif
    break;

  default: //WA - Warning on linux for not handling default case.
    break;
  }
}

/* ******************************** TOKEN MARKER UINT64 ****************************** */

CTokenMarkerUInt64::CTokenMarkerUInt64(uint64_t value) : _value(value) {}

void CTokenMarkerUInt64::Write(CBinOStream& stream) const {
  write_to_stream(stream, _value);
}

void CTokenMarkerUInt64::Read(CBinIStream& stream) {
  read_from_stream(stream, _value);
}

void CTokenMarkerUInt64::Run() {
  auto& cfg = Configurator::Get();
  if (cfg.common.shared.useEvents) {
    CGits::Instance().PlaybackEvents().markerUInt64(_value);
  }
}

uint64_t CTokenMarkerUInt64::Size() const {
  return CToken::Size() + sizeof(_value);
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

void CTokenPlayerRecorderSync::Run() {}

uint64_t CTokenPlayerRecorderSync::Size() const {
  return CToken::Size() + sizeof(_timeStamp);
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
  if (!Configurator::Get().common.player.forceOrigScreenResolution) {
    return;
  }
#ifdef GITS_PLATFORM_WINDOWS
  DEVMODE dmScreenSettings;
  dmScreenSettings.dmSize = sizeof(dmScreenSettings);
  dmScreenSettings.dmPelsWidth = _screenWidth;
  dmScreenSettings.dmPelsHeight = _screenHeight;
  dmScreenSettings.dmBitsPerPel = 32;
  dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
  LOG_INFO << "Changing screen resolution to: " << _screenWidth << "x" << _screenHeight << ".";
  ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN);
#endif
}

uint64_t CTokenScreenResolution::Size() const {
  return CToken::Size() + sizeof(_screenWidth) + sizeof(_screenHeight);
}

} // namespace gits
