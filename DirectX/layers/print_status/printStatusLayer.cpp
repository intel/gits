// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "printStatusLayer.h"
#include "log.h"

namespace gits {
namespace DirectX {

template <typename Duration>
std::string formatDuration(Duration duration) {
  using namespace std::chrono;

  auto hrs = duration_cast<hours>(duration).count();
  auto mins = duration_cast<minutes>(duration).count() % 60;
  auto secs = duration_cast<seconds>(duration).count() % 60;
  auto msecs = duration_cast<milliseconds>(duration).count() % 1000;

  std::ostringstream oss;
  oss << std::setfill('0') << std::setw(2) << hrs << ":" << std::setfill('0') << std::setw(2)
      << mins << ":" << std::setfill('0') << std::setw(2) << secs << "." << std::setfill('0')
      << std::setw(3) << msecs;
  return oss.str();
}

void PrintStatusLayer::post(StateRestoreBeginCommand& c) {
  initialTime_ = std::chrono::steady_clock::now();
  LOG_INFO << "PlayerStatus - State restore started";
}

void PrintStatusLayer::post(StateRestoreEndCommand& c) {
  LOG_INFO << "PlayerStatus - State restore duration: "
           << formatDuration(std::chrono::steady_clock::now() - initialTime_);
}

void PrintStatusLayer::post(MarkerUInt64Command& c) {
  static auto timeBegin = std::chrono::steady_clock::now();
  switch (c.value_) {
  case MarkerUInt64Command::Value::STATE_RESTORE_OBJECTS_BEGIN:
  case MarkerUInt64Command::Value::STATE_RESTORE_RTAS_BEGIN:
  case MarkerUInt64Command::Value::STATE_RESTORE_RESOURCES_BEGIN:
    timeBegin = std::chrono::steady_clock::now();
    break;
  case MarkerUInt64Command::Value::STATE_RESTORE_OBJECTS_END:
    LOG_INFO << "PlayerStatus - Objects restored in "
             << formatDuration(std::chrono::steady_clock::now() - timeBegin);
    break;
  case MarkerUInt64Command::Value::STATE_RESTORE_RTAS_END:
    LOG_INFO << "PlayerStatus - RTAS restored in "
             << formatDuration(std::chrono::steady_clock::now() - timeBegin);
    break;
  case MarkerUInt64Command::Value::STATE_RESTORE_RESOURCES_END:
    LOG_INFO << "PlayerStatus - Resources restored in "
             << formatDuration(std::chrono::steady_clock::now() - timeBegin);
    break;
  default:
    break;
  }
}

} // namespace DirectX
} // namespace gits
