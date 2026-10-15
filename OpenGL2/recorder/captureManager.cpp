// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "captureManager.h"

#include "unsupportedCommandTypes.h"

namespace gits::opengl2 {

CaptureManager& CaptureManager::Get() {
  // Thread-safe function-local static initialization (C++11 "magic statics") replaces a
  // hand-rolled, unsynchronized check-then-write of a raw pointer, which could race two
  // concurrent first-callers (GL wrapper threads) into constructing two different
  // CaptureManager instances - splitting command keys/recording across them. The instance is
  // still heap-allocated and intentionally never destroyed (see this method's declaration
  // comment in captureManager.h) - only the *construction* was unsynchronized before; the
  // "never destroyed" property is unaffected by using a static pointer instead of a static
  // value here.
  static CaptureManager* instance = new CaptureManager();
  return *instance;
}

stream::OrderingRecorder& CaptureManager::GetOrderingRecorder() {
  std::call_once(m_RecorderInitFlag,
                 [this] { m_Recorder = std::make_unique<stream::OrderingRecorder>(); });
  return *m_Recorder;
}

void CaptureManager::RecordCommand(Command& command, stream::CommandSerializer* serializer) {
  // See the header comment: command.m_Key must already be set by the caller - `serializer`
  // was built from `command` before this call, so setting it here would not affect the
  // already-encoded bytes.
  GetOrderingRecorder().Record(command.m_Key, serializer);
}

void CaptureManager::RecordUnsupportedCommand(const char* name, uint64_t key, uint32_t threadId) {
  UnsupportedCommand command(threadId, name);
  command.m_Key = key;
  RecordCommand(command, new UnsupportedCommandSerializer(command));
}

void CaptureManager::SkipCommandKey(uint64_t key) {
  GetOrderingRecorder().Skip(key);
}

} // namespace gits::opengl2
