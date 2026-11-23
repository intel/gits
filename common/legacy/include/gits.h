// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   gits.h
 *
 * @brief Declaration of GITS project information class.
 *
 */

#pragma once

#include "version.h"
#include "timer.h"
#include "tools.h"
#include "messageBus.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <ostream>

namespace gits {
class ImGuiHUD;

struct TimerSet {
  TimerSet();
  Timer playback;
};

/**
   * @brief Main GITS project class
   *
   * gits::CGits is a main class of GITS project. It provides information
   * about current version of a project.
   *
   * @note Singleton design pattern
   */
class CGits : private gits::noncopyable {
private:
  static CGits* _instance; /**< @brief singleton class instance */

  const CVersion _version; /**< @brief version of GITS project */

  uint32_t _frameNo;
  TimerSet _timers;

  MessageBus _messageBus;

  std::unique_ptr<ImGuiHUD> _imGuiHUD;

  CGits();
  ~CGits();

public:
  CGits(const CGits& other) = delete;
  CGits& operator=(const CGits& other) = delete;
  static CGits& Instance() {
    if (!_instance) {
      _instance = new CGits();
    }
    return *_instance;
  }
  static CGits* InstancePtr() {
    return _instance;
  }
  void Dispose();

  uint32_t CurrentFrame() const {
    return _frameNo;
  }
  void FrameCountUp() {
    ++_frameNo;
  }

  TimerSet& Timers() {
    return _timers;
  }

  const CVersion& Version() const {
    return _version;
  }

  MessageBus& GetMessageBus() {
    return _messageBus;
  }

  void SetImGuiHUD(std::unique_ptr<ImGuiHUD> gitsHUD);
  ImGuiHUD* GetImGuiHUD();

  friend std::ostream& operator<<(std::ostream& stream, const CGits& g);
};

// Logs the GITS version that recorded a stream and warns when it is newer than this player.
// recorderVersion is the diag.gits.version stream property, which includes build metadata.
void LogRecordingVersion(const CVersion& version,
                         const std::optional<nlohmann::ordered_json>& recorderVersion);
} // namespace gits
