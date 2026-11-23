// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   gits.cpp
 *
 * @brief Definition of GITS project information class.
 *
 */

#include "gits.h"
#include "exception.h"
#include "configurator.h"
#include "imGuiHUD.h"

#include <string>

namespace gits {
CGits* CGits::_instance;

/**
 * @brief Disposes of singleton GITS object instance
 *
 * Explicit destruction of this singleton was required for
 * proper cleanup on GNU/Linux platform
 */
void CGits::Dispose() {
  delete _instance;
  _instance = nullptr;
}

/**
 * @brief Constructor
 *
 * Constructor of CGits class.
 */
CGits::CGits() : _version(CurrentVersion()), _frameNo(1) {}

CGits::~CGits() {
  try {
    // Only create signature when recording, player can opt in through option.
    if ((Configurator::IsRecorder() && Configurator::Get().common.recorder.enabled)
#ifdef GITS_PLATFORM_WINDOWS
        || (Configurator::IsPlayer() && Configurator::Get().common.player.subcapture.enabled)
#endif
    ) {
      CGits::Instance().GetMessageBus().publish(
          {PUBLISHER_RECORDER, TOPIC_STREAM_SAVED},
          std::make_shared<StreamSavedMessage>(
              Configurator::Get().common.recorder.dumpPath.string()));
    }
  } catch (...) {
    topmost_exception_handler("CGits::~CGits");
  }
}

/**
 * @brief Text dumper of GITS project information
 *
 * Method that dumps project information in text form.
 *
 * @param stream Output stream to use
 * @param version GITS project class to use
 *
 * @return Output stream
 */
std::ostream& operator<<(std::ostream& stream, const CGits& g) {
  stream << "gitsPlayer version " << GetVersionString();
#if defined GITS_ARCH_X86
  stream << " x86";
#elif defined GITS_ARCH_X64
  stream << " x64";
#elif defined GITS_ARCH_ARM
  stream << " ARM";
#elif defined GITS_ARCH_A64
  stream << " AARCH64";
#else
#error Unknown platform
#endif
  return stream;
}

void CGits::SetImGuiHUD(std::unique_ptr<ImGuiHUD> pImGuiHUD) {
  _imGuiHUD = std::move(pImGuiHUD);
}

ImGuiHUD* CGits::GetImGuiHUD() {
  return _imGuiHUD.get();
}

void LogRecordingVersion(const CVersion& version,
                         const std::optional<nlohmann::ordered_json>& recorderVersion) {
  const std::string recorded = recorderVersion && recorderVersion->is_string()
                                   ? recorderVersion->get<std::string>()
                                   : version.ToString();
  LOG_INFO << "Sequence recorded with: " << recorded;
  if (CurrentVersion() < version) {
    LOG_WARNING << "Stream was recorded with newer GITS: " << recorded << "; this player is "
                << GetVersionString() << ". Playback may fail.";
  }
}

TimerSet::TimerSet() : playback(true) {}

} // namespace gits
